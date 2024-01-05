

#pragma once

#include "eport/detail/io/context.hpp"

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

class system_timer : public asio::system_timer {
  typedef asio::system_timer parent;
  system_timer(io_context::value_type ios)
    : parent(*ios), _ios(ios) {
  }
  io_context::value_type _ios;

public:
  io_context::value_type get_executor() const {
    return _ios;
  }
  typedef std::shared_ptr<system_timer> value_type;
  static value_type create(io_context::value_type ios) {
    return value_type(new system_timer(ios));
  }
};

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
