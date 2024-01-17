

#include <tinydir.h>
#include "luaf_dir.h"
#include "eport/detail/os/os.hpp"
#include "eport/detail/io/conv.hpp"

#define LUAC_DIR "os:dir"

/********************************************************************************/

struct ud_dir {
  int closed = 0;
  tinydir_dir tdir;
};

static std::string convto_utf8(const char* str) {
  return eport::is_utf8(str) ? std::string(str) : eport::mbs_to_utf8(str);
}

static int luaf_dir_close(lua_State* L) {
  auto ud = luaC_checkudata<ud_dir>(L, 1, LUAC_DIR);
  if (!ud->closed) {
    ud->closed = 1;
    tinydir_close(&ud->tdir);
  }
  return 0;
}

static int luaf_dir_gc(lua_State* L) {
  if (luaC_debugging()) {
    lua_ftrace("DEBUG: %s will gc\n", LUAC_DIR);
  }
  auto ud = luaC_checkudata<ud_dir>(L, 1, LUAC_DIR);
  int result = luaf_dir_close(L);
  ud->~ud_dir();
  return result;
}

static int luaf_dir_iter(lua_State* L) {
  auto ud = (ud_dir*)lua_touserdata(L, lua_upvalueindex(1));
  if (ud->closed) {
    return 0;
  }
  while (ud->tdir.has_next) {
    tinydir_file file;
    tinydir_readfile(&ud->tdir, &file);
    tinydir_next(&ud->tdir);
    auto filename = convto_utf8(file.name);
    if (filename != "." && filename != "..") {
      lua_pushlstring(L, filename.c_str(), filename.size());
      lua_pushboolean(L, file.is_dir);
      return 2;
    }
  }
  return 0;
}

static int luaf_dir_pairs(lua_State* L) {
  auto ud = luaC_checkudata<ud_dir>(L, 1, LUAC_DIR);
  lua_pushlightuserdata(L, ud);
  lua_pushcclosure(L, luaf_dir_iter, 1);
  return 1;
}

static int luaf_dir_open(lua_State* L) {
  const char* name = luaL_optstring(L, 1, "." LUA_DIRSEP);
  std::string path = convto_utf8(name);
  auto ud = luaC_newuserdata<ud_dir>(L, LUAC_DIR);
  if (tinydir_open(&ud->tdir, path.c_str()) < 0) {
    lua_pushnil(L);
  }
  return 1;
}

static int luaf_dir_make(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  auto ok = eport::os::make_dir(name);
  lua_pushboolean(L, ok ? 1 : 0);
  return 1;
}

/********************************************************************************/

static void init_metatable(lua_State* L) {
  const luaL_Reg methods[] = {
    { "__gc",     luaf_dir_close  },
    { "__pairs",  luaf_dir_pairs  },
    { "close",    luaf_dir_close  },
    { NULL,       NULL            }
  };
  luaC_newmetatable(L, LUAC_DIR, methods);
  lua_pop(L, 1);
}

LUAC_API int luaC_open_dir(lua_State* L) {
  init_metatable(L);
  const luaL_Reg methods[] = {
    { "mkdir",    luaf_dir_make   }, /* os.mkdir(name) */
    { "opendir",  luaf_dir_open   }, /* os.opendir(name) */
    { NULL,       NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return 0;
}

/********************************************************************************/
