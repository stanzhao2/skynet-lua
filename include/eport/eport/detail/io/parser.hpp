

/*******************************************************************************/
/* libeport begin */
/*******************************************************************************/

#pragma once

#include <algorithm>
#include <cctype>
#include <tuple>
#include <vector>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#ifndef CRLF
#define CRLF "\r\n"
#endif

/*******************************************************************************/
namespace eport {
namespace http  {
/*******************************************************************************/

struct header {
  std::string name, value;
};

/*******************************************************************************/

struct request {
  inline void reset() {
    method.clear();
    uri.clear();
    http_version_major = 1;
    http_version_minor = 1;
    headers.clear();
  }
  inline request() {
    reset();
  }
  std::string get_header(const std::string &name) const {
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      if (strcasecmp(name.c_str(), iter->name.c_str()) == 0) {
        return iter->value;
      }
    }
    return std::string();
  }
  void set_header(const std::string &name, const std::string &value) {
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      if (strcasecmp(name.c_str(), iter->name.c_str()) == 0) {
        iter->value = value;
        return;
      }
    }
    
    header new_header;
    new_header.name = name;
    new_header.value = value;
    headers.push_back(new_header);
  }
  std::string to_string() const {
    char line[4096];
    snprintf(line, sizeof(line), "%s %s HTTP/%d.%d" CRLF,
      method.c_str(),
      (uri.empty() ? "/" : uri.c_str()),
      http_version_major,
      http_version_major
    );
    std::string out(line);
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      snprintf(line, sizeof(line), "%s: %s" CRLF, iter->name.c_str(), iter->value.c_str());
      out.append(line);
    }
    return out.append(CRLF);
  }
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
};

/*******************************************************************************/

static bool is_ws_request(const request &req) {
  auto name    = req.get_header("Connection");
  auto value   = req.get_header(name);
  auto version = req.get_header("Sec-WebSocket-Version");
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
  return (atoi(version.c_str()) == 13);
}

/*******************************************************************************/

class request_parser {
public:
  enum result_type {
    good, bad, indeterminate
  };
  inline request_parser() {
    reset();
  }
  inline void reset() {
    parsed = 0;
    state_ = method_start;
  }
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(request& req, InputIterator begin, InputIterator end) {
    while (begin != end) {
      result_type result = consume(req, *begin++);
      if (result == good || result == bad) {
        reset();
        return std::make_tuple(result, begin);
      }
    }
    return std::make_tuple(indeterminate, begin);
  }

private:
  enum state {
    method_start,
    method,
    uri,
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3
  } state_;
  size_t parsed;

private:
  inline static bool is_char(int c) {
    return c >= 0 && c <= 127;
  }
  inline static bool is_ctl(int c) {
    return (c >= 0 && c <= 31) || (c == 127);
  }
  inline static bool is_tspecial(int c) {
    switch (c)
    {
    case '(': case ')': case '<': case '>':  case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?':  case '=':
    case '{': case '}': case ' ': case '\t':
      return true;
    default: break;
    }
    return false;
  }
  inline static bool is_digit(int c) {
    return c >= '0' && c <= '9';
  }
  result_type consume(request& req, char input) {
    if (parsed++ > 8192) {
      return bad;
    }
    switch (state_) {
    case method_start:
      req.reset();
      if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      } else {
        state_ = method;
        req.method.push_back(input);
        return indeterminate;
      }
    case method:
      if (input == ' ') {
        state_ = uri;
        return indeterminate;
      } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      } else {
        req.method.push_back(input);
        return indeterminate;
      }
    case uri:
      if (input == ' ') {
        state_ = http_version_h;
        return indeterminate;
      } else if (is_ctl(input)) {
        return bad;
      } else {
        req.uri.push_back(input);
        return indeterminate;
      }
    case http_version_h:
      if (input == 'H') {
        state_ = http_version_t_1;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_t_1:
      if (input == 'T') {
        state_ = http_version_t_2;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_t_2:
      if (input == 'T') {
        state_ = http_version_p;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_p:
      if (input == 'P') {
        state_ = http_version_slash;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_slash:
      if (input == '/') {
        req.http_version_major = 0;
        req.http_version_minor = 0;
        state_ = http_version_major_start;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_major_start:
      if (is_digit(input)) {
        req.http_version_major = req.http_version_major * 10 + input - '0';
        state_ = http_version_major;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_major:
      if (input == '.') {
        state_ = http_version_minor_start;
        return indeterminate;
      } else if (is_digit(input)) {
        req.http_version_major = req.http_version_major * 10 + input - '0';
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_minor_start:
      if (is_digit(input)) {
        req.http_version_minor = req.http_version_minor * 10 + input - '0';
        state_ = http_version_minor;
        return indeterminate;
      } else {
        return bad;
      }
    case http_version_minor:
      if (input == '\r') {
        state_ = expecting_newline_1;
        return indeterminate;
      } else if (is_digit(input)) {
        req.http_version_minor = req.http_version_minor * 10 + input - '0';
        return indeterminate;
      } else {
        return bad;
      }
    case expecting_newline_1:
      if (input == '\n') {
        state_ = header_line_start;
        return indeterminate;
      } else {
        return bad;
      }
    case header_line_start:
      if (input == '\r') {
        state_ = expecting_newline_3;
        return indeterminate;
      } else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
        state_ = header_lws;
        return indeterminate;
      } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      } else {
        req.headers.push_back(header());
        req.headers.back().name.push_back(input);
        state_ = header_name;
        return indeterminate;
      }
    case header_lws:
      if (input == '\r') {
        state_ = expecting_newline_2;
        return indeterminate;
      } else if (input == ' ' || input == '\t') {
        return indeterminate;
      } else if (is_ctl(input)) {
        return bad;
      } else {
        state_ = header_value;
        req.headers.back().value.push_back(input);
        return indeterminate;
      }
    case header_name:
      if (input == ':') {
        state_ = space_before_header_value;
        return indeterminate;
      } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      } else {
        req.headers.back().name.push_back(input);
        return indeterminate;
      }
    case space_before_header_value:
      if (input == ' ') {
        state_ = header_value;
        return indeterminate;
      } else {
        return bad;
      }
    case header_value:
      if (input == '\r') {
        state_ = expecting_newline_2;
        return indeterminate;
      } else if (is_ctl(input)) {
        return bad;
      } else {
        req.headers.back().value.push_back(input);
        return indeterminate;
      }
    case expecting_newline_2:
      if (input == '\n') {
        state_ = header_line_start;
        return indeterminate;
      } else {
        return bad;
      }
    case expecting_newline_3:
      return (input == '\n') ? good : bad;
    default: return bad;
    }
  }
};

/*******************************************************************************/

static const std::string status_text(int status) {
  switch (status) {
  case 100:
    return "Continue";
  case 101:
    return "Switching Protocols";
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 203:
    return "Non-Authoritative Information";
  case 204:
    return "No Content";
  case 205:
    return "Reset Content";
  case 206:
    return "Partial Content";
  case 300:
    return "Multiple Choices";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 303:
    return "See Other";
  case 304:
    return "Not Modified";
  case 305:
    return "Use Proxy";
  case 306:
    return "Unused";
  case 307:
    return "Temporary Redirect";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 402:
    return "Payment Required";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 406:
    return "Not Acceptable";
  case 407:
    return "Proxy Authentication Required";
  case 408:
    return "Request Time-out";
  case 409:
    return "Conflict";
  case 410:
    return "Gone";
  case 411:
    return "Length Required";
  case 412:
    return "Precondition Faile";
  case 413:
    return "Request Entity Too Large";
  case 414:
    return "Request-URI Too Large";
  case 415:
    return "Unsupported Media Type";
  case 416:
    return "Requested range not satisfiable";
  case 417:
    return "Expectation Failed";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  case 504:
    return "Gateway Time-out";
  case 505:
    return "HTTP Version not supported";
  default:
    return "Unknown Status";
  }
}

/*******************************************************************************/

class chunk_parser {
  inline void update() {
    cache = cache.substr(i);
    i = 0;
  }
  size_t i;
  std::string cache;

public:
  enum result_type {
    good, bad, indeterminate
  };
  chunk_parser() : i(0) {
    reset();
  }
  inline void reset() {
    i = 0;
    cache.clear();
  }
  bool is_hex(const char c) const {
    if (c >= '0' && c <= '9') return true;
    if (c >= 'a' && c <= 'f') return true;
    if (c >= 'A' && c <= 'F') return true;
    return false;
  }
  size_t skip_size(const char* p, size_t n, size_t& size) {
    size = 0;
    char hex[64];
    size_t i = 0, j = 0;
    for (; i < n && j < 9; i++, p++) {
      char c = *p;
      if (c != '\r' && c != '\n') {
        if (!is_hex(c)) {
          return -1; /* error */
        }
        hex[j++] = c;
        continue;
      }
      if (c == '\n') {
        hex[j++] = '\0';
        if (*hex) {
          size = std::stol(hex, 0, 16);
        }
        return i + 1;
      }
    }
    return (i == n) ? 0 : -1;
  }
  result_type consume(std::string& out, const char* p, size_t size) {
    while (true) {
      if (i == size) {
        return indeterminate;
      }
      size_t trunk_size = 0;
      size_t n = skip_size(p + i, size - i, trunk_size);
      if (n == size_t(-1)) {
        break;
      }
      if (n == 0) {
        return indeterminate;
      }
      if (trunk_size == 0) {
        i += n;
        if (n == 2) {
          update();
          return good;
        }
        continue;
      }
      if (size - i - n - 2 < trunk_size) {
        return indeterminate;
      }
      out.append(p + i + n, trunk_size);
      i += (n + trunk_size + 2);
    }
    return bad;
  }
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(std::string& out, InputIterator begin, InputIterator end) {
    cache.append(begin, end - begin);
    auto result = consume(out, cache.c_str(), cache.size());
    return std::make_tuple(result, end - cache.size());
  }
};

/*******************************************************************************/

struct response {
  inline void reset() {
    explain.clear();
    status = 0;
    http_version_major = 1;
    http_version_minor = 1;
    headers.clear();
  }
  inline response() {
    reset();
  }
  std::string get_header(const std::string &name) const {
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      if (strcasecmp(name.c_str(), iter->name.c_str()) == 0) {
        return iter->value;
      }
    }
    return std::string();
  }
  void set_header(const std::string &name, const std::string &value) {
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      if (strcasecmp(name.c_str(), iter->name.c_str()) == 0) {
        iter->value = value;
        return;
      }
    }
    header new_header;
    new_header.name = name;
    new_header.value = value;
    headers.push_back(new_header);
  }
  std::string to_string() const {
    char line[4096];
    snprintf(line, sizeof(line), "HTTP/%d.%d %d %s" CRLF,
      http_version_major,
      http_version_major,
      status,
      status_text(status).c_str()
    );
    std::string out(line);
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      snprintf(line, sizeof(line), "%s: %s" CRLF,iter->name.c_str(), iter->value.c_str());
      out.append(line);
    }
    return out.append(CRLF);
  }
  int status;
  int http_version_major;
  int http_version_minor;
  std::string explain;
  std::vector<header> headers;
};

/*******************************************************************************/

class response_parser {
public:
  enum result_type {
    good, bad, indeterminate
  };
  inline response_parser() {
    reset();
  }
  inline void reset() {
    parsed = 0;
    state_ = http_version_h;
  }
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(response& res, InputIterator begin, InputIterator end) {
    while (begin != end) {
      result_type result = consume(res, *begin++);
      if (result == good || result == bad) {
        reset();
        return std::make_tuple(result, begin);
      }
    }
    return std::make_tuple(indeterminate, begin);
  }

private:
  enum state {
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    http_status_start,
    http_status,
    http_explain_start,
    http_explain,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3
  } state_;
  size_t parsed;

private:
  static inline bool is_char(int c) {
    return c >= 0 && c <= 127;
  }
  static inline bool is_ctl(int c) {
    return (c >= 0 && c <= 31) || (c == 127);
  }
  static inline bool is_tspecial(int c) {
    switch (c)
    {
    case '(': case ')': case '<': case '>':  case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?':  case '=':
    case '{': case '}': case '\t':
      return true;
    default: break;
    }
    return false;
  }
  static inline bool is_digit(int c) {
    return c >= '0' && c <= '9';
  }
  result_type consume(response& res, char input) {
    if (parsed++ > 8192) {
      return bad;
    }
    switch (state_) {
    case http_version_h:
      res.reset();
      if (input == 'H') {
        state_ = http_version_t_1;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_t_1:
      if (input == 'T') {
        state_ = http_version_t_2;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_t_2:
      if (input == 'T') {
        state_ = http_version_p;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_p:
      if (input == 'P') {
        state_ = http_version_slash;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_slash:
      if (input == '/') {
        res.http_version_major = 0;
        res.http_version_minor = 0;
        state_ = http_version_major_start;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_major_start:
      if (is_digit(input)) {
        res.http_version_major = res.http_version_major * 10 + input - '0';
        state_ = http_version_major;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_major:
      if (input == '.') {
        state_ = http_version_minor_start;
        return indeterminate;
      }
      else if (is_digit(input)) {
        res.http_version_major = res.http_version_major * 10 + input - '0';
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_minor_start:
      if (is_digit(input)) {
        res.http_version_minor = res.http_version_minor * 10 + input - '0';
        state_ = http_version_minor;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_version_minor:
      if (input == ' ') {
        res.status = 0;
        state_ = http_status_start;
        return indeterminate;
      }
      else if (is_digit(input)) {
        res.http_version_minor = res.http_version_minor * 10 + input - '0';
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_status_start:
      if (is_digit(input)) {
        res.status = res.status * 10 + input - '0';
        state_ = http_status;
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_status:
      if (input == ' ') {
        state_ = http_explain_start;
        return indeterminate;
      }
      else if (is_digit(input)) {
        res.status = res.status * 10 + input - '0';
        return indeterminate;
      }
      else {
        return bad;
      }
    case http_explain_start:
      if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      }
      else {
        state_ = http_explain;
        res.explain.push_back(input);
        return indeterminate;
      }
    case http_explain:
      if (input == '\r') {
        state_ = expecting_newline_1;
        return indeterminate;
      }
      else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      }
      else {
        res.explain.push_back(input);
        return indeterminate;
      }
    case expecting_newline_1:
      if (input == '\n') {
        state_ = header_line_start;
        return indeterminate;
      }
      else {
        return bad;
      }
    case header_line_start:
      if (input == '\r') {
        state_ = expecting_newline_3;
        return indeterminate;
      }
      else if (!res.headers.empty() && (input == ' ' || input == '\t')) {
        state_ = header_lws;
        return indeterminate;
      }
      else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      }
      else {
        res.headers.push_back(header());
        res.headers.back().name.push_back(input);
        state_ = header_name;
        return indeterminate;
      }
    case header_lws:
      if (input == '\r') {
        state_ = expecting_newline_2;
        return indeterminate;
      }
      else if (input == ' ' || input == '\t') {
        return indeterminate;
      }
      else if (is_ctl(input)) {
        return bad;
      }
      else {
        state_ = header_value;
        res.headers.back().value.push_back(input);
        return indeterminate;
      }
    case header_name:
      if (input == ':') {
        state_ = space_before_header_value;
        return indeterminate;
      }
      else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      }
      else {
        res.headers.back().name.push_back(input);
        return indeterminate;
      }
    case space_before_header_value:
      if (input == ' ') {
        state_ = header_value;
        return indeterminate;
      }
      else {
        return bad;
      }
    case header_value:
      if (input == '\r') {
        state_ = expecting_newline_2;
        return indeterminate;
      }
      else if (is_ctl(input)) {
        return bad;
      }
      else {
        res.headers.back().value.push_back(input);
        return indeterminate;
      }
    case expecting_newline_2:
      if (input == '\n') {
        state_ = header_line_start;
        return indeterminate;
      }
      else {
        return bad;
      }
    case expecting_newline_3:
      return (input == '\n') ? good : bad;
    default: return bad;
    }
  }
};

/*******************************************************************************/
} //end of namespace http
} //end of namespace eport
/********************************************************************************/
