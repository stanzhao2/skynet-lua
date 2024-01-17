

#include <map>
#include "luaf_leak.h"

/********************************************************************************/

struct leak_node {
  size_t size;
  int    type;
  std::string filename;
};

static thread_local bool leak_enable = false;
static thread_local std::map<void*, leak_node> memory_leaks;

static lua_State *getthread(lua_State *L) {
  return lua_isthread(L, 1) ? lua_tothread(L, 1) : L;
}

static void fileline(lua_State* L, std::string& out) {
  L = getthread(L);
  char filename[1024] = { 0 };
  for (int i = 0; i < 10; i++) {
    lua_Debug ar;
    if (!lua_getstack(L, i, &ar)) {
      break;
    }
    if (!lua_getinfo(L, "Sln", &ar)) {
      break;
    }
    if (ar.currentline > 0) {
      snprintf(filename, sizeof(filename), "<%s:%d>", ar.short_src, ar.currentline);
      out.assign(filename);
      break;
    }
  }
}

static void on_memfree(lua_State* L, void* ptr, size_t osize) {
  auto iter = memory_leaks.find(ptr);
  if (iter != memory_leaks.end()) {
    memory_leaks.erase(iter);
  }
}

static void on_memralloc(lua_State* L, void* ptr, size_t osize, size_t nsize, void* pnew) {
  leak_node node;
  node.size = nsize;
  if (ptr == nullptr) {
    int type = (int)osize;
    switch (type) {
    case LUA_TNONE:
    case LUA_TNIL:
    case LUA_TBOOLEAN:
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
    case LUA_TNUMBER:
    case LUA_TSTRING:
      return;
    }
    fileline(getthread(L), node.filename);
    if (!node.filename.empty()) {
      node.type = type;
      memory_leaks[pnew] = node;
    }
    return;
  }
  if (pnew == nullptr) {
    return;
  }
  auto iter = memory_leaks.find(ptr);
  if (iter != memory_leaks.end()) {
    node.type = iter->second.type;
    node.filename = iter->second.filename;
    memory_leaks.erase(iter);
    memory_leaks[pnew] = node;
  }
}

static void lua_on_hook(lua_State* L, lua_Debug* ar) {
  /* to do nothing */
}

static int luaf_snapshot(lua_State* L) {
  luaL_checktype(L, 1, LUA_TBOOLEAN);
  leak_enable = lua_toboolean(L, 1) ? true : false;
  if (leak_enable) {
    memory_leaks.clear();
    if (!lua_gethookmask(L)) {
      lua_sethook(L, lua_on_hook, LUA_MASKLINE, 0);
    }
    return 0;
  }
  if (lua_gethook(L) == lua_on_hook) {
    lua_sethook(L, nullptr, 0, 0);
  }
  lua_newtable(L);
  auto iter = memory_leaks.begin();
  for (int i = 1; iter != memory_leaks.end(); ++iter, i++) {
    const leak_node& node = iter->second;
    lua_pushstring(L, lua_typename(L, node.type));
    lua_setfield(L, -2, node.filename.c_str());
  }
  return 1;
}

/********************************************************************************/

LUAC_API int luaC_open_leak(lua_State* L) {
  const luaL_Reg methods[] = {
    { "snapshot",   luaf_snapshot }, /* bind(f [, arg1, ...]) */
    { NULL,         NULL          }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return 0;
}

LUAC_API void* luaC_leakcheck(void* ud, void* ptr, size_t osize, size_t nsize) {
  auto pnew = luaC_realloc(ud, ptr, osize, nsize);
  if (leak_enable) {
    lua_State* L = luaC_getlocal();
    nsize ? on_memralloc(L, ptr, osize, nsize, pnew) : on_memfree(L, ptr, osize);
  }
  return pnew;
}

/********************************************************************************/
