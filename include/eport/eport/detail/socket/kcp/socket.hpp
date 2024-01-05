

#pragma once

#ifdef EPORT_KCP_ENABLE

#define KINTERVAL    10
#define KCP_HEADSIZE 24

#include <ikcp.h>
#include "eport/detail/io/context.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
namespace kcp   {
/***********************************************************************************/

enum struct event_type {
  send, receive, error
};

class tunnel final
  : public std::enable_shared_from_this<tunnel>
{
  typedef std::shared_ptr<tunnel>  tunnel_t;
  typedef io_context::value_type   ikcp_context;
  typedef steady_timer::value_type ikcp_timer;

  typedef std::function<
    void(event_type, tunnel_t, const char*, size_t)
  > ikcp_handler;

  ikcp_context      _service;
  ikcp_timer        _timer;
  ikcpcb*           _ikcp;
  const void*       _context;
  size_t            _idle;
  size_t            _next_update;
  io::ws::decoder   _decoder;
  io::ws::encoder   _encoder;
  ikcp_handler      _handler;

  tunnel(ikcp_context ios, IUINT32 conv, bool stream)
    : _service(ios)
    , _timer(steady_timer::create(ios))
    , _ikcp(ikcp_create(conv, this))
    , _context(nullptr)
    , _idle(0)
    , _next_update(0)
  {
    ikcp_setoutput(_ikcp, output);
    ikcp_setmtu   (_ikcp, 1400);
    ikcp_wndsize  (_ikcp, 64, 256);
    ikcp_nodelay  (_ikcp, 1, KINTERVAL, 2, 1);
#if 0
    _ikcp->writelog  = record;
    _ikcp->logmask   = 0x7fffffff;
#endif
    _ikcp->dead_link = 5;
    _ikcp->stream    = stream ? 1 : 0;
  }

  static inline IUINT32 time_now() {
    return (IUINT32)clock::milliseconds();
  }

  static int output(const char *buf, int len, ikcpcb *kcp, void *user) {
    ((tunnel*)user)->on_flush(buf, len);
    return len;
  }

  static void record(const char* log, ikcpcb* kcp, void* user) {
    printf("KCP> conv:%u, %s\n", kcp->conv, log);
  }

  void on_timer(const error_code& ec) {
    if (ec) {
      on_error(ec.message().c_str());
      return;
    }

    if (is_error()) {
      char err[64];
      int wsnd = ikcp_waitsnd(_ikcp);
      snprintf(err, sizeof(err), "send error: %d", wsnd);
      on_error(err);
      return;
    }

    auto now = time_now();
    while (now > _next_update) {
      ikcp_update(_ikcp, now);
      _next_update = ikcp_check(_ikcp, now);
    }

    auto wsnd = ikcp_waitsnd(_ikcp);
    if (wsnd != 0) {
      _idle = 0;
    }
    else {
      _idle++;
      if (_idle * KINTERVAL >= 10000) {
        ping(); /* do keepalive */
      }
    }

    auto expires = _next_update - now;
    if (expires > KINTERVAL) {
      expires = KINTERVAL;
    }

    _timer->expires_after(
      std::chrono::milliseconds(expires)
    );
    _timer->async_wait(
      std::bind(
        &tunnel::on_timer, shared_from_this(), std::placeholders::_1
      )
    );
  }

  void ping() {
    _encoder.encode("", 0,
      io::ws::opcode_type::ping, false,
      [this](const std::string& data, io::ws::opcode_type, bool) {
        ikcp_send(_ikcp, data.c_str(), (int)data.size());
      }
    );
  }

  void on_error(const char* err) {
    _ikcp->state = (IUINT32)-1;
    if (!_handler) {
      return;
    }
    pcall(
      _handler, event_type::error, shared_from_this(), err, 0
    );
  }

  void on_flush(const char *buf, int len) {
    if (!_handler) {
      return;
    }
    pcall(
      _handler, event_type::send, shared_from_this(), buf, len
    );
  }

  void on_send(const std::string& packet, bool flush) {
    const char* data = packet.c_str();
    size_t size = packet.size();
    _encoder.encode(
      data, size,
      io::ws::opcode_type::binary, false,
      [this](const std::string& data, io::ws::opcode_type, bool) {
        ikcp_send(_ikcp, data.c_str(), (int)data.size());
      }
    );
    if (flush) {
      ikcp_flush(_ikcp);
    }
  }

  void on_receive(const char* data, int size) {
    if (size < 1) {
      if (size < 0) {
        char err[64];
        snprintf(err, sizeof(err), "receive error: %d", size);
        on_error(err);
      }
      return;
    }
    if (!_handler) {
      return;
    }
    bool ok = _decoder.decode(data, size,
      [this](const std::string& data, io::ws::opcode_type op, bool) {
        if (op != io::ws::opcode_type::binary) {
          return;
        }
        if (data.empty()) {
          return;
        }
        pcall(
          _handler, event_type::receive, shared_from_this(), data.c_str(), data.size()
        );
      }
    );
    if (!ok) {
      char err[64];
      snprintf(err, sizeof(err), "receive error: %d", size);
      on_error(err);
    }
  }

  void on_input(const std::string& packet, bool flush) {
    int n = ikcp_input(_ikcp, packet.c_str(), (long)packet.size());
    if (n < 0) {
      char err[64];
      snprintf(err, sizeof(err), "input error: %d", n);
      on_error(err);
      return;
    }
    if (_ikcp->dead_link < 10) {
      _ikcp->dead_link = 10;
    }
    if (flush) {
      ikcp_flush(_ikcp);
    }
    int size = ikcp_peeksize(_ikcp);
    if (size < 0) {
      return;
    }
    char data[4096];
    on_receive(data, ikcp_recv(_ikcp, data, sizeof(data)));
  }

public:
  typedef tunnel_t value_type;

  static inline IUINT32 getconv(const void* data) {
    assert(data);
    return ikcp_getconv(data);
  }

  static inline IUINT32 getsn(const void* data) {
    assert(data);
    return ikcp_getsn(data);
  }

  static value_type create(ikcp_context ios, IUINT32 conv, bool stream = true) {
    return value_type(new tunnel(ios, conv, stream));
  }

  virtual ~tunnel() {
    close();
    ikcp_release(_ikcp);
  }

  virtual void close() {
    _timer->cancel();
    _ikcp->state = -1;
  }

  inline bool is_error() {
    return (_ikcp->state == (IUINT32)-1);
  }

  inline ikcpcb* lowest_layer() const {
    return _ikcp;
  }

  inline ikcp_context get_executor() const {
    return _service;
  }

  inline IUINT32 conv() const {
    return _ikcp->conv;
  }

  inline void set_client() {
    _decoder.set_client();
    _encoder.set_client();
  }

  inline void setmtu(int mtu) {
    ikcp_setmtu(_ikcp, mtu);
  }

  inline void context(const void* p) {
    _context = p;
  }

  inline const void* context() const {
    return _context;
  }

  void bind(const ikcp_handler& handler) {
    assert(handler);
    error_code ec;
    on_timer(ec);
    _handler = handler;
  }

public:
  void send(const char* data, int size) {
    send(data, size, false);
  }

  void send(const char* data, int size, bool flush) {
    assert(data && size);
    std::string packet(data, size);
    send(packet, flush);
  }

  void send(const std::string& data) {
    send(data, false);
  }

  void send(const std::string& data, bool flush) {
    on_send(data, flush);
  }

  void input(const char* data, int size) {
    input(data, size, false);
  }

  void input(const char* data, int size, bool flush) {
    assert(data && size);
    std::string packet(data, size);
    input(packet, flush);
  }

  void input(const std::string& data) {
    input(data, false);
  }

  void input(const std::string& data, bool flush) {
    on_input(data, flush);
  }

public:
  void async_send(const char* data, int size) {
    async_send(data, size, false);
  }

  void async_send(const char* data, int size, bool flush) {
    assert(data);
    std::string packet(data, size);
    async_send(packet, flush);
  }

  void async_send(const std::string& data) {
    async_send(data, false);
  }

  void async_send(const std::string& data, bool flush) {
    _service->post(
      std::bind(&tunnel::on_send, shared_from_this(), data, flush)
    );
  }

  void async_input(const char* data, int size) {
    async_input(data, size, false);
  }

  void async_input(const char* data, int size, bool flush) {
    assert(data && size);
    std::string packet(data, size);
    async_input(packet, flush);
  }

  void async_input(const std::string& data) {
    async_input(data, false);
  }

  void async_input(const std::string& data, bool flush) {
    _service->post(
      std::bind(&tunnel::on_input, shared_from_this(), data, flush)
    );
  }
};

/***********************************************************************************/
} //end of namespace kcp
} //end of namespace ip
/***********************************************************************************/

//handler: void (kcp_event ev, kcp_tunnel tunnel, const char* data, size_t size)
typedef IUINT32 kcp_conv;
typedef ip::kcp::event_type kcp_event;
typedef ip::kcp::tunnel::value_type kcp_tunnel;

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/

#endif
