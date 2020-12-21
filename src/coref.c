#include "coref.h"

/* index of free-list header */
#define freelist	0

int refvalue(lua_State *L, int *maxref, int t) {
	int ref;
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);  /* remove from stack */
		return LUA_REFNIL;  /* 'nil' has a unique fixed reference */
	}
	t = lua_absindex(L, t);
	lua_rawgeti(L, t, freelist);	 /* get first free element */
	ref = (int)lua_tointeger(L, -1); /* ref = t[freelist] */
	lua_pop(L, 1);					 /* remove it from stack */
	if (ref != 0) {					 /* any free element? */
		lua_rawgeti(L, t, ref);		 /* remove it from list */
		lua_rawseti(L, t, freelist); /* (t[freelist] = t[ref]) */
	}
	else				 /* no free elements */
		ref = ++(*maxref); /* get a new reference */
	lua_rawseti(L, t, ref);
	return ref;
}

void unrefvalue(lua_State* L, int t, int ref) {
	if (ref >= 0) {
		t = lua_absindex(L, t);
		lua_rawgeti(L, t, freelist);
		lua_rawseti(L, t, ref); /* t[ref] = t[freelist] */
		lua_pushinteger(L, ref);
		lua_rawseti(L, t, freelist); /* t[freelist] = ref */
	}
}
