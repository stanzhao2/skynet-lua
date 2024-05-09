

#include "require.h"

/********************************************************************************/

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM < 502
# define loader_field "loaders"
# define lua_read(a, b, c, d) lua_load(a, b, c, d)
#else
# define loader_field "searchers"
# define lua_read(a, b, c, d) lua_load(a, b, c, d, 0)
#endif

typedef struct {
  const char* data; /* data buffer */
  size_t size;      /* data length */
} file_buffer;

static const char* lua_reader(lua_State* L, void* ud, size_t* size) {
  file_buffer* fb = (file_buffer*)ud;
  (void)L;  /* not used */

  if (fb->size == 0) return NULL;
  *size = fb->size;
  fb->size = 0;
  return fb->data;
}

static int lua_loader(lua_State* L, const char* buff, size_t size, const char* name) {
  file_buffer fb;
  fb.data = buff;
  fb.size = size;
  return lua_read(L, lua_reader, &fb, name);
}

static const char* normal(const char* filename, char* out) {
  char sep = *LUA_DIRSEP;
  strcpy(out, filename);
  for (char *p = out; *p; p++) {
    char c = *p;
    char next_c = *(p + 1);
    if (c == '/' || c == '\\') {
      *p = '.';
      continue;
    }
    if (c == '.') {
      if (strcmp(p, ".lua") == 0) {
        *p = 0;
        break;
      }
    }
  }
  return out;
}

static bool check_utf8(const char* p, size_t n = 0) {
  unsigned char chr;
  size_t i = 0, follow = 0;
  for (; n ? (i < n) : (p[i] != 0); i++) {
    chr = p[i];
    if (follow == 0) {
      if (chr > 0x80) {
        if (chr == 0xfc || chr == 0xfd) {
          follow = 5;
        }
        else if (chr >= 0xf8) {
          follow = 4;
        }
        else if (chr >= 0xf0) {
          follow = 3;
        }
        else if (chr >= 0xe0) {
          follow = 2;
        }
        else if (chr >= 0xc0) {
          follow = 1;
        }
        else {
          return false;
        }
      }
    } else {
      follow--;
      if ((chr & 0xC0) != 0x80) {
        return false;
      }
    }
  }
  return (follow == 0);
}

static const char* skipBOM(const char* buff, size_t* size) {
  const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
  for (; (*size) > 0 && *p != '\0'; buff++, (*size)--)
    if (*buff != *p++) break;
  return buff;
}

static const char* skip_common(lua_State* L, const char* buff, size_t& size) {
  if (size < 4 || memcmp(buff, LUA_SIGNATURE, 4)) {
    buff = skipBOM(buff, &size);
    if (*buff == '#') {
      do {
        size--;
      } while (*(++buff) != '\n');
    }
    if (!check_utf8(buff, size)) {
      return NULL;
    }
  }
  return buff;
}

static int ll_requireb(lua_State* L, const char* buff, size_t size, const char* filename) {
  buff = skip_common(L, buff, size);
  if (!buff) {
    lua_pushfstring(L, "'%s' is not utf8 encoded", filename);
    return LUA_ERRERR;
  }
  char luaname[8192];
  snprintf(luaname, sizeof(luaname), "@%s", filename);
  int result = lua_loader(L, buff, size, luaname);
  if (result == LUA_OK) {
    lua_pushfstring(L, "%s", luaname + 1);
  }
  return result;
}

static int ll_requiref(lua_State* L, const char* filename) {
  FILE* fp = fopen(filename, "r");
  if (fp) {
    char signature[4] = { 0 };
    fread(signature, 1, 4, fp);
    if (memcmp(signature, LUA_SIGNATURE, 4) == 0) {
      fp = freopen(filename, "rb", fp); /* reopen in binary mode */
    }
  }
  if (!fp) {
    lua_pushfstring(L, "file %s open failed", filename);
    return LUA_ERRFILE;
  }
  size_t filesize, readed;
  fseek(fp, 0, SEEK_END);
  filesize = ftell(fp);
  char* buffer = (char*)malloc(filesize);

  if (!buffer) {
    lua_pushstring(L, "no memory");
    fclose(fp);
    return 0;
  }
  fseek(fp, 0, SEEK_SET);
  readed = fread(buffer, 1, filesize, fp);
  fclose(fp);

  std::string data(buffer, readed);
  free(buffer);
  return ll_requireb(L, data.c_str(), data.size(), filename);
}

static void pusherrornotfound(lua_State *L, const char *path) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  luaL_addstring(&b, "no file '");
  luaL_addgsub(&b, path, LUA_PATH_SEP, "'\n\tno file '");
  luaL_addstring(&b, "'");
  luaL_pushresult(&b);
}

static int readable(const char *filename) {
  FILE *fp = fopen(filename, "rb");  /* try to open file */
  if (fp == NULL) return 0; /* open failed */
  fclose(fp);
  return 1;
}

static const char* next_filename(char **path, char *end) {
  char *sep;
  char *name = *path;
  if (name == end)
    return NULL;  /* no more names */
  else if (*name == '\0') {  /* from previous iteration? */
    *name = *LUA_PATH_SEP;  /* restore separator */
    name++;  /* skip it */
  }
  sep = strchr(name, *LUA_PATH_SEP);  /* find next separator */
  if (sep == NULL)  /* separator not found? */
    sep = end;  /* name goes until the end */
  *sep = '\0';  /* finish file name */
  *path = sep;  /* will start next search from here */
  return name;
}

static const char* searchpath(lua_State *L, const char *name, const char *path, const char *sep, const char *dirsep) {
  luaL_Buffer buff;
  char *pathname;  /* path with name inserted */
  char *endpathname;  /* its end */
  const char *filename;
  /* separator is non-empty and appears in 'name'? */
  if (*sep != '\0' && strchr(name, *sep) != NULL) {
    name = luaL_gsub(L, name, sep, dirsep);  /* replace it by 'dirsep' */
  }
  luaL_buffinit(L, &buff);
  /* add path to the buffer, replacing marks ('?') with the file name */
  luaL_addgsub(&buff, path, LUA_PATH_MARK, name);
  luaL_addchar(&buff, '\0');
  pathname = luaL_buffaddr(&buff);  /* writable list of file names */
  endpathname = pathname + luaL_bufflen(&buff) - 1;
  while ((filename = next_filename(&pathname, endpathname)) != NULL) {
    if (readable(filename)) {  /* does file exist and is readable? */
      return lua_pushstring(L, filename);  /* save and return name */
    }
  }
  luaL_pushresult(&buff);  /* push path to create error message */
  pusherrornotfound(L, lua_tostring(L, -1));  /* create error message */
  return NULL;  /* not found */
}

static const char* findfile(lua_State* L, const char* path, const char* filename) {
  char temp[8192];
  filename = normal(filename, temp);
  return searchpath(L, filename, path, ".", LUA_DIRSEP);
}

static int user_require(lua_State* L) {
  return 0;
}

static int luaf_require(lua_State* L) {
  lua_getglobal(L, LUA_LOADLIBNAME);
  lua_getfield(L, -1, "path");
  auto findpath = luaL_checkstring(L, -1);
  auto filename = luaL_checkstring(L, 1);
  auto fullname = findfile(L, findpath, filename);
  /* file found */
  if (fullname) {
    if (ll_requiref(L, fullname) != LUA_OK) {
      lua_error(L);
    }
    return 2;
  }
  /* file not found */
  lua_pushcfunction(L, user_require);
  lua_pushstring(L, filename);
  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    lua_error(L);
  }
  if (lua_type(L, -1) == LUA_TNIL) {
    /* load failed */
    lua_pop(L, 1); /* pop nil */
    return 1;      /* return error message */
  }
  size_t size = 0;
  const char* buff = luaL_checklstring(L, -1, &size);
  if (ll_requireb(L, buff, size, filename) != LUA_OK) {
    lua_error(L);
  }
  return 2;
}

static void lua_newrequire(lua_State* L, lua_CFunction f) {
  lua_getglobal(L, LUA_LOADLIBNAME); /* package */
  lua_getfield(L, -1, loader_field); /* searchers or loaders */
  lua_pushcfunction(L, f); /* push new loader */
  lua_rawseti(L, -2, 2);
  lua_pop(L, 2);
}

/********************************************************************************/

int luaopen_require(lua_State* L) {
  lua_newrequire(L, luaf_require);
  return 0;
}

/********************************************************************************/
