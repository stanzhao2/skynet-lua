

/***********************************************************************************/

#ifndef __CONFIGURE_GENERIC_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "coroutine.h"

#define CONF_API inline

/***********************************************************************************/

#include <assert.h>   //assert()
#include <stdlib.h>   //malloc(), free(), exit(), rand(), srand(), abs(), div(), lldiv() ...
#include <stdio.h>    //printf(), scanf(), fgets(), fputs(), fopen(), fclose(), snprintf(), vprintf() ...
#include <math.h>     //sin(), cos(), tan(), sqrt(), pow(), log(), exp(), ceil(), floor() ...
#include <errno.h>    //errno
#include <limits.h>   //INT_MAX, INT_MIN, CHAR_MAX, CHAR_MIN, SIZE_MAX ...
#include <float.h>    //FLT_MAX, DBL_MAX, LDBL_MAX, FLT_EPSILON, DBL_EPSILON ...
#include <stdbool.h>  //bool, true, false
#include <time.h>     //time(), localtime(), strftime(), gmtime(), difftime(), mktime(), clock() ...
#include <ctype.h>    //isalpha(), isdigit(), isupper(), tolower(), toupper() ...
#include <signal.h>   //signal(), raise()
#include <string.h>   //strlen(), strcpy(), strcat(), strcmp(), strstr(), memcpy(), memset() ...
#include <stddef.h>   //size_t, NULL, offsetof(), ptrdiff_t ...
#include <stdint.h>   //int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t ...
#include <stdarg.h>   //va_start, va_arg, va_end, va_copy ...

/***********************************************************************************/

#ifdef __cplusplus
#include <exception>  //
#include <locale>     //
#include <chrono>     //
#include <string>     //
#include <list>       //
#include <map>        //
#include <vector>     //
#include <memory>     //
#include <functional> //
#include <mutex>      //
#include <atomic>     //
#include <thread>     //
#include <algorithm>  //
#endif

/***********************************************************************************/

CONF_API size_t roundup_power_of_2(size_t n) {
  n = n ? n : 1;
  size_t maxv = ~0;
  size_t andv = ~(maxv & (maxv >> 1));
  while ((andv & n) == 0) andv = andv >> 1;
  return andv << 1;
}

CONF_API size_t cpu_hardware() {
  return std::thread::hardware_concurrency();
}

CONF_API size_t steady_clock() {
  return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()
  ).count();
}

CONF_API size_t system_clock() {
  return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
}

/***********************************************************************************/

#endif  //__CONFIGURE_GENERIC_H_

/***********************************************************************************/
