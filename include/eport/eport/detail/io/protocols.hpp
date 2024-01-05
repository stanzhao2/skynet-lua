

#pragma once

#include "eport/3rd.hpp"
#include "eport/detail/io/endian.hpp"
/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
*/

/*******************************************************************************/
namespace eport {
namespace io    {
namespace ws    {
/*******************************************************************************/

enum struct session_type {
  client, server
};

enum struct opcode_type {
  frame   = 0,
  text    = 1,
  binary  = 2,
  close   = 8,
  ping    = 9,
  pong    = 10
};

struct context {
  inline context()
    : what(session_type::server)
    , inflate(false)
    , opcode(opcode_type::frame) {
  }
  bool          inflate;
  bool          mask;
  session_type  what;
  opcode_type   opcode;
  std::string   packet;
};

inline void do_convert(
  const char* data, size_t bytes, context& ctx, const char* p_mask) {
  char buffer[8192];
  const size_t max_ix = sizeof(buffer) - 1;

  for (size_t i = 0; i < bytes; i++) {
    char c1 = *data++;
    char c2 = p_mask[i & 3];
    auto ii = i & max_ix;
    buffer[ii] = c1 ^ c2;
    if ((ii == max_ix) || (i == bytes - 1)) {
      ctx.packet.append(buffer, ii + 1);
    }
  }
}

inline bool check_opcode(int opcode) {
  switch ((opcode_type)opcode) {
  case opcode_type::frame:
  case opcode_type::text:
  case opcode_type::binary:
  case opcode_type::close:
  case opcode_type::ping:
  case opcode_type::pong:
    return true;
  }
  return false;
}

inline unsigned int make_mask(const char* key, size_t len) {
  const unsigned int m = 0x5bd1e995;
  const int r = 24;
  const int seed = 97;
  unsigned int h = seed ^ (int)len;

  const unsigned char* data = (const unsigned char*)key;
  while (len >= 4) {
    unsigned int k = *(unsigned int*)data;
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
    data += 4;
    len  -= 4;
  }
  switch (len) {
  case 3: h ^= data[2] << 16;
  case 2: h ^= data[1] << 8;
  case 1: h ^= data[0];
    h *= m;
  };
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

/* Handler: void(const std::string&, bool, int) */
template <typename Handler>
inline const char* decode(const char* data, size_t size, context& ctx, Handler&& handler)
{
  const char* pend = data;
  while (data && size)
  {
    u8 v1 = 0;
    data = ep_decode8(data, &v1);
    size -= 1;

    /*check header*/
    bool fin = (v1 & 0x80) != 0;
    if (v1 & 0x40) {
      ctx.inflate = true;
    }
    bool rsv2 = (v1 & 0x20) != 0;
    if (rsv2) {
      return nullptr;
    }
    bool rsv3 = (v1 & 0x10) != 0;
    if (rsv2) {
      return nullptr;
    }
    u8 opcode = v1 & 0x0f;
    if (!check_opcode(opcode)) {
      return nullptr;
    }
    if (opcode) {
      ctx.opcode = (opcode_type)opcode;
    }
    if (size < 1) {
      break;
    }
    u8 v2 = 0;
    data = ep_decode8(data, &v2);
    size -= 1;

    /*check mask*/
    bool have_mask = (v2 & 0x80) != 0;
    if (ctx.what == session_type::server) {
      if (!have_mask) {
        return nullptr;
      }
    } else {
      if (have_mask) {
        return nullptr;
      }
    }
    /*get data size*/
    u64 payload_len = (v2 & 0x7f);
    if (payload_len == 126) {
      if (size < 2) {
        break;
      }
      u16 len;
      data = ep_decode16(data, &len);
      size -= 2;
      payload_len = len;
    }
    else if (payload_len == 127) {
      if (size < 8) {
        break;
      }
      data = ep_decode64(data, &payload_len);
      size -= 8;
    }
    /*get mask*/
    const char* p_mask = 0;
    if (have_mask) {
      if (size < 4) {
        break;
      }
      p_mask = data;
      data += 4;
      size -= 4;
    }
    if (size < payload_len) {
      break;
    }
    if (p_mask) {
      if (*(u32*)p_mask == 0) {
        p_mask = 0;
      }
    }
    /*do mask convert*/
    if (p_mask) {
      do_convert(data, (size_t)payload_len, ctx, p_mask);
    }
    else {
      ctx.packet.append(data, (size_t)payload_len);
    }
    data += (size_t)payload_len;
    size -= (size_t)payload_len;

    pend = data;
    if (fin) {
      pcall(handler, ctx.packet, ctx.opcode, ctx.inflate);
      ctx.packet.clear();
      ctx.opcode  = opcode_type::frame;
      ctx.inflate = false;
    }
  }
  return pend;
}

template <typename Handler>
inline void encode(const char* data, size_t size, context& ctx, Handler&& handler)
{
  const size_t frame_size = 0xffff;
  do {
    unsigned char v1 = (unsigned char)ctx.opcode;
    size_t bytes = size > frame_size ? frame_size : size;
    if (bytes == size) {
      v1 |= 0x80;
    }
    if (ctx.inflate) {
      v1 |= 0x40;
    }
    ctx.packet.append((char*)&v1, 1);

    char p_mask[4];
    unsigned char v2 = 0;
    if (ctx.what == session_type::client) {
      /*set mask*/
      v2 |= 0x80;
    }
    if (bytes < 126) {
      v2 |= (bytes & 0x7f);
    }
    else {
      v2 |= 126;
    }
    ctx.packet.append((char*)&v2, 1);

    /*append mask*/
    if (bytes >= 126) {
      char temp[4] = { 0 };
      ep_encode16(temp, (u16)bytes);
      ctx.packet.append(temp, 2);
    }
    if (v2 & 0x80) {
      unsigned int mask = ctx.mask ? make_mask(data, bytes) : 0;
      ep_encode32(p_mask, mask);
      ctx.packet.append(p_mask, 4);
    }
    /*append size and data*/
    if (bytes > 0) {
      if ((v2 & 0x80) && ctx.mask) {
        do_convert(data, bytes, ctx, p_mask);
      }
      else {
        ctx.packet.append(data, bytes);
      }
      data += bytes;
      size -= bytes;
    }
    pcall(handler, ctx.packet, ctx.opcode, ctx.inflate);
    ctx.packet.clear();
    ctx.opcode  = opcode_type::frame;
    ctx.inflate = false;
  } while (data && size);
}

/*******************************************************************************/
} //end of namespace ws
} //end of namespace io
} //end of namespace eport
/*******************************************************************************/
