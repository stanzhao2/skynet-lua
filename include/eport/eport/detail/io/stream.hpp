

#pragma once

#include "eport/detail/algo/sha1.hpp"
#include "eport/detail/algo/base64.hpp"
#include "eport/detail/io/parser.hpp"
#include "eport/detail/io/decoder.hpp"
#include "eport/detail/io/conv.hpp"
#include "eport/detail/zlib/gzip.hpp"
#include "eport/detail/socket/tcp/socket.hpp"

#ifndef EPORT_ZLIB_ENABLE
#define INFLATE_F false
#else
#define INFLATE_F true
#endif

#define HTTP_HWEBSOCKET_MASK                    "Websocket-Mask"
#define HTTP_HSEC_WEBSOCKET_KEY                 "Sec-WebSocket-Key"
#define HTTP_HSEC_WEBSOCKET_ACCEPT              "Sec-WebSocket-Accept"
#define HTTP_HSEC_WEBSOCKET_EXTENSIONS          "Sec-WebSocket-Extensions"
#define HTTP_HSEC_WEBSOCKET_VERSION             "Sec-WebSocket-Version"
#define HTTP_HCONNECTION                        "Connection"
#define HTTP_HCACHE_CONTROL                     "Cache-Control"
#define HTTP_HUSER_AGENT                        "User-Agent"
#define HTTP_HACCEPT_LANGUAGE                   "Accept-Language"
#define HTTP_HACCEPT                            "Accept"
#define HTTP_HACCEPT_ENCODING                   "Accept-Encoding"
#define HTTP_HSERVER                            "Server"
#define HTTP_HPRAGMA                            "Pragma"
#define HTTP_HUPGRADE                           "Upgrade"
#define HTTP_HORIGIN                            "Origin"
#define HTTP_HTRANSFER_ENCODING                 "Transfer-Encoding"
#define HTTP_HCONTENT_TYPE                      "Content-Type"
#define HTTP_HCONTENT_LENGTH                    "Content-Length"
#define HTTP_HCONTENT_ENCODING                  "Content-Encoding"
#define HTTP_HACCESS_CONTROL_ALLOW_CREDENTIALS  "Access-Control-Allow-Credentials"
#define HTTP_HACCESS_CONTROL_ALLOW_ORIGIN       "Access-Control-Allow-Origin"

/***********************************************************************************/
namespace eport {
namespace io    {
namespace ws    {
/***********************************************************************************/

#define TRUST_TIMEOUT   300 /*seconds*/
#define UNTRUST_TIMEOUT 5   /*seconds*/
#define MAX_WS_PACKAGE  8 * 1024 * 1024

typedef ip::tcp::socket lowest_socket;
typedef ip::tcp::endpoint endpoint;
typedef ip::tcp::socket::endpoint_type endpoint_type;
typedef lowest_socket::value_type socket_type;

class stream final
  : public std::enable_shared_from_this<stream>
{
  friend class ip::tcp::acceptor;
  typedef std::function<void(const error_code&, const char*, size_t)> recv_handler;
  typedef std::function<void(const error_code&)> hand_handler;

  stream(socket_type socket, bool upgrade)
    : _socket(socket)
    , _decoder(MAX_WS_PACKAGE)
    , _upgrade(upgrade) {
    memset(_buffer, 0, sizeof(_buffer));
    _socket->timeout(UNTRUST_TIMEOUT);
  }

  stream(io_context::value_type ios, bool upgrade)
    : _socket(ip::tcp::socket::create(ios))
    , _decoder(MAX_WS_PACKAGE)
    , _upgrade(upgrade) {
    memset(_buffer, 0, sizeof(_buffer));
    _socket->timeout(UNTRUST_TIMEOUT);
  }

  stream(io_context::value_type ios, ssl::context::value_type sslctx, bool upgrade)
    : _socket(ip::tcp::socket::create(ios, sslctx))
    , _decoder(MAX_WS_PACKAGE)
    , _upgrade(upgrade) {
    memset(_buffer, 0, sizeof(_buffer));
    _socket->timeout(UNTRUST_TIMEOUT);
  }

  void farewell(int code, const char* reason) {
    if (!_handshaked) {
      return;
    }
    char buffer[256];
    ep_encode16(buffer, code);
    strcpy(buffer + 2, reason);
    _encoder.encode(buffer, 16, opcode_type::close, false,
      [&](const std::string& data, opcode_type, bool) {
        lowest_layer()->async_send(data);
      }
    );
  }

  void ping(const char* data, size_t bytes) {
    if (!_handshaked) {
      return;
    }
    _encoder.encode(data, bytes, opcode_type::ping, false,
      [&](const std::string& data, opcode_type, bool) {
        lowest_layer()->async_send(data);
      }
    );
  }

  void pong(const char* data, size_t bytes) {
    if (!_handshaked) {
      return;
    }
    _encoder.encode(data, bytes, opcode_type::pong, false,
      [&](const std::string& data, opcode_type, bool) {
        lowest_layer()->async_send(data);
      }
    );
  }

  static bool is_text(const char* data) {
    while (*data) {
      char c = *data++;
      if (c < 32 || c > 126) return false;
    }
    return true;
  }

  void handshake_ok() {
    _handshaked = true;
    auto lowest = lowest_layer();
    lowest->timeout(TRUST_TIMEOUT);
    if (is_client()) {
      lowest->set_alive(
        std::bind([](stream::value_type self) {
          self->ping(nullptr, 0);
        }, shared_from_this())
      );
    }
  }

  bool do_decode(size_t n, const recv_handler& handler) {
    return _decoder.decode(_buffer, n, [&](const std::string& data, opcode_type opcode, bool inflate) {
      const char* pdata = data.c_str();
      size_t bytes = data.size();
      std::string unziped;

      if (inflate) {
        if (zlib::inflate(data, unziped)) {
          pdata = unziped.c_str();
          bytes = unziped.size();
        }
      }

      switch (opcode) {
      case opcode_type::close:
        close();
        break;

      case opcode_type::ping:
        pong(pdata, bytes);
        break;

      case opcode_type::text:
        if (!is_utf8(pdata, bytes)) {
          close();
          break;
        }

      case opcode_type::binary:
        pcall(handler, no_error(), pdata, bytes);
        return;

      default:
        break;
      }
    });
  }

  void on_decode(const error_code& ec, size_t n, const recv_handler& handler) {
    _recving = false;
    if (ec) {
      pcall(handler, ec, nullptr, 0);
      return;
    }

    if (!is_websocket()) {
      pcall(handler, ec, _buffer, n);
    }

    else if (!do_decode(n, handler)) {
      pcall(handler, ec, nullptr, 0); /*decode error*/
      return;
    }

    /*prevent multiple asynchronous receive*/
    if (!_recving) {
      async_receive(handler);
    }
  }

  void on_request(const error_code& ec, size_t n, bool inflate, const hand_handler& handler) {
    http::request& req = _request;
    if (ec) {
      pcall(handler, ec);
      return;
    }

    const char* pend = 0;
    http::request_parser::result_type result;
    std::tie(result, pend) = _request_parser.parse(
      req, _buffer, _buffer + n
    );

    if (result == http::request_parser::indeterminate) {
      /*receive continue*/
      do_handshake(inflate, handler, false);
      return;
    }

    error_code ec_new;
    if (result == http::request_parser::bad) {
      /*parse error*/
      ec_new = error::operation_aborted;
    }

    _deflate = inflate;
    auto data = make_response((ec_new ? 400 : 101), _deflate);
    lowest_layer()->async_send(data);

    http::response& res = _response;
    if (!ec_new) {
      if (res.status != 101) {
        ec_new = error::access_denied;
      }
    }

    ec_new ? close() : handshake_ok();
    pcall(handler, ec_new);
  }

  void on_response(const error_code& ec, size_t n, bool inflate, const hand_handler& handler) {
    http::response& res = _response;
    const http::request& req = _request;
    if (ec) {
      pcall(handler, ec);
      return;
    }

    const char* pend = 0;
    http::response_parser::result_type result;
    std::tie(result, pend) = _response_parser.parse(
      res, _buffer, _buffer + n
    );

    if (result == http::request_parser::indeterminate) {
      /*receive continue*/
      do_handshake(inflate, handler, false);
      return;
    }

    error_code ec_new;
    if (result == http::request_parser::bad) {
      /*parse error*/
      ec_new = error::operation_aborted;
      pcall(handler, ec_new);
      return;
    }

    if (pend < _buffer + n) {
      _surplus = n - (pend - _buffer);
      memmove(_buffer, pend, _surplus);
    }

    if (res.status != 101) {
      ec_new = error::operation_aborted;
      pcall(handler, ec_new);
      return;
    }

    if (res.get_header(HTTP_HWEBSOCKET_MASK) == "disable") {
      _encoder.disable_mask();
    }

    auto key = req.get_header(HTTP_HSEC_WEBSOCKET_KEY);
    auto accept_key = res.get_header(HTTP_HSEC_WEBSOCKET_ACCEPT);
    if (accept_key != make_key(key)) {
      ec_new = error::operation_aborted;
      pcall(handler, ec_new);
      return;
    }

    auto ext = res.get_header(HTTP_HSEC_WEBSOCKET_EXTENSIONS);
    if (!ext.empty()) {
      if (ext.find("permessage-deflate") == 0) {
        _deflate = true;
      }
    }

    if (!ec_new) {
      handshake_ok();
    }
    pcall(handler, ec_new);
  }

  void do_handshake(bool inflate, const hand_handler& handler, bool first) {
    if (!is_client()){
      auto callback = std::bind(
        &stream::on_request, shared_from_this(),
        std::placeholders::_1, std::placeholders::_2, inflate, (hand_handler)handler
      );
      lowest_layer()->async_receive(_buffer, sizeof(_buffer), callback);
      return;
    }

    /*Prevent duplicate requests from being sent*/
    const http::request& req = _request;
    if (first) {
      auto packet = make_request(nullptr, nullptr, inflate);
      lowest_layer()->async_send(packet);
    }

    auto callback = std::bind(
      &stream::on_response, shared_from_this(),
      std::placeholders::_1, std::placeholders::_2, inflate, (hand_handler)handler
    );

    lowest_layer()->async_receive(_buffer, sizeof(_buffer), callback);
  }

  bool is_ws_request() const {
    const http::request& req = _request;
    auto name = req.get_header(HTTP_HCONNECTION);
    if (stricmp(name.c_str(), "Upgrade") != 0) {
      return false;
    }
	
    auto value = req.get_header("Upgrade");
    auto version = req.get_header(HTTP_HSEC_WEBSOCKET_VERSION);

    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
      return std::tolower(c);
    });

    if (value.size() < 9) {
      return false;
    }

    if (value.substr(0, 9) != "websocket") {
      return false;
    }

    if (req.method != "GET") {
      return false;
    }

    return (version == _version);
  }

  inline std::string make_key() const {
    std::string key(std::to_string((size_t)time(0)));
    return make_key(key);
  }

  std::string make_key(std::string key) const {
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    key = hash_sha1(key);

    char data[128];
    base64::encode((unsigned char*)key.c_str(), data, (int)key.size());
    key.assign(data);
    return key;
  }

  std::string make_request(const char* host, const char* uri, bool inflate) {
    http::request& req = _request;
    req.method = "GET";
    req.http_version_major = 1;
    req.http_version_minor = 1;

    if (uri && *uri) {
      req.uri = uri;
    }
    else if (req.uri.empty()) {
      req.uri = "/";
    }

    error_code ec;
    auto remote = lowest_layer()->remote_endpoint(ec);
    auto ip = remote.address().to_string();

    if (!host || *host == 0) {
      if (*(host = _host.c_str()) == 0) {
        host = ec ? nullptr : ip.c_str();
      }
    }

    if (host) {
      auto port = remote.port();
      if (!ec) {
        char hostname[256];
        if (port == 80 || port == 443) {
          strcpy(hostname, host);
        } else {
          snprintf(hostname, sizeof(hostname), "%s:%d", host, port);
        }
        req.set_header("Host", hostname);
      }
    }

    req.set_header(HTTP_HCACHE_CONTROL, "no-cache");
    req.set_header(HTTP_HCONNECTION, "Upgrade");
    req.set_header(HTTP_HPRAGMA, "no-cache");
    req.set_header(HTTP_HUPGRADE, "websocket");
    req.set_header(HTTP_HSEC_WEBSOCKET_KEY, make_key());
    req.set_header(HTTP_HSEC_WEBSOCKET_VERSION, _version);
    req.set_header(HTTP_HWEBSOCKET_MASK, "disable");

    if (inflate) {
      req.set_header(HTTP_HSEC_WEBSOCKET_EXTENSIONS, "permessage-deflate; client_max_window_bits");
    }
    return req.to_string();
  }

  std::string make_response(int status, bool& inflate) {
    http::response& res = _response;
    const http::request& req = _request;
    std::string key;

    if (status == 101) {
      if (!is_ws_request()) {
        status   = 400;
        _upgrade = false;
      }
      else {
        key = req.get_header(HTTP_HSEC_WEBSOCKET_KEY);
        if (key.empty()) {
          status = 400;
        }
      }
    }

    res.status = status;
    res.http_version_major = 1;
    res.http_version_minor = 1;
    res.set_header(HTTP_HCACHE_CONTROL, "max-age=0");
    res.set_header(HTTP_HPRAGMA, "no-cache");
    res.set_header(HTTP_HCONNECTION, "Close");

    if (req.get_header(HTTP_HWEBSOCKET_MASK) == "disable") {
      res.set_header(HTTP_HWEBSOCKET_MASK, "disable");
    }

    if (status == 101) {
      auto origin = req.get_header(HTTP_HORIGIN);
      if (!origin.empty()) {
        res.set_header(HTTP_HACCESS_CONTROL_ALLOW_CREDENTIALS, "true");
        res.set_header(HTTP_HACCESS_CONTROL_ALLOW_ORIGIN, origin);
      }

      if (inflate) {
        auto ext = req.get_header(HTTP_HSEC_WEBSOCKET_EXTENSIONS);
        if (ext.find("permessage-deflate") == 0) {
          res.set_header(HTTP_HSEC_WEBSOCKET_EXTENSIONS, "permessage-deflate; client_no_context_takeover; server_max_window_bits=15");
        }
        else {
          inflate = false;
        }
      }

      res.set_header(HTTP_HSEC_WEBSOCKET_ACCEPT, make_key(key));
      res.set_header(HTTP_HCONNECTION, "Upgrade");
      res.set_header(HTTP_HUPGRADE, "websocket");
    }
    return res.to_string();
  }

  void check_request(bool inflate, error_code& ec) {
    http::request& req = _request;
    while (!ec) {
      size_t n = lowest_layer()->receive(_buffer, sizeof(_buffer), ec);
      if (ec) {
        break;
      }

      const char* pend = 0;
      http::request_parser::result_type result;
      std::tie(result, pend) = _request_parser.parse(
        req, _buffer, _buffer + n
      );

      if (result == http::request_parser::bad) {
        ec = error::operation_aborted;
        break;
      }

      if (result == http::request_parser::good) {
        break;
      }
    }

    _deflate = true;
    auto data = make_response((ec ? 400 : 101), _deflate);
    lowest_layer()->send(data, ec);
  }

  void check_response(error_code& ec) {
    _deflate = false;
    http::response& res = _response;
    const http::request& req = _request;

    while (!ec) {
      size_t n = lowest_layer()->receive(_buffer, sizeof(_buffer), ec);
      if (ec) {
        return;
      }

      const char* pend = 0;
      http::response_parser::result_type result;
      std::tie(result, pend) = _response_parser.parse(
        res, _buffer, _buffer + n
      );

      if (result == http::response_parser::bad) {
        ec = error::operation_aborted;
        return;
      }

      if (result == http::response_parser::good) {
        if (pend < _buffer + n) {
          _surplus = n - (pend - _buffer);
          memmove(_buffer, pend, _surplus);
        }
        break;
      }
    }

    if (res.status != 101) {
      ec = error::operation_aborted;
      return;
    }

    if (res.get_header(HTTP_HWEBSOCKET_MASK) == "disable") {
      _encoder.disable_mask();
    }

    auto key = req.get_header(HTTP_HSEC_WEBSOCKET_KEY);
    auto accept_key = res.get_header(HTTP_HSEC_WEBSOCKET_ACCEPT);
    if (accept_key != make_key(key)) {
      ec = error::operation_aborted;
      return;
    }

    auto ext = res.get_header(HTTP_HSEC_WEBSOCKET_EXTENSIONS);
    if (!ext.empty()) {
      if (ext.find("permessage-deflate") == 0) {
        _deflate = true;
      }
    }
  }

  endpoint_type dns(const char* host, unsigned short port, error_code& ec) {
    endpoint_type unknown;
    auto result = ip::tcp::resolve(host, port, ec);
    if (ec) {
      return unknown;
    }

    int n = (int)result.size();
    if (n == 0) {
      ec = error::host_not_found;
      return unknown;
    }

    if (n == 1) {
      return *result.begin();
    }

    int rnd = (int)(clock::milliseconds() % n);
    for (auto iter = result.begin(); iter != result.end(); iter++) {
      if ((--n) == 0) {
        return *iter;
      }
    }
    return unknown;
  }

  void handshake_s(bool inflate, error_code& ec) {
    if (!ec) {
      check_request(inflate, ec);
      if (!ec) {
        handshake_ok();
      }
    }
  }

  void handshake_c(bool inflate, error_code& ec) {
    if (!ec) {
      auto packet = make_request(nullptr, nullptr, inflate);
      lowest_layer()->send(packet, ec);

      if (!ec) {
        check_response(ec);
        if (!ec) {
          handshake_ok();
        }
      }
    }
  }

  template <typename Handler>
  void async_handshake_s(bool inflate, Handler&& handler) {
    do_handshake(inflate, handler, true);
  }

  template <typename Handler>
  void async_handshake_c(bool inflate, Handler&& handler) {
    do_handshake(inflate, handler, true);
  }

  void handshake(bool inflate, error_code& ec) {
    if (!is_websocket()) {
      ec.clear();
      lowest_layer()->timeout(TRUST_TIMEOUT);
      return;
    }

    if (is_client()) {
      _encoder.set_client();
      _decoder.set_client();
      handshake_c(inflate, ec);
    }
    else {
      handshake_s(inflate, ec);
    }
  }

  template <typename Handler>
  void async_handshake(bool inflate, Handler&& handler) {
    if (!is_websocket()) {
      handler(no_error());
      lowest_layer()->timeout(TRUST_TIMEOUT);
      return;
    }

    if (is_client()) {
      _encoder.set_client();
      _decoder.set_client();
      async_handshake_c(inflate, handler);
      return;
    }
    async_handshake_s(inflate, handler);
  }

  /*call by acceptor*/
  void on_accept(bool inflate, error_code& ec) {
    _socket->on_accept(ec);
    if (!ec) {
      handshake(inflate, ec);
    }
  }

  /*call by acceptor*/
  template<typename AcceptHandler>
  void on_async_accept(bool inflate, AcceptHandler&& handler) {
    _socket->on_async_accept(
      [=](const error_code& ec) {
        if (ec) {
          handler(ec);
          return;
        }
        async_handshake(inflate, handler);
      }
    );
  }

  void on_connect(const error_code& ec, bool inflate, const hand_handler& handler) {
    if (ec) {
      pcall(handler, ec);
      return;
    }
    async_handshake(inflate, handler);
  }

public:
  typedef std::shared_ptr<stream> value_type;

  /*create and associate an underlying flow*/
  static value_type create(socket_type socket, bool upgrade = false) {
    assert(socket);
    return value_type(new stream(socket, upgrade));
  }

  /*create a socket/websocket*/
  static value_type create(io_context::value_type ios, bool upgrade = false) {
    assert(ios);
    return value_type(new stream(ios, upgrade));
  }

  /*create a socket/websocket for ssl*/
  static value_type create(io_context::value_type ios, ssl::context::value_type sslctx, bool upgrade = false) {
    assert(ios && sslctx);
    return value_type(new stream(ios, sslctx, upgrade));
  }

  inline void set_context(const void* p) {
    _context = p;
  }

  inline const void* get_context() const {
    return _context;
  }

  inline int id() const {
    return lowest_layer()->id();
  }

  inline socket_type lowest_layer() const {
    return _socket;
  }

  inline bool is_open() const {
    return lowest_layer()->is_open();
  }

  inline bool is_deflate() const {
    return _deflate;
  }

  inline bool is_client() const {
    return lowest_layer()->is_client();
  }

  inline bool is_websocket() const {
    return _upgrade;
  }

  inline http::request& request_header() {
    return _request;
  }

  inline http::response& response_header() {
    return _response;
  }

  virtual void close() {
    if (is_websocket()) {
      farewell(1000, "Normal Closure");
    }
    _rcvcache.clear();
    lowest_layer()->close();
  }

  inline void clone(ssl::context::value_type sslctx) {
    lowest_layer()->clone(sslctx);
  }

  void connect(const char* host, unsigned short port, error_code& ec) {
    assert(host);
    auto remote = dns(host, port, ec);
    if (!ec) {
      _host = host;
      connect(remote, ec);
    }
  }

  void connect(const endpoint_type& remote, error_code& ec) {
    connect(remote, INFLATE_F, ec);
  }

  void connect(const char* host, unsigned short port, bool inflate, error_code& ec) {
    assert(host);
    auto remote = dns(host, port, ec);
    if (!ec) {
      _host = host;
      connect(remote, inflate, ec);
    }
  }

  void connect(const endpoint_type& remote, bool inflate, error_code& ec) {
    asio::io_context ctx;
    asio::steady_timer timer(ctx);

    timer.expires_after(std::chrono::seconds(UNTRUST_TIMEOUT));
    timer.async_wait(
      [&](const error_code& ec) {
        if (!ec) lowest_layer()->close();
      }
    );

    std::thread thd([&]() { ctx.run(); });
    lowest_layer()->connect(remote, ec);
    if (!ec) {
      handshake(inflate, ec);
    }

    if (thd.joinable()) {
      ctx.stop();
      thd.join();
    }
  }

  /*Handler: void (const error_code& ec)*/
  template<typename ConnectHandler>
  void async_connect(const char* host, unsigned short port, ConnectHandler&& handler) {
    assert(host);
    error_code ec;
    auto remote = dns(host, port, ec);

    if (!ec) {
      _host = host;
      async_connect(remote, handler);
      return;
    }

    lowest_layer()->get_executor()->post([ec, handler]() {
      pcall(handler, ec);
    });
  }

  /*Handler: void (const error_code& ec)*/
  template<typename ConnectHandler>
  void async_connect(const endpoint_type& remote, ConnectHandler&& handler) {
    async_connect(remote, INFLATE_F, handler);
  }

  /*Handler: void (const error_code& ec)*/
  template<typename ConnectHandler>
  void async_connect(const char* host, unsigned short port, bool inflate, ConnectHandler&& handler) {
    assert(host);
    error_code ec;
    auto remote = dns(host, port, ec);

    if (!ec) {
      _host = host;
      async_connect(remote, inflate, handler);
      return;
    }

    lowest_layer()->get_executor()->post([ec, handler]() {
      pcall(handler, ec);
    });
  }

  /*Handler: void (const error_code& ec)*/
  template<typename ConnectHandler>
  void async_connect(const endpoint_type& remote, bool inflate, ConnectHandler&& handler) {
    auto callback = std::bind(
      &stream::on_connect, shared_from_this(),
      std::placeholders::_1,
      inflate, (hand_handler)handler
    );
    lowest_layer()->async_connect(remote, callback);
  }

  inline size_t send(const std::string& data) {
    return send(data.c_str(), data.size());
  }

  inline size_t send(const char* data, size_t bytes) {
    return send(data, bytes, _deflate);
  }

  inline size_t send(const std::string& data, bool deflate) {
    return send(data.c_str(), data.size(), deflate);
  }

  inline size_t send(const char* data, size_t bytes, bool deflate) {
    error_code ec;
    return send(data, bytes, deflate, ec);
  }

  inline size_t send(const std::string& data, error_code& ec) {
    return send(data.c_str(), data.size(), ec);
  }

  inline size_t send(const char* data, size_t bytes, error_code& ec) {
    return send(data, bytes, _deflate, ec);
  }

  inline size_t send(const std::string& data, bool deflate, error_code& ec) {
    return send(data.c_str(), data.size(), deflate, ec);
  }

  size_t send(const char* data, size_t bytes, bool deflate, error_code& ec) {
    assert(data);
    if (!is_websocket()) {
      return lowest_layer()->send(data, bytes, ec);
    }

    auto opcode = opcode_type::binary;
    if (is_text(data)) {
      opcode = opcode_type::text;
    }

    if (_handshaked && !_deflate) {
      deflate = false;
    }

    std::string ziped;
    if (deflate) {
      if (zlib::deflate(data, bytes, ziped)) {
        data  = ziped.c_str();
        bytes = ziped.size();
      }
    }

    _encoder.encode(data, bytes, opcode, deflate, [&](const std::string& data, opcode_type, bool) {
      lowest_layer()->send(data, ec);
      });
    return ec ? 0 : bytes;
  }

  inline void async_send(const std::string& data) {
    async_send(data.c_str(), data.size());
  }

  inline void async_send(const char* data, size_t bytes) {
    async_send(data, bytes, _deflate);
  }

  inline void async_send(const std::string& data, bool inflate) {
    async_send(data.c_str(), data.size(), inflate);
  }

  inline void async_send(const char* data, size_t bytes, bool inflate) {
    async_send(data, bytes, inflate, [](const error_code&, size_t) {});
  }

  /*Handler: void (const error_code& ec, size_t trans)*/
  template<typename WriteHandler>
  void async_send(const std::string& data, WriteHandler&& handler) {
    async_send(data.c_str(), data.size(), _deflate, handler);
  }

  /*Handler: void (const error_code& ec, size_t trans)*/
  template<typename WriteHandler>
  void async_send(const char* data, size_t bytes, WriteHandler&& handler) {
    async_send(data, bytes, _deflate, handler);
  }

  /*Handler: void (const error_code& ec, size_t trans)*/
  template<typename WriteHandler>
  void async_send(const std::string& data, bool deflate, WriteHandler&& handler) {
    async_send(data.c_str(), data.size(), deflate, handler);
  }

  /*Handler: void (const error_code& ec, size_t trans)*/
  template<typename WriteHandler>
  void async_send(const char* data, size_t bytes, bool deflate, WriteHandler&& handler) {
    assert(data);
    if (!is_websocket()) {
      lowest_layer()->async_send(data, bytes, handler);
      return;
    }

    auto opcode = opcode_type::binary;
    if (is_text(data)) {
      opcode = opcode_type::text;
    }

    if (_handshaked && !_deflate) {
      deflate = false;
    }

    std::string ziped;
    if (deflate) {
      if (zlib::deflate(data, bytes, ziped)) {
        data  = ziped.c_str();
        bytes = ziped.size();
      }
    }
    _encoder.encode(data, bytes, opcode, deflate, [&](const std::string& data, opcode_type, bool) {
      lowest_layer()->async_send(data, handler);
    });
  }

  void receive(std::string& data, error_code& ec) {
    while (_rcvcache.empty()) {
      size_t n = _surplus;
      if (n == 0) {
        n = lowest_layer()->receive(_buffer, sizeof(_buffer), ec);
        if (ec) {
          _rcvcache.clear();
          return;
        }
      }
      else {
        _surplus = 0;
      }

      if (!is_websocket()) {
        data.assign(_buffer, n);
        return;
      }

      auto result = do_decode(n, [&](const error_code& ec, const char* pdata, size_t bytes) {
        if (!ec) {
          std::string s(pdata, bytes);
          _rcvcache.push_back(s);
        }
      });

      if (!result) {
        _rcvcache.clear();
        ec = error::operation_aborted;
        return;
      }
    }

    data.assign(_rcvcache.front());
    _rcvcache.pop_front();
  }

  /*Handler: void (const error_code& ec, const char* data, size_t size)*/
  template<typename ReadHandler>
  void async_receive(ReadHandler&& handler) {
    if (_recving) {
      return;
    }

    _recving = true;
    if (_surplus > 0) {
      lowest_layer()->get_executor()->post(
        std::bind(
          &stream::on_decode, shared_from_this(), no_error(), _surplus, (recv_handler)handler
        )
      );
      _surplus = 0;
      return;
    }

    lowest_layer()->async_receive(_buffer, sizeof(_buffer),
      std::bind(
        &stream::on_decode, shared_from_this(), std::placeholders::_1, std::placeholders::_2, (recv_handler)handler
      )
    );
  }

private:
  socket_type            _socket;
  bool                   _deflate    = false;
  bool                   _recving    = false;
  bool                   _upgrade    = false;
  bool                   _handshaked = false;
  const char*            _version    = "13";
  const void*            _context    = nullptr;
  char                   _buffer[8192];
  size_t                 _surplus    = 0;
  std::string            _host;
  encoder                _encoder;
  decoder                _decoder;
  http::request          _request;
  http::request_parser   _request_parser;
  http::response         _response;
  http::response_parser  _response_parser;
  std::list<std::string> _rcvcache;
};

/***********************************************************************************/
} //end of namespace ws
} //end of namespace io
} //end of namespace eport
/***********************************************************************************/
