
#pragma once

#include "eport/detail/error.hpp"
#include "eport/detail/io/buffer.hpp"

#ifdef EPORT_SSL_ENABLE
#include <asio/ssl.hpp>
#else
inline void SSL_set_SSL_CTX(void*, void*) {}
inline void SSL_set_tlsext_host_name(void*, const void*) {}
#endif

/***********************************************************************************/
namespace eport {
namespace ssl   {
/***********************************************************************************/

#ifdef EPORT_SSL_ENABLE

template <typename Stream>
class stream : public asio::ssl::stream<Stream> {
public:
  template <typename Arg>
  stream(Arg&& arg, asio::ssl::context& ctx)
    : asio::ssl::stream<Stream>(arg, ctx) {
  }
};

typedef asio::ssl::stream_base::handshake_type handshake_type;

class context : public asio::ssl::context {
  typedef asio::ssl::context parent;
  typedef std::function<void(const char*)> sni_callback;
  sni_callback _callback;

private:
  context(method what = ssl::context::tlsv12)
    : parent(what) {
    set_options(default_workarounds
      | no_sslv2
      | no_sslv3
      | single_dh_use
    );
  }
  error_code set_password(const std::string& pwd) {
    error_code ec;
    parent::set_password_callback(
      std::bind([pwd](size_t, password_purpose) {
        return pwd.c_str();
        }, std::placeholders::_1, std::placeholders::_2), ec
    );
    return ec;
  }
  static int sni_handler(SSL* ssl, int* al, void* arg) {
    context* _this = (context*)arg;
    _this->_callback(
      SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name)
    );
    return SSL_TLSEXT_ERR_OK;
  }

public:
  error_code load_verify_file(const char* filename) {
    assert(filename);
    error_code ec;
    parent::load_verify_file(filename, ec);
    if (!ec) {
      set_verify_mode(asio::ssl::verify_peer);
      set_verify_mode(asio::ssl::verify_fail_if_no_peer_cert);
    }
    return ec;
  }
  error_code load_verify_buffer(const_buffer& ca) {
    error_code ec;
    parent::add_certificate_authority(ca, ec);
    if (!ec) {
      set_verify_mode(asio::ssl::verify_peer);
      set_verify_mode(asio::ssl::verify_fail_if_no_peer_cert);
    }
    return ec;
  }
  error_code use_certificate_chain(const_buffer& chain) {
    error_code ec;
    set_default_verify_paths(ec);
    if (!ec) {
      parent::use_certificate_chain(chain, ec);
    }
    return ec;
  }
  error_code use_private_key(const_buffer& private_key, const char* pwd = nullptr) {
    error_code ec;
    parent::use_private_key(private_key, asio::ssl::context::pem, ec);
    if (!ec && pwd) {
      ec = set_password(pwd);
    }
    return ec;
  }
  template <typename Handler>
  void use_sni_callback(Handler&& handler) {
    _callback = handler;
    auto handle = native_handle();
    SSL_CTX_set_tlsext_servername_arg(handle, this);
    SSL_CTX_set_tlsext_servername_callback(handle, sni_handler);
  }
  typedef std::shared_ptr<context> value_type;
  static value_type create(method what = ssl::context::tlsv12) {
    return value_type(new context());
  }
};

#else

enum struct handshake_type { client, server };

class context final {
  const error_code _ec = error::operation_not_supported;
  typedef void(*sni_callback)(size_t, const char*, const void*);

public:
  typedef std::shared_ptr<context> value_type;
  inline void* native_handle() const {
    return nullptr;
  }
  inline error_code load_verify_file(const char* filename) {
    return _ec;
  }
  inline error_code load_verify_buffer(const const_buffer& ca) {
    return _ec;
  }
  inline error_code use_certificate_chain(const char* cert, size_t size) {
    return _ec;
  }
  inline error_code use_private_key(const char* key, size_t size, const char* pwd = nullptr) {
    return _ec;
  }
  inline static value_type create() { return value_type(); }
  inline void use_sni_callback(sni_callback handler, size_t from, const void* argv){}
};

template <typename _Ty>
class stream final {
public:
  inline stream(_Ty, context&) {}
  inline void shutdown() {}
  inline void shutdown(error_code& ec) {}
  inline void* native_handle() { return nullptr; }
  inline void handshake(handshake_type, error_code&) {}
  template <typename Handler>
  inline void async_handshake(handshake_type, Handler&&) {}
  template <typename _ConstBuffers>
  inline std::size_t read_some(_ConstBuffers&, error_code&) { return 0; }
  template <typename _ConstBuffers, typename Handler>
  inline void async_read_some(_ConstBuffers&, Handler&&) {}
  template <typename _BuffersQueue>
  inline std::size_t write_some(_BuffersQueue&, error_code&) { return 0; }
  template <typename _BuffersQueue, typename Handler>
  inline void async_write_some(_BuffersQueue&, Handler&&) {}
};

#endif

/***********************************************************************************/
} //end of namespace ssl
} //end of namespace eport
/***********************************************************************************/
