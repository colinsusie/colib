/**
 * Lua对象序列化，参考自skynet的序列化模块(https://github.com/cloudwu/skynet)，格式总体上相同，但有一些细微的差别
 * 概述：
 * 	- 仅支持的对象类型：nil, boolean, number, string, table, lightuserdata
 * 	- 数值类型为小端字节序。
 * 
 * 序列化格式：
 * 	一个对象的格式
 * 		| type  cookie | objdata |
 * 		type 和 cookie 合并为一个字节；type表示类型，占低3位；cookie为类型附带的一些信息，占高5位。
 * 		objdata 是对象相关的数据，有些类型通过 type+cookie 就能表达，所以objdata不一定存在。
 * 
 * 		type == TYPE_NIL 表示 nil，不需要cookie和objdata。
 * 
 * 		type == TYPE_BOOL 表示 boolean， cookie为0表示false, 为1表示true；不需要objdata。
 * 
 * 		type == TYPE_INT 表示 integer：
 * 			cookie == TYPE_INT_0 表示0；不需要objdata。
 * 			cookie == TYPE_INT_8 表示int8；objdata为1个字节的小端int8数据。
 * 			cookie == TYPE_INT_16 表示int16；objdata为2个字节的小端int16数据。
 * 			cookie == TYPE_INT_32 表示int32；objdata为4个字节的小端int32数据。
 * 			cookie == TYPE_INT_64 表示int64；objdata为8个字节的小端int64数据。
 * 
 * 		type == TYPE_FLOAT 表示float，不需要cookie；objdata为8个字节的小端double数据
 * 
 * 		type == TYPE_SSTR 表示短字符串；cookie为字符串长度；objdata为字符串数据
 * 
 * 		type == TYPE_LSTR 表示长字符串：
 * 			如果字符串不大于INT8_MAX；则cookie等于1；objdata为字符串数据
 * 			如果字符串不大于INT16_MAX；则cookie等于2；objdata为字符串数据
 * 			如果字符串不大于INT32_MAX；则cookie等于4；objdata为字符串数据
 * 			否则；则cookie等于8；objdata为字符串数据
 * 
 * 		type == TYPE_TABLE 表示 table，table存为数组部分和哈希表部分
 * 			cookie == 0 表示数组长度为0，objdata是 哈希表部分
 * 			cookie < 31 表示数组的长度，objdata是 数组元素 + 哈希表
 * 			cookie == 31 表示数组长度在objdata中；objdata存：数组长度 + 数组元素 + 哈希表
 * 				数组长度的存储格式是 TYPE_INT
 * 
 * 			数组元素为按顺序存储的对象。
 * 			哈希表为先存一个key对象，再存一个value对象，一直到遇见nil对象为止。
 * 
 * 		type = TYPE_USERDATA 表示lightuserdata，由于是一个指针，所以不可以跨进程传输；不需要cookie；objdata是一个指针
 * 
 * pack 可以将多对象一起打包；unpack解包出多个对象返回。
 */
#define LUA_LIB
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"

#define TYPE_NIL 0
#define TYPE_BOOL 1
#define TYPE_INT 2
#define TYPE_FLOAT 3
#define TYPE_SSTR 4
#define TYPE_LSTR 5
#define TYPE_TABLE 6
#define TYPE_USERDATA 7

#define TYPE_INT_0 0
#define TYPE_INT_8 1
#define TYPE_INT_16 2
#define TYPE_INT_32 3
#define TYPE_INT_64 4

#define MAX_COOKIE 31
#define COMBINE_TYPE(t, v) (char)((t) | (v) << 3)
#define TYPE_TYPE(t) ((t) & 7)
#define TYPE_COOKIE(t) (((t) >> 3) & 31)

#define MAX_DEPTH 32

/* mask for one character */
#define CHAR_MASK	((1 << CHAR_BIT) - 1)

/* dummy union to get native endianness */
static const union {
	int dummy;
	char little;  /* true iff machine is little endian */
} nativeendian = {1};

//---------------------------------------------------------------------------------------------
// pack

#define INIT_BUFF_SIZE 512

typedef struct WriteBuffer {
	char *buff;
	size_t size;
	size_t cap;
	char b[INIT_BUFF_SIZE];
} WriteBuffer;

static char* wb_prepbuffsize(WriteBuffer *b, size_t sz) {
	if (b->size + sz > b->cap) {
		size_t newcap = b->cap * 2;
		if (newcap < b->size + sz)
			newcap = b->size + sz;
		if (b->buff == b->b) {
			b->buff = (char*)co_malloc(newcap);
			memcpy(b->buff, b->b, b->size * sizeof(char));
		} else {
			b->buff = (char*)co_realloc(b->buff, newcap);
		}
		b->cap = newcap;
	}
	char *p = b->buff + b->size;
	b->size += sz;
	return p;
}

static inline void wb_addbuffer(WriteBuffer *b, const char *s, size_t l) {
	if (l > 0) {
		char *p = wb_prepbuffsize(b, l);
		memcpy(p, s, l * sizeof(char));
	}
}

static inline void wb_addchar(WriteBuffer *b, char c) {
	char *p = wb_prepbuffsize(b, 1);
	*p = c;
}

static void wb_buffinit(WriteBuffer *b) {
	b->buff = b->b;
	b->size = 0;
	b->cap = INIT_BUFF_SIZE;
}

static void wb_free(WriteBuffer *b) {
	if (b->buff != b->b) {
		co_free(b->buff);
	}
}

static void pack_value(lua_State *L, WriteBuffer *b, int index, int depth);

static inline void _pack_intx(WriteBuffer *b, uint64_t n, int size) {
	char *p = wb_prepbuffsize(b, size);
	if (nativeendian.little) {
		memcpy(p, (const char*)&n, size);
	} else {
		int i;
		for (i = 0; i < size; i++) {
			p[i] = (char)(n & CHAR_MASK);
			n >>= CHAR_BIT;
		}
	}
}
#define pack_intx(b, n, t) _pack_intx(b, (uint64_t)(t)n, sizeof(t))


static inline void pack_double(WriteBuffer *b, double d) {
	char *buff = wb_prepbuffsize(b, sizeof(d));
	const char *src = (const char*)&d;
	size_t size = sizeof(d);
	if (nativeendian.little) {
		memcpy(buff, src, size);
	} else {
		buff += size - 1;
		while (size-- != 0)
			*(buff--) = *(src++);
	}
}

static inline void pack_int(WriteBuffer *b, int64_t n) {
	char t;
	if (n == 0) {
		t = COMBINE_TYPE(TYPE_INT, TYPE_INT_0);
		wb_addchar(b, t);
	} else if (INT8_MIN <= n && n <= INT8_MAX) {
		t = COMBINE_TYPE(TYPE_INT, TYPE_INT_8);
		wb_addchar(b, t);
		pack_intx(b, n, int8_t);
	} else if (INT16_MIN <= n && n <= INT16_MAX) {
		t = COMBINE_TYPE(TYPE_INT, TYPE_INT_16);
		wb_addchar(b, t);
		pack_intx(b, n, int16_t);
	} else if (INT32_MIN <= n && n <= INT32_MAX) {
		t = COMBINE_TYPE(TYPE_INT, TYPE_INT_32);
		wb_addchar(b, t);
		pack_intx(b, n, int32_t);
	} else {
		t = COMBINE_TYPE(TYPE_INT, TYPE_INT_64);
		wb_addchar(b, t);
		pack_intx(b, n, int64_t);
	}
}

static inline void pack_str(WriteBuffer *b, const char *str, size_t sz) {
	if (sz <= MAX_COOKIE) {
		char t = COMBINE_TYPE(TYPE_SSTR, sz);
		wb_addchar(b, t);
		wb_addbuffer(b, str, sz);
	} else {
		char t;
		if (sz <= INT8_MAX) {
			t = COMBINE_TYPE(TYPE_LSTR, sizeof(int8_t));
			wb_addchar(b, t);
			pack_intx(b, sz, int8_t);
		} else if (sz <= INT16_MAX) {
			t = COMBINE_TYPE(TYPE_LSTR, sizeof(int16_t));
			wb_addchar(b, t);
			pack_intx(b, sz, int16_t);
		} else if (sz <= INT32_MAX) {
			t = COMBINE_TYPE(TYPE_LSTR, sizeof(int32_t));
			wb_addchar(b, t);
			pack_intx(b, sz, int32_t);
		} else {
			t = COMBINE_TYPE(TYPE_LSTR, sizeof(int64_t));
			wb_addchar(b, t);
			pack_intx(b, sz, int64_t);
		}
		wb_addbuffer(b, str, sz);
	}
}

static int pack_array(lua_State *L, WriteBuffer *b, int index, int depth) {
	int array_size = lua_rawlen(L, index);
	if (array_size >= MAX_COOKIE) {
		char t = COMBINE_TYPE(TYPE_TABLE, MAX_COOKIE);
		wb_addchar(b, t);
		pack_int(b, array_size);
	} else {
		char t = COMBINE_TYPE(TYPE_TABLE, array_size);
		wb_addchar(b, t);
	}
	int i;
	for (i=1; i <= array_size; i++) {
		lua_rawgeti(L, index, i);
		pack_value(L, b, lua_gettop(L), depth);
		lua_pop(L, 1);
	}
	return array_size;
}

static void pack_hash(lua_State *L, WriteBuffer *b, int index, int depth, int array_size) {
	lua_pushnil(L);
	while (lua_next(L, index) != 0) {
		if (lua_type(L,-2) == LUA_TNUMBER) {
			if (lua_isinteger(L, -2)) {
				lua_Integer x = lua_tointeger(L, -2);
				if (x > 0 && x <= array_size) {
					lua_pop(L,1);
					continue;
				}
			}
		}
		pack_value(L, b, lua_gettop(L)-1, depth);
		pack_value(L, b, lua_gettop(L), depth);
		lua_pop(L, 1);
	}
	wb_addchar(b, TYPE_NIL);
}

static void pack_table(lua_State *L, WriteBuffer *b, int index, int depth) {
	luaL_checkstack(L, LUA_MINSTACK, NULL);
	int array_size = pack_array(L, b, index, depth);
	pack_hash(L, b, index, depth, array_size);
}

static inline void pack_pointer(lua_State *L, WriteBuffer *b, const void *ud) {
	void *p = wb_prepbuffsize(b, sizeof(ud));
	memcpy(p, &ud, sizeof(ud));
}

static void pack_value(lua_State *L, WriteBuffer *b, int index, int depth) {
	if (depth > MAX_DEPTH) {
		wb_free(b);
		luaL_error(L, "The maximum depth of the table is %s", depth);
	}
	int luatype = lua_type(L, index);
	switch (luatype)
	{
	case LUA_TNIL: {
		wb_addchar(b, TYPE_NIL);
		break;
	}
	case LUA_TBOOLEAN: {
		char t = COMBINE_TYPE(TYPE_BOOL, lua_toboolean(L, index));
		wb_addchar(b, t);
		break;
	}
	case LUA_TNUMBER: {
		if (lua_isinteger(L, index)) {
			pack_int(b, (int64_t)lua_tointeger(L, index));
		} else {
			wb_addchar(b, TYPE_FLOAT);
			pack_double(b, (double)lua_tonumber(L, index));
		}
		break;
	}
	case LUA_TSTRING: {
		size_t sz = 0;
		const char *str = lua_tolstring(L, index, &sz);
		pack_str(b, str, sz);
		break;
	}
	case LUA_TTABLE: {
		pack_table(L, b, index, depth+1);
		break;
	}
	case LUA_TLIGHTUSERDATA: {
		wb_addchar(b, TYPE_USERDATA);
		pack_pointer(L, b, lua_touserdata(L, index));
		break;
	}
	default:
		wb_free(b);
		luaL_error(L, "Unsupport type %s", lua_typename(L, luatype));
		break;
	}
}

//---------------------------------------------------------------------------------------------
// unpack

typedef struct ReadBuffer {
	const char *buff;
	size_t len;
	size_t pos;
} ReadBuffer;

static inline void _invalid_stream(lua_State *L, ReadBuffer *b, int line) {
	int len = (int)b->len;
	luaL_error(L, "Invalid serialize stream %d (line:%d)", len, line);
}

#define invalid_stream(L, b) _invalid_stream(L, b, __LINE__)

static void unpack_value(lua_State *L, ReadBuffer *b);

static inline const char* rb_prepbuffsize(lua_State *L, ReadBuffer *b, size_t sz) {
	if (b->pos + sz <= b->len) {
		const char *p = b->buff + b->pos;
		b->pos += sz;
		return p;
	} else {
		invalid_stream(L, b);
		return NULL;
	}
}

static inline int unpack_type(lua_State *L, ReadBuffer *b) {
	const char *p = rb_prepbuffsize(L, b, sizeof(char));
	return *p;
}

static uint64_t _unpack_intx(lua_State *L, ReadBuffer *b, size_t sz) {
	const char *p = rb_prepbuffsize(L, b, sz);
	uint64_t n = 0;
	if (nativeendian.little) {
		memcpy(&n, p, sz);
	} else {
		n = 0;
		int i;
		for (i = sz-1; i >= 0; --i) {
			n <<= CHAR_BIT;
			n |= (uint64_t)(unsigned char)p[i];
		}
	}
	return n;
}
#define unpack_intx(L, b, t) (t)_unpack_intx(L, b, sizeof(t))

static lua_Integer unpack_int(lua_State *L, ReadBuffer *b, int cookie) {
	switch(cookie) {
		case TYPE_INT_0: return 0;
		case TYPE_INT_8: return (lua_Integer)unpack_intx(L, b, int8_t);
		case TYPE_INT_16: return (lua_Integer)unpack_intx(L, b, int16_t);
		case TYPE_INT_32: return (lua_Integer)unpack_intx(L, b, int32_t);
		case TYPE_INT_64: return (lua_Integer)unpack_intx(L, b, int64_t);
		default: invalid_stream(L, b);
	}
	return 0;
}

static lua_Number unpack_float(lua_State *L, ReadBuffer *b) {
	size_t size = sizeof(double);
	const char *p = rb_prepbuffsize(L, b, size);
	double n;
	if (nativeendian.little) {
		memcpy(&n, p, size);
	} else {
		char *dest = (char*)&n;
		dest += size - 1;
		while (size-- != 0)
			*(dest--) = *(p++);
	}
	return (lua_Number)n;
}

static void* unpack_pointer(lua_State *L, ReadBuffer *b) {
	void *ud;
	const char *p = rb_prepbuffsize(L, b, sizeof(ud));
	memcpy(&ud, p, sizeof(ud));
	return ud;
}

static inline const char* unpack_str(lua_State *L, ReadBuffer *b, size_t sz) {
	return rb_prepbuffsize(L, b, sz);
}

static void unpack_table(lua_State *L, ReadBuffer *b, int cookie) {
	int array_size;
	if (cookie == MAX_COOKIE) {
		int type = unpack_type(L, b);
		int t = TYPE_TYPE(type);
		int cookie = TYPE_COOKIE(type);
		if (t != TYPE_INT) {
			invalid_stream(L, b);
		}
		array_size = (int)unpack_int(L, b, cookie);
	} else {
		array_size = cookie;
	}
	luaL_checkstack(L, LUA_MINSTACK, NULL);
	lua_createtable(L, array_size, 0);
	int i;
	for (i = 1; i <= array_size; ++i) {
		unpack_value(L, b);
		lua_rawseti(L, -2, i);
	}
	while (1) {
		unpack_value(L, b);
		if (!lua_isnil(L, -1)) {
			unpack_value(L, b);
			lua_rawset(L, -3);
		} else {
			lua_pop(L, 1);
			break;
		}
	}
}

static void unpack_value(lua_State *L, ReadBuffer *b) {
	int type = unpack_type(L, b);
	int t = TYPE_TYPE(type);
	int cookie = TYPE_COOKIE(type);
	switch(t) {
	case TYPE_NIL: {
		lua_pushnil(L);
		break;
	}
	case TYPE_BOOL: {
		lua_pushboolean(L, cookie);
		break;
	}
	case TYPE_INT: {
		lua_pushinteger(L, unpack_int(L, b, cookie));
		break;
	}
	case TYPE_FLOAT: {
		lua_pushnumber(L, unpack_float(L, b));
		break;
	}
	case TYPE_SSTR: {
		lua_pushlstring(L, unpack_str(L, b, cookie), cookie);
		break;
	}
	case TYPE_LSTR: {
		size_t sz;
		if (cookie == 1) {
			sz = (size_t)unpack_intx(L, b, int8_t);
		} else if (cookie == 2) {
			sz = (size_t)unpack_intx(L, b, int16_t);
		} else if (cookie == 4) {
			sz = (size_t)unpack_intx(L, b, int32_t);
		} else {
			sz = (size_t)unpack_intx(L, b, int64_t);
		}
		lua_pushlstring(L, unpack_str(L, b, sz), sz);
		break;
	}
	case TYPE_TABLE: {
		unpack_table(L, b, cookie);
		break;
	}
	case TYPE_USERDATA: {
		lua_pushlightuserdata(L, unpack_pointer(L, b));
		break;
	}
	default:
		invalid_stream(L, b);
		break;
	}
}

//---------------------------------------------------------------------------------------------

static int l_pack(lua_State *L) {
	WriteBuffer b;
	wb_buffinit(&b);
	int n = lua_gettop(L);
	int i;
	for (i = 1; i <= n; i++) {
		pack_value(L, &b , i, 0);
	}
	lua_pushlstring(L, b.buff, b.size);
	wb_free(&b);
	return 1;
}

static int l_unpack(lua_State *L) {
	size_t sz;
	const char *buff = luaL_checklstring(L, 1, &sz);
	ReadBuffer b = {buff, sz, 0};
	while (b.pos < b.len) {
		unpack_value(L, &b);
	}
	return lua_gettop(L) - 1;
}

static const luaL_Reg lib[] = {
	{"pack", l_pack},
	{"unpack", l_unpack},
	{NULL, NULL},
};

LUAMOD_API int luaopen_colib_seri(lua_State *L) {
	luaL_newlib(L, lib);
	return 1;
}