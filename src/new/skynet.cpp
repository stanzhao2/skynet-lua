

#include "skynet.h"

/********************************************************************************/


const lua_CFunction* luaC_modules() {
  static const lua_CFunction modules[] = {
    luaopen_wrapper,
    NULL
  };
  return modules;
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

/********************************************************************************/

static const char* progname = NULL;

static void fatal(const char* message) {
  fprintf(stderr, "%s: %s\n", progname, message);
  exit(EXIT_FAILURE);
}

static int pload(lua_State* L) {
  return luaC_dofile(L, luaC_modules()) == LUA_OK ? 0 : lua_error(L);
}

static int pmain(lua_State* L) {
  int argc = (int)lua_tointeger(L, 1);
  const char** argv = (const char**)lua_touserdata(L, 2);
  if (!lua_checkstack(L, argc)) {
    fatal("too many options");
  }
  lua_pushcfunction(L, pload);
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
  if (lua_pcall(L, argc - 1, 0, 0) != LUA_OK) {
    fatal(lua_tostring(L, -1));
  }
  return 0;
}

int main(int argc, const char* argv[]) {
  progname = argv[0];
  const char* dirsep = strrchr(progname, *LUA_DIRSEP);
  if (dirsep) {
    progname = dirsep + 1;
  }
  if (argc < 2) {
    fatal("no input file given");
  }
  lua_State *L = luaC_state();
  if (L == NULL) {
    fatal("cannot create state: not enough memory");
  }
  lua_pushcfunction(L,&pmain);
  lua_pushinteger(L,argc);
  lua_pushlightuserdata(L,argv);
  if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
    fatal(lua_tostring(L, -1));
  }
  return EXIT_SUCCESS;
}

/********************************************************************************/
