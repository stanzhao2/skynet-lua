

#include <map>
#include "luaf_leak.h"

/********************************************************************************/

struct leak_node {
  size_t size;
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
  for (int i = 1; i < 100; i++) {
    lua_Debug ar;
    if (!lua_getstack(L, i, &ar)) {
      break;
    }
    if (!lua_getinfo(L, "Sl", &ar)) {
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

static void on_memralloc(lua_State* L, void* ptr, size_t nsize, void* pnew) {
  leak_node node;
  node.size = nsize;
  if (ptr == nullptr) {
    fileline(getthread(L), node.filename);
    if (!node.filename.empty()) {
      memory_leaks[pnew] = node;
    }
    return;
  }
  if (pnew == nullptr) {
    return;
  }
  auto iter = memory_leaks.find(ptr);
  if (iter != memory_leaks.end()) {
    node.filename = iter->second.filename;
    memory_leaks.erase(iter);
    memory_leaks[pnew] = node;
  }
}

static int luaf_snapshot(lua_State* L) {
  luaL_checktype(L, 1, LUA_TBOOLEAN);
  leak_enable = lua_toboolean(L, 1) ? true : false;
  if (leak_enable) {
    memory_leaks.clear();
  }
  else {
    lua_newtable(L);
    auto iter = memory_leaks.begin();
    for (int i = 1; iter != memory_leaks.end(); ++iter, i++) {
      const std::string& f = iter->second.filename;
      lua_pushlstring(L, f.c_str(), f.size());
      lua_rawseti(L, -2, i++);
    }
  }
  return lua_gettop(L) - 1;
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
    nsize ? on_memralloc(L, ptr, nsize, pnew) : on_memfree(L, ptr, osize);
  }
  return pnew;
}

/********************************************************************************/
