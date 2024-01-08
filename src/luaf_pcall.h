

#pragma once

/********************************************************************************/

#include "luaf_state.h"

/********************************************************************************/

LUAC_API int luaC_open_pcall(lua_State* L);
LUAC_API int luaC_pcall (lua_State* L, int n, int r);
LUAC_API int luaC_xpcall(lua_State* L, int n, int r);

/********************************************************************************/
