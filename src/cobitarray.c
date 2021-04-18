/**
 * 位图数组
 * @author colin
 * @date 2021/4/16
 */
#define LUA_LIB
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"

#define MAX_BITS (CHAR_BIT * sizeof(lua_Integer))
#define I_WORD(i, b) (int)((i) / (b))
#define I_BIT(i, b) ((lua_Integer)1 << ((i) % (b)))
#define WORD_SIZE(bitsz, b) (1 + I_WORD((bitsz)-1, (b)))

#define GET_VALUE(a, i) !!(a->values[I_WORD(i, a->wordbits)] & I_BIT(i, a->wordbits))
#define SET_VALUE(a, i, v) do {\
	if (v) a->values[I_WORD(i, a->wordbits)] |= I_BIT(i, a->wordbits); \
	else a->values[I_WORD(i, a->wordbits)] &= ~I_BIT(i, a->wordbits); \
} while (0)

typedef struct bitarray {
	lua_Integer *values;
	lua_Integer bitsize;
	int wordsize;
	int wordbits;
} bitarray_t;

const char *const BITARRAY = "CO_BITARRAY";


static void resize_words(bitarray_t *array, int wordsz) {
	if (array->wordsize != wordsz) {
		array->values = (lua_Integer*)co_realloc(array->values, wordsz * sizeof(lua_Integer));
		if (wordsz > array->wordsize) {
			memset(array->values + array->wordsize, 0, (wordsz - array->wordsize) * sizeof(lua_Integer));
		}
		array->wordsize = wordsz;
		array->bitsize = wordsz * array->wordbits;
	}
}

static int bitarray_new(lua_State *L) {
	int wordsz = (int)luaL_checkinteger(L, 1);
	luaL_argcheck(L, wordsz > 0, 1, "invalid size");

	int wordbits = (int)luaL_checkinteger(L, 2);
	if (0 >= wordbits || wordbits > MAX_BITS) {
		luaL_error(L, "Integer bit size must in [1, %d]", MAX_BITS);
		return 0;
	}

	bitarray_t *array = (bitarray_t*)co_newuserdata(L, sizeof(bitarray_t));
	array->values = (lua_Integer*)co_calloc(wordsz, sizeof(lua_Integer));
	array->wordbits = wordbits;
	array->wordsize = wordsz;
	array->bitsize = wordbits * wordsz;
	luaL_setmetatable(L, BITARRAY);
	return 1;
}

static int bitarray_len(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_pushinteger(L, array->bitsize);
	return 1;
}

static int bitarray_gc(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	co_free(array->values);
	return 0;
}

static int bitarray_tostring(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_pushfstring(L, "bitarray: %p", array);
	return 1;
}

static int bitarray_index(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_Integer idx = luaL_checkinteger(L, 2);
	if (idx > 0) idx--; 
	else if (idx < 0) idx += array->bitsize;
	if (idx < 0 || idx >= array->bitsize) {
		lua_pushnil(L);
	} else {
		lua_pushboolean(L, GET_VALUE(array, idx));
	}
	return 1;
}

static int bitarra_newindex(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_Integer idx = luaL_checkinteger(L, 2) - 1;
	luaL_argcheck(L, 0 <= idx && idx < array->bitsize, 2, "index out of range");
	luaL_checkany(L, 3);
	SET_VALUE(array, idx, lua_toboolean(L, 3));
	return 0;
}

static int bitarray_concat(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	size_t lsep;
	const char *sep = luaL_optlstring(L, 2, "", &lsep);
	lua_Integer i = luaL_optinteger(L, 3, 1);
	if (i < 1) i = 1;
	lua_Integer last = luaL_optinteger(L, 4, array->bitsize);
	if (last > array->bitsize) last = array->bitsize;

	luaL_Buffer b;
	luaL_buffinit(L, &b);
	lua_Integer idx;
	for (; i < last; i++) {
		idx = i - 1;
		luaL_addstring(&b, (GET_VALUE(array, idx) ? "true" : "false"));
		luaL_addlstring(&b, sep, lsep);
	}
	if (i == last) {
		idx = i - 1;
		luaL_addstring(&b, (GET_VALUE(array, idx) ? "true" : "false"));
	}
	luaL_pushresult(&b);
	return 1;
}

static int bitarray_clear(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	memset(array->values, 0, array->wordsize * sizeof(lua_Integer));
	return 0;
}

static int bitarray_exchange(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_Integer idx1 = luaL_checkinteger(L, 2) - 1;
	lua_Integer idx2 = luaL_checkinteger(L, 3) - 1;
	if (idx1 == idx2) return 0;
	luaL_argcheck(L, 0 <= idx1 && idx1 < array->bitsize, 2, "index out of range");
	luaL_argcheck(L, 0 <= idx2 && idx2 < array->bitsize, 3, "index out of range");
	int b1 = GET_VALUE(array, idx1);
	int b2 = GET_VALUE(array, idx2);
	SET_VALUE(array, idx1, b2);
	SET_VALUE(array, idx2, b1);
	return 0;
}

static int bitarray_tointegers(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	int i;
	lua_createtable(L, array->wordsize, 0);
	lua_Integer mask = (lua_Integer)(-1) >> (MAX_BITS - array->wordbits);
	for (i = 0; i < array->wordsize; ++i) {
		lua_pushinteger(L, array->values[i] & mask);
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

static int bitarray_fromintegers(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	luaL_checktype(L, 2, LUA_TTABLE);
	int wordsz = (int)luaL_len(L, 2);
	resize_words(array, wordsz);
	lua_Integer mask = (lua_Integer)(-1) >> (MAX_BITS - array->wordbits);
	int i;
	for (i = 0; i < wordsz; ++i) {
		lua_geti(L, 2, i+1);
		array->values[i] = luaL_checkinteger(L, -1) & mask;
		lua_pop(L, 1);
	}
	return 0;
}

static int bitarray_tobooleans(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_createtable(L, array->bitsize, 0);
	lua_Integer i;
	for (i = 0; i < array->bitsize; ++i) {
		lua_pushboolean(L, GET_VALUE(array, i));
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

static int bitarray_frombooleans(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_Integer bitsz = luaL_len(L, 2);
	int wordsz = WORD_SIZE(bitsz, array->wordbits);
	resize_words(array, wordsz);
	lua_Integer i;
	for (i = 0; i < bitsz; ++i) {
		lua_rawgeti(L, 2, i+1);
		SET_VALUE(array, i, lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	for (i = bitsz; i < array->bitsize; ++i) {
		SET_VALUE(array, i, 0);
	}
	return 0;
}

static int bitarray_resize(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	int wordsz = (int)luaL_checkinteger(L, 2);
	luaL_argcheck(L, 0 < wordsz, 2, "Invalid size");
	resize_words(array, wordsz);
	return 0;
}

static int bitarray_wordsize(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_pushinteger(L, array->wordsize);
	return 1;
}

static int bitarray_wordbits(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_pushinteger(L, array->wordbits);
	return 1;
}

static int bitarray_set(lua_State *L) {
	bitarray_t *array = (bitarray_t*)luaL_checkudata(L, 1, BITARRAY);
	lua_Integer idx = luaL_checkinteger(L, 2) - 1;
	luaL_argcheck(L, 0 <= idx, 2, "index out of range");
	luaL_checkany(L, 3);
	if (idx >= array->bitsize) {
		int wordsz = WORD_SIZE(idx+1, array->wordbits);
		resize_words(array, wordsz);
	}
	SET_VALUE(array, idx, lua_toboolean(L, 3));
	return 0;
}

//---------------------------------------------------------------------------------
static luaL_Reg f[] = {
	{"new", bitarray_new},
	{"tointegers", bitarray_tointegers},
	{"tobooleans", bitarray_tobooleans},
	{"fromintegers", bitarray_fromintegers},
	{"frombooleans", bitarray_frombooleans},
	{"concat", bitarray_concat},
	{"clear", bitarray_clear},
	{"exchange", bitarray_exchange},
	{"resize", bitarray_resize},
	{"wordsize", bitarray_wordsize},
	{"wordbits", bitarray_wordbits},
	{"set", bitarray_set},
	{NULL, NULL},
};

static luaL_Reg m[] = {
	{"__len", bitarray_len},
	{"__gc", bitarray_gc},
	{"__tostring", bitarray_tostring},
	{"__index", bitarray_index},
	{"__newindex", bitarra_newindex},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colib_bitarray(lua_State *L) {
	luaL_checkversion(L);
	luaL_newmetatable(L, BITARRAY);
	luaL_setfuncs(L, m, 0);
	luaL_newlib(L, f);
	return 1;
}