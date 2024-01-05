

#pragma once

/********************************************************************************/

#include "luaf_state.h"

/********************************************************************************/

LUAC_API int   luaC_newmetatable(lua_State* L, const char* name, const luaL_Reg methods[]);
LUAC_API void  luaC_setmetatable(lua_State* L, const char* name);
LUAC_API void* luaC_newuserdata (lua_State* L, const char* name, size_t size);

/********************************************************************************/

template <typename _Ty>
inline _Ty* luaC_checkudata(lua_State* L, int index, const char* name) {
  return (_Ty*)luaL_checkudata(L, index, name);
}

template <typename _Ty, typename ...Args>
inline _Ty* luaC_newuserdata(lua_State* L, const char* name, Args... args) {
  return new (luaC_newuserdata(L, name, sizeof(_Ty))) _Ty(args...);
}

/********************************************************************************/
