

#include <chrono>
#include "luaf_clock.h"

/********************************************************************************/

static int luaf_clock(lua_State* L) {
  size_t now = luaC_clock();
  lua_pushinteger(L, (lua_Integer)now);
  return 1;
}

/********************************************************************************/

LUAC_API size_t luaC_clock() {
  return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()
  ).count();
}

LUAC_API int luaC_open_clock(lua_State* L) {
  const luaL_Reg methods[] = {
    { "steady_clock", luaf_clock }, /* os.steady_clock() */
    { NULL,       NULL          }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return 0;
}

/********************************************************************************/
