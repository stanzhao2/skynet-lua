

#include "luaf_metatable.h"

/********************************************************************************/

LUAC_API void luaC_setmetatable(lua_State* L, const char* name) {
  luaL_getmetatable(L, name);
  lua_setmetatable(L, -2);
}

LUAC_API int luaC_newmetatable(lua_State* L, const char* name, const luaL_Reg methods[]) {
  if (!luaL_newmetatable(L, name)) {
    return 0;
  }
  /* define methods */
  if (methods) {
    luaL_setfuncs(L, methods, 0);
  }

  /* define metamethods */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);

  lua_pushliteral(L, "__metatable");
  lua_pushliteral(L, "you're not allowed to get this metatable");
  lua_settable(L, -3);
  return 1;
}

LUAC_API void* luaC_newuserdata(lua_State* L, const char* name, size_t size) {
  void* userdata = lua_newuserdata(L, size);
  luaC_setmetatable(L, name);
  return userdata;
}

/********************************************************************************/
