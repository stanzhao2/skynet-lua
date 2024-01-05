

#pragma once

#include "eport/3rd.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
/***********************************************************************************/

typedef asio::ip::address address;

inline address make_address(const std::string& addr, error_code& ec) {
  return asio::ip::make_address(addr, ec);
}

inline address make_address(const std::string& addr) {
  error_code ec;
  return make_address(addr, ec);
}

/***********************************************************************************/
} //end of namespace ip
} //end of namespace eport
/***********************************************************************************/
