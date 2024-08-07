

#include "luaf_rpcall.h"
#include "socket.io/socket.io.hpp"

#include <mutex>
#include <map>
#include <set>
#include <vector>

/********************************************************************************/

typedef struct __node_type {
  size_t who = 0;
  int rcb = 0; /* callback refer */
  int opt = 0; /* enable call by remote */
  inline bool operator<(const __node_type& r) const {
    return who < r.who;
  }
} node_type;

struct pend_invoke {
  lws_int caller;
  size_t  timeout;
};

typedef std::string topic_type;

typedef std::set<
  node_type
> rpcall_set_type;

typedef std::map<
  topic_type, rpcall_set_type
> rpcall_map_type;

typedef std::map<
  int, pend_invoke
> invoke_map_type;

#define max_expires  10000
#define is_local(what) (what <= 0xffff)
#define unique_mutex_lock(what) std::unique_lock<std::mutex> lock(what)

static std::mutex      rpcall_lock;
static lws_int         watcher_timer = 0;
static lws_int         watcher_ios   = 0;
static lws_int         watcher_luaf  = 0;
static lua_CFunction   watcher_cfn   = nullptr;
static rpcall_map_type rpcall_handlers;
static thread_local size_t rpcall_caller = 0;
static thread_local bool rpcall_complete = false;
static thread_local std::string rpcall_result;
static thread_local invoke_map_type invoke_pendings;

/********************************************************************************/

static int watch_handler(lua_State* L) {
  if (watcher_cfn != nullptr) {
    return watcher_cfn(L);
  }
  if (watcher_luaf == 0) {
    return 0;
  }
  int argc = lua_gettop(L);
  luaC_rawgeti(L, watcher_luaf);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    return 0;
  }
  lua_rotate(L, 1, 1);
  if (luaC_xpcall(L, argc, 0) != LUA_OK) {
    lua_ferror("%s\n", lua_tostring(L, -1));
  }
  return 0;
}

static void cancel_invoke(int rcf) {
  auto L = luaC_getlocal();
  revert_if_return revert(L);
  unref_if_return  unref_rcf(L, rcf);

  luaC_rawgeti(L, rcf);
  int typeof_rcf = lua_type(L, -1);

  if (typeof_rcf == LUA_TTHREAD) {
    auto coL = lua_tothread(L, -1);
    if (lua_status(coL) != LUA_YIELD) {
      return;
    }
    int argc = 1;
    lua_pushboolean(coL, 0); /* false */
    lua_pushstring(coL, "timeout");
    lua_resume(coL, L, 2, &argc);
    return;
  }

  if (typeof_rcf == LUA_TFUNCTION) {
    lua_pushboolean(L, 0); /* false */
    if (luaC_xpcall(L, 1, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  }
}

/* calling the caller callback function */
static void back_to_local(const std::string& data, int rcf) {
  lua_State* L = luaC_getlocal();
  revert_if_return revert(L);
  unref_if_return  unref_rcf(L, rcf);

  auto iter = invoke_pendings.find(rcf);
  if (iter == invoke_pendings.end()) {
    return;
  }

  luaC_rawgeti(L, rcf);
  int typeof_rcf = lua_type(L, -1);
  invoke_pendings.erase(iter);

  if (typeof_rcf == LUA_TTHREAD) {
    auto coL = lua_tothread(L, -1);
    if (lua_status(coL) != LUA_YIELD) {
      return;
    }
    lua_pushlstring(coL, data.c_str(), data.size());
    int argc = luaC_unpack(coL);
    lua_resume(coL, L, argc, &argc);
    return;
  }

  if (typeof_rcf == LUA_TFUNCTION) {
    lua_pushlstring(L, data.c_str(), data.size());
    int argc = luaC_unpack(L);
    if (luaC_xpcall(L, argc, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  }
}

/* back to response of request */
static void forword(const std::string& data, size_t caller, int rcf) {
  lws_int ok = lws::post(watcher_ios, [=]() {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    lua_pushcfunction(L, watch_handler);
    lua_newtable(L);
    lua_pushstring(L, evr_response);
    lua_setfield(L, -2, "what");
    lua_pushlstring(L, data.c_str(), data.size());
    lua_setfield(L, -2, "data");
    lua_pushinteger(L, (lua_Integer)caller);
    lua_setfield(L, -2, "caller");
    lua_pushinteger(L, (lua_Integer)rcf);
    lua_setfield(L, -2, "rcf");
    if (luaC_xpcall(L, 1, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
}

/* send deliver request */
/*
** who: receiver
** rcf: callback function reference for the caller
*/
static int forword(const topic_type& topic, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf) {
  std::string argv;
  if (data && size) {
    argv.assign(data, size);
  }
  lws_int ok = lws::post(watcher_ios, [=]() {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    lua_pushcfunction(L, watch_handler);
    lua_newtable(L);
    lua_pushstring(L, evr_deliver);
    lua_setfield(L, -2, "what");
    lua_pushlstring(L, topic.c_str(), topic.size());
    lua_setfield(L, -2, "name");
    lua_pushlstring(L, argv.c_str(), argv.size());
    lua_setfield(L, -2, "argv");
    lua_pushinteger(L, (lua_Integer)mask);
    lua_setfield(L, -2, "mask");
    lua_pushinteger(L, (lua_Integer)who);
    lua_setfield(L, -2, "who");
    lua_pushinteger(L, (lua_Integer)caller);
    lua_setfield(L, -2, "caller");
    lua_pushinteger(L, (lua_Integer)rcf);
    lua_setfield(L, -2, "rcf");
    if (luaC_xpcall(L, 1, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
  return (ok == lws_true) ? 1 : 0;
}

/* send bind or unbind request */
static int dispatch(const topic_type& topic, const std::string& what, int rcb, size_t caller) {
  return lws::post(watcher_ios, [=]() {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    lua_pushcfunction(L, watch_handler);
    lua_newtable(L);
    lua_pushlstring(L, what.c_str(), what.size());
    lua_setfield(L, -2, "what");
    lua_pushlstring(L, topic.c_str(), topic.size());
    lua_setfield(L, -2, "name");
    lua_pushinteger(L, (lua_Integer)rcb);
    lua_setfield(L, -2, "rcb");
    lua_pushinteger(L, (lua_Integer)caller);
    lua_setfield(L, -2, "caller");
    if (luaC_xpcall(L, 1, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
}

/* calling the receiver function */
/*
** who: receiver
** rcf: callback function reference for the caller
*/
static int dispatch(const topic_type& topic, int rcb, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf) {
  /* who is receiver */
  if (!is_local(who)) {
    return forword(topic, data, size, mask, who, caller, rcf);
  }
  std::string argv;
  if (data && size) {
    argv.assign(data, size);
  }
  lws_int ok = lws::post((lws_int)who, [=]() {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);

    luaC_rawgeti(L, rcb);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
      return;
    }
    int argc = 0;
    if (!argv.empty()) {
      lua_pushlstring(L, argv.c_str(), argv.size());
      argc += luaC_unpack(L);
    }
    size_t previous = rpcall_caller;
    rpcall_caller = caller;

    int callok = luaC_xpcall(L, argc, LUA_MULTRET);
    if (callok != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
    rpcall_caller = previous;
    /* don't need result */
    if (rcf == 0) {
      return;
    }
    lua_pushboolean(L, callok == LUA_OK ? 1 : 0);
    int count = lua_gettop(L) - revert.top();
    if (count > 1) {
      lua_rotate(L, -count, 1);
    }
    /* return result */
    luaC_pack(L, count);
    size_t nsize;
    const char* result = luaL_checklstring(L, -1, &nsize);
    std::string temp(result, nsize);
    if (is_local(caller)) {
      luaC_r_response(temp, caller, rcf);
      return;
    }
    forword(temp, caller, rcf);
  });
  return (ok == lws_true) ? 1 : 0;
}

/********************************************************************************/

static int luaf_caller(lua_State* L) {
  lua_pushinteger(L, (lua_Integer)rpcall_caller);
  return 1;
}

/* ignoring return values */
static int luaf_deliver(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t mask = luaL_checkinteger(L, 2);
  size_t who  = luaL_checkinteger(L, 3);

  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 3;
  if (argc > 0) {
    luaC_pack(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto caller = lws::getlocal();
  int count = luaC_r_deliver(name, data, size, mask, who, caller, 0);
  lua_pushinteger(L, count);
  return 1;
}

/* async wait return values */
static int luaf_invoke(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);
  int rcf = luaC_ref(L, 1);
  const char* name = luaL_checkstring(L, 2);
  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 2;
  if (argc > 0) {
    luaC_pack(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto caller = lws::getlocal();
  int count = luaC_r_deliver(name, data, size, rand(), 0, caller, rcf);
  if (count > 0) {
    pend_invoke pend;
    pend.caller  = caller;
    pend.timeout = luaC_clock() + max_expires;
    invoke_pendings[rcf] = pend;
  }
  else {
    luaC_unref(L, rcf);
  }
  lua_pushboolean(L, count > 0 ? 1 : 0);
  return 1;
}

static int luaf_rpcall(lua_State* L) {
  if (lua_type(L, 1) == LUA_TFUNCTION) {
    return luaf_invoke(L);
  }
  int rcf = 0;
  if (lua_isyieldable(L)) {
    /* in coroutine */
    lua_State* main = luaC_getlocal();
    lua_pushthread(L);
    lua_xmove(L, main, 1);
    rcf = luaC_ref(main, -1);
    lua_pop(main, 1);
  }
  else {
    /* not in coroutine */
    rpcall_result.clear();
    rpcall_complete = false;
    rcf = 0 - lws::newstate();
  }
  const char* name = luaL_checkstring(L, 1);
  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 1;
  if (argc > 0) {
    luaC_pack(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto caller = lws::getlocal();
  int count = luaC_r_deliver(name, data, size, rand(), 0, caller, rcf);
  if (count == 0) {
    if (rcf > 0) {
      luaC_unref(L, rcf);
    } else {
      lws::close(std::abs(rcf));
    }
    lua_pushboolean(L, 0); /* false */
    lua_pushfstring(L, "%s not found", name);
    return 2;
  }
  /* in coroutine */
  if (rcf > 0) {
    pend_invoke pend;
    pend.caller  = caller;
    pend.timeout = luaC_clock() + max_expires;
    invoke_pendings[rcf] = pend;
    return lua_yieldk(L, 0, 0, 0);
  }
  /* not in coroutine */
  rcf = std::abs(rcf);
  auto begin = luaC_clock();
  while (!rpcall_complete) {
    if (lws::stopped()) {
      break;
    }
    if (!luaC_debugging()) {
      //check timeout
      if (luaC_clock() - begin >= max_expires) {
        break;
      }
    }
    lws::poll();
    lws::runone_for(rcf, 10); //wait
  }
  lws::close(rcf);
  if (lws::stopped()) {
    lua_pushboolean(L, 0); /* false */
    lua_pushstring(L, "cancel");
    return 2;
  }
  if (rpcall_result.empty()) {
    lua_pushboolean(L, 0); /* false */
    lua_pushstring(L, "timeout");
    return 2;
  }
  lua_pushlstring(L, rpcall_result.c_str(), rpcall_result.size());
  return luaC_unpack(L);
}

/* register function */
static int luaf_declare(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  int opt = 0;
  if (lua_type(L, 3) == LUA_TBOOLEAN) {
    opt = lua_toboolean(L, 3);
  }
  int ios = lws::getlocal();
  int rcb = luaC_ref(L, 2);
  int result = luaC_r_bind(name, ios, rcb, opt);
  if (result != LUA_OK) {
    luaC_unref(L, rcb);
    lua_pushboolean(L, 0);
  }
  else {
    lua_pushboolean(L, 1);
    if (opt) dispatch(name, evr_bind, rcb, ios);
  }
  return 1;
}

/* unregister function */
static int luaf_undeclare(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  int ios = lws::getlocal();
  int opt = 0;
  int rcb = luaC_r_unbind(name, ios, &opt);
  if (rcb) {
    luaC_unref(L, rcb);
    if (opt) dispatch(name, evr_unbind, rcb, ios);
  }
  lua_pushboolean(L, rcb ? 1 : 0);
  return 1;
}

/* watch events */
static int luaf_lookout(lua_State* L) {
  if (lua_isnoneornil(L, 1)) {
    if (watcher_luaf) {
      luaC_unref(L, watcher_luaf);
      watcher_ios  = 0;
      watcher_luaf = 0;
    }
    return 0;
  }
  luaL_checktype(L, 1, LUA_TFUNCTION);
  if (watcher_luaf) {
    luaL_error(L, "lookout can only be called once");
  }
  watcher_ios  = lws::getlocal();
  watcher_luaf = luaC_ref(L, 1);
  return 0;
}

static int luaf_r_bind(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t who = luaL_checkinteger(L, 2);
  int rcb = (int)luaL_checkinteger(L, 3);
  int result = luaC_r_bind(name, who, rcb, 0);
  lua_pushboolean(L, result == LUA_OK ? 1 : 0);
  return 1;
}

static int luaf_r_unbind(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t who = luaL_checkinteger(L, 2);
  int rcb = luaC_r_unbind(name, who, nullptr);
  lua_pushboolean(L, rcb > 0 ? 1 : 0);
  return 1;
}

static int luaf_r_response(lua_State* L) {
  size_t size;
  const char* data = luaL_checklstring(L, 1, &size);
  size_t caller = luaL_checkinteger(L, 2);
  if (!is_local(caller)) {
    luaL_error(L, "#2 out of range: %d", caller);
  }
  int rcf = (int)luaL_checkinteger(L, 3);
  std::string argv(data, size);
  int result = luaC_r_response(argv, caller, rcf);
  lua_pushboolean(L, result == LUA_OK ? 1 : 0);
  return 1;
}

static int luaf_r_deliver(lua_State* L) {
  size_t size;
  const char* name = luaL_checkstring(L, 1);
  const char* data = luaL_checklstring(L, 2, &size);
  size_t mask      = luaL_checkinteger(L, 3);
  size_t who       = luaL_checkinteger(L, 4);
  size_t caller    = luaL_checkinteger(L, 5);
  if (!is_local(who)) {
    luaL_error(L, "#4 out of range: %d", who);
  }
  if (is_local(caller)) {
    luaL_error(L, "#5 out of range: %d", caller);
  }
  int rcf = (int)luaL_checkinteger(L, 6);
  int count = luaC_r_deliver(name, data, size, mask, who, caller, rcf);
  lua_pushinteger(L, count);
  return 1;
}

/********************************************************************************/

LUAC_API int luaC_open_rpcall(lua_State* L) {
  const luaL_Reg methods[] = {
    { "lookout",    luaf_lookout    },
    { "declare",    luaf_declare    },
    { "undeclare",  luaf_undeclare  },
    { "caller",     luaf_caller     },
    { "rpcall",     luaf_rpcall     },
    { "deliver",    luaf_deliver    },

    { "r_deliver",  luaf_r_deliver  },
    { "r_bind",     luaf_r_bind     },
    { "r_unbind",   luaf_r_unbind   },
    { "r_response", luaf_r_response },
    { NULL,         NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */

  if (!watcher_ios) {
    watcher_ios = lws::getlocal();
  }
  return 0;
}

LUAC_API int luaC_lookout(lua_CFunction f) {
  unique_mutex_lock(rpcall_lock);
  watcher_ios = lws::getlocal();
  watcher_cfn = f;
  return LUA_OK;
}

LUAC_API int luaC_clsexpires() {
  if (luaC_debugging()) {
    return LUA_OK;
  }
  auto now  = luaC_clock();
  auto iter = invoke_pendings.begin();
  for (; iter != invoke_pendings.end();) {
    auto timeout = iter->second.timeout;
    if (now < timeout) {
      ++iter;
      continue;
    }
    auto rcf = iter->first;
    auto caller = iter->second.caller;
    iter = invoke_pendings.erase(iter);
    lws::post(caller, lws_bind(cancel_invoke, rcf));
  }
  return LUA_OK;
}

LUAC_API int luaC_r_unbind(const char* name, size_t who, int* opt) {
  node_type node;
  node.who = who;

  topic_type topic(name);
  unique_mutex_lock(rpcall_lock);
  auto iter = rpcall_handlers.find(topic);

  if (iter == rpcall_handlers.end()) {
    return 0;
  }
  rpcall_set_type& val = iter->second;
  auto find = val.find(node);
  if (find == val.end()) {
    return 0;
  }
  if (opt) {
    *opt = find->opt;
  }
  int rcb = find->rcb;
  val.erase(find);
  if (val.empty()) {
    rpcall_handlers.erase(iter);
  }
  return rcb;
}

LUAC_API int luaC_r_bind(const char* name, size_t who, int rcb, int opt) {
  node_type node;
  node.who = who;
  node.rcb = rcb;
  node.opt = opt;

  topic_type topic(name);
  unique_mutex_lock(rpcall_lock);
  auto iter = rpcall_handlers.find(topic);

  if (iter == rpcall_handlers.end()) {
    rpcall_set_type val;
    val.insert(node);
    rpcall_handlers[topic] = val;
    return LUA_OK;
  }
  rpcall_set_type& val = iter->second;
  auto find = val.find(node);
  if (find == val.end()) {
    val.insert(node);
    return LUA_OK;
  }
  return LUA_ERRRUN;
}

LUAC_API int luaC_r_response(const std::string& data, size_t caller, int rcf) {
  lws_int ok = lws_false;
  if (is_local(caller)) {
    if (rcf < 0) {
      lws::post(std::abs(rcf), [data]() {
        rpcall_result.assign(data);
        rpcall_complete = true;
      });
      return LUA_OK;
    }
    ok = lws::post((int)caller, lws_bind(back_to_local, data, rcf));
  }
  return (ok == lws_true) ? LUA_OK : LUA_ERRRUN;
}

LUAC_API int luaC_r_deliver(const char* name, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf) {
  node_type node;
  node.who = who;
  topic_type topic(name);

  unique_mutex_lock(rpcall_lock);
  auto iter = rpcall_handlers.find(topic);
  if (iter == rpcall_handlers.end()) {
    return 0;
  }
  rpcall_set_type& val = iter->second;
  if (val.empty()) {
    return 0;
  }
  if (!is_local(caller) && !is_local(who)) {
    return 0;
  }
  /* if there is a receiver */
  if (who) {
    auto find = val.find(node);
    if (find == val.end()) {
      return 0;
    }
    if (!is_local(caller)) {
      /* can't call by remote */
      if (find->opt == 0) {
        return 0;
      }
    }
    auto rcb = find->rcb;
    return dispatch(topic, rcb, data, size, mask, who, caller, rcf);
  }
  /* if the mask is set */
  if (mask) {
    std::vector<node_type> select;
    auto find = val.begin();
    for (; find != val.end(); ++find) {
      /* can't call by remote */
      if (find->opt == 0) {
        if (!is_local(caller)) {
          continue;
        }
      }
      if (is_local(caller) || is_local(who)) {
        select.push_back(*find);
      }
    }
    if (select.empty()) {
      return 0;
    }
    size_t n = select.size();
    size_t i = mask % n;
    node.who = who = select[i].who;
  }
  /* dispatch to all receivers */
  int count = 0;
  auto find = val.begin();
  for (; find != val.end(); ++find) {
    auto rcb = find->rcb;
    auto who = find->who;
    count += dispatch(topic, rcb, data, size, mask, who, caller, rcf);
  }
  return count;
}

/********************************************************************************/
