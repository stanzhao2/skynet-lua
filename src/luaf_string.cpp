

#include "luaf_string.h"

/********************************************************************************/

static int luaf_isalnum(lua_State* L) {
  const char* p = luaL_checkstring(L, 1);
  while (*p) {
    if (isalnum(*p++) == 0) {
      lua_pushboolean(L, 0);
      return 1;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

static int luaf_isalpha(lua_State* L) {
  const char* p = luaL_checkstring(L, 1);
  while (*p) {
    if (isalpha(*p++) == 0) {
      lua_pushboolean(L, 0);
      return 1;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

static int luaf_split(lua_State* L) {
  const char *s = luaL_checkstring(L, 1);
  const char *sep = luaL_checkstring(L, 2);
  const char *e;
  int i = 1;

  lua_newtable(L);  /* result */
                    /* repeat for each separator */
  while ((e = strchr(s, *sep)) != NULL) {
    lua_pushlstring(L, s, e-s);  /* push substring */
    lua_rawseti(L, -2, i++);
    s = e + 1;  /* skip separator */
  }
  /* push last substring */
  lua_pushstring(L, s);
  lua_rawseti(L, -2, i);
  return 1;  /* return the table */
}

/********************************************************************************/

LUAC_API int luaC_open_string(lua_State* L) {
  const luaL_Reg methods[] = {
    { "split",    luaf_split    }, /* string.split(s, r) */
    { "isalnum",  luaf_isalnum  }, /* string.isalnum(s)  */
    { "isalpha",  luaf_isalpha  }, /* string.isalpha(s)  */
    { NULL,       NULL          }
  };
  lua_getglobal(L, "string");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'string' from stack */
  return 0;
}

/********************************************************************************/
