

#pragma once

#include "eport/detail/socket/udp/endpoint.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
namespace udp   {
/***********************************************************************************/

class socket
  : public std::enable_shared_from_this<socket>
  , public asio::ip::udp::socket
{
  typedef asio::ip::udp::socket parent;
  typedef std::function<void(const error_code&, size_t)> trans_handler;
  typedef std::function<void(const error_code&)> wait_handler;

private:
  socket(io_context::value_type ios)
    : parent(*ios), _ios(ios) {
  }
  socket(const socket&) = delete;

public:
  virtual ~socket() {
    close();
  }
  inline int id() const {
    return _id.value();
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
  size_t send(const std::string& data, error_code& ec) {
    return send(data.c_str(), data.size(), ec);
  }
  size_t send(const char* data, size_t bytes, error_code& ec) {
    return send(buffer(data, bytes), ec);
  }
  template<typename ConstBufferSequence>
  size_t send(const ConstBufferSequence& buffers, error_code& ec) {
    return parent::send(buffers, 0, ec);
  }
  size_t send_to(const std::string& data, const endpoint_type& remote, error_code& ec) {
    return send_to(data.c_str(), data.size(), remote, ec);
  }
  size_t send_to(const char* data, size_t bytes, const endpoint_type& remote, error_code& ec) {
    return send_to(buffer(data, bytes), remote, ec);
  }
  template<typename ConstBufferSequence>
  size_t send_to(const ConstBufferSequence& buffers, const endpoint_type& remote, error_code& ec) {
    return parent::send_to(buffers, remote, 0, ec);
  }
  size_t receive(char* out, size_t size, error_code& ec) {
    return receive(buffer(out, size), ec);
  }
  template<typename MutableBufferSequence>
  size_t receive(const MutableBufferSequence& buffers, error_code& ec) {
    return parent::receive(buffers, 0, ec);
  }
  size_t receive_from(char* out, size_t size, endpoint_type& remote, error_code& ec) {
    return receive_from(buffer(out, size), remote, ec);
  }
  template<typename MutableBufferSequence>
  size_t receive_from(const MutableBufferSequence& buffers, endpoint_type& remote, error_code& ec) {
    return parent::receive_from(buffers, remote, 0, ec);
  }
  io_context::value_type get_executor() const {
    return _ios;
  }
  virtual void close() {
    if (!is_open()) {
      return;
    }
    error_code ec;
    shutdown(shutdown_type::shutdown_both, ec);
    parent::close(ec);
  }
  void shutdown(shutdown_type what, error_code& ec) {
    parent::shutdown(what, ec);
  }
  typedef std::shared_ptr<socket> value_type;
  static value_type create(io_context::value_type ios) {
    return value_type(new socket(ios));
  }

public:
  template<typename WaitHandler>
  void async_wait(wait_type what, WaitHandler&& handler) {
    auto callback = std::bind(
      &socket::on_wait, shared_from_this(),
      std::placeholders::_1,
      (wait_handler)handler
    );
    parent::async_wait(what, callback);
  }
  template<typename WriteHandler>
  void async_send(const std::string& data, WriteHandler&& handler) {
    async_send(data.c_str(), data.size(), handler);
  }
  template<typename WriteHandler>
  void async_send_to(const std::string& data, const endpoint_type& remote, WriteHandler&& handler) {
    async_send_to(data.c_str(), data.size(), remote, handler);
  }
  template<typename WriteHandler>
  void async_send(const char* data, size_t bytes, WriteHandler&& handler) {
    auto callback = std::bind(
      &socket::on_send, shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2,
      (trans_handler)handler
    );
    parent::async_send(buffer(data, bytes), callback);
  }
  template<typename WriteHandler>
  void async_send_to(const char* data, size_t bytes, const endpoint_type& remote, WriteHandler&& handler) {
    auto callback = std::bind(
      &socket::on_send, shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2,
      (trans_handler)handler
    );
    parent::async_send_to(buffer(data, bytes), remote, callback);
  }
  template<typename ReadHandler>
  void async_receive(char* data, size_t size, ReadHandler&& handler) {
    auto callback = std::bind(
      &socket::on_receive, shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2,
      (trans_handler)handler
    );
    parent::async_receive(buffer(data, size), callback);
  }
  template<typename ReadHandler>
  void async_receive_from(char* data, size_t size, endpoint_type& remote, ReadHandler&& handler) {
    auto callback = std::bind(
      &socket::on_receive, shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2,
      (trans_handler)handler
    );
    parent::async_receive_from(buffer(data, size), remote, callback);
  }

private:
  void on_wait(const error_code& ec, const wait_handler& handler) {
    pcall(handler, ec);
  }
  void on_send(const error_code& ec, size_t bytes, const trans_handler& handler) {
    pcall(handler, ec, bytes);
  }
  void on_receive(const error_code& ec, size_t bytes, const trans_handler& handler) {
    pcall(handler, ec, bytes);
  }

private:
  identifier _id;
  io_context::value_type _ios;
};

/***********************************************************************************/
} //end of namespace udp
} //end of namespace ip
} //end of namespace eport
/***********************************************************************************/
