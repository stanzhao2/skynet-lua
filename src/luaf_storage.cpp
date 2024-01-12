

#include <mutex>
#include <map>
#include "luaf_storage.h"

/*******************************************************************************/

static std::recursive_mutex _mutex;
static std::map<std::string, std::string> _storage;

/*******************************************************************************/

static int luaf_storage_set(lua_State* L) {
  std::string old_value;
  std::string key = luaL_checkstring(L, 1);
  int top = lua_gettop(L);
  if (top < 2) {
    luaL_error(L, "no value");
  }
  luaC_pack(L, lua_gettop(L) - 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter != _storage.end()) {
    old_value = iter->second;
  }
  _storage[key] = luaL_checkstring(L, -1);
  if (old_value.empty()) {
    lua_pushnil(L);
    return 1;
  }
  lua_pushlstring(L, old_value.c_str(), old_value.size());
  return luaC_unpack(L);
}

static int luaf_storage_set_if(lua_State* L) {
  std::string old_value;
  std::string key = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  int top = lua_gettop(L);
  if (top < 3) {
    luaL_error(L, "no values");
  }
  int argc = top - 2;
  lua_pushvalue(L, 2); /* push function to stack */
  for (int i = 3; i <= top; i++) {
    lua_pushvalue(L, i); /* push params to stack */
  }
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter != _storage.end()) {
    old_value = iter->second;
  }
  if (!old_value.empty()) {
    lua_pushlstring(L, old_value.c_str(), old_value.size());
    argc += luaC_unpack(L);
  }
  if (luaC_xpcall(L, argc, 1) != LUA_OK) {
    lua_ferror("%s\n", luaL_checkstring(L, -1));
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    return 2;
  }
  if (lua_toboolean(L, -1) == 0) {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    return 2;
  }
  lua_settop(L, top);
  luaC_pack (L, top - 2);
  size_t size = 0;
  const char* data = luaL_checklstring(L, -1, &size);
  _storage[key] = std::move(std::string(data, size));
  if (old_value.empty()) {
    lua_pushboolean(L, 1);
    lua_pushnil(L);
    return 2;
  }
  lua_pushboolean(L, 1);
  lua_pushlstring(L, old_value.c_str(), old_value.size());
  return luaC_unpack(L) + 1;
}

static int luaf_storage_get(lua_State* L) {
  std::string key = luaL_checkstring(L, 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);

  auto iter = _storage.find(key);
  if (iter == _storage.end()) {
    lua_pushnil(L);
    return 1;
  }
  std::string value = iter->second;
  lua_pushlstring(L, value.c_str(), value.size());
  return luaC_unpack(L);
}

static int luaf_storage_exist(lua_State* L) {
  std::string key = luaL_checkstring(L, 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter == _storage.end()) {
    lua_pushboolean(L, 0);
  }
  else {
    lua_pushboolean(L, 1);
  }
  return 1;
}

static int luaf_storage_erase(lua_State* L) {
  std::string key = luaL_checkstring(L, 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter == _storage.end()) {
    lua_pushnil(L);
    return 1;
  }
  std::string value = iter->second;
  _storage.erase(iter);
  lua_pushlstring(L, value.c_str(), value.size());
  return luaC_unpack(L);
}

static int luaf_storage_size(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  lua_pushinteger(L, (lua_Integer)_storage.size());
  return 1;
}

static int luaf_storage_empty(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  lua_pushboolean(L, _storage.empty() ? 1 : 0);
  return 1;
}

static int luaf_storage_clear(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  _storage.clear();
  return 0;
}

/*******************************************************************************/

LUAC_API int luaC_open_storage(lua_State* L) {
  lua_newtable(L);
  const luaL_Reg methods[] = {
    { "exist",    luaf_storage_exist  },
    { "size",     luaf_storage_size   },
    { "empty",    luaf_storage_empty  },
    { "set",      luaf_storage_set    },
    { "set_if",   luaf_storage_set_if },
    { "get",      luaf_storage_get    },
    { "erase",    luaf_storage_erase  },
    { "clear",    luaf_storage_clear  },
    { NULL,       NULL                }
  };
  luaL_setfuncs(L, methods, 0);
  lua_setglobal(L, "storage");
  return 0;
}

/*******************************************************************************/
