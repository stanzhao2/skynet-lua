

#include "allotor.h"
#include "configure.h"
#include "require.h"

/********************************************************************************/

#ifdef _MSC_VER
# define LIBEXT    ".dll"
# define readlink  GetModuleFileNameA
# define procself  NULL
#else
# define LIBEXT    ".so"
# define procself  "/proc/self/exe"
#endif

/********************************************************************************/
/* init path of lua */

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
    luaL_addchar(&buf, *LUA_PATH_SEP);
  }
  char dir[1024];
  if (reallink(dir, sizeof(dir))) {
    for (int i = 0; path[i]; i++) {
      luaL_addstring(&buf, dir);
      luaL_addstring(&buf, LUA_DIRSEP);
      luaL_addstring(&buf, path[i]);
      if (path[i + 1]) {
        luaL_addchar(&buf, *LUA_PATH_SEP);
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
  static const char* path[] = {
    "lua" LUA_DIRSEP LUA_PATH_MARK ".lua", "lua" LUA_DIRSEP LUA_PATH_MARK LUA_DIRSEP "init.lua", NULL
  };
  static const char* cpath[] = {
    "lib" LUA_DIRSEP "lua" LUA_DIRSEP LUA_PATH_MARK LIBEXT, NULL
  };
  setpath(L, "path",  path);
  setpath(L, "cpath", cpath);
}

/********************************************************************************/
/* pcall and xpcall */

static int finishpcall(lua_State *L, int status, lua_KContext extra) {
  if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    lua_pushboolean(L, 0);  /* first result (false) */
    lua_pushvalue(L, -2);  /* error message */
    return 2;  /* return false, msg */
  }
  return lua_gettop(L) - (int)extra;  /* return all results */
}

static lua_State *getthread(lua_State *L, int *arg) {
  if (lua_isthread(L, 1)) {
    *arg = 1;
    return lua_tothread(L, 1);
  }
  *arg = 0;
  return L;  /* function will operate over current thread */
}

static int traceback(lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  const char *msg = lua_tostring(L, arg + 1);
  if (strstr(msg, "stack traceback")) {
    return 1;
  }
  if (msg == NULL && !lua_isnoneornil(L, arg + 1))  /* non-string 'msg'? */
    lua_pushvalue(L, arg + 1);  /* return it untouched */
  else {
    int level = (int)luaL_optinteger(L, arg + 2, (L == L1) ? 1 : 0);
    luaL_traceback(L, L1, msg, level);
  }
  return 1;
}

static int luaf_xpcall(lua_State *L) {
  int status;
  int n = lua_gettop(L);
  luaL_checktype(L, 2, LUA_TFUNCTION);  /* check error function */
  lua_pushboolean(L, 1);  /* first result */
  lua_pushvalue(L, 1);    /* function */
  lua_rotate(L, 3, 2);    /* move them below function's arguments */
  status = luaC_pcallk(L, n - 2, LUA_MULTRET, 2, 2);
  return finishpcall(L, status, 2);
}

static int luaf_pcall(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);  /* check pcall function */
  lua_pushcfunction(L, traceback);
  lua_insert(L, 2);
  return luaf_xpcall(L);
}

static int luaopen_pcall(lua_State* L) {
  static const luaL_Reg methods[] = {
    { "pcall",    luaf_pcall    }, /* pcall (f [, arg1, ...]) */
    { "xpcall",   luaf_xpcall   }, /* xpcall(f, msgh [, arg1, ...]) */
    { NULL,       NULL          }
  };
  lua_getglobal(L, LUA_GNAME);
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop '_G' from stack */
  return 0;
}

/********************************************************************************/
/* bind */

static int call_bind(lua_State* L) {
  int i, n, status;
  int index = lua_upvalueindex(1); /* get the first up-value */
  int up_values = (int)luaL_checkinteger(L, index); /* get the number of up-values */
  lua_checkstack(L, up_values);
  for (i = 2; i <= up_values; i++) {
    lua_pushvalue(L, lua_upvalueindex(i));
  }
  if (up_values > 1) { /* if the function has bound parameters */
    lua_rotate(L, 1, up_values - 1); /* rotate the calling parameters after the binding parameters */
  }
  n = lua_gettop(L) - 1; /* calculate the number of all parameters */
  status = lua_pcall(L, n, LUA_MULTRET, 0);
  if (status != LUA_OK) {
    lua_error(L);
  }
  return lua_gettop(L);
}

static int luaf_bind(lua_State* L) {
  int up_values = lua_gettop(L) + 1; /* number of up-values (function + argvs)*/
  luaL_checktype(L, 1, LUA_TFUNCTION); /* function type check */
  lua_pushinteger(L, up_values); /* push up-values onto the stack */
  lua_rotate(L, 1, 1); /* rotate the function to the top of the stack */
  lua_pushcclosure(L, call_bind, up_values);
  return 1;
}

static int luaopen_bind(lua_State* L) {
  static const luaL_Reg methods[] = {
    { "bind",   luaf_bind }, /* bind(f [, arg1, ...]) */
    { NULL,     NULL      }
  };
  lua_getglobal(L, LUA_GNAME);
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop '_G' from stack */
  return 0;
}

/********************************************************************************/
/* init state of lua */

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

static int luachecker(lua_State* L) {
  lua_Debug ar;
  if (lua_getstack(L, 1, &ar) == 0) {
    lua_rawset(L, 1);
    return 0;
  }
  if (lua_getinfo(L, "Sl", &ar) && ar.currentline < 0) {
    lua_rawset(L, 1);
    return 0;
  }
  const char* name = luaL_checkstring(L, -2);
  if (lua_isfunction(L, -1)) {
    if (strcmp(name, lua_pmain) == 0) {
      lua_rawset(L, 1);
      return 0;
    }
  }
  return luaL_error(L, "missing local (%s is global)", name);
}

static void globalsafe(lua_State* L) {
  lua_getglobal(L, LUA_GNAME);
  lua_getfield(L, -1, "__newindex");
  if (lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return;
  }
  lua_pop(L, 1);
  luaL_newmetatable(L, "placeholders");
  lua_pushliteral(L, "__newindex");
  lua_pushcfunction(L, luachecker);

  lua_rawset(L, -3);
  lua_setmetatable(L, -2);
  lua_pop(L, 1);
}

static void* mallotor(void* ud, void* ptr, size_t osize, size_t nsize) {
  (void)ud;  /* not used */
  lua_allotor* allotor = (lua_allotor*)ud;
  if (nsize == 0) {
    allotor->p_free(ptr, osize);
    return NULL;
  }
  return allotor->p_realloc(ptr, osize, nsize);
}

static int openlibs(lua_State* L) {
  luaL_openlibs(L);
  initpath(L);
  luaopen_require(L);
  luaopen_bind(L);
  luaopen_pcall(L);
  return LUA_OK;
}

static lua_State* newstate(lua_allotor* allotor) {
  lua_State* L = lua_newstate(mallotor, allotor);
  if (L) {
    luaL_checkversion(L);
    lua_atpanic(L, &panic);
    lua_setwarnf(L, warnfoff, L);  /* default is warnings off */
    lua_gc(L, LUA_GCSTOP);
    lua_pushcfunction(L, openlibs);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      lua_close(L);
      return NULL;
    }
    lua_gc(L, LUA_GCRESTART);
    lua_gc(L, LUA_GCGEN, 0, 0);
    globalsafe(L);

#ifdef _DEBUG
    lua_pushboolean(L, 1);
#else
    lua_pushboolean(L, 0);
#endif
    lua_setglobal(L, "DEBUG");
  }
  return L;
}

/********************************************************************************/
/* methods */

lua_State* luaC_state() {
  static thread_local class LL {
    lua_allotor _allotor;
    lua_State*  _L;
  public:
    inline  LL() : _L(newstate(&_allotor)) {};
    inline ~LL() { lua_close(_L); }
    inline lua_State* get() const { return _L; }
  } local;
  return local.get();
}

int luaC_pcallk(lua_State* L, int n, int r, int f, lua_KContext k) {
  return lua_pcallk(L, n, r, f, k, finishpcall);
}

int luaC_dofile(lua_State* L, const lua_CFunction *f) {
  char name[1024];
  strcpy(name, luaL_checkstring(L, 1));
  char* pdot = strrchr(name, '.');
  if (pdot) {
    if (strcmp(pdot, ".lua") == 0) {
      *pdot = '\0';
    }
  }
  for (int i = 0; ;i++) {
    char c = name[i];
    if (c == 0) {
      break;
    }
    if (c == '/' || c == '\\') {
      name[i] = '.';
    }
  }
  lua_pushstring(L, name);
  lua_replace(L, 1);
  lua_pushvalue(L, 1);
  lua_setglobal(L, "progname");

  while (f && *f) {
    lua_pushcfunction(L, *f++);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      return LUA_ERRERR;
    }
  }

  int top = lua_gettop(L);
  lua_getglobal(L, LUA_LOADLIBNAME); /* package */
  lua_getfield(L, -1, "searchers");  /* searchers or loaders */
  lua_rawgeti(L, -1, 2);
  lua_pushvalue(L, 1);
  int result = lua_pcall(L, 1, LUA_MULTRET, 0);
  if (result != LUA_OK) {
    return LUA_ERRERR;
  }
  int retcnt = lua_gettop(L) - top - 2;
  if (retcnt != 2) {
    return LUA_ERRERR;
  }
  result = lua_pcall(L, 1, 0, 0);
  if (result != LUA_OK) {
    return LUA_ERRERR;
  }
  lua_settop(L, top);
  lua_getglobal(L, lua_pmain);
  if (lua_isfunction(L, -1)) {
    lua_rotate(L, 2, 1);
    return lua_pcall(L, top - 1, 0, 0);
  }
  return LUA_OK;
}

void luaC_setmetatable(lua_State* L, const char* name) {
  luaL_getmetatable(L, name);
  lua_setmetatable(L, -2);
}

int luaC_newmetatable(lua_State* L, const char* name, const luaL_Reg methods[]) {
  if (!luaL_newmetatable(L, name)) {
    return 0;
  }
  /* define methods */
  if (methods) {
    luaL_setfuncs(L, methods, 0);
  }
  /* define metamethods */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);

  lua_pushliteral(L, "__metatable");
  lua_pushliteral(L, "you're not allowed to get this metatable");
  lua_settable(L, -3);
  return 1;
}

void* luaC_newuserdata(lua_State* L, const char* name, size_t size) {
  void* userdata = lua_newuserdata(L, size);
  luaC_setmetatable(L, name);
  return userdata;
}

/********************************************************************************/
