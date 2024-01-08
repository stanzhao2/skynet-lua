

#pragma once

/********************************************************************************/

#include "luaf_state.h"

/********************************************************************************/

enum struct color_type {
  print, trace, error
};

LUAC_API void luaC_printf(
  color_type color, const char* fmt, ...
);

#define lua_fprint(f, ...) luaC_printf(color_type::print, f, __VA_ARGS__)
#define lua_ftrace(f, ...) luaC_printf(color_type::trace, f, __VA_ARGS__)
#define lua_ferror(f, ...) luaC_printf(color_type::error, f, __VA_ARGS__)

/********************************************************************************/

LUAC_API int luaC_open_print(lua_State* L);

/********************************************************************************/
