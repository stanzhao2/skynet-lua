

#pragma once

/********************************************************************************/

#include "luaf_state.h"

#define LUAC_PROGNAME "progname"

/********************************************************************************/

LUAC_API int luaC_open_require(lua_State* L);
LUAC_API int luaC_execute(lua_State* L);

/********************************************************************************/
