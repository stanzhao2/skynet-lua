

/********************************************************************************/

#include <signal.h>
#include <string.h>
#include "luaf_state.h"
#include "luaf_leak.h"
#include "luaf_skynet.h"

/********************************************************************************/

#ifdef  _MSC_VER
#define SHRDIR '\\'
#else
#define SHRDIR '/'
#endif
#define PARSE_ERROR (-1)

static char         progname[256];
static const char*  progargs[256];
static int          progargc  = 0;
static bool         progdevel = false;

LUAC_API bool luaC_debugging() {
  return progdevel;
}

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
  for (int i = 0; i < argc; i++) {
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

static void on_signal(int code) {
#ifndef _MSC_VER
  printf("\r");
#endif
  luaC_exit();
  signal(code, on_signal);
}

static int pmain(lua_State* L) {
  int argc = (int)luaL_checkinteger(L, 1);
  const char** argv = (const char**)lua_touserdata(L, 2);
  lua_pushcfunction(L, luaC_execute);
  pargv(argc, argv);
  if (luaC_xpcall(L, argc, 0) != LUA_OK) {
    lua_ferror("%s\n", lua_tostring(L, -1)); /* error? */
  }
  return 0;
}

/********************************************************************************/

static void usage(const char* filename) {
  printf("------------------------------------------------------\n");
  printf("%s\n", "Copyright (C) iccgame.com, All right reserved");
  printf("%s\n", "https://gitee.com/stancpp/skynet-lua.git");
  printf("Version(R): %s\n", SKYNET_VERSION);
  printf("------------------------------------------------------\n");
  printf("Usage: %s [<-d>/<-h>] module [, ...]\n\n", filename);
}

static int check_devel(int n, const char* argv[]) {
  progdevel = true;
  return 0;
}

static int check_helper(int n, const char* argv[]) {
  return PARSE_ERROR;
}

static const struct {
  const char* name;
  int (*parser)(int, const char*[]); } methods[] = {
  { "d",        check_devel    },
  { "devel",    check_devel    },
  { "h",        check_helper   },
  { "?",        check_helper   },
  { "help",     check_helper   },
  { NULL,       NULL           }
};

static int parseargs(int argc, const char* argv[]) {
  auto setprogname = [](const char* s) {
    auto p = strrchr(s, SHRDIR);
    strcpy(progname, p ? p + 1 : s);
  };
  auto nextargv = [](const char* s, int n, const char* v[]) {
    auto find = methods;
    while (find->name) {
      if (strcmp(find->name, s) == 0) {
        return find->parser(n, v);
      }
      find++;
    }
    //option not found
    return PARSE_ERROR;
  };
  auto pushargv = [](const char* argv) {
    progargs[progargc++] = argv;
  };
  int i = 0;
  for (setprogname(argv[i++]); i < argc; i++) {
    const char* name = *(argv + i);
    if (*name != '-' && *name != '/') {
      pushargv(name);
      continue;
    }
    int n = nextargv(name + 1, argc - i - 1, argv + i + 1);
    if (n < 0) {
      //option not found or parse error
      break;
    }
    i += n;
  }
  return i;
}

int main(int argc, const char *argv[]) {
  signal(SIGINT,  on_signal);
  signal(SIGTERM, on_signal);
#ifdef _MSC_VER
  SetConsoleOutputCP(65001);
#endif
  if (parseargs(argc, argv) != argc) {
    usage(progname);
    return 1;
  }
  if (progargc == 0) {
    usage(progname);
    return 1;
  }
  lua_State* L = luaC_newstate(luaC_leakcheck, nullptr);
  lua_pushcfunction(L, pmain);
  lua_pushinteger(L, progargc);
  lua_pushlightuserdata(L, progargs);
  if (luaC_pcall(L, 2, 0) != LUA_OK) {
    lua_ferror("%s\n", lua_tostring(L, -1)); /* error? */
  }
  else {
    lua_fprint("%s exited normally, goodbye!\n", progname);
  }
  luaC_close(L);
  return 0;
}

/********************************************************************************/
