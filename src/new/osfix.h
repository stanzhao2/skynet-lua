

#ifndef __LUA_OSFIX_H_
#define __LUA_OSFIX_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "configure.h"

/********************************************************************************/

int luaopen_osfix(lua_State* L);
size_t luaC_clock();

/********************************************************************************/

#endif //__LUA_OSFIX_H_
