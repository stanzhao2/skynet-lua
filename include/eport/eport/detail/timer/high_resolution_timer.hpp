

#pragma once

#include "eport/detail/io/context.hpp"

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

class high_resolution_timer : public asio::high_resolution_timer {
  typedef asio::high_resolution_timer parent;
  high_resolution_timer(io_context::value_type ios)
    : parent(*ios), _ios(ios) {
  }
  io_context::value_type _ios;

public:
  io_context::value_type get_executor() const {
    return _ios;
  }
  typedef std::shared_ptr<high_resolution_timer> value_type;
  static value_type create(io_context::value_type ios) {
    return value_type(new high_resolution_timer(ios));
  }
};

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
