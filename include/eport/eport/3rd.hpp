

#pragma once

/***********************************************************************************/

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef ASIO_NO_DEPRECATED
#define ASIO_NO_DEPRECATED
#endif

#include <asio.hpp>
#include "eport/detail/os/os.hpp"

/***********************************************************************************/

#ifndef pcall
#define pcall(f, ...) try { \
    f(__VA_ARGS__);  \
  }                         \
  catch(const std::exception& e) { \
    printf("%s exception: %s\n", __FUNCTION__, e.what()); \
  }
#endif

/***********************************************************************************/
