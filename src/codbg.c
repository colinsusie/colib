#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"

/**
 * 打印堆栈
 * (msg, level, max) -> string
 * @param msg 附加消息
 * @param level 从第几层级开始，默认为1
 * @param max 打印的最大层次，默认是20
 * @return string 返回堆栈字符串
 */
static int l_traceback(lua_State *L) {
	size_t msgsz;
	const char *msg = luaL_checklstring(L, 1, &msgsz);
	int level = (int)luaL_optinteger(L, 2, 1);
	int max = (int)luaL_optinteger(L, 3, 20) + level;

	lua_Debug ar;
	int top = lua_gettop(L);
	if (msg) lua_pushfstring(L, "%s\n", msg);
	luaL_checkstack(L, 10, NULL);
	lua_pushliteral(L, "STACK TRACEBACK:");
	int n;
	const char *name;
	while (lua_getstack(L, level++, &ar)) {
		lua_getinfo(L, "Slntu", &ar);
		lua_pushfstring(L, "\n=> %s:%d in ", ar.short_src, ar.currentline);
		if (ar.name)
			lua_pushstring(L, ar.name);
		else if (ar.what[0] == 'm')
			lua_pushliteral(L, "mainchunk");
		else
			lua_pushliteral(L, "?");
		if (ar.istailcall)
			lua_pushliteral(L, "\n(...tail calls...)");
		lua_concat(L, lua_gettop(L) - top);		// <str>

		// varargs
		n = -1;
		while ((name = lua_getlocal(L, &ar, n--)) != NULL) {	// <str|value>
			lua_pushfstring(L, "\n    %s = ", name);		// <str|value|name>
			luaL_tolstring(L, -2, NULL);	// <str|value|name|valstr>
			lua_remove(L, -3);	// <str|name|valstr>
			lua_concat(L, lua_gettop(L) - top);		// <str>
		}

		// arg and local
		n = 1;
		while ((name = lua_getlocal(L, &ar, n++)) != NULL) {	// <str|value>
			if (name[0] == '(') {
				lua_pop(L, 1);		// <str>
			} else {
				if (n <= ar.nparams+1)
					lua_pushfstring(L, "\n    param %s = ", name);		// <str|value|name>
				else
					lua_pushfstring(L, "\n    local %s = ", name);		// <str|value|name>
				luaL_tolstring(L, -2, NULL);	// <str|value|name|valstr>
				lua_remove(L, -3);	// <str|name|valstr>
				lua_concat(L, lua_gettop(L) - top);		// <str>
			}
		}

		if (level > max)
			break;
	}
	lua_concat(L, lua_gettop(L) - top);
	return 1;
}

static int l_getclock(lua_State *L) {
	struct timespec ti;
	clock_gettime(CLOCK_MONOTONIC, &ti);
	lua_Integer c = (uint64_t)1000000000 * ti.tv_sec + ti.tv_nsec;
	lua_pushinteger(L, c);
	return 1;
}

static const luaL_Reg lib[] = {
	{"traceback", l_traceback},
	{"getclock", l_getclock},
	{NULL, NULL},
};

int luaopen_colib_dbg(lua_State *L) {
	luaL_newlib(L, lib);
	return 1;
}
