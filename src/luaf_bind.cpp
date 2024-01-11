

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

static int xcallback(lua_State* L) {
  int index = lua_upvalueindex(1); /* get the first up-value */
  int up_values = (int)luaL_checkinteger(L, index); /* get the number of up-values */
  const char* fname = luaL_checkstring(L, lua_upvalueindex(2)); /* function name */

  index = lua_upvalueindex(3);
  if (lua_type(L, index) != LUA_TTABLE) {
    luaL_error(L, "function %s not found", fname);
  }
  int rotate = 0;
  lua_getfield(L, index, "__init");
  auto type_class = lua_type(L, -1) == LUA_TFUNCTION;
  lua_pop(L, 1);
  for (int i = (type_class ? 3 : 4); i <= up_values; i++) {
    lua_pushvalue(L, lua_upvalueindex(i));
    rotate++;
  }
  lua_getfield(L, index, fname);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    luaL_error(L, "function %s not found", fname);
  }
  lua_insert(L, 1);
  if (rotate > 0) { /* if the function has bound parameters */
    lua_rotate(L, 2, rotate); /* rotate the calling parameters after the binding parameters */
  }
  int n = lua_gettop(L) - 1; /* calculate the number of all parameters */
  int status = luaC_pcall(L, n, LUA_MULTRET);
  if (status != LUA_OK) {
    lua_error(L);
  }
  return lua_gettop(L);
}

static int luaf_bind_ex(lua_State* L) {
  if (lua_type(L, 1) == LUA_TFUNCTION) {
    return luaf_bind(L);
  }
  const char* fname = luaL_checkstring(L, 1); /* function name */
  luaL_checktype(L, 2, LUA_TTABLE);  /* table type check  */
  lua_getfield(L, 2, fname);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    luaL_error(L, "function %s not found", fname);
  }
  lua_pop(L, 1);
  int up_values = lua_gettop(L) + 1; /* number of up-values (function + argvs)*/
  lua_pushinteger(L, up_values); /* push up-values onto the stack */
  lua_insert(L, 1);
  lua_pushcclosure(L, xcallback, up_values);
  return 1;
}

/********************************************************************************/

LUAC_API int luaC_open_bind(lua_State* L) {
  const luaL_Reg methods[] = {
    { "bind",   luaf_bind_ex  }, /* bind(f [, arg1, ...]) */
    { NULL,     NULL        }
  };
  lua_getglobal(L, LUA_GNAME);
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop '_G' from stack */
  return 0;
}

/********************************************************************************/
