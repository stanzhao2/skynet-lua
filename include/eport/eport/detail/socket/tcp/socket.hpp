

#pragma once

#include "eport/detail/io/context.hpp"
#include "eport/detail/ssl/context.hpp"
#include "eport/detail/socket/tcp/endpoint.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
namespace tcp   {
/***********************************************************************************/

class acceptor;
class socket
  : public std::enable_shared_from_this<socket>
  , public asio::ip::tcp::socket
{
  friend class acceptor;
  typedef asio::ip::tcp::socket parent;
  typedef std::function<void(const error_code&, size_t)> trans_handler;
  typedef std::function<void(const error_code&)> wait_handler;
  typedef std::function<void(void)> alive_handler;

private:
  struct cache_node {
    std::string   data;
    trans_handler handler;
  };

  inline void shutdown() {
    _stream ? _stream->shutdown() : void();
  }

  inline void shutdown(error_code& ec) {
    _stream ? _stream->shutdown(ec) : void();
  }

  void on_timer(const error_code& ec, size_t expires = 0) {
    if (ec) {
      return;
    }
    if (expires > 0) {
      _keepalive += expires;
      if (_keepalive > _timeout) {
        close(); /*timeout*/
        return;
      }
      if (_alive_handler) {
        pcall(_alive_handler);
      }
    }
    expires = _timeout / 5;
    if (expires < 1) {
      expires = 1;
    }
    if (expires > 60) {
      expires = 60;
    }
    _timer.expires_after(std::chrono::seconds(expires));
    _timer.async_wait(
      std::bind(
        &socket::on_timer, shared_from_this(), std::placeholders::_1, expires
      )
    );
  }

  socket(io_context::value_type ios)
    : parent(*ios)
    , _ios(ios)
    , _timer(*ios) {
  }

  socket(io_context::value_type ios, ssl::context::value_type ssl_context)
    : parent(*ios)
    , _ios(ios)
    , _timer(*ios)
    , _ssl_context(ssl_context)
    , _stream(new ssl::stream<parent&>(*this, *ssl_context)) {
  }

  socket(const socket&) = delete;

public:
  typedef ssl::handshake_type handshake_type;
  virtual ~socket() {
    delete _stream;
  }

  inline bool is_client() const {
    return !_recipient;
  }

  inline bool is_open() const {
    return parent::is_open() && !_closing;
  }

  inline int id() const {
    return _id.value();
  }

  inline size_t timeout() const {
    return _timeout;
  }

  inline void timeout(size_t expires) {
    _timeout = expires;
  }

  inline void set_hostname(const char* name) {
    if (!_stream) {
      return;
    }
    auto sh = _stream->native_handle();
    SSL_set_tlsext_host_name(sh, name);
  }

  inline void set_alive(const alive_handler& handler) {
    _alive_handler = handler;
  }

  void bind(const endpoint_type& local, error_code& ec) {
    if (!is_open()) {
      parent::open(local.protocol(), ec);
    }
    if (!ec) {
#ifndef _MSC_VER
      reuse_address option(true);
      set_option(option, ec);
      if (ec) {
        return;
      }
#endif
      parent::bind(local, ec);
    }
  }

  void connect(const endpoint_type& remote, error_code& ec) {
    _recipient = false;
    if (!is_open()) {
      parent::open(remote.protocol(), ec);
    }

    if (!ec) {
      parent::connect(remote, ec);
      if (!ec) {
        handshake(handshake_type::client, ec);
      }
    }
  }

  void clone(ssl::context::value_type ssl_context) {
    if (_stream) {
      _ssl_context = ssl_context;
      auto handle = _stream->native_handle();
      SSL_set_SSL_CTX(handle, ssl_context->native_handle());
    }
  }

  size_t receive(char* out, size_t size, error_code& ec) {
    assert(out);
    return receive(buffer(out, size), ec);
  }

  template<typename MutableBufferSequence>
  size_t receive(const MutableBufferSequence& buffers, error_code& ec) {
    return read_some(buffers, ec);
  }

  template<typename MutableBufferSequence>
  size_t read_some(const MutableBufferSequence& buffers, error_code& ec) {
    if (!is_open()) {
      ec = error::bad_descriptor;
      return 0;
    }
    return _stream ? _stream->read_some(buffers, ec) : parent::read_some(buffers, ec);
  }

  size_t send(const std::string& data, error_code& ec) {
    return send(data.c_str(), data.size(), ec);
  }

  size_t send(const char* data, size_t bytes, error_code& ec) {
    assert(data);
    return send(buffer(data, bytes), ec);
  }

  template<typename ConstBufferSequence>
  size_t send(const ConstBufferSequence& buffers, error_code& ec) {
    return write_some(buffers, ec);
  }

  template<typename ConstBufferSequence>
  size_t write_some(const ConstBufferSequence& buffers, error_code& ec) {
    if (!is_open()) {
      ec = error::bad_descriptor;
      return 0;
    }
    return _stream ? _stream->write_some(buffers, ec) : parent::write_some(buffers, ec);
  }

  io_context::value_type get_executor() const {
    return _ios;
  }

  ssl::context::value_type get_context() const {
    return _ssl_context;
  }

  void no_delay(error_code& ec) {
    asio::ip::tcp::no_delay no_delay(true);
    set_option(no_delay, ec);
  }

  virtual void close() {
    error_code ec;
    try { _timer.cancel(); }
    catch (...) { }

    if (_cache.empty()) {
      shutdown(shutdown_type::shutdown_both, ec);
      parent::close(ec);
      return;
    }

    if (!_closing) {
      _closing = true;
      shutdown(shutdown_type::shutdown_receive, ec);
      return;
    }
  }

  void shutdown(shutdown_type what, error_code& ec) {
    shutdown(ec);
    parent::shutdown(what, ec);
  }

  typedef std::shared_ptr<socket> value_type;
  static value_type create(io_context::value_type ios) {
    return value_type(new socket(ios));
  }

  static value_type create(io_context::value_type ios, ssl::context::value_type ssl_context) {
    return value_type(ssl_context ? new socket(ios, ssl_context) : new socket(ios));
  }

public:
  template<typename ConnectHandler>
  void async_connect(const endpoint_type& remote, ConnectHandler&& handler) {
    _recipient = false;
    auto callback = std::bind(
      &socket::on_connect, shared_from_this(),
      std::placeholders::_1,
      (wait_handler)handler
    );
    parent::async_connect(remote, callback);
  }

  void async_send(const std::string& data) {
    async_send(data, [](const error_code&, size_t) {});
  }

  template<typename WriteHandler>
  void async_send(const std::string& data, WriteHandler&& handler) {
    async_send(data.c_str(), data.size(), handler);
  }

  void async_send(const char* data, size_t bytes) {
    async_send(data, bytes, [](const error_code&, size_t) {});
  }

  template<typename WriteHandler>
  void async_send(const char* data, size_t bytes, WriteHandler&& handler) {
    error_code ec;
    size_t size = _cache.size();

    if (!is_open()) {
      ec = error::bad_descriptor;
    }
    else if (!data || size > 1024) {
      ec = error::no_buffer_space;
    }

    if (ec) {
      _ios->post(std::bind(
        &socket::on_send, shared_from_this(), ec, 0, (trans_handler)handler
      ));
      return;
    }

    cache_node node;
    node.data.append(data, bytes);
    node.handler = (trans_handler)handler;

    _cache.push_back(std::move(node));
    if (size == 0) {
      flush();  /*send begin*/
    }
  }

  template<typename ReadHandler>
  void async_receive(char* out, size_t size, ReadHandler&& handler) {
    assert(out);
    async_receive(buffer(out, size), handler);
  }

  template<typename MutableBufferSequence, typename ReadHandler>
  void async_receive(const MutableBufferSequence& buffers, ReadHandler&& handler) {
    async_read_some(buffers, handler);
  }

  template<typename WaitHandler>
  void async_wait(wait_type what, WaitHandler&& handler) {
    auto callback = std::bind(
      &socket::on_wait, shared_from_this(),
      std::placeholders::_1, (wait_handler)handler
    );

    if (_keepalive == 0) {
      on_timer(no_error());
    }
    parent::async_wait(what, callback);
  }

  template<typename MutableBufferSequence, typename ReadHandler>
  void async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {
    auto callback = std::bind(
      &socket::on_receive, shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2,
      (trans_handler)handler
    );

    if (_keepalive == 0) {
      on_timer(no_error());
    }
    _stream ? _stream->async_read_some(buffers, callback) : parent::async_read_some(buffers, callback);
  }

  template<typename ConstBufferSequence, typename Handler>
  void async_write_some(const ConstBufferSequence& buffers, Handler&& handler) {
    _stream ? _stream->async_write_some(buffers, handler) : parent::async_write_some(buffers, handler);
  }

public:
  void complete(const error_code& ec, size_t bytes) {
    while (true) {
      if (_cache.empty()) {
        break;
      }

      const cache_node& front = _cache.front();
      const auto& data = front.data;
      size_t size = data.size();
      on_send(ec, size, front.handler);

      _cache.pop_front();
      if (!ec && bytes >= size) {
        bytes -= size;
        if (bytes == 0) {
          break;
        }
      }
    }
    _cache.empty() ? (_closing ? close() : void()) : flush();
  }

  void flush() {
    std::vector<const_buffer> queue;
    auto iter = _cache.begin();

    for (; iter != _cache.end(); ++iter) {
      const std::string& data = iter->data;
      queue.push_back(buffer(data.c_str(), data.size()));
    }

    if (!queue.empty()) {
      asio::async_write(*this, queue, std::bind(
        &socket::complete, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      ));
    }
  }
  /*call by acceptor*/
  void on_accept(error_code& ec) {
    handshake(socket::handshake_type::server, ec);
  }

  /*call by acceptor*/
  template<typename AcceptHandler>
  void on_async_accept(AcceptHandler&& handler) {
    async_handshake(handshake_type::server, handler);
  }

  void handshake(handshake_type type, error_code& ec) {
    _stream ? _stream->handshake(type, ec) : ec.clear();
  }

  template<typename HandshakeHandler>
  void async_handshake(handshake_type type, HandshakeHandler&& handler) {
    auto callback = std::bind(
      &socket::on_handshake, shared_from_this(),
      std::placeholders::_1, (wait_handler)handler
    );

    _stream ? _stream->async_handshake(type, callback) : _ios->post([callback]() {
      callback(no_error());
    });
  }

  void on_connect(const error_code& ec, const wait_handler& handler) {
    if (ec) {
      pcall(handler, ec);
      return;
    }
    async_handshake(handshake_type::client, handler);
  }

  void on_handshake(const error_code& ec, const wait_handler& handler) {
    pcall(handler, ec);
    if (!ec) {
      _keepalive = 1;
    }
  }

  void on_wait(const error_code& ec, const wait_handler& handler) {
    pcall(handler, ec);
    if (!ec) {
      _keepalive = 1;
    }
  }

  void on_send(const error_code& ec, size_t bytes, const trans_handler& handler) {
    pcall(handler, ec, bytes);
    if (!ec) {
      _keepalive = 1;
    }
  }

  void on_receive(const error_code& ec, size_t bytes, const trans_handler& handler) {
    pcall(handler, ec, bytes);
    if (!ec) {
      _keepalive = 1;
    }
  }

private:
  identifier _id;
  io_context::value_type _ios;
  alive_handler _alive_handler;
  asio::steady_timer _timer;
  std::list<cache_node> _cache;
  ssl::context::value_type _ssl_context;
  ssl::stream<parent&>* _stream = nullptr;
  size_t _timeout   = 300; /*seconds*/
  size_t _keepalive = 0;
  bool _closing     = false;
  bool _recipient   = true;
};

/***********************************************************************************/
} //end of namespace tcp
} //end of namespace ip
} //end of namespace eport
/***********************************************************************************/
