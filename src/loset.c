/**
 * 有序集合：用skiplist实现，集合有三元素：
 * 		- rank 排名，从1开始，分数越高排名越前
 * 		- score 分数，分数可以重复，后面加进来的排在同分数的后面
 * 		- value 是一个Lua对象，在集合中必须唯一
 */
#define LUA_LIB
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"
#include "corand.h"

#define ORDERED_SET "CO_ORDERED_SET"
#define to_ordered_set(L) ((oset_t*)luaL_checkudata((L), 1, ORDERED_SET))
#define MAX_LEVEL 32
#define LEVEL_P (double)0.3

struct osetnode;

// 代表结点层级
typedef struct osetlevel {
	struct osetnode *next;
	size_t span;		// 跨度，用于计算排名
} osetlevel_t;

// 结点
typedef struct osetnode {
	lua_Integer score;			// 分数
	size_t seq;					// 用于确定相同分数结点的先后顺序，值越大排得越后
	int nlv;					// 结点的层级
	struct osetnode *prev;		// 前一个结点
	osetlevel_t level[];		// 层级数组
} osetnode_t;

// 有序集合
typedef struct oset {
	osetnode_t *head;		// 链表头
	size_t length;			// 集合长度
	size_t seq;				// 用于生成结点的seq
	int level;				// 集合最高层级
} oset_t;

// 创建结点
static inline osetnode_t* _oset_createnode(oset_t *oset, int level, lua_Integer score) {
	osetnode_t *node = (osetnode_t*)co_malloc(sizeof(osetnode_t) + level * sizeof(osetlevel_t));
	node->score = score;
	node->nlv = level;
	node->seq = oset->seq++;
	return node;
}

// 创建集合 oset.new() -> osetud
static int oset_new(lua_State *L) {
	oset_t *oset = (oset_t *)lua_newuserdata(L, sizeof(oset_t));		// S: osetud
	luaL_setmetatable(L, ORDERED_SET);
	lua_newtable(L);			// S: osetud, dict
	lua_setuservalue(L, -2);	// S: osetud

	oset->level = 1;
	oset->length = 0;
	oset->seq = 0;
	oset->head = _oset_createnode(oset, MAX_LEVEL, 0);
	int i;
	for (i = 0; i < MAX_LEVEL; i++) {
		oset->head->level[i].next = NULL;
		oset->head->level[i].span = 0;
	}
	oset->head->prev = NULL;
	return 1;
}

// 集合GC
static int oset_gc(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	osetnode_t *node = oset->head;
	osetnode_t *temp;
	while (node) {
		temp = node;
		node = node->level[0].next;
		co_free(temp);
	}
	return 0;
}

static int oset_tostring(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	lua_pushfstring(L, "oset: %p", oset);
	return 1;
}

// 取集合长度: #oset 或 oset:getlen()
static int oset_getlen(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	lua_pushinteger(L, (lua_Integer)oset->length);
	return 1;
}

// 生成结点的级数，概率是LEVEL_P
static inline int _oset_randomlevel(lua_State *L) {
	randstate_t *state = (randstate_t*)lua_touserdata(L, lua_upvalueindex(1));
	int level = 1;
	while (level < MAX_LEVEL && ((double)(randnext(state) & 0xFFFFFFFF) < LEVEL_P * 0xFFFFFFFF))
		level += 1;
	return level;
}

// 将结点加到链表
static void _oset_add_to_link(oset_t *oset, osetnode_t *node) {
	osetnode_t *update[MAX_LEVEL];
	size_t rank[MAX_LEVEL];
	int i;
	// 扩展层级
	int level = node->nlv;
	if (level > oset->level) {
		for (i = oset->level; i < level; ++i) {
			oset->head->level[i].span = oset->length;
		}
		oset->level = level;
	}
	// 找到插入的位置
	osetnode_t *xn;
	lua_Integer score = node->score;
	size_t seq = node->seq;
	xn = oset->head;
	for (i = oset->level-1; i >= 0; i--) {
		rank[i] = i == (oset->level-1) ? 0 : rank[i+1];
		while (xn->level[i].next && 
			(xn->level[i].next->score > score ||
				(xn->level[i].next->score == score && xn->level[i].next->seq < seq))) {
			rank[i] += xn->level[i].span;
			xn = xn->level[i].next;
		}
		update[i] = xn;
	}
	for (i = 0; i < level; i++) {
		// 将结点插入到链表
		node->level[i].next = update[i]->level[i].next;
		update[i]->level[i].next = node;
		// 更新跨度
		node->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
		update[i]->level[i].span = (rank[0] - rank[i]) + 1;
	}
	// 增加未链接层级的跨度
	for (i = level; i < oset->level; ++i) {
		update[i]->level[i].span++;
	}
	node->prev = (update[0] == oset->head) ? NULL : update[0];
	if (node->level[0].next)
		node->level[0].next->prev = node;
	oset->length++;
}

// 插入结点： osetud:add(value, score) -> boolean
// 如果value已经存在，则返回false
static int oset_add(lua_State *L) {		// S: oset val score
	oset_t *oset = to_ordered_set(L);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "can not be nil");
	lua_Integer score = luaL_checkinteger(L, 3);

	// 看看value是否已经存在
	lua_getuservalue(L, 1);				// S: oset val score dict
	lua_pushvalue(L, 2);				// S: oset val score dict val
	if (lua_rawget(L, -2) == LUA_TLIGHTUSERDATA) {	// S: oset val score dict node
		luaL_pushfail(L);
		return 1;
	}
	lua_pop(L, 1);						// S: oset val score dict

	// 创建结点
	int level = _oset_randomlevel(L);
	osetnode_t *node = _oset_createnode(oset, level, score);
	_oset_add_to_link(oset, node);

	// 加到字典: dict[value] = node, dict[node] = value
	lua_pushvalue(L, 2);			// S: osetud val score dict val
	lua_pushlightuserdata(L, node); // S: osetud val score dict val node
	lua_rawset(L, -3);				// S: osetud val score dict
	lua_pushvalue(L, 2);			// S: osetud val score dict val
	lua_rawsetp(L, -2, node);		// S: osetud val score dict

	lua_pushboolean(L, 1);
	return 1;
}

// 将结点从链表删除
static inline void _oset_remove_from_link(oset_t *oset, osetnode_t *node) {
	osetnode_t *update[MAX_LEVEL];
	osetnode_t *xn = oset->head;
	int i;
	// 找到每层级的前结点
	lua_Integer score = node->score;
	size_t seq = node->seq;
	for (i = oset->level-1; i >= 0; i--) {
		while (xn->level[i].next && 
			(xn->level[i].next->score > score ||
				(xn->level[i].next->score == score && xn->level[i].next->seq < seq))) {
			xn = xn->level[i].next;
		}
		update[i] = xn;
	}
	// 从链表删除
	for (i = 0; i < oset->level; i++) {
		if (update[i]->level[i].next == node) {
			update[i]->level[i].span += node->level[i].span - 1;
			update[i]->level[i].next = node->level[i].next;
		} else {
			update[i]->level[i].span -= 1;
		}
	}
	if (node->level[0].next)
        node->level[0].next->prev = node->prev;
	// 调整层级
	while(oset->level > 1 && oset->head->level[oset->level-1].next == NULL)
		oset->level--;
	oset->length--;
}

// 删除结点： oset:remove(value) -> boolean
// 返回删除成功与否
static int oset_remove(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "can not be nil");		// S: oset val
	// 查结点
	lua_getuservalue(L, 1);		// S: oset val dict
	lua_pushvalue(L, 2);		// S: oset val dict val
	if (lua_rawget(L, -2) != LUA_TLIGHTUSERDATA) {	// S: oset val dict node
		luaL_pushfail(L);
		return 1;
	}
	osetnode_t *node = (osetnode_t*)lua_touserdata(L, -1);
	lua_pop(L, 1);		// S: oset val dict

	// 从链表删除
	_oset_remove_from_link(oset, node);

	// 从字典删除
	lua_pushvalue(L, 2);	// S: oset val dict val
	lua_pushnil(L);			// S: oset val dict val nil
	lua_rawset(L, -3);		// dict[val] = nil  S: oset val dict
	lua_pushnil(L);			// S: oset val dict nil
	lua_rawsetp(L, -2, node);	// dict[node] = nil  S: oset val dict

	// 释放结点
	co_free(node);
	lua_pushboolean(L, 1);
	return 1;
}

// 更新分数： oset:update(value, newscore) -> boolean
static int oset_update(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "can not be nil");
	lua_Integer newscore = luaL_checkinteger(L, 3);		// S: oset val nscore
	// 查结点
	lua_getuservalue(L, 1);				// S: oset val nscore dict
	lua_pushvalue(L, 2);				// S: oset val nscore dict val
	if (lua_rawget(L, -2) != LUA_TLIGHTUSERDATA) {	// S: oset val nscore dict node
		luaL_pushfail(L);
		return 1;
	}
	// 新分数还在前后结点的中间，只更新分数即可，不用移动。
	osetnode_t *node = (osetnode_t*)lua_touserdata(L, -1);
	if ((node->prev == NULL || node->prev->score > newscore) &&
        (node->level[0].next == NULL || node->level[0].next->score < newscore)) {
		node->score = newscore;
        lua_pushboolean(L, 1);
		return 1;
    }
	// 先从链表删除
	_oset_remove_from_link(oset, node);
	// 然后再重新加入
	node->score = newscore;
	_oset_add_to_link(oset, node);

	lua_pushboolean(L, 1);
	return 1;
}

// 通过值取信息： oset:getbyvalue(value) -> rank, value, score
// 找不到返回nil，成功返回三元组：排名，值，分数
// 排名从1开始
static int oset_getbyvalue(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "can not be nil");		// S: oset val
	// 查结点
	lua_getuservalue(L, 1);				// S: oset val dict
	lua_pushvalue(L, 2);				// S: oset val dict val
	if (lua_rawget(L, -2) != LUA_TLIGHTUSERDATA) {	// node = dict[val]  S: oset val dict node
		luaL_pushfail(L);
		return 1;
	}
	osetnode_t *node = (osetnode_t*)lua_touserdata(L, -1);

	size_t rank = 0;
	int i;
	osetnode_t *xn = oset->head;
	for (i = oset->level-1; i >= 0; i--) {
		while (xn->level[i].next && 
			(xn->level[i].next->score > node->score ||
				(xn->level[i].next->score == node->score && xn->level[i].next->seq <= node->seq))) {
			rank += xn->level[i].span;
			xn = xn->level[i].next;
		}
		if (xn == node) {
			lua_pushinteger(L, (lua_Integer)rank);		// S: oset value rank
			lua_pushvalue(L, 2);						// S: oset value rank value
			lua_pushinteger(L, node->score);			// S: oset value rank value score
			return 3;
		}
	}
	luaL_pushfail(L);
	return 1;
}

// 通过排名取信息：oset:getbyrank(rank) -> rank, value, score
// 找不到返回nil，成功返回三元组：排名，值，分数
static int oset_getbyrank(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	size_t rank = (size_t)luaL_checkinteger(L, 2);
	if (rank >= 1 && rank <= oset->length) {
		size_t traversed = 0;
		int i;
		osetnode_t *xn = oset->head;
		for (i = oset->level-1; i >= 0; i--) {
			while (xn->level[i].next && (traversed + xn->level[i].span <= rank)) {
				traversed += xn->level[i].span;
				xn =  xn->level[i].next;
			}
			if (traversed == rank) {
				// 从字典取出值
				lua_getuservalue(L, 1);			// S: oset rank dict
				lua_rawgetp(L, -1, xn);			// S: oset rank dict val
				lua_remove(L, -2);				// S: oset rank val
				lua_pushinteger(L, xn->score);	// S: oset rank val score
				return 3;
			}
		}
	}
	luaL_pushfail(L);
	return 1;
}

// 通过分数取信息：oset:getbyscore(score) -> rank, value, score
// 找不到返回nil，成功返回三元组：排名，值，分数
// 总是找到排名最前的分数
static int oset_getbyscore(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	lua_Integer score = luaL_checkinteger(L, 2);
	osetnode_t *xn = oset->head;
	size_t rank = 0;
	int i;
	for (i = oset->level-1; i >= 0; i--) {
		while (xn->level[i].next && (xn->level[i].next->score > score)) {
			rank += xn->level[i].span;
			xn =  xn->level[i].next;
		}
		if (xn->level[0].next && (xn->level[0].next->score == score)) {
			xn = xn->level[0].next;
			rank++;
			lua_pushinteger(L, rank);		// S: oset score rank
			lua_getuservalue(L, 1);			// S: oset score rank dict
			lua_rawgetp(L, -1, xn);			// S: oset score rank dict val
			lua_remove(L, -2);				// S: oset score rank val
			lua_pushinteger(L, xn->score);	// S: oset score rank val score
			return 3;
		}
	}
	luaL_pushfail(L);
	return 1;
}


// 迭代器，注意不要在迭代的时候，删除结点，否则可能crash
static int _oset_next(lua_State *L) {
	osetnode_t *node = (osetnode_t*)lua_touserdata(L, lua_upvalueindex(2));
	size_t rank = (size_t)lua_tointeger(L, lua_upvalueindex(3));
	int reverse = lua_toboolean(L, lua_upvalueindex(4));
	if (!node) {
		lua_pushnil(L);
		return 1;
	} else {
		lua_pushinteger(L, (lua_Integer)rank);		// S: rank
		lua_getuservalue(L, lua_upvalueindex(1));	// S: rank dict
		lua_rawgetp(L, -1, node);					// S: rank dict val
		lua_remove(L, -2);							// S: rank val
		lua_pushinteger(L, node->score);			// S: rank val score
		
		// 替换upvalue
		if (!reverse) {
			node = node->level[0].next;
			rank++;
		} else {
			node = node->prev;
			rank--;
		}
		lua_pushlightuserdata(L, node);				// S: rank val score node
		lua_replace(L, lua_upvalueindex(2));		// S: rank val score
		lua_pushinteger(L, rank);					// S: rank val score rank
		lua_replace(L, lua_upvalueindex(3));		// S: rank val score
		
		return 3;
	}
}

// 返回迭代器，从某个排名开始迭代，如果不传默认从第1个元素开始：
// for rank, value, score in oset:itrbyrank(10, reverse) do ... end
static int oset_itrbyrank(lua_State *L) {
	oset_t *oset = to_ordered_set(L);			// S: osetud rank
	size_t rank = (size_t)luaL_optinteger(L, 2, 1);
	int reverse = lua_toboolean(L, 3);
	osetnode_t *node = NULL;
	if (rank == 1) {
		node = oset->head->level[0].next;
	} else if (rank > 1 && rank <= oset->length) { 
		osetnode_t *xn = oset->head;
		size_t traversed = 0;
		int i;
		for (i = oset->level-1; i >= 0; i--) {
			while (xn->level[i].next && (traversed + xn->level[i].span <= rank)) {
				traversed += xn->level[i].span;
				xn =  xn->level[i].next;
			}
			if (traversed == rank) {
				node = xn;
				break;
			}
		}
	}
	// 返回 closure
	lua_pushvalue(L, 1);						// S: oset
	lua_pushlightuserdata(L, node);				// S: oset node
	lua_pushinteger(L, rank);					// S: oset node rank
	lua_pushboolean(L, reverse);	// S: oset node rank rev
	lua_pushcclosure(L, _oset_next, 4);			// S: closure
	return 1;
}

// 返回迭代器，从某个积分开始迭代，如果不传默认从第1个元素开始：
// for rank, value, score in oset:itrbyscore(100321, reverse) do ... end
static int oset_itrbyscore(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	int reverse = lua_toboolean(L, 3);
	osetnode_t *node = NULL;
	size_t rank;
	if (lua_isnoneornil(L, 2)) {
		node = oset->head->level[0].next;
		rank = 1;
	} else {
		lua_Integer score = luaL_checkinteger(L, 2);
		osetnode_t *xn = oset->head;
		rank = 0;
		int i;
		for (i = oset->level-1; i >= 0; i--) {
			while (xn->level[i].next && (xn->level[i].next->score > score)) {
				rank += xn->level[i].span;
				xn =  xn->level[i].next;
			}
			if (xn->level[0].next && (xn->level[0].next->score == score)) {
				rank++;
				node = xn->level[0].next;
				break;
			}
		}
	}
	// 返回 closure
	lua_pushvalue(L, 1);						// S: oset
	lua_pushlightuserdata(L, node);				// S: oset node
	lua_pushinteger(L, rank);					// S: oset node rank
	lua_pushboolean(L, reverse);	// S: oset node rank rev
	lua_pushcclosure(L, _oset_next, 4);
	return 1;
}

// 返回迭代器，从某个值开始迭代，如果不传默认从第1个元素开始：
// for rank, value, score in oset:itrbyvalue(val, reverse) do ... end
static int oset_itrbyvalue(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	int reverse = lua_toboolean(L, 3);
	osetnode_t *node = NULL;
	size_t rank = 0;
	if (lua_isnoneornil(L, 2)) {
		node = oset->head->level[0].next;
		rank = 1;
	} else {
		// 查结点
		lua_getuservalue(L, 1);				// S: oset val rev dict
		lua_pushvalue(L, 2);				// S: oset val rev dict val
		if (lua_rawget(L, -2) != LUA_TLIGHTUSERDATA) {	// S: oset val rev dict node
			node = NULL;
		} else {
			node = (osetnode_t*)lua_touserdata(L, -1);
			rank = 0;
			int i;
			osetnode_t *xn = oset->head;
			for (i = oset->level-1; i >= 0; i--) {
				while (xn->level[i].next && 
					(xn->level[i].next->score > node->score ||
						(xn->level[i].next->score == node->score && xn->level[i].next->seq <= node->seq))) {
					rank += xn->level[i].span;
					xn = xn->level[i].next;
				}
				if (xn == node) {
					break;
				}
			}
		}
		lua_pop(L, 2);			// S: oset val rev
	}

	lua_pushvalue(L, 1);						// S: oset
	lua_pushlightuserdata(L, node);				// S: oset node
	lua_pushinteger(L, rank);					// S: oset node rank
	lua_pushboolean(L, reverse);	// S: oset node rank rev
	lua_pushcclosure(L, _oset_next, 4);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////
// 调试函数

static inline void _print_chars(int c, int count, int newline) {
	int i;
	for (i = 0; i < count; ++i) putchar(c);
	if (newline) printf("\n");
}

static inline void _print_span(size_t span) {
	printf("[%3zu]", span);
	if (span > 0) {
		size_t count = span * 3 + (span-1) * 5; 
		_print_chars('-', count - 1, 0);
		putchar('>');
	}
}

static inline void _print_rank(size_t rank) {
	printf("%-5zu", rank);
	printf("   ");
}

static inline void _print_score(lua_Integer score) {
	printf("%-5lld", score);
	printf("   ");
}

static inline void _print_value(lua_State *L, osetnode_t *node) {
	lua_getuservalue(L, 1);		// S: dict
	int type = lua_rawgetp(L, -1, node); // S: dict val
	if (type == LUA_TSTRING) {	
		const char *str = lua_tostring(L, -1);
		printf("%-8s", str);
	} else if (lua_isinteger(L, -1)) {
		printf("%-8lld", lua_tointeger(L, -1));
	} else {
		printf("%-8s", lua_typename(L, type));
	}
	lua_pop(L, 2);
}

// 打印集合的内存，仅用于调试
static int oset_dump(lua_State *L) {
	oset_t *oset = to_ordered_set(L);
	int verb = luaL_optinteger(L, 2, 3);
	size_t len = oset->length;
	printf("length: %zu, level: %d\n", len, oset->level);
	if (verb <= 1) return 0;

	_print_chars('=', len * 3 + (len + 1) * 5, 1);
	int i;
	osetnode_t *node;
	for (i = oset->level-1; i >= 0; i--) {
		node = oset->head;
		while (node) {
			_print_span(node->level[i].span);
			node = node->level[i].next;
		}
		printf("\n");
	}
	_print_chars('=', len * 3 + (len + 1) * 5, 1);

	if (verb <= 2) return 0;
	// rank
	size_t rank = 1;
	node = oset->head;
	while (node) {
		if (node == oset->head) printf("rank    ");
		else _print_rank(rank++);
		node = node->level[0].next;
	}
	printf("\n");
	// score
	node = oset->head;
	while (node) {
		if (node == oset->head) printf("score   ");
		else _print_score(node->score);
		node = node->level[0].next;
	}
	printf("\n");
	// value
	node = oset->head;
	while (node) {
		if (node == oset->head) printf("value   ");
		else _print_value(L, node);
		node = node->level[0].next;
	}
	printf("\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////

// skiplist 元表
static const luaL_Reg oset_mt[] = {
	{"__index", NULL},
	{"__gc", oset_gc},
	{"__tostring", oset_tostring},
	{"__len", oset_getlen},
	{"getlen", oset_getlen},
	{"add", oset_add},
	{"remove", oset_remove},
	{"update", oset_update},
	{"getbyvalue", oset_getbyvalue},
	{"getbyrank", oset_getbyrank},
	{"getbyscore", oset_getbyscore},
	{"itrbyrank", oset_itrbyrank},
	{"itrbyscore", oset_itrbyscore},
	{"itrbyvalue", oset_itrbyvalue},
	{"dump", oset_dump},
	{NULL, NULL}
};

// 模块函数
static const luaL_Reg lib[] = {
	{"new", oset_new}, 
	{NULL, NULL}
};

LUAMOD_API int luaopen_colibc_oset(lua_State *L) {
	luaL_checkversion(L);

	// create metatable of oset
	luaL_newmetatable(L, ORDERED_SET);	// S: mt

	// create pseudo random number generator(PRNG)
	randstate_t *state = (randstate_t*)co_newuserdata(L, sizeof(randstate_t));		// S: mt rand
	randseed(state, (uint64_t)time(NULL), (uint64_t)(size_t)L);
	int i;
	for (i = 0; i < 16; i++)
    	randnext(state);  /* discard initial values to "spread" seed */

	luaL_setfuncs(L, oset_mt, 1);		// S: mt
	lua_pushvalue(L, -1);				// S: mt mt
	lua_setfield(L, -2, "__index");		// S: mt

	luaL_newlib(L, lib);				// S: mt lib
	return 1;
}