

#pragma once

#include "luaf_state.h"

/********************************************************************************/

LUAC_API int luaopen_package(lua_State* L);
LUAC_API int luaC_pack  (lua_State* L, int n);
LUAC_API int luaC_unpack(lua_State* L);

/********************************************************************************/
