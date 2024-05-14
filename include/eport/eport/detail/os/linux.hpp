

#pragma once

#if defined(os_apple)
# include <mm_malloc.h>   //malloc/free
# include <mach/clock.h>  //host_get_clock_service
# include <mach/mach.h>   //host_get_clock_service
#else
# include <sys/types.h>
# include <malloc.h>
# include <memory>
# include <sys/time.h>
#endif

#include <linux/limits.h>
#include <sys/stat.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <dlfcn.h>        //dlopen, dlclose, dlsym
#include <time.h>
#include <chrono>
#include <dirent.h>
#include <linux/kernel.h>

/***********************************************************************************/

#define MAX_PATH PATH_MAX

#ifndef __int64
typedef long long __int64;
#endif
#ifndef __uint64
typedef unsigned long long __uint64;
#endif

#define stricmp strcasecmp

/***********************************************************************************/
namespace eport {
namespace os     {
/***********************************************************************************/

inline void* dlopen(const char* filename) {
  return (void*)::dlopen(filename, RTLD_LAZY);
}

/***********************************************************************************/

inline void* dlsym(void* mod, const char* funcname) {
  return ::dlsym(mod, funcname);
}

/***********************************************************************************/

inline void dlclose(void* mod) {
  ::dlclose(mod);
}

/***********************************************************************************/

inline std::string pwd() {
  char path[MAX_PATH];
  return getcwd(path, sizeof(path));
}

/***********************************************************************************/

inline std::string pmd() {
  int i;
  char path[MAX_PATH];
  int rslt = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (rslt < 0) {
    return "";
  }
  path[rslt] = '\0';
  for (i = rslt; i >= 0; i--) {
    if (path[i] == '/') {
      path[i] = '\0';
      break;
    }
  }
  return path;
}

/***********************************************************************************/

inline bool make_dir(const char* path) {
  DIR* dir = opendir(path);
  if (dir != 0) {
    closedir(dir);
    return true;
  }
  mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
  return (mkdir(path, mode) == 0);
}

/***********************************************************************************/

inline size_t cpu_count() {
  return std::thread::hardware_concurrency();
}

/***********************************************************************************/
} //end of namespace os
} //end of namespace eport
/***********************************************************************************/
