

#ifndef __LUA_CONFIGURE_H_
#define __LUA_CONFIGURE_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <string>
#include <string.h>
#include <lua.hpp>

/********************************************************************************/

#ifndef lua_pmain
#define lua_pmain "main"
#endif

#ifdef  lua_pcall
#undef  lua_pcall
#endif

#ifndef lua_pcall
#define lua_pcall(L,n,r,f) luaC_pcallk(L,(n),(r),(f), 0)
#endif

/********************************************************************************/

lua_State* luaC_state(
  /* void */
);
int   luaC_dofile(
  lua_State* L, const lua_CFunction *f = NULL
);
int   luaC_pcallk(
  lua_State* L, int n, int r, int f, lua_KContext k
);
int   luaC_newmetatable(
  lua_State* L, const char* name, const luaL_Reg methods[]
);
void  luaC_setmetatable(
  lua_State* L, const char* name
);
void* luaC_newuserdata(
  lua_State* L, const char* name, size_t size
);

/********************************************************************************/

template <typename _Ty> 
_Ty* luaC_checkudata(lua_State* L, const char* name, int index = 1) {
  return (_Ty*)luaL_checkudata(L, index, name);
}
template <typename _Ty, typename ...Args>
_Ty* luaC_newuserdata(lua_State* L, const char* name, Args... args) {
  return new (luaC_newuserdata(L, name, sizeof(_Ty))) _Ty(args...);
}

/********************************************************************************/

#endif //__LUA_CONFIGURE_H_
