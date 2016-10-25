#ifndef _H_CONNECT_SCRIPT_H_
#define _H_CONNECT_SCRIPT_H_

struct lua_State;

//调用脚本函数，当收到服务器消息时
void lua_process_msg (const char * route, const char *pMsg);

void lua_luapomeloapi_register(lua_State *L);

#endif
