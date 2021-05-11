/**
 * 随机数生成器
 */
#define LUA_LIB
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"
#include "corand.h"

#define RAND_UD "CO_RAND"
#define torandstate(L) ((randstate_t*)luaL_checkudata((L), 1, RAND_UD))

// rand.new([seed1, seed2]) ->randud
static int rand_new(lua_State *L) {
	randstate_t *state = (randstate_t*)co_newuserdata(L, sizeof(randstate_t));
	if (lua_isinteger(L, 1) && lua_isinteger(L, 2)) {
		lua_Integer seed1 = lua_tointeger(L, 1);
		lua_Integer seed2 = lua_tointeger(L, 2);
		randseed(state, (uint64_t)seed1, (uint64_t)seed2);
	} else {
		// use default seeds
		randseed(state, (uint64_t)time(NULL), (uint64_t)(size_t)state);
	}
	luaL_setmetatable(L, RAND_UD);
	return 1;
}

static int rand_tostring(lua_State *L) {
	randstate_t *state = torandstate(L);
	lua_pushfstring(L, "rand: %p", state);
	return 1;
}

// randobj:nextfloat([a, b]) -> number
static int rand_nextfloat(lua_State *L) {
	randstate_t *state = torandstate(L);
	double r;
	switch (lua_gettop(L)) {
		case 1:
			r = randfloat(state);
			lua_pushnumber(L, (lua_Number)r);
			return 1;
		case 3:
			r = randfltrange(state, (double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3));
			lua_pushnumber(L, (lua_Number)r);
			return 1;
		default:
			return luaL_error(L, "wrong number of arguments");
	}
}

// randobj:nextint([a, b]) -> integer
static int rand_nextint(lua_State *L) {
	randstate_t *state = torandstate(L);
	int64_t r;
	switch (lua_gettop(L)) {
		case 1:
			r = randint(state);
			lua_pushinteger(L, (int64_t)r);
			return 1;
		case 3:
			r = randintrange(state, (int64_t)luaL_checkinteger(L, 2), (int64_t)luaL_checkinteger(L, 3));
			lua_pushinteger(L, (int64_t)r);
			return 1;
		default:
			return luaL_error(L, "wrong number of arguments");
	}
}

// randobj:setseed(seed1, seed2)
static int rand_setseed(lua_State *L) {
	randstate_t *state = torandstate(L);
	lua_Integer seed1 = luaL_checkinteger(L, 2);
	lua_Integer seed2 = luaL_checkinteger(L, 3);
	randseed(state, (uint64_t)seed1, (uint64_t)seed2);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////

// 模块函数
static const luaL_Reg lib[] = {
	{"new", rand_new}, 
	{NULL, NULL}
};

static const luaL_Reg rand_mt[] = {
	{"__index", NULL},
	{"__tostring", rand_tostring},
	{"nextfloat", rand_nextfloat},
	{"nextint", rand_nextint},
	{"setseed", rand_setseed},
	{NULL, NULL}
};

LUAMOD_API int luaopen_colibc_rand(lua_State *L) {
	luaL_checkversion(L);

	// create metatable of oset
	luaL_newmetatable(L, RAND_UD);		// S: mt
	luaL_setfuncs(L, rand_mt, 0);		// S: mt
	lua_pushvalue(L, -1);				// S: mt mt
	lua_setfield(L, -2, "__index");		// S: mt

	luaL_newlib(L, lib);				// S: mt lib
	return 1;
}