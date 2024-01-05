

#pragma once

#include <cstdint>
#include <assert.h>

/***********************************************************************************/

#if defined(__APPLE__) && defined(__GNUC__)
#define os_macx
#elif defined(__MACOSX__)
#define os_macx
#elif defined(macintosh)
#define os_mac9
#elif defined(__CYGWIN__)
#define os_cygwin
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) 
#define os_win64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define os_win32
#elif defined(__sun) || defined(sun)
#define os_solaris
#elif defined(hpux)  || defined(__hpux)
#define os_hpux
#elif defined(__linux__) || defined(__linux)
#define os_linux
#elif defined(__FreeBSD__)
#define os_freebsd
#elif defined(__NetBSD__)
#define os_netbsd
#elif defined(__OpenBSD__)
#define os_openbsd
#else
#error "Has not been ported to this OS."
#endif

/***********************************************************************************/

#if defined(os_mac9) || defined(os_macx)
#define os_apple
#endif

#if defined(os_freebsd) || defined(os_netbsd) || defined(os_openbsd)
#define os_bsdx
#endif

#if defined(_MSC_VER) || defined(os_win32) || defined(os_win64)
#define os_windows
#endif

#ifdef os_windows
#include "eport/detail/os/windows.hpp"
#else
#include "eport/detail/os/linux.hpp"
#endif

#include "eport/detail/os/clock.hpp" /*include clock*/

/***********************************************************************************/
