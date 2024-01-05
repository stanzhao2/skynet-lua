

#pragma once

#include "eport/detail/io/context.hpp"

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

class steady_timer : public asio::steady_timer {
  typedef asio::steady_timer parent;
  steady_timer(io_context::value_type ios)
    : parent(*ios), _ios(ios) {
  }
  io_context::value_type _ios;

public:
  io_context::value_type get_executor() const {
    return _ios;
  }
  typedef std::shared_ptr<steady_timer> value_type;
  static value_type create(io_context::value_type ios) {
    return value_type(new steady_timer(ios));
  }
};

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
