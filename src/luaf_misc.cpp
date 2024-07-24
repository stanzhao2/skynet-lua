

#include "luaf_misc.h"

/********************************************************************************/

static int example(lua_State* L) {
  return 0;
}

/********************************************************************************/
#ifdef _MSC_VER //for windows
/********************************************************************************/

/********************************************************************************/
#else //for linux
/********************************************************************************/

/********************************************************************************/
#endif
/********************************************************************************/

static int luaset_methods(lua_State* L, const char* name, const luaL_Reg* methods) {
  for (int i = 0; i < 2; i++) {
    lua_getglobal(L, name);
    if (!lua_isnil(L, -1)) {
      break;
    }
    lua_pop(L, 1);    /* pop nil from stack */
    lua_newtable(L);  /* push new table in stack */
    lua_setglobal(L, name);
  }
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1);      /* pop table from stack */
  return 0;
}

LUAC_API int luaC_open_misc(lua_State* L) {
  static const luaL_Reg methods[] = {
    { NULL,     NULL        }
  };
  luaL_checkversion(L);
  return luaset_methods(L, "mmi", methods);
}

/********************************************************************************/
