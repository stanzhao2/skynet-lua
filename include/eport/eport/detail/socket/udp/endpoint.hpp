

#pragma once

#include "eport/detail/socket/address.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
namespace udp   {
/***********************************************************************************/

typedef asio::ip::udp::endpoint endpoint;
typedef asio::ip::udp::resolver::results_type results_type;

/***********************************************************************************/

inline results_type resolve(
  const char* host, unsigned short port, error_code& ec) {
  char service[8];
  snprintf(service, sizeof(service), "%u", port);
  if (!host || !host[0]) {
    host = "0.0.0.0";
  }
  asio::io_context ios;
  asio::ip::udp::resolver resolver(ios);
  return resolver.resolve(host, service, ec);
}

/***********************************************************************************/
} //end of namespace udp
} //end of namespace ip
} //end of namespace eport
/***********************************************************************************/
