

#pragma once

#include "eport/detail/identifier.hpp"

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

class io_context : public asio::io_context {
  typedef asio::io_context parent;
  typedef asio::executor_work_guard<executor_type> work_guard_t;
  identifier   _id;
  work_guard_t _work_guard;

public:
  size_t run() {
    size_t count = 0;
    try {
      count = parent::run();
    }
    catch (const std::exception& e) {
      printf("%s: %s\n", __FUNCTION__, e.what());
    }
    return count;
  }

  size_t run_for(size_t expires) {
    size_t count = 0;
    try {
      count = parent::run_for(std::chrono::milliseconds(expires));
    }
    catch (const std::exception& e) {
      printf("%s: %s\n", __FUNCTION__, e.what());
    }
    return count;
  }

  size_t run_one() {
    size_t count = 0;
    try {
      count = parent::run_one();
    }
    catch (const std::exception& e) {
      printf("%s: %s\n", __FUNCTION__, e.what());
    }
    return count;
  }

  size_t run_one_for(size_t expires) {
    size_t count = 0;
    try {
      count = parent::run_one_for(std::chrono::milliseconds(expires));
    }
    catch (const std::exception& e) {
      printf("%s: %s\n", __FUNCTION__, e.what());
    }
    return count;
  }

  size_t poll() {
    size_t count = 0;
    try {
      count = parent::poll();
    }
    catch (const std::exception& e) {
      printf("%s: %s\n", __FUNCTION__, e.what());
    }
    return count;
  }

  size_t poll_one() {
    size_t count = 0;
    try {
      count = parent::poll_one();
    }
    catch (const std::exception& e) {
      printf("%s: %s\n", __FUNCTION__, e.what());
    }
    return count;
  }

public:
  template <typename Handler>
  inline void post(Handler handler) {
    asio::post(*this, handler);
  }

  template <typename Handler>
  inline void dispatch(Handler handler) {
    asio::dispatch(*this, handler);
  }

  typedef std::shared_ptr<io_context> value_type;
  static value_type create() { return value_type(new io_context()); }
  inline int id() const { return _id.value(); }

private:
  io_context()
    : _work_guard(asio::make_work_guard(*this)) { }
  io_context(const service&) = delete;
};
  
/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
