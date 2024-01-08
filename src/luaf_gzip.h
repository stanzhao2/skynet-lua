
/*
** gzip.deflate(data[, is_gzip]);
** gzip.inflate(data[, is_gzip]);
*/

#pragma once

#include "luaf_state.h"

/********************************************************************************/

LUALIB_API int luaC_open_gzip(lua_State* L);

/********************************************************************************/
