#ifndef __POMELO_CLIENT___H__
#define __POMELO_CLIENT___H__
#include "pomelo.h"
#include <vector>
#include <string>
#include <functional>
#include <map>


#define MSG_EVENT_LOGIN "MSG_EVENT_LOGIN"
#define event_type_disconnected "event_type_disconnected"
#define event_type_connectederror "event_type_connectederror"
#define event_type_connected "event_type_connected"

// 协议相关数据
struct MsgCbData{

    // 协议title
    std::string userdefine;
    
    // 协议信息
    std::string msg;
};

//链接服务器信息
struct ServerInfo{
	//服务器ip
	std::string serverip;

	//服务器端口号
	int serverport;
};

// 连接服务器回调
static void onConnectServerCb(pc_client_t *client, int ev_type, void* ex_data, const char* arg1, const char* arg2);

// request请求回调,处理网络回调的信息
static void requestCb(const pc_request_t * req, int rc, const char * resp);

// notify回调,处理推送回调的信息
static void notifyCb(const pc_notify_t * req, int rc);

class PomeloClient
{
	public:

		static PomeloClient * getInstance();
    public:

        // 初始化网络库环境
        // 初始化pomelo库, 为其绑定日志回调,内存分配/释放,平台等信息
        bool init(void (*pc_log)(int level, const char* msg, ...),
            void* (*pc_alloc)(size_t len), void (*pc_free)(void* d), const char* platform);

        // 发送request请求
        void requestWithTimeout(const char* route, const char* msg, void* ex_data = NULL,
            int timeout = -1);

        // 发送通知
        void notifyWithTimeout(const char* route, const char* msg, void* ex_data = NULL,
            int timeout = -1);

        // 连接服务器,需手动调用
		int connect(const char* host = NULL, int port = 0, const char* handshake_opts = NULL);

        // 断开服务器连接
		int disconnect();

        // 获得client状态
		int getState();

        // 设置日志等级
		void setLogLevel(int level);

		// 把返回的通知信息压入vec中
		void pushMsg(const std::string &route, const std::string &msg);

		// 心跳
		void runOnce();

    private:

		// 通过初始化列表初始化client指针, 初始化client后返回的handleid, 初始化connStatus为未连接状态
		PomeloClient() :client(NULL), eventHandler(PC_EV_INVALID_HANDLER_ID)
		{
		}

		// 析构函数
		~PomeloClient()
		{
			cleanup();
		}

        PomeloClient(PomeloClient&);

        operator=(PomeloClient &);

        // 清空数据
        void cleanup();

    private:

        // pomelo对象指针
        pc_client_t * client;

		//链接服务器信息
		ServerInfo mServerInfo;

        // 初始化pomelo客户端后返回的handleid
        int eventHandler;
            
        // 回调信息存放
        std::vector<MsgCbData> msgCbs;
};
#endif