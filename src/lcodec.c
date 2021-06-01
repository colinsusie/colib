#define LUA_LIB
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"

static const char *etable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int l_b64encode(lua_State *L) {	
	size_t sz;
	const uint8_t *bytes = (const uint8_t*)luaL_checklstring(L, 1, &sz);
	size_t esz = ((sz + 2) / 3) * 4;
	luaL_Buffer b;
	luaL_buffinitsize(L, &b, esz);
	int i;
	char e;
	for (i = 0; i < (int)sz - 2; i += 3) {		// 11111111 11111111 11111111 -> (111111)(11 1111)(1111 11)(111111)
		e = etable[(bytes[i] >> 2)];
		luaL_addchar(&b, e);
		e = etable[((bytes[i] & 0x03) << 4) | ((bytes[i+1] & 0xF0) >> 4)];
		luaL_addchar(&b, e);
		e = etable[((bytes[i+1] & 0x0F) << 2) | ((bytes[i+2] & 0xC0) >> 6)];
		luaL_addchar(&b, e);
		e = etable[bytes[i+2] & 0x3F];
		luaL_addchar(&b, e);
	}
	uint8_t b1, b2;
	if (i < sz) {
		b1 = bytes[i];
		e = etable[(b1 >> 2)];
		luaL_addchar(&b, e);
		b2 = i + 1 < sz ? bytes[i+1] : 0;
		e = etable[((b1 & 0x03) << 4) | ((b2 & 0xF0) >> 4)];
		luaL_addchar(&b, e);
		e = i + 1 < sz ? etable[(b2 & 0x0F) << 2] : '=';
		luaL_addchar(&b, e);
		luaL_addchar(&b, '=');
	}
	luaL_pushresult(&b);
	return 1;
}

static const uint8_t dtable[] = {
	/* ASCII table */
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 62, 80, 80, 80, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 80, 80, 80, 80, 80, 80,
    80,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 80, 80, 80, 80, 80,
    80, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80
};

static int l_b64decode(lua_State *L) {
	size_t sz;
	const uint8_t *bytes = (const uint8_t*)luaL_checklstring(L, 1, &sz);
	size_t dsz = ((sz + 3) / 4) * 3;
	luaL_Buffer b;
	luaL_buffinitsize(L, &b, dsz);
	char c;
	uint8_t b1, b2, b3, b4;
	size_t i = 0;
	while (i+4 <= sz) {			// (111111)(11 1111)(1111 11)(111111) -> 11111111 11111111 11111111
		b1 = dtable[bytes[i]];
		if (b1 == 80) goto EXIT;
		b2 = dtable[bytes[i+1]];
		if (b2 == 80) goto EXIT;
		c = (char)(b1 << 2 | ((b2 & 0xF0) >> 4));
		luaL_addchar(&b, c);

		b3 = dtable[bytes[i+2]];
		if (b3 == 80) goto EXIT;
		c = (char)(((b2 & 0x0F) << 4) | ((b3 & 0x3C) >> 2));
		luaL_addchar(&b, c);

		b4 = dtable[bytes[i+3]];
		if (b4 == 80) goto EXIT;
		c = (char)(((b3 & 0x03) << 6) | (b4 & 0x3F));
		luaL_addchar(&b, c);

		i +=4;
	}
	switch (sz - i) {
	case 2:
		b1 = dtable[bytes[i]];
		if (b1 == 80) goto EXIT;
		b2 = dtable[bytes[i+1]];
		if (b2 == 80) goto EXIT;
		c = (char)(b1 << 2 | ((b2 & 0xF0) >> 4));
		luaL_addchar(&b, c);
		break;
	case 3:
		b1 = dtable[bytes[i]];
		if (b1 == 80) goto EXIT;
		b2 = dtable[bytes[i+1]];
		if (b2 == 80) goto EXIT;
		c = (char)(b1 << 2 | ((b2 & 0xF0) >> 4));
		luaL_addchar(&b, c);

		b3 = dtable[bytes[i+2]];
		if (b3 == 80) goto EXIT;
		c = (char)(((b2 & 0x0F) << 4) | ((b3 & 0x3C) >> 2));
		luaL_addchar(&b, c);
		break;
	}
EXIT:
	luaL_pushresult(&b);
	return 1;
}


static const luaL_Reg lib[] = {
	{"b64encode", l_b64encode},
	{"b64decode", l_b64decode},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colibc_codec(lua_State *L) {
	luaL_newlib(L, lib);
	return 1;
}