/**
 * luaL_ref与luaL_unref的另一个实现，逻辑基本相同，除了最大引用由外部维护。
 * 之所以要加这个，是因为luaL_ref的最大引用通过lua_rawlen来得到，这个函数的时间复杂度是O(logN)
 */


#ifndef coref_h
#define coref_h
#include "lua.h"
#include "lauxlib.h"

/**
 * 与luaL_ref类似，引用一个值，该值在栈顶
 * @param maxref 最大引用，由外部维护
 * @param t table所在的栈索引
 * @return 返回引用址
 */
int refvalue(lua_State *L, int *maxref, int t);

/**
 * 与luaL_unref类似，解引用一个值
 * @param t table所在的栈索引
 * @param ref 引用值，从table中删除该值。
 */
void unrefvalue(lua_State* L, int t, int ref);

#endif