

#pragma once

#include <mutex>
#include <list>
#include <memory>

#include "eport/3rd.hpp"

#define MAX_IDENTIFIER 0xffff

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

class identifiers final {
  friend class identifier;
  typedef std::shared_ptr<identifiers> value_type;

  static value_type instance() {
    static value_type _self(new identifiers());
    return _self;
  }

  int pop() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_next <= MAX_IDENTIFIER) {
      return _next++;
    }
    if (_unused.empty()) {
      return 0;
    }
    auto newid = _unused.front();
    _unused.pop_front();
    return newid;
  }

  void push(int v) {
    std::unique_lock<std::mutex> lock(_mutex);
    _unused.push_back(v);
  }

  std::mutex _mutex;
  int _next = 1;
  std::list<int> _unused;
};

/***********************************************************************************/

class identifier final {
  identifier(const identifier&) = delete;
  identifiers::value_type _hold = identifiers::instance();
  int _value = _hold->pop();

public:
  identifier() = default;
  inline ~identifier() { _hold->push(_value); }
  inline int value() const { return _value; }
};

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
