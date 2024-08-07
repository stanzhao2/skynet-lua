

#include "luaf_state.h"
#include "luaf_misc.h"
#include "luaf_allotor.h"
#include "luaf_string.h"
#include "luaf_compile.h"
#include "luaf_http.h"
#include "luaf_json.h"
#include "luaf_leak.h"
#include "luaf_list.h"
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
#include "luaf_skynet.h"
#include "eport/detail/os/os.hpp"

/********************************************************************************/

static int luaf_os_version(lua_State* L) {
  lua_pushstring(L, SKYNET_VERSION);
  return 1;
}

static int luaC_open_version(lua_State* L) {
  const luaL_Reg methods[] = {
    { "version",    luaf_os_version },
    { NULL,         NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'io' from stack */
  return 0;
}

static const lua_CFunction modules[] = {
  luaC_open_version,
  luaC_open_misc,
  luaC_open_pcall,
  luaC_open_bind,
  luaC_open_compile,
  luaC_open_string,
  luaC_open_print,
  luaC_open_http,
  luaC_open_json,
  luaC_open_leak,
  luaf_open_list,
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

/********************************************************************************/

#ifdef _MSC_VER
#define LIBEXT   ".dll"
#define readlink GetModuleFileNameA
#define procself NULL
#else
#define LIBEXT   ".so"
#define procself "/proc/self/exe"
#endif

static const char* reallink(char* path, int size) {
  auto rslt = readlink(procself, path, size - 1);
  if (rslt <= 0) {
    return NULL;
  }
  path[rslt] = '\0';
  for (int i = rslt; i >= 0; i--) {
    if (path[i] == *LUA_DIRSEP) {
      path[i] = '\0';
      break;
    }
  }
  return path;
}

static void setpath(lua_State* L, const char* name, const char* path[]) {
  luaL_Buffer buf;
  luaL_buffinit(L, &buf);
  for (int i = 0; path[i]; i++) {
    luaL_addstring(&buf, path[i]);
    luaL_addchar(&buf, ';');
  }
  char dir[1024];
  if (reallink(dir, sizeof(dir))) {
    for (int i = 0; path[i]; i++) {
      luaL_addstring(&buf, dir);
      luaL_addstring(&buf, LUA_DIRSEP);
      luaL_addstring(&buf, path[i]);
      if (path[i + 1]) {
        luaL_addchar(&buf, ';');
      }
    }
  }
  luaL_pushresult(&buf);
  lua_getglobal(L, LUA_LOADLIBNAME);
  lua_rotate   (L, -2, -1);
  lua_setfield (L, -2, name);
  lua_pop(L, 1);
}

static void initpath(lua_State* L) {
  const char* path[] = {
    "lua" LUA_DIRSEP "?.lua", "lua" LUA_DIRSEP "?" LUA_DIRSEP "init.lua", NULL
  };
  const char* cpath[] = {
    "lib" LUA_DIRSEP "lua" LUA_DIRSEP "?" LIBEXT, NULL
  };
  setpath(L, "path",  path);
  setpath(L, "cpath", cpath);
}

/********************************************************************************/

static thread_local lua_State* LL = nullptr;

LUAC_API lua_State* luaC_newstate(lua_Alloc alloc, void* ud) {
  alloc = alloc ? alloc : luaC_realloc;
  lua_State* L = lua_newstate(alloc, ud);
  if (L) {
    luaL_checkversion(L);
    lua_gc(L, LUA_GCSTOP);
    lua_atpanic  (L, &panic);
    lua_setwarnf (L, warnfoff, L);  /* default is warnings off */
    luaL_openlibs(L);
    luaC_openlibs(L, modules);

    initpath(L);
    disable_global(L);
    lua_gc(L, LUA_GCRESTART);
    lua_gc(L, LUA_GCGEN, 0, 0);
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
