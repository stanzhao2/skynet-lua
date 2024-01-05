

#include <string.h>
#include "luaf_socket.h"
#include "socket.io/socket.io.hpp"

#define LUAC_SOCKET "io:socket"

/********************************************************************************/

struct ud_context {
  bool accept;
  lws_int handle;
};

static lws_cainfo* init_cainfo(lua_State* L, lws_cainfo* pinfo) {
  memset(pinfo, 0, sizeof(lws_cainfo));
  int top = lua_gettop(L);
  if (top == 2) {
    return nullptr;
  }
  if (top == 3) {
    pinfo->caf = luaL_checklstring(L, 3, &pinfo->caf_size);
    return pinfo;
  }
  pinfo->crt.data = luaL_checklstring(L, 4, &pinfo->crt.size);
  pinfo->key.data = luaL_checklstring(L, 5, &pinfo->key.size);
  pinfo->pwd = luaL_optstring(L, 6, nullptr);
  return pinfo;
}

static lws_int new_socket(lua_State* L) {
  lws_cainfo info;
  lws_cainfo* ca = init_cainfo(L, &info);
  const char* family = luaL_checkstring(L, 1);

  if (strcmp(family, "tcp") == 0) {
    return lws::socket(lws_family::tcp, nullptr);
  }
  if (strcmp(family, "ws") == 0) {
    return lws::socket(lws_family::ws, nullptr);
  }
  if (strcmp(family, "ssl") == 0) {
    return lws::socket(lws_family::ssl, ca);
  }
  if (strcmp(family, "wss") == 0) {
    return lws::socket(lws_family::wss, ca);
  }
  luaL_error(L, "invalid socket family: ", family);
  return 0;
}

static int luaf_socket(lua_State* L) {
  auto ud = luaC_newuserdata<ud_context>(L, LUAC_SOCKET);
  ud->accept = false;
  ud->handle = new_socket(L);
  return 1;
}

static int luaf_acceptor(lua_State* L) {
  auto ud = luaC_newuserdata<ud_context>(L, LUAC_SOCKET);
  ud->accept = true;
  ud->handle = lws::acceptor();
  return 1;
}

static int luaf_close(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  lws::close(ud->handle);
  return 0;
}

static int luaf_gc(lua_State* L) {
#ifdef _DEBUG
  lua_ftrace("%s will gc\n", LUAC_SOCKET);
#endif
  return luaf_close(L);
}

static int luaf_id(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  lua_pushinteger(L, ud->handle);
  return 1;
}

static int luaf_listen(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (!ud->accept) {
    luaL_error(L, "invalid method");
  }
  lws_ushort  port = (lws_ushort)luaL_checkinteger(L, 2);
  const char* host = luaL_optstring(L, 3, nullptr);
  int backlog = (int)luaL_optinteger(L, 4, 32);
  lws_int  ok = lws::listen(ud->handle, port, host, backlog);
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_accept(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (!ud->accept) {
    luaL_error(L, "invalid method");
  }
  auto peer = luaC_checkudata<ud_context>(L, 2, LUAC_SOCKET);
  if (peer->accept) {
    luaL_error(L, "invalid method");
  }
  if (lua_isnoneornil(L, 3)) {
    lws_int ok = lws::accept(ud->handle, peer->handle);
    lua_pushboolean(L, ok == lws_true ? 1 : 0);
    return 1;
  }
  luaL_checktype(L, 3, LUA_TFUNCTION);
  int rud   = luaC_ref(L, 1);
  int rpeer = luaC_ref(L, 2);
  int rcb   = luaC_ref(L, 3);
  lws_int ok = lws::accept(ud->handle, peer->handle, [rud, rcb, rpeer](int ec, int peer) {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    unref_if_return  unref_rud(L, rud);
    unref_if_return  unref_rcb(L, rcb);
    unref_if_return  unref_rpeer(L, rpeer);

    luaC_rawgeti(L, rcb);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
      return;
    }
    luaC_rawgeti(L, rcb);
    luaC_rawgeti(L, rpeer);
    lua_pushinteger(L, ec);
    lua_rotate(L, -2, 1);
    if (luaC_xpcall(L, 2, 1) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
      return;
    }
    if (lua_type(L, -1) != LUA_TUSERDATA) {
      return;
    }
    luaC_checkudata<ud_context>(L, -1, LUAC_SOCKET);
    lua_pushcfunction(L, luaf_accept);
    luaC_rawgeti(L, rud);
    lua_rotate(L, -3, 2);
    lua_rotate(L, -4, 3);
    if (luaC_xpcall(L, 3, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
  if (ok != lws_true) {
    luaC_unref(L, rud);
    luaC_unref(L, rcb);
    luaC_unref(L, rpeer);
  }
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_read(lua_State* L) {
  std::string packet;
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  lws_int ok = lws::read(ud->handle, [&](int ec, const char* data, lws_size size) {
    if (!ec) {
      packet.assign(data, size);
    }
  });
  if (ok == lws_true) {
    lua_pushboolean(L, 1);
    lua_pushlstring(L, packet.c_str(), packet.size());
  }
  else {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
  }
  return 2;
}

static int luaf_write(lua_State* L) {
  size_t size;
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  const char* data = luaL_checklstring(L, 2, &size);
  lws_int sent = lws::write(ud->handle, data, size);
  lua_pushboolean(L, sent < 0 ? 0 : 1);
  lua_pushinteger(L, sent);
  return 2;
}

static int luaf_send(lua_State* L) {
  size_t size;
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  const char* data = luaL_checklstring(L, 2, &size);
  lws_int ok = lws::send(ud->handle, data, size);
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_receive(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  luaL_checktype(L, 2, LUA_TFUNCTION);
  int rcb = luaC_ref(L, 2);
  lws_int ok = lws::receive(ud->handle, [rcb](int ec, const char* data, lws_size size) {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    unref_if_return  unref_rcb(L, rcb);

    luaC_rawgeti(L, rcb);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
      return;
    }
    lua_pushinteger(L, ec);
    lua_pushlstring(L, data, size);
    if (luaC_xpcall(L, 2, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
    if (!ec) {
      unref_rcb.cancel();
    }
  });
  if (ok != lws_true) {
    luaC_unref(L, rcb);
  }
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_endpoint(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  lws_endtype type = lws_endtype::remote;
  const char* option = luaL_optstring(L, 2, "remote");
  if (strcmp(option, "local") == 0) {
    type = lws_endtype::local;
  }
  lws_endinfo info;
  lws_int ok = lws::endpoint(ud->handle, &info, type);
  if (ok == lws_true) {
    lua_pushboolean(L, 1);
    lua_pushstring (L, info.ip);
    lua_pushinteger(L, info.port);
  }
  else {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 3;
}

static int luaf_connect(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  const char* host = luaL_checkstring(L, 2);
  lws_ushort port = (lws_ushort)luaL_checkinteger(L, 3);
  if (lua_isnoneornil(L, 4)) {
    lws_int ok = lws::connect(ud->handle, host, port);
    lua_pushboolean(L, ok == lws_true ? 1 : 0);
    return 1;
  }
  luaL_checktype(L, 4, LUA_TFUNCTION);
  int rcb = luaC_ref(L, 4);
  lws_int ok = lws::connect(ud->handle, host, port, [rcb](int ec) {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    unref_if_return  unref_rcb(L, rcb);

    luaC_rawgeti(L, rcb);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
      return;
    }
    lua_pushinteger(L, ec);
    if (luaC_xpcall(L, 1, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
  if (ok != lws_true) {
    luaC_unref(L, rcb);
  }
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_geturi(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  char* uri = nullptr;
  lws_int ok = lws::geturi(ud->handle, &uri);
  if (ok == lws_true) {
    lua_pushboolean(L, 1);
    lua_pushstring(L, uri);
  }
  else {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
  }
  return 2;
}

static int luaf_seturi(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  const char* uri = luaL_checkstring(L, 2);
  lws_int ok = lws::seturi(ud->handle, uri);
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_getheader(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  const char* name = luaL_checkstring(L, 2);
  char* value = nullptr;
  lws_int ok = lws::getheader(ud->handle, name, &value);
  if (ok == lws_true) {
    lua_pushboolean(L, 1);
    lua_pushstring(L, value);
  }
  else {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
  }
  return 2;
}

static int luaf_setheader(lua_State* L) {
  auto ud = luaC_checkudata<ud_context>(L, 1, LUAC_SOCKET);
  if (ud->accept) {
    luaL_error(L, "invalid method");
  }
  const char* name  = luaL_checkstring(L, 2);
  const char* value = luaL_checkstring(L, 3);
  lws_int ok = lws::setheader(ud->handle, name, value);
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

static int luaf_wwwget(lua_State* L) {
  const char* url = luaL_checkstring(L, 1);
  std::string data;
  lws_int ok = lws::wwwget(url, data);
  if (ok == lws_true) {
    lua_pushboolean(L, 1);
    lua_pushlstring(L, data.c_str(), data.size());
  }
  else {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
  }
  return 2;
}

/********************************************************************************/

static void init_metatable(lua_State* L) {
  const luaL_Reg methods[] = {
    { "__gc",       luaf_gc         },
    { "close",      luaf_close      },
    { "id",         luaf_id         },
    { "connect",    luaf_connect    },
    { "listen",     luaf_listen     },
    { "accept",     luaf_accept     },
    { "endpoint",   luaf_endpoint   },
    { "read",       luaf_read       },
    { "write",      luaf_write      },
    { "send",       luaf_send       },
    { "receive",    luaf_receive    },
    { "geturi",     luaf_geturi     },
    { "getheader",  luaf_getheader  },
    { "seturi",     luaf_seturi     },
    { "setheader",  luaf_setheader  },
    { NULL,         NULL            }
  };
  luaC_newmetatable(L, LUAC_SOCKET, methods);
  lua_pop(L, 1);
}

LUAC_API int luaopen_socket(lua_State* L) {
  init_metatable(L);
  const luaL_Reg methods[] = {
    { "wwwget",   luaf_wwwget   },
    { "socket",   luaf_socket   },
    { "acceptor", luaf_acceptor },
    { NULL,       NULL          }
  };
  lua_getglobal(L, "io");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'io' from stack */
  return 0;
}

/********************************************************************************/
