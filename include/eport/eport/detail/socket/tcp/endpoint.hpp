

#pragma once

#include "eport/detail/socket/address.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
namespace tcp   {
/***********************************************************************************/

typedef asio::ip::tcp::endpoint endpoint;
typedef asio::ip::tcp::resolver::results_type results_type;

/***********************************************************************************/

inline results_type resolve(
  const char* host, unsigned short port, error_code& ec) {
  char service[8];
  snprintf(service, sizeof(service), "%u", port);
  if (!host || !host[0]) {
    host = "0.0.0.0";
  }
  asio::io_context ios;
  asio::ip::tcp::resolver resolver(ios);
  return resolver.resolve(host, service, ec);
}

/***********************************************************************************/
} //end of namespace tcp
} //end of namespace ip
} //end of namespace eport
/***********************************************************************************/
