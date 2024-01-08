

#include "luaf_gzip.h"
#include "eport/detail/zlib/gzip.hpp"

/********************************************************************************/

static int gzip_deflate(lua_State *L) {
  size_t size = 0;
  const char* data = (char*)luaL_checklstring(L, 1, &size);
  bool gzip = true;
  if (!lua_isnoneornil(L, 2)){
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    gzip = lua_toboolean(L, 2) ? true : false;
  }
  std::string output;
  if (!eport::zlib::do_deflate(data, size, gzip, output)) {
    lua_pushnil(L);
  }
  else {
    lua_pushlstring(L, output.c_str(), output.size());
  }
  return 1;
}

static int gzip_inflate(lua_State *L) {
  size_t size = 0;
  const char* data = (char*)luaL_checklstring(L, 1, &size);
  bool gzip = true;
  if (!lua_isnoneornil(L, 2)){
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    gzip = lua_toboolean(L, 2) ? true : false;
  }
  std::string output;
  if (!eport::zlib::do_inflate(data, size, gzip, output)){
    lua_pushnil(L);
  }
  else {
    lua_pushlstring(L, output.c_str(), output.size());
  }
  return 1;
}

/********************************************************************************/

LUALIB_API int luaC_open_gzip(lua_State* L) {
  lua_getglobal(L, LUA_GNAME);
  lua_newtable(L);
  const luaL_Reg methods[] = {
    { "deflate",    gzip_deflate   },  /* gzip.deflate  */
    { "inflate",    gzip_inflate   },  /* gzip.inflate  */
    { NULL,         NULL                  },
  };
  luaL_setfuncs(L, methods, 0);
  lua_setfield (L, -2, "gzip");
  lua_pop(L, 1);
  return 0;
}

/********************************************************************************/
