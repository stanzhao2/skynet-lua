

#ifndef __LIB_SOCKET_IO_H_INCLUDED_
#define __LIB_SOCKET_IO_H_INCLUDED_

/********************************************************************************/

#ifndef LIB_CAPI
#ifndef __cplusplus
#define LIB_CAPI
#else
#define LIB_CAPI extern "C"
#endif
#endif

typedef void            lws_void;
typedef int             lws_bool;
typedef char            lws_char;
typedef short           lws_short;
typedef int             lws_int;
typedef long long       lws_long;
typedef unsigned char   lws_uchar;
typedef unsigned short  lws_ushort;
typedef unsigned int    lws_uint;
typedef size_t          lws_size;

#define lws_false       (0)
#define lws_true        (1)
#define lws_error       (-1)

typedef const lws_void* lws_context;
typedef lws_void (*lws_on_post)   (lws_context ud);
typedef lws_void (*lws_on_connect)(lws_int ec, lws_context ud);
typedef lws_void (*lws_on_send)   (lws_int ec, lws_size size, lws_context ud);
typedef lws_void (*lws_on_receive)(lws_int ec, const char* data, lws_size size, lws_context ud);
typedef lws_void (*lws_on_accept) (lws_int ec, lws_int peer, lws_context ud);
typedef lws_void (*lws_on_timer)  (lws_int ec, lws_context ud);
typedef lws_void (*lws_on_wwwget) (const char* data, lws_size size, lws_context ud);

/********************************************************************************/

#pragma pack(push, 8)

enum struct lws_family {
  tcp, ssl, ws, wss
};

enum struct lws_endtype {
  local, remote
};

struct lws_endinfo {
  int  family;        /* address family */
  char ip[64];        /* ip address */
  lws_ushort port;    /* host's byte order */
};

struct lws_cainfo {
  struct {
    const char* data; /* certificate chain */
    size_t size;      /* size of Certificate chain */
  } crt;
  struct {
    const char* data; /* certificate private key */
    size_t size;      /* size of certificate private key */
  } key;
  const char* pwd;    /* the password of private key */
  const char* caf;
  size_t      caf_size;
};

#pragma pack(pop)

/********************************************************************************/

LIB_CAPI lws_int lws_newstate();
LIB_CAPI lws_int lws_state     (lws_int what);
LIB_CAPI lws_int lws_close     (lws_int what);
LIB_CAPI lws_int lws_valid     (lws_int what);
LIB_CAPI lws_int lws_resolve   (lws_int st, const char* host, const char** addr);
LIB_CAPI lws_int lws_wwwget    (lws_int st, const char* url, lws_on_wwwget f, lws_context ud);
LIB_CAPI lws_int lws_getlocal();

/********************************************************************************/

LIB_CAPI lws_int lws_restart   (lws_int st);
LIB_CAPI lws_int lws_post      (lws_int st, lws_on_post f, lws_context ud);
LIB_CAPI lws_int lws_dispatch  (lws_int st, lws_on_post f, lws_context ud);
LIB_CAPI lws_int lws_stop      (lws_int st);
LIB_CAPI lws_int lws_stopped   (lws_int st);
LIB_CAPI lws_int lws_run       (lws_int st);
LIB_CAPI lws_int lws_run_for   (lws_int st, lws_size ms);
LIB_CAPI lws_int lws_runone    (lws_int st);
LIB_CAPI lws_int lws_runone_for(lws_int st, lws_size ms);
LIB_CAPI lws_int lws_poll      (lws_int st);
LIB_CAPI lws_int lws_pollone   (lws_int st);

/********************************************************************************/

LIB_CAPI lws_int lws_acceptor  (lws_int st);
LIB_CAPI lws_int lws_accept    (lws_int id, lws_int peer, lws_on_accept f, lws_context ud);
LIB_CAPI lws_int lws_listen    (lws_int id, lws_ushort port, const char* host, int backlog);

/********************************************************************************/

LIB_CAPI lws_int lws_timer     (lws_int st);
LIB_CAPI lws_int lws_expires   (lws_int id, lws_size ms, lws_on_timer f, lws_context ud);
LIB_CAPI lws_int lws_cancel    (lws_int id);

/********************************************************************************/

LIB_CAPI lws_int lws_socket    (lws_int st, lws_family family, const lws_cainfo* ca);
LIB_CAPI lws_int lws_connect   (lws_int id, const char* host, lws_ushort port, lws_on_connect f, lws_context ud);
LIB_CAPI lws_int lws_read      (lws_int id, lws_on_receive f, lws_context ud);
LIB_CAPI lws_int lws_write     (lws_int id, const char* data, lws_size size);
LIB_CAPI lws_int lws_receive   (lws_int id, lws_on_receive f, lws_context ud);
LIB_CAPI lws_int lws_send      (lws_int id, const char* data, lws_size size, lws_on_send f, lws_context ud);
LIB_CAPI lws_int lws_endpoint  (lws_int id, lws_endinfo* inf, lws_endtype type);

/********************************************************************************/

LIB_CAPI lws_int lws_setudata  (lws_int id, lws_context ud);
LIB_CAPI lws_int lws_seturi    (lws_int id, const char* uri);
LIB_CAPI lws_int lws_setheader (lws_int id, const char* name, const char* value);
LIB_CAPI lws_int lws_getudata  (lws_int id, lws_context* ud);
LIB_CAPI lws_int lws_geturi    (lws_int id, char** uri);
LIB_CAPI lws_int lws_getheader (lws_int id, const char* name, char** value);

/********************************************************************************/

#endif //__LIB_SOCKET_IO_H_INCLUDED_
