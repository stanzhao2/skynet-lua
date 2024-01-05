

#pragma once

#include "eport/3rd.hpp"

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

typedef asio::const_buffer const_buffer;
typedef asio::mutable_buffer mutable_buffer;

inline mutable_buffer buffer(char* p, size_t s) {
  return asio::buffer(p, s);
}

inline const_buffer buffer(const char* p, size_t s) {
  return asio::buffer(p, s);
}

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
