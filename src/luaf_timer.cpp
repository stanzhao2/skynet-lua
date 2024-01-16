

#include "luaf_timer.h"
#include "socket.io/socket.io.hpp"

#define LUAC_TIMER "os:timer"

/********************************************************************************/

struct ud_timer {
  lws_int timer;
  std::string name;
};

static int luaf_timer(lua_State* L) {
  const char* name = luaL_optstring(L, 1, "unknown");
  auto ud = luaC_newuserdata<ud_timer>(L, LUAC_TIMER);
  ud->name  = name;
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

static void on_timer(int ec, int timer, int rcb, size_t expires, const std::string& name) {
  lua_State* L = luaC_getlocal();
  revert_if_return revert(L);
  unref_if_return  unref_rcb(L, rcb);
  luaC_rawgeti(L, rcb);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    return;
  }
  lua_pushinteger(L, ec);
  if (luaC_xpcall(L, 1, 2) != LUA_OK) {
    lua_ferror("(%s)%s\n", lua_tostring(L, -1));
    return;
  }
  if (ec > 0) {
    return;
  }
  if (lua_type(L, -2) != LUA_TBOOLEAN) {
    lua_ferror("timer '%s' not return boolean\n", name.c_str());
    return;
  }
  int result = lua_toboolean(L, -2);
  if (result == 0) {
    return;
  }
  auto wait = luaL_optinteger(L, -1, expires);
  lws_int ok = lws::expires(
    timer, wait, lws_bind(on_timer, lws_arg1, timer, rcb, expires, name)
  );
  if (ok == lws_true) {
    unref_rcb.cancel();
  }
}

static int luaf_expires(lua_State* L) {
  auto ud = luaC_checkudata<ud_timer>(L, 1, LUAC_TIMER);
  size_t expires = luaL_checkinteger(L, 2);
  luaL_checktype(L, 3, LUA_TFUNCTION);
  int rcb = luaC_ref(L, 3);
  lws_int ok = lws::expires(
    ud->timer, expires, lws_bind(on_timer, lws_arg1, ud->timer, rcb, expires, ud->name)
  );
  if (ok != lws_true) {
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
