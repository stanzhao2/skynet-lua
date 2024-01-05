

#pragma once

#include "eport/detail/socket/tcp/stream.hpp"

/***********************************************************************************/
namespace eport {
namespace ip    {
namespace tcp   {
/***********************************************************************************/

typedef stream::value_type session;

class acceptor
  : public std::enable_shared_from_this<acceptor>
  , public asio::ip::tcp::acceptor {

  identifier _id;
  io_context::value_type _ios;
  bool _accepting = false;
  typedef asio::ip::tcp::acceptor parent;
  typedef std::function<void(const error_code&, session)> accept_handler;
  typedef std::function<session(void)> stream_alloter;

  acceptor(io_context::value_type ios)
    : parent(*ios), _ios(ios) {
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
  void on_callback(const error_code& ec,
    session peer, const accept_handler& handler) {
    pcall(handler, ec, peer);
  }
  void on_accept(
    const error_code& ec, session peer, bool inflate,
    const accept_handler& handler)
  {
    _accepting = false;
    if (ec) {
      pcall(handler, ec, peer);
      return;
    }
    auto callback = std::bind(
      &acceptor::on_callback, shared_from_this(),
      std::placeholders::_1,
      peer, (accept_handler)handler
    );
    peer->on_async_accept(inflate, callback);
  }

public:
  typedef std::shared_ptr<acceptor> value_type;
  static value_type create(io_context::value_type ios) {
    return value_type(new acceptor(ios));
  }
  io_context::value_type get_executor() const {
    return _ios;
  }
  inline int id() const {
    return _id.value();
  }
  void listen(const endpoint_type& local, error_code& ec) {
    listen(local, 1024, ec);
  }
  void listen(const endpoint_type& local, int backlog, error_code& ec) {
    bind(local, ec);
    if (!ec) {
      parent::listen(backlog, ec);
    }
  }
  void accept(session peer, error_code& ec) {
    accept(peer, INFLATE_F, ec);
  }
  void accept(session peer, bool inflate, error_code& ec) {
    parent::accept(*peer->lowest_layer(), ec);
    if (!ec) {
      peer->on_accept(inflate, ec);
    }
  }

public:
  /*Handler: void (const error_code& ec, session peer)*/
  template <typename AcceptHandler>
  void async_accept(session peer,  AcceptHandler&& handler) {
    async_accept(peer, INFLATE_F, handler);
  }
  /*Handler: void (const error_code& ec, session peer)*/
  template <typename AcceptHandler>
  void async_accept(session peer, bool inflate, AcceptHandler&& handler) {
    auto callback = std::bind(
      &acceptor::on_accept, shared_from_this(),
      std::placeholders::_1,
      peer, inflate, (accept_handler)handler
    );
    _accepting = true;
    parent::async_accept(*peer->lowest_layer(), callback);
  }
  /*Handler: void (const error_code& ec, session peer)*/
  template <typename AcceptHandler>
  void async_accept(const stream_alloter& alloter, AcceptHandler&& handler) {
    async_accept(alloter, INFLATE_F, handler);
  }
  /*Handler: void (const error_code& ec, session peer)*/
  template <typename AcceptHandler>
  void async_accept(const stream_alloter& alloter, bool inflate, AcceptHandler&& handler) {
    auto callback = (accept_handler)handler;
    async_accept(alloter(), inflate,
      std::bind(
        [alloter, callback, inflate](const error_code& ec, session peer, value_type self) {
          pcall(callback, ec, peer);
          if (self->_accepting) {
            return;
          }
          if (self->is_open()) {
            self->async_accept(alloter, inflate, callback);
          }
        },
        std::placeholders::_1,
        std::placeholders::_2, shared_from_this()
      )
    );
  }
};

/***********************************************************************************/
} //end of namespace tcp
} //end of namespace ip
} //end of namespace eport
/***********************************************************************************/
