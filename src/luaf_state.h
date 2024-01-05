

#pragma once

/********************************************************************************/

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef __cplusplus
#define LUAC_API extern "C"
#else
#define LUAC_API
#endif

#define LUAC_MAIN "main"
#define luaC_max(a, b) ((a) < (b) ? (b) : (a))
#define luaC_min(a, b) ((a) < (b) ? (a) : (b))

/********************************************************************************/

#include <lua.hpp>
#include <string>

#include "luaf_ref.h"
#include "luaf_metatable.h"
#include "luaf_clock.h"
#include "luaf_pcall.h"
#include "luaf_pack.h"
#include "luaf_pload.h"
#include "luaf_print.h"
#include "luaf_rpcall.h"
#include "luaf_require.h"
#include "socket.io/socket.io.hpp"

/********************************************************************************/

LUAC_API lua_State* luaC_getlocal();
LUAC_API lua_State* luaC_newstate(lua_Alloc alloc = NULL, void* ud = NULL);
LUAC_API bool  luaC_debugging();
LUAC_API void  luaC_close(lua_State* L);
LUAC_API void  luaC_openlibs(lua_State* L, const lua_CFunction f[]);
LUAC_API void* luaC_realloc(void* ud, void* ptr, size_t osize, size_t nsize);

/********************************************************************************/
