

#pragma once

#include <eport/detail/socket/tcp/stream.hpp>

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

class httpfile final {
public:
  httpfile(io_context::value_type ios) {
    _ios = ios;
  }
  bool open(const std::string& url) {
    return open(url, "");
  }

  bool open(const std::string& url, const std::string& ca) {
    if (!parse_url(url)) {
      return false;
    }
    if (!init_socket(ca)) {
      return false;
    }
    error_code ec;
    auto port  = (unsigned short)atoi(_port.c_str());
    auto plist = ip::tcp::resolve(_host.c_str(), port, ec);
    if (plist.empty()) {
      return false;
    }
    _request.reset();
    _socket->connect(*plist.begin(), ec);
    return ec.value() == 0;
  }

  bool open(const std::string& url, const char* ca, size_t size) {
    return open(url, std::string(ca, size));
  }

  void close() {
    if (_socket && _socket->is_open()) {
      _socket->close();
    }
    _length = _recved = 0;
    _socket.reset();
    _proto.clear();
    _uri.clear();
    _host.clear();
    _port.clear();
    _cache.clear();
    _encoding.clear();
    _request.reset();
    _response.reset();
    _chunk_parser.reset();
    _response_parser.reset();
  }

  bool read(std::string& out, size_t rbegin = 0, size_t rend = 0) {
    out.clear();
    return read([&out, this](const char* data, size_t size, size_t total) {
        if (data) {
          out.append(data, size);
          return;
        }
        std::string encode = encoding();
        if (encode.empty()) {
          return;
        }
        if (encode == "gzip") {
          zlib::ungzip(out, out);
          return;
        }
        if (encode == "deflate") {
          zlib::inflate(out, out);
          return;
        }
      }, rbegin, rend
    );
  }

  template <typename Handler>
  bool read(Handler&& handler, size_t rbegin = 0, size_t rend = 0) {
    if (!_socket || !_socket->is_open()) {
      return false;
    }
    error_code ec;
    init_request("GET", rbegin, rend);
    _socket->send(_request.to_string(), ec);
    if (ec) {
      return false;
    }
    if (!read_header()) {
      return false;
    }
    auto chunked = _response.get_header("Transfer-Encoding");
    return (chunked == "chunked") ? read_chunked(handler) : read_content(handler);
  }

  inline std::string encoding() const {
    return _encoding;
  }

  std::string get_header(const std::string& name) const {
    return _response.get_header(name);
  }

  void set_header(const std::string& name, const std::string& value) {
    _request.set_header(name, value);
  }

private:
  static inline bool is_digit(int c) {
    return c >= '0' && c <= '9';
  }

  static inline bool is_char(int c) {
    return c >= 0 && c <= 127;
  }

  static inline bool is_separator(char c) {
    return (c == '/');
  }

  static void to_lower(std::string& str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
  }

  bool read_header() {
    error_code ec;
    char data[8192];
    const char *begin = data, *pend = data;
    while (!ec) {
      size_t n = receive(data, sizeof(data), ec);
      if (ec) {
        return false;
      }
      begin = data;
      pend  = data + n;
      std::tie(_result, begin) = _response_parser.parse(_response, begin, pend);
      switch (_result) {
      case http::response_parser::result_type::indeterminate:
        continue;
      case http::response_parser::result_type::bad:
        return false;
      }
      break;
    }
    auto status = _response.status;
    if (status != 200 && status != 206) {
      return false;
    }
    auto length = _response.get_header("Content-Length");
    if (!length.empty()) {
      _length = (size_t)std::stoll(length);
    }
    _encoding = _response.get_header("Content-Encoding");
    _cache.assign(begin, pend - begin);
    return true;
  }

  template <typename Handler>
  void callback(Handler&& handler, const char* data, size_t size) {
    handler(data, size, _length);
  }

  template <typename Handler>
  inline bool read_chunked(Handler&& handler) {
    error_code ec;
    char buffer[8192];
    while (!ec) {
      auto n = receive(buffer, sizeof(buffer), ec);
      if (ec) {
        return false;
      }
      const char* begin = buffer;
      const char* pend  = buffer + n;
      http::chunk_parser::result_type result;

      std::string chunk;
      std::tie(result, pend) = _chunk_parser.parse(chunk, begin, pend);
      if (result == http::chunk_parser::result_type::bad) {
        return false;
      }
      if (!chunk.empty()) {
        callback(handler, chunk.c_str(), chunk.size());
      }
      if (result == http::chunk_parser::result_type::good) {
        callback(handler, nullptr, 0);
        break;
      }
    }
    return ec.value() == 0;
  }

  template <typename Handler>
  inline bool read_content(Handler&& handler) {
    error_code ec;
    char buffer[8192];
    while (!ec) {
      if (_recved > _length) {
        return false;
      }
      if (_recved == _length) {
        callback(handler, nullptr, 0);
        break;
      }
      auto n = receive(buffer, sizeof(buffer), ec);
      if (!ec) {
        callback(handler, buffer, n);
        _recved += n;
      }
    }
    return ec.value() == 0;
  }

  bool init_socket(const std::string& ca) {
    if (_proto == "http") {
      _socket = ip::tcp::socket::create(_ios);
    }
    else {
      auto ctx = ssl::context::create();
      if (!ca.empty()) {
        auto crt = buffer(ca.c_str(), ca.size());
        ctx->load_verify_buffer(crt);
        ctx->set_verify_mode(SSL_VERIFY_PEER);
      }
      _socket = ip::tcp::socket::create(_ios, ctx);
    }
    return _socket != nullptr;
  }

  bool parse_url(const std::string& url) {
    _proto = "";
    _uri   = '/';
    _host  = _port = "";

    int state = 0;
    const char* p = url.c_str();
    while (*p) {
      const char c = *p++;
      if (!is_char(c)) {
        return false;
      }
      switch (state) {
      case 0:
        if (c == ':') {
          to_lower(_proto);
          if (_proto != "http" && _proto != "https") {
            return false;
          }
          state = 1;
          break;
        }
        _proto += c;
        break;
      case 1:
        if (!is_separator(c)) {
          return false;
        }
        state = 2;
        break;
      case 2:
        if (!is_separator(c)) {
          return false;
        }
        state = 3;
        break;
      case 3:
        if (c == ':') {
          state = 4;
          break;
        }
        if (is_separator(c)) {
          state = 5;
          break;
        }
        _host += c;
        break;
      case 4:
        if (is_separator(c)) {
          state = 5;
          break;
        }
        if (!is_digit(c)) {
          return false;
        }
        _port += c;
        break;
      case 5: _uri += c;
        break;
      }
    }
    if (_port.empty()) {
      _port = (_proto == "http" ? "80" : "443");
    }
    auto port = atoi(_port.c_str());
    return (port > 0 && port <= 0xffff);
  }

  size_t receive(char* buffer, size_t size, error_code& ec) {
    if (!_cache.empty()) {
      size = _cache.size();
      memcpy(buffer, _cache.c_str(), size);
      _cache.clear();
      return size;
    }
    return _socket->receive(buffer, size, ec);
  }

  void init_request(const char* method, size_t rbegin, size_t rend) {
    set_if_empty("Host", _host);
    set_if_empty("Connection",      "keep-alive");
    set_if_empty("Cache-Control",   "no-cache");
    set_if_empty("Pragma",          "no-cache");
    set_if_empty("Accept-Encoding", "gzip, deflate");
    set_if_empty("User-Agent",      "httpfile/1.0 - (C)iccgame.com");

    _request.method = method;
    _request.uri    = _uri;
    if (rbegin > 0) {
      char range[64];
      if (rend == 0) {
        snprintf(range, sizeof(range), "Bytes=%zu-", rbegin - 1);
      }
      else {
        snprintf(range, sizeof(range), "Bytes=%zu-%zu", rbegin - 1, rend - 1);
      }
      set_if_empty("Range", range);
    }
  }

  void set_if_empty(const std::string& name, const std::string& value) {
    auto header = _request.get_header(name);
    if (header.empty()) {
      _request.set_header(name, value);
    }
  }

private:
  io_context::value_type _ios;
  std::string _proto;
  std::string _uri;
  std::string _host, _port;
  ip::tcp::socket::value_type _socket;

  std::string _cache;
  std::string _encoding;
  size_t _length = 0;
  size_t _recved = 0;
  http::chunk_parser _chunk_parser;

  http::request  _request;
  http::response _response;
  http::response_parser _response_parser;
  http::response_parser::result_type _result;
};

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
