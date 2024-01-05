

#pragma once

#include "eport/3rd.hpp"
#include "eport/detail/io/conv.hpp"

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

typedef asio::error_code error_code;

inline const error_code& no_error() {
  const static error_code ec;
  return ec;
}

inline std::string error_message(const error_code& ec) {
#ifndef _MSC_VER
  return ec.message();
#else
  std::string msg = ec.message();
  if (!is_utf8(msg.c_str(), msg.size())) {
    msg = mbs_to_utf8(msg);
  }
  return msg;
#endif
}

namespace error { using namespace asio::error; }

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
