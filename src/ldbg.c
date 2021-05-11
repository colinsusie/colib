#define LUA_LIB
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"

#if defined (_WIN32)
#include <windows.h>
#endif

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

static int l_hrclock(lua_State *L) {
#if defined(_WIN32)
	LARGE_INTEGER f;
	LARGE_INTEGER t;
	QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&t);
	lua_Number c = (lua_Number)((t.QuadPart / (double)f.QuadPart));
	lua_pushnumber(L, c);
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	lua_Number c = (lua_Number)(ts.tv_sec + (double)ts.tv_nsec / 1000000000);
	lua_pushnumber(L, c);
#endif
	return 1;
}


////////////////////////////////////////////////////////////////////////////
// stopwatch

#define STOPWATCH          "CO_STOPSWATH"

typedef struct stopwatch {
#if defined(_WIN32)
	LARGE_INTEGER c;
	LARGE_INTEGER f;
#else
	struct timespec c;
#endif
	double elapsed;
} stopwatch_t;


#define tostopwatch(L)	((stopwatch_t *)luaL_checkudata(L, 1, STOPWATCH))

static int l_stopwatch(lua_State *L) {
	co_newuserdata(L, sizeof(stopwatch_t));
	luaL_setmetatable(L, STOPWATCH);
	return 1;
}

static int sw_tostring(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
	lua_pushfstring(L, "stopwatch (%p)", sw);
	return 1;
}

static int sw_start(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
#if defined(_WIN32)
	LARGE_INTEGER f;
	LARGE_INTEGER t;
	QueryPerformanceFrequency(&sw->f);
	QueryPerformanceCounter(&sw->c);
#else
	clock_gettime(CLOCK_MONOTONIC, &sw->c);
#endif
	return 0;
}

static int sw_stop(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
#if defined(_WIN32)
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	sw->elapsed = (double)(c.QuadPart - sw->c.QuadPart) / sw->f.QuadPart;
#else
	struct timespec c;
	clock_gettime(CLOCK_MONOTONIC, &c);
	time_t sec = c.tv_sec - sw->c.tv_sec;
    long nsec = c.tv_nsec - sw->c.tv_nsec;
    sw->elapsed =  (double)(sec + (double)nsec / 1000000000);
#endif
	return 0;
}

static int sw_elapsed(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
	lua_pushnumber(L, (lua_Number)sw->elapsed);
	return 1;
}

static int sw_elapsed_ms(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
	lua_pushinteger(L, (lua_Integer)(sw->elapsed * 1000));
	return 1;
}

static int sw_elapsed_us(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
	lua_pushinteger(L, (lua_Integer)(sw->elapsed * 1000000));
	return 1;
}

static int sw_elapsed_ns(lua_State *L) {
	stopwatch_t *sw = tostopwatch(L);
	lua_pushinteger(L, (lua_Integer)(sw->elapsed * 1000000000));
	return 1;
}

static const luaL_Reg swme[] = {
	{"start", sw_start},
	{"stop", sw_stop},
	{"elapsed", sw_elapsed},
	{"elapsed_ms", sw_elapsed_ms},
	{"elapsed_us", sw_elapsed_us},
	{"elapsed_ns", sw_elapsed_ns},
	{NULL, NULL}
};

static const luaL_Reg swmt[] = {
	{"__index", NULL},
	{"__tostring", sw_tostring},
	{NULL, NULL}
};

static void init_stopwatch_mt(lua_State *L) {
	luaL_newmetatable(L, STOPWATCH);
	luaL_setfuncs(L, swmt, 0);
	luaL_newlibtable(L, swme);
	luaL_setfuncs(L, swme, 0);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}

static const luaL_Reg lib[] = {
	{"traceback", l_traceback},
	{"hrclock", l_hrclock},
	{"stopwatch", l_stopwatch},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colibc_dbg(lua_State *L) {
	luaL_newlib(L, lib);
	init_stopwatch_mt(L);
	return 1;
}
