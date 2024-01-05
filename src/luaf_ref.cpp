

#include "luaf_ref.h"

/********************************************************************************/

LUAC_API void luaC_unref(lua_State* L, int i) {
  luaL_unref(L, LUA_REGISTRYINDEX, i);
}

LUAC_API int luaC_ref(lua_State* L, int i) {
  lua_pushvalue(L, i);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

LUAC_API int luaC_rawgeti(lua_State* L, int i) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, i);
  return lua_type(L, -1);
}

/********************************************************************************/
