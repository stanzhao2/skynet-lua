

/***********************************************************************************/

#ifndef __OS_CONFIGURE_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/***********************************************************************************/

#if defined(__APPLE__) && defined(__GNUC__)
#define IF_MACX
#elif defined(__MACOSX__)
#define IF_MACX
#elif defined(macintosh)
#define IF_MACINTOSH
#elif defined(__CYGWIN__)
#define IF_CYGWIN
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) 
#define IF_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define IF_WIN32
#elif defined(__sun) || defined(sun)
#define IF_SOLARIS
#elif defined(hpux)  || defined(__hpux)
#define IF_HPUX
#elif defined(__linux__) || defined(__linux)
#define IF_LINUX
#elif defined(__FreeBSD__)
#define IF_FREEBSD
#elif defined(__NetBSD__)
#define IF_NETBSD
#elif defined(__OpenBSD__)
#define IF_OPENBSD
#else
#error "Has not been ported to this OS."
#endif

/***********************************************************************************/

#if defined(_MSC_VER) || defined(IF_WIN32) || defined(IF_WIN64)
#include "windows.h"
#elif defined(__GNUC__) || defined(IF_LINUX)
#include "linux.h"
#endif

/***********************************************************************************/

#endif  //__OS_CONFIGURE_H_

/***********************************************************************************/
