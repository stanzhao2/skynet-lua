

#pragma once

/********************************************************************************/

#include <functional>
#include <assert.h>
#include "socket.io.h"

typedef std::function<void(void)> post_handler;
typedef std::function<void(int ec, int peer)> accept_handler;
typedef std::function<void(int ec)> connect_handler;
typedef std::function<void(int ec, lws_size size)> send_handler;
typedef std::function<void(int ec, const char* data, lws_size size)> receive_handler;
typedef std::function<void(int ec)> timer_handler;
typedef std::function<void(const char* data, lws_size size)> wwwget_handler;

#define lws_bind std::bind
#define lws_arg1 std::placeholders::_1
#define lws_arg2 std::placeholders::_2
#define lws_arg3 std::placeholders::_3
#define lws_arg4 std::placeholders::_4
#define lws_arg5 std::placeholders::_5
#define lws_arg6 std::placeholders::_6
#define lws_arg7 std::placeholders::_7
#define lws_arg8 std::placeholders::_8
#define lws_arg9 std::placeholders::_9

/********************************************************************************/
namespace lws {
/********************************************************************************/

inline lws_int newstate(){
  return ::lws_newstate();
}

inline lws_int getlocal() {
  return ::lws_getlocal();
}

inline lws_int state(lws_int what) {
  assert(what > 0);
  return ::lws_state(what);
}

inline lws_int close(lws_int what) {
  assert(what > 0);
  return ::lws_close(what);
}

inline lws_int valid(lws_int what) {
  assert(what > 0);
  return ::lws_valid(what);
}

inline lws_int restart() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_restart(st);
}

inline lws_int restart(lws_int st) {
  assert(st > 0);
  return ::lws_restart(st);
}

inline lws_int stop() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_stop(st);
}

inline lws_int stop(lws_int st) {
  assert(st > 0);
  return ::lws_stop(st);
}

inline lws_int stopped() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_stopped(st);
}

inline lws_int stopped(lws_int st) {
  assert(st > 0);
  return ::lws_stopped(st);
}

inline lws_int run() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_run(st);
}

inline lws_int run(lws_int st) {
  assert(st > 0);
  return ::lws_run(st);
}

inline lws_int run_for(lws_size expires) {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_run_for(st, expires);
}

inline lws_int run_for(lws_int st, lws_size expires) {
  assert(st > 0);
  return ::lws_run_for(st, expires);
}

inline lws_int runone() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_runone(st);
}

inline lws_int runone(lws_int st) {
  assert(st > 0);
  return ::lws_runone(st);
}

inline lws_int runone_for(lws_size expires) {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_runone_for(st, expires);
}

inline lws_int runone_for(lws_int st, lws_size expires) {
  assert(st > 0);
  return ::lws_runone_for(st, expires);
}

inline lws_int poll() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_poll(st);
}

inline lws_int poll(lws_int st) {
  assert(st > 0);
  return ::lws_poll(st);
}

inline lws_int pollone() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_pollone(st);
}

inline lws_int pollone(lws_int st) {
  assert(st > 0);
  return ::lws_pollone(st);
}

inline lws_int resolve(const char* host, const char** addr) {
  lws_int st = getlocal();
  assert(st > 0);
  assert(addr);
  return ::lws_resolve(st, host, addr);
}

inline lws_int resolve(lws_int st, const char* host, const char** addr) {
  assert(st > 0);
  assert(addr);
  return ::lws_resolve(st, host, addr);
}

inline lws_int acceptor() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_acceptor(st);
}

inline lws_int acceptor(lws_int st) {
  assert(st > 0);
  return ::lws_acceptor(st);
}

inline lws_int socket(lws_family type, const lws_cainfo* ca = nullptr) {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_socket(st, type, ca);
}

inline lws_int socket(lws_int st, lws_family type, const lws_cainfo* ca = nullptr) {
  assert(st > 0);
  return ::lws_socket(st, type, ca);
}

inline lws_int timer() {
  lws_int st = getlocal();
  assert(st > 0);
  return ::lws_timer(st);
}

inline lws_int timer(lws_int st) {
  assert(st > 0);
  return ::lws_timer(st);
}

inline lws_int cancel(lws_int id) {
  assert(id > 0);
  return ::lws_cancel(id);
}

inline lws_int listen(lws_int id, lws_ushort port, const char* host = nullptr, int backlog = 16) {
  assert(id > 0);
  return ::lws_listen(id, port, host, backlog);
}

inline lws_int setudata(lws_int id, lws_context ud) {
  assert(id > 0);
  return ::lws_setudata(id, ud);
}

inline lws_int seturi(lws_int id, const char* uri) {
  assert(id > 0);
  return ::lws_seturi(id, uri);
}

inline lws_int setheader(lws_int id, const char* name, const char* value) {
  assert(id > 0);
  return ::lws_setheader(id, name, value);
}

inline lws_int getudata(lws_int id, lws_context* ud) {
  assert(id > 0);
  return ::lws_getudata(id, ud);
}

inline lws_int geturi(lws_int id, char** uri) {
  assert(id > 0);
  return ::lws_geturi(id, uri);
}

inline lws_int getheader(lws_int id, const char* name, char** value) {
  assert(id > 0);
  return ::lws_getheader(id, name, value);
}

inline lws_int endpoint(lws_int id, lws_endinfo* info, lws_endtype type) {
  assert(id > 0);
  assert(info);
  return ::lws_endpoint(id, info, type);
}

/* void(int ec) */
template <typename Handler>
inline lws_int expires(lws_int id, lws_size expires, Handler&& handler) {
  assert(id > 0);
  static auto cb = [](lws_int ec, lws_context ud) {
    timer_handler* f = (timer_handler*)ud;
    (*f)(ec);
    delete f;
  };
  auto ud = new timer_handler(handler);
  return ::lws_expires(id, expires, cb, ud);
}

/* void(void) */
template <typename Handler>
inline lws_int post(lws_int st, Handler&& handler) {
  assert(st > 0);
  static auto cb = [](lws_context ud) {
    post_handler* f = (post_handler*)ud;
    (*f)();
    delete f;
  };
  auto ud = new post_handler(handler);
  return ::lws_post(st, cb, ud);
}

/* void(void) */
template <typename Handler>
inline lws_int post(Handler&& handler) {
  return post(getlocal(), handler);
}

/* void(void) */
template <typename Handler>
inline lws_int dispatch(lws_int st, Handler&& handler) {
  assert(st > 0);
  static auto cb = [](lws_context ud) {
    post_handler* f = (post_handler*)ud;
    (*f)();
    delete f;
  };
  auto ud = new post_handler(handler);
  return ::lws_dispatch(st, cb, ud);
}

/* void(void) */
template <typename Handler>
inline lws_int dispatch(Handler&& handler) {
  return dispatch(getlocal(), handler);
}

inline lws_int wwwget(lws_int st, const char* url, std::string& data) {
  assert(st > 0);
  static auto cb = [](const char* p, lws_size s, lws_context ud) {
    std::string* pdata = (std::string*)ud;
    pdata->assign(p, s);
  };
  return ::lws_wwwget(st, url, cb, &data);
}

inline lws_int wwwget(const char* url, std::string& data) {
  return wwwget(getlocal(), url, data);
}

/* void(const char* data, lws_size size) */
template <typename Handler>
inline lws_int wwwget(lws_int st, const char* url, Handler&& handler) {
  assert(st > 0);
  static auto cb = [](const char* data, lws_size size, lws_context ud) {
    wwwget_handler* f = (wwwget_handler*)ud;
    (*f)(data, size);
    delete f;
  };
  auto ud = new wwwget_handler(handler);
  return ::lws_wwwget(st, url, cb, ud);
}

inline lws_int accept(lws_int id, lws_int peer) {
  assert(id > 0);
  assert(peer > 0);
  return ::lws_accept(id, peer, nullptr, nullptr);
}

/* void(int ec, int peer) */
template <typename Handler>
inline lws_int accept(lws_int id, lws_int peer, Handler&& handler) {
  assert(id > 0);
  static auto cb = [](lws_int ec, lws_int peer, lws_context ud) {
    accept_handler* f = (accept_handler*)ud;
    (*f)(ec, peer);
    delete f;
  };
  auto ud = new accept_handler(handler);
  return ::lws_accept(id, peer, cb, ud);
}

inline lws_int connect(lws_int id, const char* host, lws_ushort port){
  assert(id > 0);
  assert(host && port);
  return ::lws_connect(id, host, port, nullptr, nullptr);
}

/* void(int ec) */
template <typename Handler>
inline lws_int connect(lws_int id, const char* host, lws_ushort port, Handler&& handler) {
  assert(id > 0);
  assert(host && port);
  static auto cb = [](lws_int ec, lws_context ud) {
    connect_handler* f = (connect_handler*)ud;
    (*f)(ec);
    delete f;
  };
  auto ud = new connect_handler(handler);
  return ::lws_connect(id, host, port, cb, ud);
}

inline lws_int write(lws_int id, const char* data, lws_size size) {
  assert(id > 0);
  assert(data);
  return ::lws_write(id, data, size);
}

inline lws_int send(lws_int id, const char* data, lws_size size) {
  assert(id > 0);
  assert(data);
  return ::lws_send(id, data, size, nullptr, nullptr);
}

/* void(int ec, lws_size size) */
template <typename Handler>
inline lws_int send(lws_int id, const char* data, lws_size size, Handler&& handler) {
  assert(id > 0);
  assert(data);
  static auto cb = [](lws_int ec, lws_size bytes, lws_context ud) {
    send_handler* f = (send_handler*)ud;
    (*f)(ec, bytes);
    delete f;
  };
  auto ud = new send_handler(handler);
  return ::lws_send(id, data, size, cb, ud);
}

inline lws_int read(lws_int id, lws_on_receive f, lws_context ud) {
  assert(id > 0);
  return ::lws_read(id, f, ud);
}

/* void(int ec, const char* data, lws_size size) */
template <typename Handler>
inline lws_int read(lws_int id, Handler&& handler) {
  assert(id > 0);
  static auto cb = [](lws_int ec, const char* data, lws_size bytes, lws_context ud) {
    receive_handler* f = (receive_handler*)ud;
    (*f)(ec, data, bytes);
    delete f;
  };
  auto ud = new receive_handler(handler);
  return ::lws_read(id, cb, ud);
}

/* void(int ec, const char* data, lws_size size) */
template <typename Handler>
inline lws_int receive(lws_int id, Handler&& handler) {
  assert(id > 0);
  static auto cb = [](lws_int ec, const char* data, lws_size bytes, lws_context ud) {
    receive_handler* f = (receive_handler*)ud;
    (*f)(ec, data, bytes);
    if (ec) {
      delete f;
    }
  };
  auto ud = new receive_handler(handler);
  return ::lws_receive(id, cb, ud);
}

/********************************************************************************/
} //namespace lnf
/********************************************************************************/
