

#include <string.h>
#include <chrono>
#include "luaf_clock.h"

/********************************************************************************/

#define SIZETIMEFMT	250

#if defined(LUA_USE_WINDOWS)
#define LUA_STRFTIMEOPTIONS  "aAbBcdHIjmMpSUwWxXyYzZ%" \
    "||" "#c#x#d#H#I#j#m#M#S#U#w#W#y#Y"  /* two-char options */
#elif defined(LUA_USE_C89)  /* ANSI C 89 (only 1-char options) */
#define LUA_STRFTIMEOPTIONS  "aAbBcdHIjmMpSUwWxXyYZ%"
#else  /* C99 specification */
#define LUA_STRFTIMEOPTIONS  "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%" \
    "||" "EcECExEXEyEY" "OdOeOHOIOmOMOSOuOUOVOwOWOy"  /* two-char options */
#endif

#if !defined(LUA_NUMTIME)	/* { */
#define l_timet			lua_Integer
#define l_pushtime(L,t)		lua_pushinteger(L,(lua_Integer)(t))
#define l_gettime(L,arg)	luaL_checkinteger(L, arg)
#else				/* }{ */
#define l_timet			lua_Number
#define l_pushtime(L,t)		lua_pushnumber(L,(lua_Number)(t))
#define l_gettime(L,arg)	luaL_checknumber(L, arg)
#endif				/* } */

#if defined(LUA_USE_WINDOWS)
#define l_gmtime(t,r)     (gmtime_s(r,t),r)
#define l_localtime(t,r)	(localtime_s(r,t),r)
#else
#define l_gmtime(t,r)		  (gmtime_r(t,r))
#define l_localtime(t,r)	(localtime_r(t,r))
#endif

/********************************************************************************/

static void setfield(lua_State *L, const char *key, int value, int delta) {
#if (defined(LUA_NUMTIME) && LUA_MAXINTEGER <= INT_MAX)
  if (l_unlikely(value > LUA_MAXINTEGER - delta))
    luaL_error(L, "field '%s' is out-of-bound", key);
#endif
  lua_pushinteger(L, (lua_Integer)value + delta);
  lua_setfield(L, -2, key);
}

static void setboolfield(lua_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  lua_pushboolean(L, value);
  lua_setfield(L, -2, key);
}

static void setallfields(lua_State *L, struct tm *stm) {
  setfield(L, "year",      stm->tm_year, 1900);
  setfield(L, "month",     stm->tm_mon, 1);
  setfield(L, "day",       stm->tm_mday, 0);
  setfield(L, "hour",      stm->tm_hour, 0);
  setfield(L, "min",       stm->tm_min, 0);
  setfield(L, "sec",       stm->tm_sec, 0);
  setfield(L, "yday",      stm->tm_yday, 1);
  setfield(L, "wday",      stm->tm_wday, 1);
  setboolfield(L, "isdst", stm->tm_isdst);
}

static const char *checkoption(lua_State *L, const char *conv, ptrdiff_t convlen, char *buff) {
  const char *option = LUA_STRFTIMEOPTIONS;
  int oplen = 1;  /* length of options being checked */
  for (; *option != '\0' && oplen <= convlen; option += oplen) {
    if (*option == '|')  /* next block? */
      oplen++;  /* will check options with next length (+1) */
    else if (memcmp(conv, option, oplen) == 0) {  /* match? */
      memcpy(buff, conv, oplen);  /* copy valid option to buffer */
      buff[oplen] = '\0';
      return conv + oplen;  /* return next item */
    }
  }
  luaL_argerror(L, 1,
    lua_pushfstring(L, "invalid conversion specifier '%%%s'", conv));
  return conv;  /* to avoid warnings */
}

static time_t l_checktime(lua_State *L, int arg) {
  l_timet t = l_gettime(L, arg);
  luaL_argcheck(L, (time_t)t == t, arg, "time out-of-bounds");
  return (time_t)t;
}

static int luaf_date(lua_State *L) {
  size_t slen;
  const char *s = luaL_optlstring(L, 1, "%c", &slen);
  time_t t = luaL_opt(L, l_checktime, 2, time(NULL));
  const char *se = s + slen;  /* 's' end */
  struct tm tmr, *stm;
  if (*s == '!') {  /* UTC? */
    stm = l_gmtime(&t, &tmr);
    s++;  /* skip '!' */
  }
  else
    stm = l_localtime(&t, &tmr);
  if (stm == NULL)  /* invalid date? */
    return luaL_error(L,
      "date result cannot be represented in this installation");
  if (strcmp(s, "*t") == 0) {
    lua_createtable(L, 0, 9);  /* 9 = number of fields */
    setallfields(L, stm);
  }
  else {
    char cc[4];  /* buffer for individual conversion specifiers */
    luaL_Buffer b;
    cc[0] = '%';
    luaL_buffinit(L, &b);
    while (s < se) {
      if (*s != '%')  /* not a conversion specifier? */
        luaL_addchar(&b, *s++);
      else {
        size_t reslen;
        char *buff = luaL_prepbuffsize(&b, SIZETIMEFMT);
        s++;  /* skip '%' */
        s = checkoption(L, s, se - s, cc + 1);  /* copy specifier to 'cc' */
        reslen = strftime(buff, SIZETIMEFMT, cc, stm);
        luaL_addsize(&b, reslen);
      }
    }
    luaL_pushresult(&b);
  }
  return 1;
}

/********************************************************************************/

static const size_t runclock = luaC_clock();

static int luaf_clock(lua_State *L) {
  const char* opt = luaL_optstring(L, 1, "s");
  auto now = luaC_clock() - runclock;
  if (strcmp(opt, "ms") == 0) {
    lua_pushinteger(L, (lua_Integer)now);
  }else{
    lua_pushnumber(L, (lua_Number)(now) / 1000.0f);
  }
  return 1;
}

/********************************************************************************/

LUAC_API size_t luaC_clock() {
  return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()
  ).count();
}

LUAC_API int luaC_open_clock(lua_State* L) {
  const luaL_Reg methods[] = {
    { "date",   luaf_date   }, /* os.date()  */
    { "clock",  luaf_clock  }, /* os.clock() */
    { NULL,     NULL        }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return 0;
}

/********************************************************************************/
