

#pragma once

/********************************************************************************/

#include "luaf_state.h"

/********************************************************************************/

#define evr_deliver   "deliver"
#define evr_bind      "bind"
#define evr_unbind    "unbind"
#define evr_response  "response"

/********************************************************************************/

LUAC_API int luaopen_rpcall(lua_State* L);
LUAC_API int luaC_lookout  (lua_CFunction f);
LUAC_API int luaC_r_bind   (const char* name, size_t who, int rcb, int opt);
LUAC_API int luaC_r_unbind (const char* name, size_t who, int* opt);
LUAC_API int luaC_r_deliver(const char* name, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf);
LUAC_API int luaC_r_response(const std::string& data, size_t caller, int rcf);

/********************************************************************************/
