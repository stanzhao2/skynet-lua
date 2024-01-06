

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <thread>

#include "luaf_state.h"
#include "socket.io/socket.io.hpp"

LUAC_API int luaC_clsexpires();

/********************************************************************************/

#define LUAC_STOPCALL  "os:cancel"
#define LUAC_THREAD    "os:thread"

enum struct job_state {
  pending, exited, error, successfully
};

struct ud_thread {
  job_state     state;
  void*         ud;
  lws_int       ios;
  lua_Alloc     alloter;
  std::string   name;
  std::string   argv;
  std::shared_ptr<std::thread> thread;
};

/********************************************************************************/

static int luaf_callback(lua_State* L) {
  int index = lua_upvalueindex(1); /* get the first up-value */
  ud_thread *job = (ud_thread*)lua_touserdata(L, index);
  job->state = job_state::successfully;
  lua_pushnil(L);
  lua_setglobal(L, LUAC_STOPCALL);
  return 0;
}

static void clonepath(lua_State* from, lua_State* to, const char* name) {
  int topfrom = lua_gettop(from);
  int topto   = lua_gettop(to);

  lua_getglobal(from, LUA_LOADLIBNAME);
  lua_getfield (from, -1, name);
  const char* path = luaL_checkstring(from, -1);

  lua_getglobal (to, LUA_LOADLIBNAME);
  lua_pushstring(to, path);
  lua_setfield  (to, -2, name);

  lua_settop(to, topto);
  lua_settop(from, topfrom);
}

static void lua_thread(lua_State* PL, ud_thread* job) {
  auto& name = job->name;
  auto& argv = job->argv;
  job->ios   = lws::getlocal();

  lua_State* L = luaC_newstate(job->alloter, job->ud);
  luaC_openlibs(L, nullptr);
  clonepath(PL, L, "path");
  clonepath(PL, L, "cpath");

  lua_pushlightuserdata(L, job);
  lua_pushcclosure(L, luaf_callback, 1);
  lua_setglobal(L, LUAC_STOPCALL);

  int argc = 1;
  lua_pushcfunction(L, luaC_execute);
  lua_pushlstring(L, name.c_str(), name.size());

  if (!argv.empty()) {
    lua_pushlstring(L, argv.c_str(), argv.size());
    argc += luaC_unpack(L);
  }

  int status = luaC_xpcall(L, argc, 0);
  if (status != LUA_OK) {
    lua_ferror("%s\n", luaL_checkstring(L, -1));
    job->state = job_state::error;
  }
  if (job->state == job_state::pending) {
    job->state = job_state::exited;
  }
  luaC_close(L);
}

static int luaf_free(lua_State* L) {
  ud_thread* job = luaC_checkudata<ud_thread>(L, 1, LUAC_THREAD);
  if (!job || !job->thread->joinable()) {
    return 0;
  }
  lws::post(job->ios, []() {
    lws::stop();
  });
  job->thread->join();
  job->state = job_state::exited;
  return 0;
}

static int luaf_gc(lua_State* L) {
#ifdef _DEBUG
  lua_ftrace("%s will gc\n", LUAC_THREAD);
#endif
  return luaf_free(L);
}

static int luaf_wait(lua_State* L) {
  lua_getglobal(L, LUAC_STOPCALL);
  if (lua_type(L, -1) == LUA_TFUNCTION) {
    luaC_pcall(L, 0, 0);
    lua_pushnil(L);
    lua_setglobal(L, LUAC_STOPCALL);
  }
  else {
    lua_pop(L, 1);
  }
  lua_Integer expires = -1;
  if (lua_gettop(L) > 0) {
    expires = luaL_checkinteger(L, 1);
  }
  lua_Integer count = 0;
  if (expires < 0) {
    while (!lws::stopped()) {
      count += lws::run_for(1000);
      luaC_clsexpires();
    }
    lua_pushinteger(L, count);
    return 1;
  }
  if (expires == 0) {
    count = lws::poll();
    luaC_clsexpires();
    lua_pushinteger(L, count);
    return 1;
  }
  while (!lws::stopped() && expires > 0) {
    auto wait = luaC_min(1000, expires);
    expires -= wait;
    count += lws::run_for(wait);
    luaC_clsexpires();
  }
  lua_pushinteger(L, count);
  return 1;
}

static int luaf_restart(lua_State* L) {
  lws::restart();
  return 0;
}

static int luaf_exit(lua_State* L) {
  luaC_exit();
  return 0;
}

static int luaf_stop(lua_State* L) {
  lws::stop();
  return 0;
}

static int luaf_stopped(lua_State* L) {
  lua_pushboolean(L, lws::stopped());
  return 1;
}

static int luaf_pload(lua_State* L) {
  size_t size;
  int argc = lua_gettop(L) - 1;
  if (argc < 0) {
    luaL_error(L, "no name");
  }
  const char* argv = "";
  const char* name = luaL_checkstring(L, 1);
  if (*name == 0) {
    luaL_error(L, "no name");
  }
  if (argc > 0) {
    luaC_pack(L, argc);
    argv = luaL_checklstring(L, -1, &size);
  }
  ud_thread* job = luaC_newuserdata<ud_thread>(L, LUAC_THREAD);
  if (job == NULL) {
    luaL_error(L, "no memory");
    return 0;
  }
  job->state   = job_state::pending;
  job->name    = name;
  job->argv    = argv;
  job->alloter = lua_getallocf(L, &job->ud);
  job->thread  = std::make_shared<std::thread>(std::bind(lua_thread, L, job));

  while (job->state == job_state::pending) {
    std::this_thread::sleep_for(
      std::chrono::milliseconds(10)
    );
  }
  if (job->state == job_state::successfully) {
    lua_pushboolean(L, 1);
    lua_rotate(L, -2, 1);
  } else {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    if (job->thread->joinable()) {
      job->thread->join();
    }
  }
  return 2;
}

static int luaf_name(lua_State* L) {
  lua_getglobal(L, LUAC_PROGNAME);
  return 1;
}

static int luaf_id(lua_State* L) {
  auto ios = lws::getlocal();
  lua_pushinteger(L, ios);
  return 1;
}

static int luaf_dirsep(lua_State* L) {
  lua_pushstring(L, LUA_DIRSEP);
  return 1;
}

static int luaf_debugging(lua_State* L) {
  auto debug = luaC_debugging();
  lua_pushboolean(L, debug ? 1 : 0);
  return 1;
}

static int luaf_post(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);
  int argc = lua_gettop(L);
  int rcb  = luaC_ref(L, 1);
  std::vector<int> argv;
  for (int i = 2; i <= argc; i++) {
    int ref = luaC_ref(L, i);
    argv.push_back(ref);
  }
  lws_int ok = lws::post([rcb, argv]() {
    lua_State* L = luaC_getlocal();
    revert_if_return revert(L);
    unref_if_return  unref_rcb(L, rcb);

    luaC_rawgeti(L, rcb);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
      return;
    }
    int argc = (int)argv.size();
    for (int i = 0; i < argc; i++) {
      int ref = argv[i];
      luaC_rawgeti(L, ref);
      luaC_unref(L, ref);
    }
    if (luaC_xpcall(L, argc, 0) != LUA_OK) {
      lua_ferror("%s\n", lua_tostring(L, -1));
    }
  });
  if (ok != lws_true) {
    luaC_unref(L, rcb);
    argc = (int)argv.size();
    for (int i = 0; i < argc; i++) {
      int ref = argv[i];
      luaC_unref(L, ref);
    }
  }
  lua_pushboolean(L, ok == lws_true ? 1 : 0);
  return 1;
}

/********************************************************************************/

static int main_ios = 0;

static void init_metatable(lua_State* L) {
  if (main_ios == 0) {
    main_ios = lws::getlocal();
  }  
  const luaL_Reg methods[] = {
    { "__gc",       luaf_gc         },
    { "free",       luaf_free       },
    { NULL,         NULL            }
  };
  luaC_newmetatable(L, LUAC_THREAD, methods);
  lua_pop(L, 1);
}

LUAC_API void luaC_exit() {
  lws::stop(main_ios);
}

LUAC_API int luaopen_pload(lua_State* L) {
  init_metatable(L);
  const luaL_Reg methods[] = {
    { "pload",      luaf_pload      },
    { "name",       luaf_name       },
    { "dirsep",     luaf_dirsep     },
    { "id",         luaf_id         },
    { "post",       luaf_post       },
    { "wait",       luaf_wait       },
    { "restart",    luaf_restart    },
    { "exit",       luaf_exit       },
    { "stop",       luaf_stop       },
    { "stopped",    luaf_stopped    },
    { "debugging",  luaf_debugging  },
    { NULL,         NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return 0;
}

/********************************************************************************/
