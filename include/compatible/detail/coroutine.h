

/***********************************************************************************/

#ifdef USING_COROUTINE
#ifndef __COROUTINE_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef COROUTINE_IMPL
#define MINICORO_IMPL
#endif

#include <assert.h>
#include "minicoro.h"

/***********************************************************************************/

namespace coroutine {
  enum struct state{
    DEAD = 0,
    NORMAL,
    RUNNING,
    SUSPENDED
  };
  static mco_coro* create(void (*func)(mco_coro* co)) {
    mco_coro*  co   = nullptr;
    mco_desc   desc = mco_desc_init(func, 0);
    mco_result coro = mco_create(&co, &desc);
    return coro == MCO_SUCCESS ? co : nullptr;
  }
  static state status(mco_coro* co) {
    switch (mco_status(co)) {
    case MCO_DEAD:
      return state::DEAD;
    case MCO_NORMAL:
      return state::NORMAL;
    case MCO_RUNNING:
      return state::RUNNING;
    case MCO_SUSPENDED:
      return state::SUSPENDED;
    }
    return state::DEAD;
  }
  static void close(mco_coro* co) {
    assert(co);
    assert(status(co) == state::DEAD);
    mco_destroy(co);
  }
  static mco_coro* current() {
    return mco_running();
  }
  static bool isyieldable() {
    return current() != nullptr;
  }
  static const void* context(mco_coro* co) {
    assert(co);
    return mco_get_user_data(co);
  }
  static const void* context() {
    return context(current());
  }
  static bool yield(const void* ud = nullptr) {
    mco_coro* co = current();
    assert(co);
    co->user_data = (void*)ud;
    return mco_yield(co) == MCO_SUCCESS;
  }
  static bool resume(mco_coro* co, const void* ud = nullptr) {
    assert(co);
    assert(status(co) == state::SUSPENDED);
    co->user_data = (void*)ud;
    return mco_resume(co) == MCO_SUCCESS;
  }
};

/***********************************************************************************/

#endif //__COROUTINE_H_
#endif //USING_COROUTINE

/***********************************************************************************/
