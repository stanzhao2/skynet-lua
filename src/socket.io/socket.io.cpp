

/********************************************************************************/

#include <eport.hpp>
#include <map>
#include "socket.io.hpp"

using namespace eport;

#define empty_context  io_context::value_type()
#define empty_socket   ip::tcp::session()
#define empty_timer    steady_timer::value_type()
#define empty_acceptor ip::tcp::acceptor::value_type()

#define return_if_empty(what) if (!what) return lws_error;
#define unique_mutex_lock(what) std::unique_lock<std::mutex> lock(what)

/********************************************************************************/

static std::mutex lws_mutex;

class lws_local{
  const lws_int id;
public:
  inline lws_local() : id(lws_newstate()) {}
  inline ~lws_local() { lws_close(id); }
  inline lws_int state() const { return id; }
};

static std::map<
  lws_int, io_context::value_type
> lws_services_pool;

static std::map<
  lws_int, ip::tcp::session
> lws_sockets_pool;

static std::map<
  lws_int, ip::tcp::acceptor::value_type
> lws_acceptors_pool;

static std::map<
  lws_int, steady_timer::value_type
> lws_timers_pool;

static io_context::value_type find_service(lws_int id) {
  unique_mutex_lock(lws_mutex);
  auto iter = lws_services_pool.find(id);
  return (iter == lws_services_pool.end() ? empty_context : iter->second);
}

static ip::tcp::session find_socket(lws_int id) {
  unique_mutex_lock(lws_mutex);
  auto iter = lws_sockets_pool.find(id);
  return (iter == lws_sockets_pool.end() ? empty_socket : iter->second);
}

static ip::tcp::acceptor::value_type find_acceptor(lws_int id) {
  unique_mutex_lock(lws_mutex);
  auto iter = lws_acceptors_pool.find(id);
  return (iter == lws_acceptors_pool.end() ? empty_acceptor : iter->second);
}

static steady_timer::value_type find_timer(lws_int id) {
  unique_mutex_lock(lws_mutex);
  auto iter = lws_timers_pool.find(id);
  return (iter == lws_timers_pool.end() ? empty_timer : iter->second);
}

/********************************************************************************/

LIB_CAPI lws_int lws_newstate() {
  auto state = io_context::create();
  return_if_empty(state);
  auto id = state->id();

  unique_mutex_lock(lws_mutex);
  lws_services_pool[id] = state;
  return id;
}

LIB_CAPI lws_int lws_getlocal() {
  static thread_local lws_local local;
  return local.state();
}

LIB_CAPI lws_int lws_state(lws_int what) {
  auto socket = find_socket(what);
  if (socket) {
    return socket->lowest_layer()->get_executor()->id();
  }
  auto timer = find_timer(what);
  if (timer) {
    return timer->get_executor()->id();
  }
  auto acceptor = find_acceptor(what);
  return_if_empty(acceptor);
  return acceptor->get_executor()->id();
}

LIB_CAPI lws_int lws_close(lws_int what) {
  auto socket = find_socket(what);
  if (socket) {
    socket->close();
    unique_mutex_lock(lws_mutex);
    lws_sockets_pool.erase(what);
    return lws_true;
  }
  auto timer = find_timer(what);
  if (timer) {
    try { timer->cancel(); } catch (...) {}
    unique_mutex_lock(lws_mutex);
    lws_timers_pool.erase(what);
    return lws_true;
  }
  auto state = find_service(what);
  if (state) {
    state->stop();
    unique_mutex_lock(lws_mutex);
    lws_services_pool.erase(what);
    return lws_true;
  }
  auto acceptor = find_acceptor(what);
  if (acceptor) {
    acceptor->close();
    unique_mutex_lock(lws_mutex);
    lws_acceptors_pool.erase(what);
    return lws_true;
  }
  return lws_false;
}

LIB_CAPI lws_int lws_valid(lws_int what) {
  auto socket = find_socket(what);
  if (socket) {
    return lws_true;
  }
  auto timer = find_timer(what);
  if (timer) {
    return true;
  }
  auto state = find_service(what);
  if (state) {
    return true;
  }
  auto acceptor = find_acceptor(what);
  return acceptor ? lws_true : lws_false;
}

/********************************************************************************/

LIB_CAPI lws_int lws_post(lws_int st, lws_on_post f, lws_context ud) {
  auto state = find_service(st);
  return_if_empty(state);
  state->post([f, ud]() { pcall(f, ud); });
  return lws_true;
}

LIB_CAPI lws_int lws_dispatch(lws_int st, lws_on_post f, lws_context ud) {
  auto state = find_service(st);
  return_if_empty(state);
  state->dispatch([f, ud]() { pcall(f, ud); });
  return lws_true;
}

LIB_CAPI lws_int lws_restart(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  state->restart();
  return lws_true;
}

LIB_CAPI lws_int lws_stop(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  state->stop();
  return lws_true;
}

LIB_CAPI lws_int lws_stopped(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  return state->stopped() ? lws_true : lws_false;
}

LIB_CAPI lws_int lws_run(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  return (lws_int)state->run();
}

LIB_CAPI lws_int lws_run_for(lws_int st, lws_size ms) {
  auto state = find_service(st);
  return_if_empty(state);
  return (lws_int)state->run_for(ms);
}

LIB_CAPI lws_int lws_runone(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  return (lws_int)state->run_one();
}

LIB_CAPI lws_int lws_runone_for(lws_int st, lws_size ms) {
  auto state = find_service(st);
  return_if_empty(state);
  return (lws_int)state->run_one_for(ms);
}

LIB_CAPI lws_int lws_poll(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  return (lws_int)state->poll();
}

LIB_CAPI lws_int lws_pollone(lws_int st) {
  auto state = find_service(st);
  return_if_empty(state);
  return (lws_int)state->poll_one();
}

LIB_CAPI lws_int lws_resolve(lws_int st, const char* host, const char** addr) {
  auto state = find_service(st);
  return_if_empty(state);

  error_code ec;
  asio::ip::tcp::resolver resolver(*state);
  auto result = resolver.resolve(host, "0", ec);
  if (ec) {
    return lws_false;
  }
  static thread_local std::string __local;
  __local = result.begin()->endpoint().address().to_string();
  *addr = __local.c_str();
  return lws_true;
}

LIB_CAPI lws_int lws_wwwget(lws_int st, const char* url, lws_on_wwwget f, lws_context ud) {
  auto state = find_service(st);
  return_if_empty(state);

  httpfile file(state);
  if (!file.open(url)) {
    return lws_error;
  }
  std::string data;
  auto result = file.read(data, 0, 0);
  if (result) {
    pcall(f, data.c_str(), data.size(), ud);
  }
  file.close();
  return result ? lws_true : lws_false;
}

/********************************************************************************/

static ssl::context::value_type newssl(const lws_cainfo* ca) {
  auto sslca = ssl::context::create();
  if (!ca || !ca->crt.data) {
    return sslca;
  }
  if (ca->caf) {
    auto buf = buffer(ca->caf, ca->caf_size);
    sslca->load_verify_buffer(buf);
    sslca->set_verify_mode(SSL_VERIFY_PEER);
    return sslca;
  }
  if (ca->crt.data) {
    auto buf = buffer(ca->crt.data, ca->crt.size);
    sslca->use_certificate_chain(buf);
  }
  if (ca->key.data) {
    auto buf = buffer(ca->key.data, ca->key.size);
    sslca->use_private_key(buf, ca->pwd);
  }
  return sslca;
}

LIB_CAPI lws_int lws_acceptor(lws_int id) {
  auto state = find_service(id);
  return_if_empty(state);

  auto acceptor = ip::tcp::acceptor::create(state);
  return_if_empty(acceptor);

  unique_mutex_lock(lws_mutex);
  lws_acceptors_pool[acceptor->id()] = acceptor;
  return acceptor->id();
}

LIB_CAPI lws_int lws_listen(lws_int id, lws_ushort port, const char* host, int backlog) {
  if (!host) host = "::";
  auto acceptor = find_acceptor(id);
  return_if_empty(acceptor);

  error_code ec;
  ip::tcp::endpoint local;
  local.address(ip::make_address(host));
  local.port(port);
  acceptor->listen(local, backlog, ec);
  return ec ? (0 - ec.value()) : lws_true;
}

LIB_CAPI lws_int lws_accept(lws_int id, lws_int peer, lws_on_accept f, lws_context ud) {
  auto acceptor = find_acceptor(id);
  return_if_empty(acceptor);
  auto socket = find_socket(peer);
  return_if_empty(socket);

  if (f == NULL) {
    error_code ec;
    acceptor->accept(socket, ec);
    return ec ? (0 - ec.value()) : lws_true;
  }
  acceptor->async_accept(socket, 
    [f, ud](const error_code& ec, ip::tcp::session peer) {
      pcall(f, ec.value(), peer->id(), ud);
      if (ec) {
        lws_close(peer->id());
      }
    }
  );
  return lws_true;
}

LIB_CAPI lws_int lws_setudata(lws_int id, lws_context ud) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  socket->set_context(ud);
  return lws_true;
}

LIB_CAPI lws_int lws_getudata(lws_int id, lws_context* ud) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  *ud = socket->get_context();
  return lws_true;
}

LIB_CAPI lws_int lws_seturi(lws_int id, const char* uri) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  return_if_empty(socket->is_websocket());

  auto& header = socket->request_header();
  header.uri = uri ? uri : "/";
  return lws_true;
}

LIB_CAPI lws_int lws_geturi(lws_int id, char** uri) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  return_if_empty(uri);
  return_if_empty(socket->is_websocket());
  auto& header = socket->request_header();

  static thread_local std::string __uri;
  __uri = header.uri;
  *uri = (char*)__uri.c_str();
  return (**uri) ? lws_true : lws_false;
}

LIB_CAPI lws_int lws_setheader(lws_int id, const char* name, const char* value) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  return_if_empty(name);
  return_if_empty(value);
  return_if_empty(socket->is_websocket());

  auto& header = socket->request_header();
  header.set_header(name, value);
  return lws_true;
}

LIB_CAPI lws_int lws_getheader(lws_int id, const char* name, char** value) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  return_if_empty(value);
  return_if_empty(socket->is_websocket());
  auto& header = socket->request_header();

  static thread_local std::string __value;
  __value = header.get_header(name);
  *value = (char*)__value.c_str();
  return (**value) ? lws_true : lws_false;
}

LIB_CAPI lws_int lws_socket(lws_int id, lws_family family, const lws_cainfo* cert) {
  auto state = find_service(id);
  return_if_empty(state);

  lws_int sid = 0;
  ip::tcp::session socket;
  switch (family) {
  case lws_family::tcp:
    socket = ip::tcp::stream::create(state, false);
    break;
  case lws_family::ssl:
    socket = ip::tcp::stream::create(state, newssl(cert), false);
    break;
  case lws_family::ws:
    socket = ip::tcp::stream::create(state, true);
    break;
  case lws_family::wss:
    socket = ip::tcp::stream::create(state, newssl(cert), true);
    break;
  default: break;
  }
  unique_mutex_lock(lws_mutex);
  if (socket) {
    lws_sockets_pool[socket->id()] = socket;
  }
  return socket ? socket->id() : lws_error;
}

LIB_CAPI lws_int lws_timer(lws_int id) {
  auto state = find_service(id);
  return_if_empty(state);
  auto timer = steady_timer::create(state);
  return_if_empty(timer);
  
  unique_mutex_lock(lws_mutex);
  static lws_int tnext = MAX_IDENTIFIER + 1;
  lws_timers_pool[tnext] = timer;
  return tnext++;
}

LIB_CAPI lws_int lws_expires(lws_int id, lws_size ms, lws_on_timer f, lws_context ud) {
  auto timer = find_timer(id);
  return_if_empty(timer);
  timer->expires_after(std::chrono::milliseconds(ms));
  timer->async_wait(
    [f, ud](const error_code& ec) {
      pcall(f, ec.value(), ud);
    }
  );
  return lws_true;
}

LIB_CAPI lws_int lws_cancel(lws_int id) {
  auto timer = find_timer(id);
  return_if_empty(timer);
  try { timer->cancel(); } catch (...) {}
  return lws_true;
}

LIB_CAPI lws_int lws_connect(lws_int id, const char* host, lws_ushort port, lws_on_connect f, lws_context ud) {
  auto socket = find_socket(id);
  return_if_empty(socket);

  if (f == NULL) {
    error_code ec;
    socket->connect(host, port, ec);
    return ec ? (0 - ec.value()) : lws_true;
  }
  socket->async_connect(host, port,
    [f, ud](const error_code& ec) {
      pcall(f, ec.value(), ud);
    }
  );
  return lws_true;
}

LIB_CAPI lws_int lws_write(lws_int id, const char* data, lws_size size) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  error_code ec;
  lws_size bytes = socket->send(data, size, ec);
  return ec ? (0 - ec.value()) : (lws_int)bytes;
}

LIB_CAPI lws_int lws_send(lws_int id, const char* data, lws_size size, lws_on_send f, lws_context ud) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  if (f == NULL) {
    f = [](lws_int, lws_size, lws_context) {};
  }
  std::string packet(data, size);
  auto state = socket->lowest_layer()->get_executor();
  state->post([=]() {
    socket->async_send(packet, 
      [f, ud](const error_code& ec, lws_size trans) {
        pcall(f, ec.value(), trans, ud);
      }
    );
  });
  return lws_true;
}

LIB_CAPI lws_int lws_read(lws_int id, lws_on_receive f, lws_context ud) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  return_if_empty(f);

  error_code ec;
  std::string data;
  socket->receive(data, ec);
  if (ec) {
    data = error_message(ec);
  }
  pcall(f, ec.value(), data.c_str(), data.size(), ud);
  return ec ? (0 - ec.value()) : (lws_int)data.size();
}

LIB_CAPI lws_int lws_receive(lws_int id, lws_on_receive f, lws_context ud) {
  auto socket = find_socket(id);
  return_if_empty(socket);
  return_if_empty(f);

  socket->async_receive(
    [f, ud, id](const error_code& ec, const char* data, lws_size size) {
      std::string err;
      if (ec) {
        err = error_message(ec);
        data = err.c_str();
        size = err.size();
      }
      pcall(f, ec.value(), data, size, ud);
      if (ec) {
        lws_close(id);
      }
    }
  );
  return lws_true;
}

LIB_CAPI lws_int lws_endpoint(lws_int id, lws_endinfo* inf, lws_endtype type) {
  error_code ec;
  if (inf == NULL) {
    return lws_error;
  }
  auto socket = find_socket(id);
  ip::tcp::endpoint endpoint;
  if (socket) {
    auto lowest = socket->lowest_layer();
    endpoint = (type == lws_endtype::local ? lowest->local_endpoint(ec) : lowest->remote_endpoint(ec));
    if (ec) {
      return (0 - ec.value());
    }
    inf->port   = endpoint.port();
    inf->family = endpoint.protocol().family();
    auto addr = endpoint.address().to_string();
    strcpy(inf->ip, addr.c_str());
    return lws_true;
  }
  auto acceptor = find_acceptor(id);
  return_if_empty(acceptor);
  if (type != lws_endtype::local) {
    return lws_error;
  }
  endpoint = acceptor->local_endpoint(ec);
  if (ec) {
    return (0 - ec.value());
  }
  inf->port   = endpoint.port();
  inf->family = endpoint.protocol().family();
  auto addr = endpoint.address().to_string();
  strcpy(inf->ip, addr.c_str());
  return lws_true;
}

/********************************************************************************/
