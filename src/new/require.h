

#ifndef __LUA_REQUIRE_H_
#define __LUA_REQUIRE_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "configure.h"

/********************************************************************************/

int luaopen_require(lua_State* L);

/********************************************************************************/

#endif //__LUA_REQUIRE_H_
