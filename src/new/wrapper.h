

#ifndef __LUA_WRAPPER_H_
#define __LUA_WRAPPER_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "configure.h"

/********************************************************************************/

int luaopen_wrapper(lua_State* L);
int luaC_wrap      (lua_State* L, int n);
int luaC_unwrap    (lua_State* L);

/********************************************************************************/

#endif //__LUA_WRAPPER_H_
