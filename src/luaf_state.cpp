

#include "luaf_state.h"
#include "luaf_allotor.h"
#include "luaf_string.h"
#include "luaf_http.h"
#include "luaf_json.h"
#include "luaf_bind.h"
#include "luaf_gzip.h"
#include "luaf_crypto.h"
#include "luaf_pload.h"
#include "luaf_timer.h"
#include "luaf_dir.h"
#include "luaf_clock.h"
#include "luaf_socket.h"
#include "luaf_rpcall.h"
#include "luaf_storage.h"

#include <string.h>
#include "eport/detail/os/os.hpp"

/********************************************************************************/

static const lua_CFunction modules[] = {
  luaC_open_pcall,
  luaC_open_bind,
  luaC_open_string,
  luaC_open_print,
  luaC_open_http,
  luaC_open_json,
  luaC_open_pack,
  luaC_open_socket,
  luaC_open_timer,
  luaC_open_clock,
  luaC_open_dir,
  luaC_open_gzip,
  luaC_open_base64,
  luaC_open_crypto,
  luaC_open_pload,
  luaC_open_require,
  luaC_open_rpcall,
  luaC_open_storage,
  /* ... */
  NULL
};

/********************************************************************************/

#ifdef _MSC_VER
#define FSO_EXT ".dll"
#else
#define FSO_EXT ".so"
#endif
#define LUAC_DIR "lua" LUA_DIRSEP LUA_VERSION_MAJOR "." LUA_VERSION_MINOR

static void warnfoff (void *ud, const char *message, int tocont);
static void warnfon  (void *ud, const char *message, int tocont);
static void warnfcont(void *ud, const char *message, int tocont);

static int panic(lua_State *L) {
  const char *msg = lua_tostring(L, -1);
  if (msg == NULL) {
    msg = "error object is not a string";
  }
  lua_writestringerror("PANIC: unprotected error in call to Lua API (%s)\n", msg);
  return 0;
}

static int checkcontrol(lua_State *L, const char *message, int tocont) {
  if (tocont || *(message++) != '@')  /* not a control message? */
    return 0;
  else {
    if (strcmp(message, "off") == 0)
      lua_setwarnf(L, warnfoff, L);  /* turn warnings off */
    else if (strcmp(message, "on") == 0)
      lua_setwarnf(L, warnfon, L);   /* turn warnings on */
    return 1;  /* it was a control message */
  }
}

static void warnfcont(void *ud, const char *message, int tocont) {
  lua_State *L = (lua_State *)ud;
  lua_writestringerror("%s", message);  /* write message */
  if (tocont) { /* not the last part? */
    lua_setwarnf(L, warnfcont, L);  /* to be continued */
  }
  else {  /* last part */
    lua_writestringerror("%s", "\n");  /* finish message with end-of-line */
    lua_setwarnf(L, warnfon, L);  /* next call is a new message */
  }
}

static void warnfon(void *ud, const char *message, int tocont) {
  if (checkcontrol((lua_State*)ud, message, tocont)) { /* control message? */
    return;  /* nothing else to be done */
  }
  lua_writestringerror("%s", "Lua warning: ");  /* start a new warning */
  warnfcont(ud, message, tocont);  /* finish processing */
}

static void warnfoff(void *ud, const char *message, int tocont) {
  checkcontrol((lua_State *)ud, message, tocont);
}

static int global_check(lua_State* L) {
  lua_Debug ar;
  if (lua_getstack(L, 1, &ar) == 0) {
    lua_rawset(L, 1);
    return 0;
  }
  if (lua_getinfo(L, "Sln", &ar) && ar.currentline < 0) {
    lua_rawset(L, 1);
    return 0;
  }
  const char* name = luaL_checkstring(L, -2);
  if (lua_isfunction(L, -1)) {
    if (strcmp(name, LUAC_MAIN) == 0) {
      lua_rawset(L, 1);
      return 0;
    }
  }
  return luaL_error(L, "Cannot use global variables: %s", name);
}

static void disable_global(lua_State* L) {
  lua_getglobal(L, LUA_GNAME);
  lua_getfield(L, -1, "__newindex");
  if (lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return;
  }
  lua_pop(L, 1);
  luaL_newmetatable(L, "placeholders");
  lua_pushliteral(L, "__newindex");
  lua_pushcfunction(L, global_check);

  lua_rawset(L, -3);
  lua_setmetatable(L, -2);
  lua_pop(L, 1);
}

static std::string exedir() {
  return eport::os::pmd();
}

static void setpath(lua_State* L) {
  auto module_path = exedir();
  const char* path[] = {
    "lua" LUA_DIRSEP LUA_VERSION_MAJOR "." LUA_VERSION_MINOR LUA_DIRSEP "?.lua",
    "lua" LUA_DIRSEP LUA_VERSION_MAJOR "." LUA_VERSION_MINOR LUA_DIRSEP "?" LUA_DIRSEP "init.lua",
  };
  auto count = sizeof(path) / sizeof(const char*);

  std::string dir;
  for (size_t i = 0; i < count; i++) {
    dir.append(path[i]);
    dir.append(";");
  }
  for (size_t i = 0; i < count; i++) {
    dir.append(module_path + LUA_DIRSEP + path[i]);
    if (i + 1 < count) {
      dir.append(";");
    }
  }

  lua_getglobal(L, LUA_LOADLIBNAME);
  lua_pushlstring(L, dir.c_str(), dir.size());
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);
}

static void setcpath(lua_State* L) {
  auto module_path = exedir();
  const char* path[] = {
    "lib" LUA_DIRSEP "lua" LUA_DIRSEP LUA_VERSION_MAJOR "." LUA_VERSION_MINOR LUA_DIRSEP "?" FSO_EXT
  };
  std::string dir;
  auto count = sizeof(path) / sizeof(const char*);
  for (size_t i = 0; i < count; i++) {
    dir.append(path[i]);
    dir.append(";");
  }
  for (size_t i = 0; i < count; i++) {
    dir.append(module_path + LUA_DIRSEP + path[i]);
    if (i + 1 < count) {
      dir.append(";");
    }
  }
  lua_getglobal(L, LUA_LOADLIBNAME);
  lua_pushlstring(L, dir.c_str(), dir.size());
  lua_setfield(L, -2, "cpath");
  lua_pop(L, 1);
}

/********************************************************************************/

static thread_local lua_State* LL = nullptr;

LUAC_API lua_State* luaC_newstate(lua_Alloc alloc, void* ud) {
  alloc = alloc ? alloc : luaC_realloc;
  lua_State* L = lua_newstate(alloc, ud);
  if (L) {
    lua_atpanic  (L, &panic);
    lua_setwarnf (L, warnfoff, L);  /* default is warnings off */
    luaL_openlibs(L);
    luaC_openlibs(L, modules);

    setpath (L);
    setcpath(L);
    disable_global(L);
    if (LL == nullptr) LL = L;
  }
  return L;
}

LUAC_API void luaC_openlibs(lua_State* L, const lua_CFunction f[]) {
  static thread_local const lua_CFunction* pf = nullptr;
  if (f == nullptr) {
    f = pf;
  }
  else if (f != modules) {
    pf = modules;
  }
  while (f && *f) {
    lua_pushcfunction(L, *f++);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      lua_error(L);
    }
  }
}

LUAC_API bool luaC_debugging() {
  return (*LUA_DIRSEP == '\\');
}

LUAC_API lua_State* luaC_getlocal() {
  return LL;
}

LUAC_API void luaC_close(lua_State* L) {
  if (L) lua_close(L);
}

LUAC_API void* luaC_realloc(void* ud, void* ptr, size_t osize, size_t nsize) {
  (void)ud;  /* not used */
  static thread_local lua_allotor allotor;
  if (nsize == 0) {
    allotor.p_free(ptr, osize);
    return NULL;
  }
  return allotor.p_realloc(ptr, osize, nsize);
}

/********************************************************************************/
