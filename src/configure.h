

#pragma once

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <lua.hpp>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#ifdef  lua_pcall
#undef  lua_pcall
#endif

#ifndef lua_pcall
#define lua_pcall(L,n,r,f) luaC_pcallk(L,(n),(r),(f), 0)
#endif

#ifndef LUAC_API
#define LUAC_API extern "C"
#endif

////////////////////////////////////////////////////////////////////////////////

lua_State* luaC_state(
  /* void */
);
int   luaC_dofile(
  lua_State* L, const char* entry = "main"
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

////////////////////////////////////////////////////////////////////////////////

template <typename _Ty> 
_Ty* luaC_checkudata(lua_State* L, const char* name, int index = 1) {
  return (_Ty*)luaL_checkudata(L, index, name);
}
template <typename _Ty, typename ...Args>
_Ty* luaC_newuserdata(lua_State* L, const char* name, Args... args) {
  return new (luaC_newuserdata(L, name, sizeof(_Ty))) _Ty(args...);
}

////////////////////////////////////////////////////////////////////////////////
