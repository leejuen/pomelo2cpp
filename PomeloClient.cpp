#include "PomeloClient.h"
#include <assert.h>
#include <mutex>
#include "connectScript.h"


static PomeloClient *_g_pc = NULL;

// 生成全局互斥量对象
static std::mutex eventLock;

// 连接服务器回调
void onConnectServerCb(pc_client_t *client, int ev_type, void* ex_data,
	const char* arg1, const char* arg2)
{
	PomeloClient *owner = (PomeloClient*)pc_client_ex_data(client);
	assert(owner);
	if (owner)
	{
		std::string type = MSG_EVENT_LOGIN;
		std::string msg = "";
		switch (ev_type)
		{
		case PC_EV_CONNECTED:
			msg = event_type_connected;
			break;
		case PC_EV_DISCONNECT:
		case PC_EV_UNEXPECTED_DISCONNECT:
			msg = event_type_disconnected;
			break;
		case PC_EV_CONNECT_ERROR:
		case PC_EV_CONNECT_FAILED:
			msg = event_type_connectederror;
			break;
		default:
			if (arg1 && arg2)
			{
				type = arg1;
				msg = arg2;
			}
			break;
		}
		if (type != "" && msg != "")
		{
			owner->pushMsg(type, msg);
		}
	}
}

void requestCb(const pc_request_t * req, int rc, const char * msg)
{
	if (!req || !msg) return;

	PomeloClient * pomeloClient = (PomeloClient *)pc_client_ex_data(pc_request_client(req));
	const char * userdefine = pc_request_route(req);
	if (!userdefine)
	{
		assert(userdefine);
		return;
	}
	pomeloClient->pushMsg(userdefine, msg);
}

void notifyCb(const pc_notify_t * req, int rc)
{
	if (!req) return;

	PomeloClient * pomeloClient = (PomeloClient *)pc_client_ex_data(pc_notify_client(req));
	const char * userdefine = pc_notify_route(req);
	if (!userdefine)
	{
		//assert(userdefine);
		return;
	}
	pomeloClient->pushMsg(userdefine, "");
}


PomeloClient *PomeloClient::getInstance()
{
	if (!_g_pc)
	{
		_g_pc = new PomeloClient();
		if (!_g_pc || !_g_pc->init(NULL, NULL, NULL, NULL))
		{
			delete _g_pc;
			_g_pc = NULL;
		}
	}
	return _g_pc;
}

// 初始化,会初始化pomelo环境, 需手动调用
bool PomeloClient::init(void (*pc_log)(int level, const char* msg, ...),
            void* (*pc_alloc)(size_t len), void (*pc_free)(void* d), const char* platform)
{   
    // 初始化pomelo库, 为其绑定日志回调,内存分配/释放,平台等信息
	// 参数1: 日志回调
    // 参数2: 分配内存方法
    // 参数3: 释放内存方法
    // 参数4: 平台信息
    pc_lib_init(pc_log, pc_alloc, pc_free, platform);

    // 初始化pomelo客户端config参数
 	pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;

    // 分配pomelo空间, 填充pomelo对象
	client = (pc_client_t *)malloc(pc_client_size());
    if(!client || pc_client_init(client, (void*)this, &config) != PC_RC_OK || pc_client_state(client) != PC_ST_INITED) return false;

    // 第四个参数需要填free方法, 用于销毁第三个参数指向的对象,防止内存泄露,
    // pc_client_rm_ev_handler方法会调用free方法,记得调用完成后把pomelo对象置为NULL
    // 将pomelo对象加入libev环境中
    // 将返回的handleid保存下来,用于释放用
	eventHandler = pc_client_add_ev_handler(client, onConnectServerCb, (void *)this, NULL);

	bool isinit = PC_EV_INVALID_HANDLER_ID != eventHandler;
	if (isinit)
		g_runTime.init();
    return isinit;
}

// 客户端心跳时,把存储的消息发送出去
void PomeloClient::runOnce()
{
    eventLock.lock();
    for(MsgCbData data: msgCbs)
        lua_process_msg(data.userdefine.c_str(), data.msg.c_str());

 	msgCbs.clear();
    eventLock.unlock();
}

// 把返回的用户自定义事件压入vec中
void PomeloClient::pushMsg(const std::string &route, const std::string &msg)
{
    eventLock.lock();
	msgCbs.push_back(MsgCbData{ route, msg });
    eventLock.unlock();
}

// 请求
void PomeloClient::requestWithTimeout(const char* route,const char* msg, void* ex_data,
    int timeout)
{
    pc_request_with_timeout(client, route, msg, ex_data, timeout, requestCb);
}

// 通知
void PomeloClient::notifyWithTimeout(const char* route,const char* msg, void* ex_data,
    int timeout)
{
	pc_notify_with_timeout(client, route, msg, ex_data, timeout, notifyCb);
}


// 连接服务器,需手动调用
int PomeloClient::connect(const char* host, int port, const char* handshake_opts)
{
	// 先断开连接,在重新连接
	disconnect();
	if (host && port != 0)
	{
		mServerInfo.serverip = host;
		mServerInfo.serverport = port;
	}
	return pc_client_connect(client, mServerInfo.serverip.c_str(), mServerInfo.serverport, handshake_opts);
}

// 断开服务器连接
int PomeloClient::disconnect()
{
	return pc_client_disconnect(client);
}

// 获得client状态
int  PomeloClient::getState()
{
	return pc_client_state(client);
}

// 设置日志等级
void PomeloClient::setLogLevel(int level)
{
	pc_lib_set_default_log_level(level);
}

// 清空数据
void PomeloClient::cleanup()
{
    // 先断开连接
    disconnect();

    // 然后从libuv环境中移除客户端
	pc_client_rm_ev_handler(client, eventHandler);

    // 清理client配置
	pc_client_cleanup(client);

    // 如果状态不正常, assert
    if(getState() != PC_ST_NOT_INITED) assert(false);

    // 释放client内存,并初始化指针
    free(client);
    client = NULL;

    // 清空队列中的数据, 记得上锁
    eventLock.lock();
    msgCbs.clear();
    eventLock.unlock();
    
    // 销毁网络库相关环境
    pc_lib_cleanup();
}

