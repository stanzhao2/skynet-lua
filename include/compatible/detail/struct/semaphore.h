

/***********************************************************************************/

#ifndef __CONFIGURE_SEMAPHORE_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <mutex>
#include <condition_variable>

/***********************************************************************************/

struct condition {
  std::mutex mt;
  long counter = 0;
  std::condition_variable cond;
};

/***********************************************************************************/

CONF_API void cv_signal(condition& ref) {
  std::unique_lock<std::mutex> unique(ref.mt);
  if ((++ref.counter) <= 0) {
    ref.cond.notify_one();
  }
}

CONF_API void cv_wait(condition& ref) {
  std::unique_lock<std::mutex> unique(ref.mt);
  if ((--ref.counter) < 0) {
    ref.cond.wait(unique);
  }
}

CONF_API bool cv_wait_for(condition& ref, size_t expires) {
  std::unique_lock<std::mutex> unique(ref.mt);
  if ((--ref.counter) >= 0) {
    return true;
  }
  auto st = ref.cond.wait_for(
    unique,
    std::chrono::milliseconds(
      expires
    )
  );
  ref.counter += (int)st;
  return st == std::cv_status::no_timeout;
}

/***********************************************************************************/

class semaphore final {
public:
  explicit semaphore(long count = 0) {
    cond.counter = count;
  }
  inline void signal() {
    cv_signal(cond);
  }
  inline void wait() {
    cv_wait(cond);
  }
  inline bool wait_for(size_t expires) {
    return cv_wait_for(cond, expires);
  }
private:
  condition cond;
  semaphore(const semaphore&) = delete;
};

/***********************************************************************************/

#endif //__CONFIGURE_SEMAPHORE_H_

/***********************************************************************************/
