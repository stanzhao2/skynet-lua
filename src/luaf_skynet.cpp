

/********************************************************************************/

#include <signal.h>
#include <string.h>
#include "luaf_state.h"
#include "luaf_skynet.h"

/********************************************************************************/

static bool is_integer(const char* p) {
  while (*p) {
    if (*p < '0' || *p > '9') return false;
    p++;
  }
  return true;
}

static bool is_number(const char* p) {
  int dotcnt = 0;
  while (*p) {
    if (*p < '0' || *p > '9') {
      if (*p != '.' || dotcnt++) {
        return false;
      }
    }
    p++;
  }
  return true;
}

static bool is_boolean(const char* p) {
  return (strcmp(p, "true") == 0 || strcmp(p, "false") == 0);
}

static void pargv(int argc, const char** argv) {
  lua_State* L = luaC_getlocal();
  lua_pushstring(L, argv[1]);
  for (int i = 2; i < argc; i++) {
    const char* p = argv[i];
    if (is_integer(p)) {
      lua_pushinteger(L, std::stoll(p));
      continue;
    }
    if (is_number(p)) {
      lua_pushnumber(L, std::stod(p));
      continue;
    }
    if (is_boolean(p)) {
      lua_pushboolean(L, strcmp(p, "true") == 0 ? 1 : 0);
      continue;
    }
    lua_pushstring(L, p);
  }
}

static int pmain(lua_State* L) {
  int argc = (int)luaL_checkinteger(L, 1);
  const char** argv = (const char**)lua_touserdata(L, 2);
  lua_pushcfunction(L, luaC_execute);
  pargv(argc, argv);
  if (luaC_xpcall(L, luaC_max(2, argc) - 1, 0) != LUA_OK) {
    auto err = lua_tostring(L, -1);
    lua_ferror("%s\n", err); /* error? */
  }
  return 0;
}

static void on_signal(int code) {
#ifndef _MSC_VER
  printf("\r");
#endif
  luaC_exit();
  signal(code, on_signal);
}

static void usage(const char* filename) {
  const char* p = strrchr(filename, *LUA_DIRSEP);
  filename = p ? p + 1 : filename;
  printf("%s\n", "Copyright (C) iccgame.com, All right reserved");
  printf("%s\n", "https://gitee.com/stancpp/skynet-lua.git");
  printf("Version(R): %s\n\n", SKYNET_VERSION);
  printf("Usage: %s module [, ...]\n\n", filename);
}

int main(int argc, const char *argv[]) {
  signal(SIGINT,  on_signal);
  signal(SIGTERM, on_signal);
#ifdef _MSC_VER
  SetConsoleOutputCP(65001);
#endif
  if (argc == 1) {
    usage(argv[0]);
    return 0;
  }
  lua_State* L = luaC_newstate(nullptr, nullptr);
  lua_pushcfunction(L, pmain);
  lua_pushinteger(L, argc);
  lua_pushlightuserdata(L, argv);
  if (luaC_pcall(L, 2, 0) != LUA_OK) {
    lua_ferror("%s\n", lua_tostring(L, -1)); /* error? */
  }
  luaC_close(L);
  return 0;
}

/********************************************************************************/
