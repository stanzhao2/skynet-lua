

#include "luaf_timer.h"
#include "socket.io/socket.io.hpp"

#define LUAC_TIMER "os:timer"

/********************************************************************************/

struct ud_timer {
  lws_int timer;
};

static int luaf_timer(lua_State* L) {
  auto ud = luaC_newuserdata<ud_timer>(L, LUAC_TIMER);
  ud->timer = lws::timer();
  return 1;
}

static int luaf_close(lua_State* L) {
  auto ud = luaC_checkudata<ud_timer>(L, 1, LUAC_TIMER);
  lws::close(ud->timer);
  return 0;
}

static int luaf_gc(lua_State* L) {
#ifdef _DEBUG
  lua_ftrace("%s will gc\n", LUAC_TIMER);
#endif
  auto ud = luaC_checkudata<ud_timer>(L, 1, LUAC_TIMER);
  int result = luaf_close(L);
  ud->~ud_timer();
  return result;
}

static int luaf_cancel(lua_State* L) {
  auto ud = luaC_checkudata<ud_timer>(L, 1, LUAC_TIMER);
  lws::cancel(ud->timer);
  return 0;
}

static int luaf_expires(lua_State* L) {
  auto ud = luaC_checkudata<ud_timer>(L, 1, LUAC_TIMER);
  size_t expires = luaL_checkinteger(L, 2);
  luaL_checktype(L, 3, LUA_TFUNCTION);
  int rud = luaC_ref(L, 1);
  int rcb = luaC_ref(L, 3);
  lws_int ok = lws::expires(ud->timer, expires, [rcb, rud, expires](int ec) {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    unref_if_return  unref_rcb(L, rcb);
    unref_if_return  unref_rud(L, rud);

    luaC_rawgeti(L, rcb);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
      return;
    }
    luaC_rawgeti(L, rcb);
    lua_pushinteger(L, ec);
    if (luaC_xpcall(L, 1, 1) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
      return;
    }
    if (ec || lua_type(L, -1) != LUA_TBOOLEAN) {
      return;
    }
    int result = lua_toboolean(L, -1);
    if (result == 0) {
      return;
    }
    lua_pop(L, 1); /* pop result */
    lua_pushcfunction(L, luaf_expires);
    luaC_rawgeti(L, rud);
    lua_pushinteger(L, (lua_Integer)expires);
    lua_rotate(L, -4, 3);
    if (luaC_xpcall(L, 3, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
  if (ok != lws_true) {
    luaC_unref(L, rud);
    luaC_unref(L, rcb);
  }
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

/********************************************************************************/

static void init_metatable(lua_State* L) {
  const luaL_Reg methods[] = {
    { "__gc",       luaf_gc         },
    { "close",      luaf_close      },
    { "cancel",     luaf_cancel     },
    { "expires",    luaf_expires    },
    { NULL,         NULL            }
  };
  luaC_newmetatable(L, LUAC_TIMER, methods);
  lua_pop(L, 1);
}

LUAC_API int luaC_open_timer(lua_State* L) {
  init_metatable(L);
  const luaL_Reg methods[] = {
    { "timer",      luaf_timer      },
    { NULL,         NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'io' from stack */
  return 0;
}

/********************************************************************************/
