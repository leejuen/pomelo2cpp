#include <cstdlib>
#include "connectscript.h"
#include "CCLuaEngine.h"
#include "CCLuaStack.h"
#include "PomeloClient.h"

static PomeloClient *pc = NULL;

void lua_process_msg (const char * route, const char *msg)
{
	cocos2d::LuaEngine *pEngine = cocos2d::LuaEngine::defaultEngine();
	cocos2d::LuaStack *pStack = pEngine->getLuaStack();
	lua_State* L = pStack->getLuaState();
	const char *functionname = "serverevent";
	lua_getglobal(L, functionname);

	lua_pushstring(L, route);

	lua_pushstring(L, msg);

	if (lua_pcall(L, 2, 0, 0) != 0)
		lua_pop(L, 1);
}

static void instance()
{
	if(!pc) pc = PomeloClient::getInstance();
}

static int luapomeloapi_getInstance(lua_State *L)
{
	if (!L) return 0;

	if (!pc) instance();

	lua_pushlightuserdata(L, pc);
	return 1;
}

static int luapomeloapi_requestWithTimeout(lua_State *L)
{
	if (!L) return 0;

	if (!pc) instance();

	int argc = lua_gettop(L);
	if (argc < 2)
	{
		return 0;
	}
	const char *remote = NULL;
	const char *msg = NULL;
	void *exdata = NULL;
	int timeout = -1;
	
	remote = luaL_checkstring(L, 1);
	msg = luaL_checkstring(L, 2);
	
	if (3 == argc)
	{
		exdata = (void *)lua_touserdata(L, 3);
	}

	if (4 == argc)
	{
		exdata = (void *)lua_touserdata(L, 3);
		timeout = luaL_checkint(L, 4);
	}
	
	pc->requestWithTimeout(remote, msg, exdata, timeout);

	return 1;
}

static int luapomeloapi_notifyWithTimeout(lua_State *L)
{
	if (!L) return 0;
	if (!pc) instance();
	int argc = lua_gettop(L);
	if (argc < 2)
	{
		return 0;
	}
	const char *remote = NULL;
	const char *msg = NULL;
	void *exdata = NULL;
	int timeout = -1;

	remote = luaL_checkstring(L, 1);
	msg = luaL_checkstring(L, 2);

	if (3 == argc)
	{
		exdata = (void *)lua_touserdata(L, 3);
	}

	if (4 == argc)
	{
		exdata = (void *)lua_touserdata(L, 3);
		timeout = luaL_checkint(L, 4);
	}

	
	pc->notifyWithTimeout(remote, msg, exdata, timeout);
	return 1;
}

static int luapomeloapi_connect(lua_State *L)
{
	if (!L) return 0;
	if (!pc) instance();
	int argc = lua_gettop(L);
	if (0 == argc)
	{
		pc->connect();
	}
	const char *host = NULL;
	const char *opts = NULL;
	int port = 0;

	if (2 == argc)
	{
		host = luaL_checkstring(L, 1);
		port = luaL_checkint(L, 2);
	}

	if (3 == argc)
	{
		host = luaL_checkstring(L, 1);
		port = luaL_checkint(L, 2);
		opts = luaL_checkstring(L, 3);
	}

	pc->connect(host, port, opts);
	return 1;
}

static int luapomeloapi_disconnect(lua_State *L)
{
	if (!L) return 0;
	if (!pc) instance();
	pc->disconnect();
	return 1;
}


static const struct luaL_reg pomeloapi_function[] = {
		{ "getInstance", luapomeloapi_getInstance },
		{ "request", luapomeloapi_requestWithTimeout },
		{ "notify", luapomeloapi_notifyWithTimeout },
		{ "connect", luapomeloapi_connect },
		{ "disconnect", luapomeloapi_disconnect },
};

void lua_luapomeloapi_register(lua_State *L)
{
	luaL_register(L, "luapomelo", pomeloapi_function);
	lua_pop(L, 1);
}