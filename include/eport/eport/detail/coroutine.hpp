

#pragma once

/***********************************************************************************/

#define MINICORO_IMPL
#include "eport/3rd.hpp"
#include "eport/detail/os/minicoro.hpp"
#include <functional>

/***********************************************************************************/
namespace eport {
/***********************************************************************************/
  
class coroutine final {
  mco_desc  _desc;
  mco_coro* _co;
  std::function<void(coroutine*)> _handler;
  typedef std::shared_ptr<coroutine> value_type;

public:
  enum struct status {
    dead = 0,
    normal,
    running,
    suspended
  };
  static inline coroutine* running() {
    auto co = mco_running();
    return (coroutine*)(co ? co->user_data : nullptr);
  }
  template <typename Handler>
  static value_type create(Handler&& handler, size_t stack_size = 0) {
    /*Don't use multiples of 64K to avoid D-cache aliasing conflicts.*/
    assert(stack_size && stack_size % 0xffff);
    return value_type(new coroutine(handler, stack_size));
  }
  virtual ~coroutine() {
    if (_co) {
      mco_uninit(_co), mco_destroy(_co);
    }
  }
  template <typename _Ty>
  inline bool peek(_Ty& v) {
    return (_co && mco_peek(_co, &v, sizeof(v)) == MCO_SUCCESS);
  }
  template <typename _Ty>
  inline bool pop(_Ty& v) {
    return (_co && mco_pop(_co, &v, sizeof(v)) == MCO_SUCCESS);
  }
  template <typename _Ty>
  inline bool push(const _Ty& v) {
    return (_co && mco_push(_co, &v, sizeof(v)) == MCO_SUCCESS);
  }
  inline bool yield()  {
    return (_co && mco_yield(_co) == MCO_SUCCESS);
  }
  inline bool resume() {
    return (_co && mco_resume(_co) == MCO_SUCCESS);
  }
  inline status state() const {
    return _co ? (status)mco_status(_co) : status::dead;
  }

private:
  template <typename Handler>
  inline coroutine(Handler&& handler, size_t stack_size)
  {
    _handler = handler;
    _desc = mco_desc_init([](mco_coro* co) {
      coroutine *_running = (coroutine*)co->user_data;
      pcall(_running->_handler, _running);
      }, stack_size
    );
    _desc.user_data = this;
    mco_create(&_co, &_desc);
  }
};

/*******************************************************************************/

template <typename Handler>
static coroutine::value_type co_create(Handler&& handler, size_t stack_size = 0) {
  return coroutine::create(handler, stack_size);
}

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
