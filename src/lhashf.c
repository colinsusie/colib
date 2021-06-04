#define LUA_LIB
#include <stddef.h>
#include <stdlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"
#include "cohashf.h"

static int fnv1a64(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint64_t hash = hashf_fnv1a_64((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int fnv1a32(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_fnv1a_32((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int djb2(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_djb2((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int sdbm(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_sdbm((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int rs(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_rs((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int js(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_js((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int bkdr(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_bkdr((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int dek(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_dek((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static int ap(lua_State *L) {
	size_t sz;
	const char *str = luaL_checklstring(L, 1, &sz);
	uint32_t hash = hashf_ap((const uint8_t *)str, sz);
	lua_pushinteger(L, (lua_Integer)hash);
	return 1;
}

static const luaL_Reg lib[] = {
	{"fnv1a64", fnv1a64},
	{"fnv1a32", fnv1a32},
	{"djb2", djb2},
	{"sdbm", sdbm},
	{"fnv1a32", fnv1a32},
	{"rs", rs},
	{"js", js},
	{"bkdr", bkdr},
	{"dek", dek},
	{"ap", ap},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colibc_hashf(lua_State *L) {
	luaL_newlib(L, lib);
	return 1;
}