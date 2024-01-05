

#include "luaf_bind.h"

/********************************************************************************/

static int callback(lua_State* L) {
  int index = lua_upvalueindex(1); /* get the first up-value */
  int up_values = (int)luaL_checkinteger(L, index); /* get the number of up-values */
  for (int i = 2; i <= up_values; i++) {
    lua_pushvalue(L, lua_upvalueindex(i));
  }
  if (up_values > 1) { /* if the function has bound parameters */
    lua_rotate(L, 1, up_values - 1); /* rotate the calling parameters after the binding parameters */
  }
  int n = lua_gettop(L) - 1; /* calculate the number of all parameters */
  int status = luaC_pcall(L, n, LUA_MULTRET);
  if (status != LUA_OK) {
    lua_error(L);
  }
  return lua_gettop(L);
}

static int luaf_bind(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION); /* function type check */
  int up_values = lua_gettop(L) + 1; /* number of up-values (function + argvs)*/
  lua_pushinteger(L, up_values); /* push up-values onto the stack */
  lua_rotate(L, 1, 1); /* rotate the function to the top of the stack */
  lua_pushcclosure(L, callback, up_values);
  return 1;
}

/********************************************************************************/

LUAC_API int luaopen_bind(lua_State* L) {
  const luaL_Reg methods[] = {
    { "bind",     luaf_bind     }, /* bind  (f [, arg1, ...]) */
    { NULL,       NULL          }
  };
  lua_getglobal(L, LUA_GNAME);
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop '_G' from stack */
  return 0;
}

/********************************************************************************/
