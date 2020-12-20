
/**
 * 一个高效稳定的list实现，list是一个userdata，里面存放的是值的“引用”，真正的值放在一个table中。
 * 这个table关联在list的uservalue。通过list的元素引用来访问table的内容。
 * 
 * @author colin
 * @date 2020/8/10
 */
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"

typedef struct refdata_t {
	int ref;
	int key;
} refdata_t;

typedef struct list_t {
	int cap;
	int size;
	int ref;
	refdata_t *ary;
} list_t;

#define LIST_MT "CO_LIST_MT"
#define checklist(L) (list_t*)luaL_checkudata(L, 1, LIST_MT)

//------------------------------------------------------------------------------------------------
/* index of free-list header */
#define freelist	0

int ref_value(lua_State *L, list_t *ls, int t) {
	int ref;
	t = lua_absindex(L, t);
	lua_rawgeti(L, t, freelist);	 /* get first free element */
	ref = (int)lua_tointeger(L, -1); /* ref = t[freelist] */
	lua_pop(L, 1);					 /* remove it from stack */
	if (ref != 0) {					 /* any free element? */
		lua_rawgeti(L, t, ref);		 /* remove it from list */
		lua_rawseti(L, t, freelist); /* (t[freelist] = t[ref]) */
	}
	else				 /* no free elements */
		ref = ++ls->ref; /* get a new reference */
	lua_rawseti(L, t, ref);
	return ref;
}

void unref_value(lua_State* L, int t, int ref) {
	if (ref >= 0) {
		t = lua_absindex(L, t);
		lua_rawgeti(L, t, freelist);
		lua_rawseti(L, t, ref); /* t[ref] = t[freelist] */
		lua_pushinteger(L, ref);
		lua_rawseti(L, t, freelist); /* t[freelist] = ref */
	}
}
//------------------------------------------------------------------------------------------------

// list.new([cap]) -> ud
static int list_new(lua_State *L) {
	int cap = (int)luaL_optinteger(L, 1, 4);
	luaL_argcheck(L, cap > 0, 1, "invalid size");
	list_t *ls = (list_t*)lua_newuserdata(L, sizeof(list_t));			// <ud>
	ls->cap = cap;
	ls->size = 0;
	ls->ref = 0;
	ls->ary = (refdata_t*)malloc(ls->cap * sizeof(refdata_t));
	luaL_setmetatable(L, LIST_MT);		// <ud>
	lua_createtable(L, ls->cap, 0);		// <ud|tab>
	lua_setuservalue(L, -2);			// <ud>
	return 1;
}

static void check_and_grow_size(list_t *ls, int n) {
	int newcap = ls->cap;
	while (ls->size + n > newcap)
		newcap <<= 1;
	if (newcap != ls->cap) {
		ls->ary = (refdata_t*)realloc(ls->ary, newcap*sizeof(refdata_t));
		ls->cap = newcap;
	}
}

// list.insert(ls, [pos,] value)
static int list_insert(lua_State *L) {
	list_t *ls = checklist(L);
	int pos, n, vidx;
	if (lua_gettop(L) == 2) {
		pos = ls->size;
		vidx = 2;
	} else {
		pos = (int)luaL_checkinteger(L, 2) - 1;
		vidx = 3;
	}
	if (lua_isnoneornil(L, vidx))
		luaL_argerror(L, vidx, "value can not be nil");
	luaL_argcheck(L, 0 <= pos && pos <= ls->size, 2, "index out of range");

	check_and_grow_size(ls, 1);
	if (pos != ls->size)
		memmove(ls->ary + pos + 1, ls->ary + pos, (ls->size - pos) * sizeof(refdata_t));

	lua_getuservalue(L, 1);		// <ls|...|uv>
	lua_pushvalue(L, vidx);		// <ls|...|uv|v>
	ls->ary[pos].ref = ref_value(L, ls, -2);		// <ls|...|uv>
	ls->size++;
	return 0;
}

// list.remove(ls[, pos]) -> v
static int list_remove(lua_State *L) {
	list_t *ls = checklist(L);
	int pos = luaL_optinteger(L, 2, ls->size) - 1;
	luaL_argcheck(L, 0 <= pos && pos < ls->size, 2, "index out of range");
	int ref = ls->ary[pos].ref;
	if (pos != ls->size - 1)
		memmove(ls->ary + pos, ls->ary + pos + 1, (ls->size - pos - 1) * sizeof(refdata_t));
	ls->size--;

	lua_getuservalue(L, 1);		// <ls|...|uv>
	lua_rawgeti(L, -1, ref);	// <ls|...|uv|v>
	unref_value(L, -2, ref);
	return 1;
}

// list.totable(ls[, i [, j]]) -> table
static int list_totable(lua_State *L) {
	list_t *ls = checklist(L);

	int s = luaL_optinteger(L, 2, 1) - 1;
	int e = luaL_optinteger(L, 3, ls->size) - 1;
	if (s < 0) s = 0;
	if (e >= ls->size) e = ls->size-1;

	if (s > e) {
		lua_newtable(L);					// <ls|t>
	} else {
		lua_createtable(L, e-s+1, 0);		// <ls|t>
		lua_getuservalue(L, 1);				// <ls|t|uv>
		int i, ref;
		for (i = s; i <= e; ++i) {
			ref = ls->ary[i].ref;
			lua_rawgeti(L, -1, ref);		// <ls|t|uv|v>
			lua_rawseti(L, -3, i-s+1);		// <ls|t|uv>
		}
		lua_pop(L, 1);						// <ls|t>
	}
	return 1;
}

// list.fromtable(ls, t, [, i [, j]])
static int list_fromtable(lua_State *L) {
	list_t *ls = checklist(L);
	luaL_checktype(L, 2, LUA_TTABLE);

	ls->size = 0;
	ls->ref = 0;
	lua_newtable(L);			// <ls|..|t>
	lua_setuservalue(L, 1);		// <ls|...>

	int n = (int)luaL_len(L, 2);
	int s = (int)luaL_optinteger(L, 3, 1);
	int e = (int)luaL_optinteger(L, 4, n);
	if (s < 1) s = 1;
	if (e > n) e = n;
	if (e < s) return 0;

	n = e-s+1;
	check_and_grow_size(ls, n);
	lua_getuservalue(L, 1);			// <ls|t|...|uv>
	int i;
	for (i = s; i <= e; ++i) {
		lua_geti(L, 2, i);		// <ls|t|...|uv|v>
		ls->ary[ls->size++].ref = ref_value(L, ls, -2);		// <ls|...|uv>
	}
	return 0;
}

// list.extend(ls, ls2|tab)
static int list_extend(lua_State *L) {
	list_t *ls = checklist(L);
	int tp = lua_type(L, 2);
	if (tp == LUA_TTABLE) {		// <ls|t>
		int n = luaL_len(L, 2);
		check_and_grow_size(ls, n);
		lua_getuservalue(L, 1);		// <ls|t|uv>
		int i, ref;
		for (i = 0; i < n; ++i) {
			lua_geti(L, 2, i+1);		// <ls|t|uv|v>
			ref = ref_value(L, ls, 3);		// <ls|t|uv>
			ls->ary[ls->size++].ref = ref;
		}
	} else if (tp == LUA_TUSERDATA) {	// <ls|ls2>
		list_t *ls2 = (list_t*)luaL_checkudata(L, 2, LIST_MT);
		int n = ls2->size;
		check_and_grow_size(ls, n);
		lua_getuservalue(L, 1);		// <ls|ls2|uv>
		lua_getuservalue(L, 2);		// <ls|ls2|uv|uv2>
		int i, ref;
		for (i = 0; i < n; ++i) {
			lua_rawgeti(L, 4, ls2->ary[i].ref);	// <ls|ls2|uv|uv2|v>
			ref = ref_value(L, ls, 3);			// <ls|ls2|uv|uv2>
			ls->ary[ls->size++].ref = ref;
		}
	} else {
		luaL_argerror(L, 2, "muse be a table or list");
	}
	return 0;
}

// list.indexof(ls, v) -> number|nil
static int list_indexof(lua_State *L) {
	list_t *ls = checklist(L);
	lua_getuservalue(L, 1);		// <ls|v|uv>
	int idx = -1;
	int i;
	for (int i = 0; i < ls->size; ++i) {
		lua_rawgeti(L, 3, ls->ary[i].ref);	// <ls|v|uv|v>
		if (lua_compare(L, 2, 4, LUA_OPEQ)) {
			idx = i;
			break;
		}
		lua_pop(L, 1);
	}
	if (idx < 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, idx+1);
	return 1;
}

// ls[idx] = v
static int list_newindex(lua_State *L) {
	list_t *ls = checklist(L);
	int idx = (int)luaL_checkinteger(L, 2);
	if (idx > 0) idx--; 
	else if (idx < 0) idx += ls->size;
	luaL_argcheck(L, 0 <= idx && idx <= ls->size, 2, "index out of range");
	if (lua_isnoneornil(L, 3))
		luaL_argerror(L, 3, "value can not be nil");

	lua_getuservalue(L, 1);		// <ls|...|uv>
	lua_pushvalue(L, 3);		// <ls|...|uv|v>
	if (idx == ls->size) {
		check_and_grow_size(ls, 1);
		ls->ary[ls->size++].ref = ref_value(L, ls, -2);
	} else {
		lua_rawseti(L, -2, ls->ary[idx].ref);
	}
	return 0;
}

// ls[idx]
static int list_index(lua_State *L) {
	list_t *ls = checklist(L);
	int idx = (int)luaL_checkinteger(L, 2);
	if (idx > 0) idx--; 
	else if (idx < 0) idx += ls->size;
	if (idx < 0 || idx >= ls->size) {
		lua_pushnil(L);
		return 1;
	}
	int ref = ls->ary[idx].ref;
	lua_getuservalue(L, 1);		// <ls|idx|t>
	lua_rawgeti(L, -1, ref);	// <ls|idx|t|v>
	return 1;
}

// #ls
static int list_len(lua_State *L) {
	list_t *ls = checklist(L);
	lua_pushinteger(L, ls->size);
	return 1;
}

// free
static int list_gc(lua_State *L) {
	list_t *ls = checklist(L);
	free(ls->ary);
	return 0;
}

// tostring(ls)
static int list_tostring(lua_State *L) {
	list_t *ls = checklist(L);
	lua_pushfstring(L, "list: %p", ls);
	return 1;
}

// list.clear(ls[, shink])
static int list_clear(lua_State *L) {
	list_t *ls = checklist(L);
	int shink = lua_toboolean(L, 2);
	ls->size = 0;
	ls->ref = 0;
	if (shink) {
		ls->cap = 4;
		ls->ary = (refdata_t*)realloc(ls->ary, ls->cap * sizeof(refdata_t));
	}
	lua_createtable(L, ls->cap, 0);
	lua_setuservalue(L, 1);
	return 0;
}

// list.exchange(ls, idx1, idx2)
static int list_exchange(lua_State *L) {
	list_t *ls = checklist(L);
	int idx1 = (int)luaL_checkinteger(L, 2) - 1;
	int idx2 = (int)luaL_checkinteger(L, 3) - 1;
	if (idx1 == idx2) return 0;

	luaL_argcheck(L, 0 <= idx1 && idx1 < ls->size, 2, "index out of range");
	luaL_argcheck(L, 0 <= idx2 && idx2 < ls->size, 3, "index out of range");
	refdata_t ref = ls->ary[idx1];
	ls->ary[idx1] = ls->ary[idx2];
	ls->ary[idx2] = ref;
	return 0;
}

static void addfield(lua_State* L, luaL_Buffer* b, lua_Integer i, int uvidx, int ref) {
	lua_geti(L, uvidx, ref);
	if (!lua_isstring(L, -1))
		luaL_error(L, "invalid value (%s) at index %d in table for 'concat'",
			luaL_typename(L, -1), i);
	luaL_addvalue(b);
}

// list.concat(ls [, sep [, i [, j]]])
static int list_concat(lua_State *L) {
	list_t *ls = checklist(L);
	size_t lsep;
	const char *sep = luaL_optlstring(L, 2, "", &lsep);
	lua_Integer i = luaL_optinteger(L, 3, 1);
	lua_Integer last = luaL_optinteger(L, 4, ls->size);
	lua_getuservalue(L, 1);		// <ls|...|uv>
	int uvidx = lua_gettop(L);
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for (; i < last; i++) {
		addfield(L, &b, i, uvidx, ls->ary[i-1].ref);
		luaL_addlstring(&b, sep, lsep);
	}
	if (i == last)
		addfield(L, &b, i, uvidx, ls->ary[i-1].ref);
  	luaL_pushresult(&b);
	return 1;
}

//------------------------------------------------------------------------------------------------
// sort
#define M_LT 0
#define M_CMP 1
#define M_KEY 2
#define swap_value(x,y)  do {refdata_t tmp = (x); (x) = (y); (y) = tmp;} while(0)
#define sort_comp(L, mode, a, b) mode == M_KEY ? (int)(a.key < b.key) : _sort_comp(L, mode, a, b)

// ls=1, cmp=2, key=3, uv=4
static int _sort_comp(lua_State *L, int mode, refdata_t a, refdata_t b) {
	int res;
	if (mode ==  M_CMP) {
		lua_pushvalue(L, 2);	// <cmp>
		lua_geti(L, 4, a.ref);	// <cmp|a>
		lua_geti(L, 4, b.ref);	// <cmp|a|b>
		lua_call(L, 2, 1);      // <res>
		res = lua_toboolean(L, -1);
		lua_pop(L, 1);
		return res;
	} else {
		lua_geti(L, 4, a.ref);		// <a>
		lua_geti(L, 4, b.ref);		// <a|b>
		res = lua_compare(L, -2, -1, LUA_OPLT);
		lua_pop(L, 2);
		return res;
	}
}

static int choose_pivot(int lo, int up, unsigned int rnd) {
	int r4 = (up - lo) >> 2;
	int p = rnd % (r4 << 1) + (lo + r4);
	return p;
}

static int partition(lua_State *L, int mode, refdata_t* ary, int lo, int up, int p) {
	refdata_t value = ary[p];
	swap_value(ary[p], ary[up-1]);	// a[lo] <= P == a[up-1] <= a[up]
	
	int i = lo;
	int j = up - 1;
	for (;;) {
		while (++i, sort_comp(L, mode, ary[i], value));
		while (--j, sort_comp(L, mode, value, ary[j]));
		if (j < i) {
			swap_value(ary[up-1], ary[i]);
			return i;
		}
		swap_value(ary[i], ary[j]);
	}
}

static void quick_sort(lua_State *L, int mode, refdata_t* a, int lo, int up, unsigned int rnd) {
	int p;
	int n;
	while (lo < up) {
		if (sort_comp(L, mode, a[up], a[lo]))
			swap_value(a[up], a[lo]);
		if (up - lo == 1)		// 只有2个元素
			return;

		if (up - lo < 100 || rnd == 0)
			p = (lo + up) >> 1;
		else
			p = choose_pivot(lo, up, rnd);
		
		if (sort_comp(L, mode, a[p], a[lo])) {
			swap_value(a[p], a[lo]);
		} else if (sort_comp(L, mode, a[up], a[p])) {
			swap_value(a[p], a[up]);
		}
		if (up - lo == 2)	// 只有3个元素
			return;

		// 前提条件：a[lo] <= a[p] <= a[up]
		p = partition(L, mode, a, lo, up, p);
		if (p - lo < up - p) {
			quick_sort(L, mode, a, lo, p - 1, rnd);	// 低部分小，递归之
			n = p - lo;
			lo = p + 1;	// 高部分循环
		} else {
			quick_sort(L, mode, a, p + 1, up, rnd);	// 高部分小，递归之
			n = up - p;
			up = p - 1;	// 低部分循环
		}
		if ((up - lo) / 128 > n)	// 高低不太不平衡，用随机的中枢
			rnd = 0x7FFFFFFF;
	}
}

static void repare_keys(lua_State *L, list_t *ls) {
	int i, ref;
	int isnum;
	for (i = 0; i < ls->size; ++i) {
		ref = ls->ary[i].ref;
		lua_pushvalue(L, 3);	// <key>
		lua_geti(L, 4, ref);	// <key|a>
		lua_call(L, 1, 1);      // <res>
		lua_Integer d = lua_tointegerx(L, -1, &isnum);
		if (!isnum || d > INT_MAX || d < INT_MIN)
			luaL_error(L, "key function must return int value");
		ls->ary[i].key = (int)d;
		lua_pop(L, 1);
	}
}

// list.sort(ls [, cmp [, key]])
static int list_sort(lua_State *L) {
	list_t *ls = checklist(L);
	if (ls->size > 1) {
		lua_settop(L, 3);
		lua_getuservalue(L, 1);
		int mode = M_LT;
		if (!lua_isnoneornil(L, 3)) {
			luaL_checktype(L, 3, LUA_TFUNCTION);
			repare_keys(L, ls);
			mode = M_KEY;
		} else if (!lua_isnoneornil(L, 2)) {
			luaL_checktype(L, 2, LUA_TFUNCTION);
			mode = M_CMP;
		}
		quick_sort(L, mode, ls->ary, 0, ls->size-1, 0);
	}
	return 0;
}
//------------------------------------------------------------------------------------------------


// functions for list library
static const luaL_Reg lib[] = {
	// list.new([cap]) -> ud 新建一个list并返回，可选的cap为预分配的容量
	{"new", list_new},
	// list.insert(ls, [pos,] value) 插入value。
	// 如果没有pos则插入到最后，否则插入到pos
	{"insert", list_insert},
	// list.remove(ls[, pos]) -> v 删除一个值，则返回这个值
	// 如果没有pos则删除最后的值，否则删除pos位置的值
	{"remove", list_remove},
	// list.totable(ls[, i [, j]]) -> table 将list保存转换为table返回
	// i, j为保存的范围，默认i=1, j=#ls
	{"totable", list_totable},
	// list.fromtable(ls, t, [, i [, j]]) 从table加载值到ls
	// i, j为加载的范围，默认i=1, j=#ls
	{"fromtable", list_fromtable},
	// list.extend(ls, ls2|t) 将ls2或t的值追加到ls尾
	// ls2为列表，t为table
	{"extend", list_extend},
	// list.indexof(ls, v) -> number|nil 返回v在ls中的索引，如果找不到返回nil
	{"indexof", list_indexof},
	// list.concat(ls [, sep [, i [, j]]]) -> string 将ls的内容串连成字符串返回
	// sep为分隔符，默认是空；i,j为范围，默认i=1, j=#ls
	{"concat", list_concat},
	// list.clear(ls[, shink]) 清空ls的内容，shink指定是否收缩内存
	{"clear", list_clear},
	// list.exchange(ls, idx1, idx2) 交换两个值的位置
	{"exchange", list_exchange},
	// list.sort(ls [, cmp [, key]]) 原地排序ls
	// cmp为比较函数，function(a, b) -> boolean，a < b 返回true
	// key函数返回元素的整型值用于排序，function(v) -> number
	// 如果key函数指定，则优先用key
	// 如果cmp函数指定，则用cmp
	// 否则用默认的a < b比较
	{"sort", list_sort},
	{NULL, NULL}
};

// functions for list metatable
static const luaL_Reg mlib[] = {
	// ls[i] = v 设值，v不能是nil
	// ls[#ls+1] = v 可增加项
	{"__newindex", list_newindex},
	// v = ls[i]，取值
	{"__index", list_index},
	// #ls 取长度
	{"__len", list_len},
	// gc时自动释放内容
	{"__gc", list_gc},
	// tostring(ls)
	{"__tostring", list_tostring},
	{NULL, NULL}
};

LUAMOD_API int luaopen_colua_list(lua_State *L) {
	luaL_checkversion(L);
	luaL_newmetatable(L, LIST_MT);
	luaL_setfuncs(L, mlib, 0);
	luaL_newlib(L, lib);
	return 1;
}