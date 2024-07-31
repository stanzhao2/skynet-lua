

/***********************************************************************************/

#ifndef __CONFIGURE_LINUX_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "semaphore.h"

#include <linux/kernel.h>
#include <unistd.h>
#include <dlfcn.h>    //dlopen, dlclose, dlsym
#include <dirent.h>

/***********************************************************************************/

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#ifndef PSDIR
#define PSDIR "/"
#endif

#define stricmp strcasecmp

/***********************************************************************************/

CONF_API const char* getpwd(char* path, size_t size) {
  int i, rslt = readlink("/proc/self/exe", path, size - 1);
  if (rslt < 0) {
    return nullptr;
  }
  path[rslt] = '\0';
  for (i = rslt; i >= 0; i--) {
    if (path[i] == *PSDIR) {
      path[i ? i : 1] = '\0';
      break;
    }
  }
  return path;
}

/***********************************************************************************/

#endif //__CONFIGURE_LINUX_H_

/***********************************************************************************/
