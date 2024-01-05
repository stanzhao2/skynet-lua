

#pragma once

/***********************************************************************************/
namespace eport {
namespace clock  {
/***********************************************************************************/

inline static size_t nanoseconds() {
  return (size_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
inline static size_t microseconds() {
  return (size_t)std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
inline static size_t milliseconds() {
  return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
inline static size_t seconds() {
  return (size_t)std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

/***********************************************************************************/
} //end of namespace clock
} //end of namespace eport
/***********************************************************************************/
