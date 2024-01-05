
/*
** gzip.deflate(data[, is_gzip]);
** gzip.inflate(data[, is_gzip]);
*/

#pragma once

#include "luaf_state.h"

/********************************************************************************/

LUALIB_API int luaopen_gzip(lua_State* L);

/********************************************************************************/
