/**
 * 字符串处理函数
 */
#define LUA_LIB
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lua.h"
#include "lauxlib.h"


static size_t abspos(lua_Integer pos, size_t len) {
	if (pos > 0)
		return (size_t)pos;
	else if (pos == 0)
		return 1;
	else if (pos < -(lua_Integer)len)
		return 1;
	else
		return len + (size_t)pos + 1;
}

static int l_startswith(lua_State *L) {
	size_t ls, lp;
	const char *str = luaL_checklstring(L, 1, &ls);
	const char *prefix = luaL_checklstring(L, 2, &lp);
	if (lp == 0) {		// 空字符串，一定匹配
		lua_pushboolean(L, 1);
		return 1;
	}
	size_t start = abspos(luaL_optinteger(L, 3, 1), ls) - 1;
	size_t end = abspos(luaL_optinteger(L, 4, ls), ls);
	if (lp > end - start) {		// 后缀比较长，一定不匹配
		lua_pushboolean(L, 0);
		return 1;
	}
	int ok = memcmp(str+start, prefix, lp) == 0;
	lua_pushboolean(L, ok);
	return 1;
}

static int l_endswith(lua_State *L) {
	size_t ls, lp;
	const char *str = luaL_checklstring(L, 1, &ls);
	const char *prefix = luaL_checklstring(L, 2, &lp);
	if (lp == 0) {		// 空字符串，一定匹配
		lua_pushboolean(L, 1);
		return 1;
	}
	size_t start = abspos(luaL_optinteger(L, 3, 1), ls) - 1;
	size_t end = abspos(luaL_optinteger(L, 4, ls), ls);
	if (lp > end - start) {		// 后缀比较长，一定不匹配
		lua_pushboolean(L, 0);
		return 1;
	}
	int ok = 1;
	const char *ps = str + end - 1;
	const char *pp = prefix + lp - 1;
	for (; pp - prefix >= 0; --ps, --pp) {
		if (*ps != *pp) {
			ok = 0;
			break;
		}
	}
	lua_pushboolean(L, ok);
	return 1;
}

static int l_ljust(lua_State *L) {
	size_t ls, lf;
	const char *str = luaL_checklstring(L, 1, &ls);
	size_t width = (size_t)luaL_optinteger(L, 2, ls);
	if (width <= ls) {
		lua_pushlstring(L, str, ls);
		return 1;
	}
	const char *fillchar = luaL_optlstring(L, 3, " ", &lf);
	int fc = lf == 0 ? ' ' : fillchar[0];
	luaL_Buffer b;
	char *p = luaL_buffinitsize(L, &b, width);
	memcpy(p, str, ls * sizeof(char));
	memset(p+ls, fc, (width-ls) * sizeof(char));
	luaL_pushresultsize(&b, width);
	return 1;
}

static int l_rjust(lua_State *L) {
	size_t ls, lf;
	const char *str = luaL_checklstring(L, 1, &ls);
	size_t width = (size_t)luaL_optinteger(L, 2, ls);
	if (width <= ls) {
		lua_pushlstring(L, str, ls);
		return 1;
	}
	const char *fillchar = luaL_optlstring(L, 3, " ", &lf);
	int fc = lf == 0 ? ' ' : fillchar[0];
	luaL_Buffer b;
	char *p = luaL_buffinitsize(L, &b, width);
	memset(p, fc, (width-ls) * sizeof(char));
	memcpy(p+(width-ls), str, ls * sizeof(char));
	luaL_pushresultsize(&b, width);
	return 1;
}

static int l_center(lua_State *L) {
	size_t ls, lf;
	const char *str = luaL_checklstring(L, 1, &ls);
	size_t width = (size_t)luaL_optinteger(L, 2, ls);
	if (width <= ls) {
		lua_pushlstring(L, str, ls);
		return 1;
	}
	const char *fillchar = luaL_optlstring(L, 3, " ", &lf);
	int fc = lf == 0 ? ' ' : fillchar[0];
	size_t lsz = (width - ls) / 2;
	size_t rsz = width - ls - lsz;

	luaL_Buffer b;
	char *p = luaL_buffinitsize(L, &b, width);
	memset(p, fc, lsz * sizeof(char));
	memcpy(p+lsz, str, ls * sizeof(char));
	memset(p+lsz+ls, fc, rsz * sizeof(char));
	luaL_pushresultsize(&b, width);
	return 1;
}

static const char *spacechars = "\t\n\v\f\r ";

static int l_lstrip(lua_State *L) {
	size_t ls, lc;
	const char *str = luaL_checklstring(L, 1, &ls);
	const char *chars;
	if (!lua_isnoneornil(L, 2)) {
		chars = luaL_checklstring(L, 2, &lc);
	} else {
		chars = spacechars;
		lc = 6;
	}
	const char *p = str;
	size_t i = 0;
	for (; i < ls; ++i, ++p) {
		if (!memchr(chars, *p, lc)) break;
	}
	lua_pushlstring(L, str+i, ls-i);
	return 1;
}

static int l_rstrip(lua_State *L) {
	size_t ls, lc;
	const char *str = luaL_checklstring(L, 1, &ls);
	const char *chars;
	if (!lua_isnoneornil(L, 2)) {
		chars = luaL_checklstring(L, 2, &lc);
	} else {
		chars = spacechars;
		lc = 6;
	}
	const char *p = str + ls - 1;
	size_t i = 0;
	for (; i < ls; ++i, --p) {
		if (!memchr(chars, *p, lc)) break;
	}
	lua_pushlstring(L, str, ls-i);
	return 1;
}

static int l_strip(lua_State *L) {
	size_t ls, lc;
	const char *str = luaL_checklstring(L, 1, &ls);
	const char *chars;
	if (!lua_isnoneornil(L, 2)) {
		chars = luaL_checklstring(L, 2, &lc);
	} else {
		chars = spacechars;
		lc = 6;
	}
	const char *p = str;
	size_t i = 0;
	for (; i < ls; ++i, ++p) {
		if (!memchr(chars, *p, lc)) break;
	}

	p = str + ls - 1;
	size_t j = ls;
	for (; j > i; --j, --p) {
		if (!memchr(chars, *p, lc)) break;
	}
	lua_pushlstring(L, str+i, j-i);
	return 1;
}

static int l_split(lua_State *L) {
	const char *str = luaL_checkstring(L, 1);
	size_t lp;
	const char *sep = luaL_checklstring(L, 2, &lp);
	if (lp == 0) {
		luaL_error(L, "cannot be empty separator");
	}
	int matchchar = lua_toboolean(L, 3);
	lua_newtable(L);
	const char *ps = str;
	const char *pe;
	lua_Integer n = 1;
	if (matchchar) {
		pe = strpbrk(ps, sep);
		while (pe) {
			lua_pushlstring(L, ps, pe-ps);
			lua_rawseti(L, -2, n++);
			ps = pe + strspn(pe, sep);
			pe = strpbrk(ps, sep);
		}
	} else {
		pe = strstr(ps, sep);
		while (pe) {
			lua_pushlstring(L, ps, pe-ps);
			lua_rawseti(L, -2, n++);
			ps = pe + lp;
			pe = strstr(ps, sep);
		}
	}
	if (*ps) {
		lua_pushstring(L, ps);
		lua_rawseti(L, -2, n);
	} else {
		lua_pushliteral(L, "");
		lua_rawseti(L, -2, n);
	}
	return 1;
}

static const luaL_Reg lib[] = {
	{"startswith", l_startswith},
	{"endswith", l_endswith},
	{"ljust", l_ljust},
	{"rjust", l_rjust},
	{"center", l_center},
	{"lstrip", l_lstrip},
	{"rstrip", l_rstrip},
	{"strip", l_strip},
	{"split", l_split},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colib_str(lua_State *L) {
	luaL_newlib(L, lib);
	return 1;
}