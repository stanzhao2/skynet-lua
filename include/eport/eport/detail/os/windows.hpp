

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <chrono>
#include <io.h>
#include <thread>
#include <direct.h>
#include <string>

/***********************************************************************************/

#ifndef __uint64
typedef unsigned long long __uint64;
#endif

#define chdir _chdir

/***********************************************************************************/
namespace eport {
namespace os     {
/***********************************************************************************/

inline void* dlopen(const char* filename) {
  return (void*)LoadLibraryA(filename);
}

/***********************************************************************************/

inline void* dlsym(void* mod, const char* funcname) {
  return GetProcAddress((HMODULE)mod, funcname);
}

/***********************************************************************************/

inline void dlclose(void* mod) {
  FreeLibrary((HMODULE)mod);
}

/***********************************************************************************/

inline std::string pwd() {
  char path[MAX_PATH];
  GetCurrentDirectoryA(sizeof(path), path);
  return path;
}

/***********************************************************************************/

inline std::string pmd() {
  char filename[MAX_PATH] = { 0 };
  GetModuleFileNameA(NULL, filename, MAX_PATH);
  char* pos = strrchr(filename, '\\');
  if (pos) {
    *pos = 0;
  }
  return filename;
}

/***********************************************************************************/

inline bool make_dir(const char* path) {
  HANDLE hfind;
  WIN32_FIND_DATAA data;
  hfind = FindFirstFileA(path, &data);

  if (hfind != INVALID_HANDLE_VALUE) {
    FindClose(hfind);
    return true;
  }
  return (_mkdir(path) == 0);
}

/***********************************************************************************/

inline size_t cpu_count() {
  return std::thread::hardware_concurrency();
}

/***********************************************************************************/
} //end of namespace os
} //end of namespace eport
/***********************************************************************************/
