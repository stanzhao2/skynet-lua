

#include <string.h>
#include "luaf_pcall.h"

/********************************************************************************/

static int finishpcall(lua_State *L, int status, lua_KContext extra) {
  if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    lua_pushboolean(L, 0);  /* first result (false) */
    lua_pushvalue(L, -2);  /* error message */
    return 2;  /* return false, msg */
  }
  return lua_gettop(L) - (int)extra;  /* return all results */
}

static lua_State *getthread(lua_State *L, int *arg) {
  if (lua_isthread(L, 1)) {
    *arg = 1;
    return lua_tothread(L, 1);
  }
  *arg = 0;
  return L;  /* function will operate over current thread */
}

static int traceback(lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  const char *msg = lua_tostring(L, arg + 1);
  if (strstr(msg, "stack traceback")) {
    return 1;
  }
  if (msg == NULL && !lua_isnoneornil(L, arg + 1))  /* non-string 'msg'? */
    lua_pushvalue(L, arg + 1);  /* return it untouched */
  else {
    int level = (int)luaL_optinteger(L, arg + 2, (L == L1) ? 1 : 0);
    luaL_traceback(L, L1, msg, level);
  }
  return 1;
}

static int luaf_xpcall(lua_State *L) {
  int status;
  int n = lua_gettop(L);
  luaL_checktype(L, 2, LUA_TFUNCTION);  /* check error function */
  lua_pushboolean(L, 1);  /* first result */
  lua_pushvalue(L, 1);    /* function */
  lua_rotate(L, 3, 2);    /* move them below function's arguments */
  status = lua_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishpcall);
  return finishpcall(L, status, 2);
}

static int luaf_pcall(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);  /* check pcall function */
  lua_pushcfunction(L, traceback);
  lua_insert(L, 2);
  return luaf_xpcall(L);
}

/********************************************************************************/

LUAC_API int luaC_open_pcall(lua_State* L) {
  const luaL_Reg methods[] = {
    { "pcall",    luaf_pcall    }, /* pcall (f [, arg1, ...]) */
    { "xpcall",   luaf_xpcall   }, /* xpcall(f, msgh [, arg1, ...]) */
    { NULL,       NULL          }
  };
  lua_getglobal(L, LUA_GNAME);
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop '_G' from stack */
  return 0;
}

LUAC_API int luaC_pcall(lua_State* L, int n, int r) {
  return lua_pcallk(L, n, r, 0, 0, finishpcall);
}

LUAC_API int luaC_xpcall(lua_State* L, int n, int r) {
  int top = lua_gettop(L);
  lua_pushcfunction(L, traceback);
  int errfunc = top - n;
  lua_insert(L, errfunc);
  int status = lua_pcallk(L, n, r, errfunc, 1, finishpcall);
  lua_remove(L, errfunc); /* remove traceback from stack */
  return status;
}

/********************************************************************************/
