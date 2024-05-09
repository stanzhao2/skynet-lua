

#include "json.h"
#include "rapidjson/rapidjson.h"

/********************************************************************************/

int luaopen_json(lua_State* L) {
  lua_getglobal(L, LUA_GNAME);
  luaL_rapidjson(L);
  lua_setfield(L, -2, "json");
  lua_pop(L, 1);
  return 0;
}

/********************************************************************************/
