#ifndef _COCONF_H
#define _COCONF_H

#include "lua.h"

// lua 5.4以上用lua_newuserdatauv更好
#if LUA_VERSION_NUM >= 504
#define co_newuserdata(L, sz) lua_newuserdatauv(L, sz, 0)
#else
#define co_newuserdata(L, sz) lua_newuserdata(L, sz)
#endif // LUA_VERSION_NUM >= 504

// 内存分配函数，方便替换
#define co_malloc malloc
#define co_free free
#define co_realloc realloc
#define co_calloc calloc


#endif // _COCONF_H