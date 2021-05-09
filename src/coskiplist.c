/**
 * skiplist的实现
 */
#define LUA_LIB
#include <stdio.h>
#include <stdlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"

#define SKIPLIST "CO_SKIP_LIST"
#define toskiplist(L) ((skiplist_t*)luaL_checkudata((L), 1, SKIPLIST))
#define MAX_LEVEL 32

struct slnode;

// 代表一下层次
typedef struct sllevel {
	struct slnode *next;
	size_t span;
} sllevel_t;

// 结点包含一个score用于排序，排序规则：
// 		1. score 大的排在前面
//		2. score 相等的，先进来的排在前面，慢进来的排在后面
// span代表当前结点到下一结点的跨度，用于通过排名找到结点
typedef struct slnode {
	lua_Integer score;
	struct slnode *prev;
	sllevel_t level[];
} slnode_t;

// skiplist结构
typedef struct skiplist {
	slnode_t *head;
	slnode_t *tail;
	size_t length;
	int level;
} skiplist_t;

static slnode_t* _sl_createnode(int level, lua_Integer score) {
	slnode_t *node = (slnode_t*)co_malloc(sizeof(slnode_t) + level * sizeof(sllevel_t));
	node->score = score;
	return node;
}

static int sl_new(lua_State *L) {
	skiplist_t *sl = (skiplist_t *)co_newuserdata(L, sizeof(skiplist_t));
	luaL_setmetatable(L, SKIPLIST);

	sl->level = 1;
	sl->length = 0;
	sl->head = _sl_createnode(MAX_LEVEL, 0);
	int i;
	for (i = 0; i < MAX_LEVEL; i++) {
		sl->head->level[i].next = NULL;
		sl->head->level[i].span = 0;
	}
	sl->head->prev = NULL;
	sl->tail = NULL;
	return 1;
}

static int sl_gc(lua_State *L) {
	return 0;
}

static int sl_tostring(lua_State *L) {
	skiplist_t *sl = toskiplist(L);
	lua_pushfstring(L, "skiplist: %p", sl);
	return 1;
}

// 模块函数
static const luaL_Reg lib[] = {
	{"new", sl_new}, 
	{NULL, NULL}
};

// 元表
static const luaL_Reg mt[] = {
	{"__index", NULL},
	{"__gc", sl_gc},
	{"__tostring", sl_tostring},
	{NULL, NULL}
};

// skiplist 对象元方法
static const luaL_Reg mtmeth[] = {
	{NULL, NULL}
};

LUAMOD_API int luaopen_colib_skiplist(lua_State *L) {
	luaL_newlib(L, lib);		// skiplist lib

	luaL_newmetatable(L, SKIPLIST);	// skiplib metatable
	luaL_setfuncs(L, mt, 0);
	luaL_newlibtable(L, mtmeth);
	luaL_setfuncs(L, mtmeth, 0);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}