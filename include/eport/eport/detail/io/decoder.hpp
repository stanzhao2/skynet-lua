

#pragma once

#include "eport/detail/io/protocols.hpp"

/*******************************************************************************/
namespace eport {
namespace io    {
namespace ws    {
/*******************************************************************************/

class decoder final {
  const size_t max_packet;
  std::string _cache;
  ws::context _context;

public:
  inline decoder(size_t max_size = 0)
    : max_packet(max_size) {
    _context.mask = true;
  }
  inline void set_client() {
    _context.what = ws::session_type::client;
  }
  inline bool is_client() const {
    return _context.what == ws::session_type::client;
  }
  template <typename Handler>
  bool decode(const char* data, size_t size, Handler&& handler) {
    _cache.append(data, size);
    data = _cache.c_str();
    size = _cache.size();

    auto begin = data;
    auto end   = ws::decode(data, size, _context, handler);
    if (end == nullptr) {
      return false;
    }
    if (max_packet) {
      auto psize = _context.packet.size();
      if (psize > max_packet) {
        return false;
      }
    }
    size_t removed = (size_t)(end - begin);
    if (removed == size) {
      _cache.clear();
    }
    else if (removed) {
      _cache = _cache.substr(removed);
    }
    return true;
  }
};

/*******************************************************************************/

class encoder final {
  ws::context _context;

public:
  inline encoder() {
    _context.mask = true;
  }
  inline void set_client() {
    _context.what = ws::session_type::client;
  }
  inline bool is_client() const {
    return _context.what == ws::session_type::client;
  }
  inline void disable_mask() {
    _context.mask = false;
  }
  template <typename Handler>
  void encode(const char* data, size_t size, opcode_type opcode, bool inflate, Handler handler) {
    _context.inflate = inflate;
    _context.opcode  = opcode;
    ws::encode(data, size, _context, handler);
  }
};

/*******************************************************************************/
} //end of namespace ws
} //end of namespace io
} //end of namespace eport
/*******************************************************************************/
