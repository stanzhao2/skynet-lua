

#pragma once

/********************************************************************************/

#include "luaf_state.h"

/********************************************************************************/

LUAC_API void luaC_unref  (lua_State* L, int i);
LUAC_API int  luaC_ref    (lua_State* L, int i);
LUAC_API int  luaC_rawgeti(lua_State* L, int i);

/********************************************************************************/

class unref_if_return final {
  unref_if_return(const unref_if_return&) = delete;
  int _unref;
  int _ref;
  lua_State* _L;

public:
  inline unref_if_return(lua_State* L, int r)
    : _L(L), _ref(r), _unref(1) {
  }
  void cancel() {
    _unref = 0;
  }
  inline ~unref_if_return() {
    if (_unref && _ref) { luaC_unref(_L, _ref); }
  }
};

class revert_if_return final {
  revert_if_return(const revert_if_return&) = delete;
  int _top;
  lua_State* _L;

public:
  inline ~revert_if_return() {
    lua_settop(_L, _top);
  }
  inline revert_if_return(lua_State* L)
    : _L(L), _top(lua_gettop(L)) {
  }
  inline int top() const { return _top; }
};

/********************************************************************************/
