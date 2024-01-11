

#include <list>
#include "luaf_list.h"

/********************************************************************************/

#define LUAC_LIST "std:list"

struct ud_list {
  ud_list() : iter(data.end()) {}
  std::list<int> data;
  std::list<int>::iterator iter;
};

/********************************************************************************/

static ud_list* luaf_checklist(lua_State* L, int i) {
  return luaC_checkudata<ud_list>(L, i, LUAC_LIST);
}

static int luaf_list_clear(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  for (auto iter = ud->data.begin(); iter != ud->data.end(); ++iter) {
    luaC_unref(L, *iter);
  }
  ud->data.clear();
  return 0;
}

static int luaf_list_gc(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  luaf_list_clear(L);
  ud->~ud_list();
  return 0;
}

static int luaf_list_size(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  lua_pushinteger(L, (lua_Integer)ud->data.size());
  return 1;
}

static int luaf_list_empty(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  lua_pushboolean(L, ud->data.empty() ? 1 : 0);
  return 1;
}

static int luaf_list_erase(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  if (ud->iter != ud->data.end()) {
    auto i = --ud->iter;
    luaC_unref(L, *i);
    ud->iter++;
    ud->data.erase(i);
  }
  else if (!ud->data.empty()) {
    luaC_unref(L, ud->data.back());
    ud->data.pop_back();
  }
  return 0;
}

static int luaf_list_reverse(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  ud->data.reverse();
  return 0;
}

static int luaf_list_front(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  if (ud->data.empty()) {
    lua_pushnil(L);
  }
  else {
    luaC_rawgeti(L, ud->data.front());
  }
  return 1;
}

static int luaf_list_back(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  if (ud->data.empty()) {
    lua_pushnil(L);
  }
  else {
    luaC_rawgeti(L, ud->data.back());
  }
  return 1;
}

static int luaf_list_pop_front(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  if (!ud->data.empty()) {
    int front = ud->data.front();
    ud->data.pop_front();
    luaC_unref(L, front);
  }
  return 0;
}

static int luaf_list_pop_back(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  if (!ud->data.empty()) {
    if (ud->iter != ud->data.end()) {
      --ud->iter;
    }
    int back = ud->data.back();
    ud->data.pop_back();
    luaC_unref(L, back);
    if (ud->data.empty()) {
      ud->iter = ud->data.end();
    }
    else if (ud->iter != ud->data.end()) {
      ++ud->iter;
    }
  }
  return 0;
}

static int luaf_list_push_front(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  luaL_checkany(L, 2);
  ud->data.push_front(luaC_ref(L, 2));
  return 0;
}

static int luaf_list_push_back(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  luaL_checkany(L, 2);
  ud->data.push_back(luaC_ref(L, 2));
  return 0;
}

static int luaf_list_iterator(lua_State* L) {
  auto ud = (ud_list*)lua_touserdata(L, lua_upvalueindex(1));
  if (ud->iter == ud->data.end()) {
    return 0;
  }
  luaC_rawgeti(L, *(ud->iter++));
  return 1;
}

static int luaf_list_pairs(lua_State* L) {
  auto ud = luaf_checklist(L, 1);
  ud->iter = ud->data.begin();

  lua_pushlightuserdata(L, ud);
  lua_pushcclosure(L, luaf_list_iterator, 1);
  return 1;
}

static void init_metatable(lua_State* L) {
  const luaL_Reg methods[] = {
    { "__gc",         luaf_list_gc          },
    { "__pairs",      luaf_list_pairs       },
    { "__len",        luaf_list_size        },
    { "size",         luaf_list_size        },
    { "empty",        luaf_list_empty       },
    { "clear",        luaf_list_clear       },
    { "erase",        luaf_list_erase       },
    { "front",        luaf_list_front       },
    { "back",         luaf_list_back        },
    { "reverse",      luaf_list_reverse     },
    { "push_front",   luaf_list_push_front  },
    { "pop_front",    luaf_list_pop_front   },
    { "push_back",    luaf_list_push_back   },
    { "pop_back",     luaf_list_pop_back    },
    { NULL,           NULL                  }
  };
  luaC_newmetatable(L, LUAC_LIST, methods);
  lua_pop(L, 1);
}

static int luaf_list_new(lua_State* L) {
  auto pp = luaC_newuserdata<ud_list>(L, LUAC_LIST);
  if (!pp) {
    lua_pushnil(L);
    lua_pushstring(L, "no memory");
    return 2;
  }
  auto ud = new (pp) ud_list();
  return 1;
}

/********************************************************************************/

LUALIB_API int luaf_open_list(lua_State* L) {
  init_metatable(L);
  bool need_pop = true;
  lua_getglobal(L, "std");
  if (lua_type(L, -1) != LUA_TTABLE) {
    lua_pop(L, 1);
    lua_newtable(L);
    need_pop = false;
  }
  const luaL_Reg methods[] = {
    { "list",     luaf_list_new   },  /* std.list()  */
    { NULL,       NULL            },
  };
  luaL_setfuncs(L, methods, 0);
  if (need_pop) {
    lua_pop(L, 1); /* pop 'std' from stack */
  }
  else {
    lua_setglobal(L, "std");
  }
  return 0;
}

/********************************************************************************/
