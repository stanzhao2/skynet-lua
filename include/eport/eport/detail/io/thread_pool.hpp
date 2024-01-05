

#pragma once

#include <vector>
#include "eport/detail/io/context.hpp"

/***********************************************************************************/
namespace eport {
namespace io    {
/***********************************************************************************/

class thread_pool final {
public:
  typedef std::shared_ptr<thread_pool> value_type;

  static value_type create(size_t num_thread = 0) {
    return value_type(new thread_pool(num_thread)
    );
  }

  void start() {
    for (size_t i = 0; i < _pool.size(); i++) {
      _pool[i]->start();
    }
  }

  void stop() {
    for (size_t i = 0; i < _pool.size(); i++) {
      _pool[i]->stop();
    }
  }

  void join() {
    for (size_t i = 0; i < _pool.size(); i++) {
      _pool[i]->join();
    }
  }

  inline operator io_context::value_type() const {
    return least_use_count();
  }

  inline io_context::value_type get_executor() const {
    return least_use_count();
  }

private:
  thread_pool(size_t num_thread = 0) {
    size_t cpu_count = os::cpu_count();
    if (num_thread == 0) {
      num_thread = cpu_count;
    }
    else if (num_thread > cpu_count * 2) {
      num_thread = cpu_count * 2;
    }
    for (size_t i = 0; i < num_thread; i++) {
      _pool.push_back(io_thread::create());
    }
  }

  io_context::value_type least_use_count() const {
    io_context::value_type ios;
    for (size_t i = 0; i < _pool.size(); i++) {
      auto ptr = _pool[i]->get_executor();
      if (!ios || ptr.use_count() < ios.use_count()) {
        ios = ptr;
      }
    }
    return ios;
  }

  class io_thread final {
  public:
    typedef std::shared_ptr<io_thread> value_type;

    static inline value_type create() {
      return value_type(new io_thread());
    }

    virtual ~io_thread() {
      delete _thread;
    }

    inline void start() {
      assert(!_thread);
      _thread = new std::thread(std::bind(&io_context::run, _ios));
    }

    inline void stop() {
      _ios->stop();
    }

    inline void join() {
      if (_thread && _thread->joinable()) {
        _thread->join();
      }
    }

    inline io_context::value_type get_executor() const {
      return _ios;
    }

  private:
    io_context::value_type _ios;
    std::thread* _thread = nullptr;
    io_thread() : _ios(io_context::create()) { }
  };

  std::vector<io_thread::value_type> _pool;
};

/***********************************************************************************/
} //end of namespace io
} //end of namespace eport
/***********************************************************************************/
