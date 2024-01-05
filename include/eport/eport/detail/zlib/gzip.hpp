

#pragma once

#include <string>
#ifdef EPORT_ZLIB_ENABLE
#define	ZLIB_CONST
#include <zlib.h>
#endif

/*******************************************************************************/
namespace eport {
namespace zlib   {
/*******************************************************************************/

inline void* zmalloc(void *q, unsigned n, unsigned m) {
  (void)q;
  return calloc(n, m);
}

inline void zfree(void *q, void *p) {
  (void)q;
  free(p);
}

#ifdef EPORT_ZLIB_ENABLE

inline bool do_inflate(const char* data, size_t bytes, bool gzip, std::string& out) {
  int ret;
  size_t outputsz = bytes;
  size_t inputsz = bytes;
  unsigned char *output = (unsigned char*)malloc(outputsz);
  const unsigned char *input = (unsigned char*)data;
  z_stream d_stream; /* decompression stream */

  d_stream.zalloc   = zmalloc;
  d_stream.zfree    = zfree;
  d_stream.opaque   = (voidpf)0;
  d_stream.next_in  = input;
  d_stream.avail_in = (uInt)inputsz;

  ret = inflateInit2(&d_stream, gzip ? MAX_WBITS + 16 : -MAX_WBITS);
  if (ret != Z_OK) {
    inflateEnd(&d_stream);
    free(output);
    return false;
  }
  d_stream.next_out = output;
  d_stream.avail_out = (uInt)outputsz;
  for (;d_stream.avail_in;)
  {
    ret = inflate(&d_stream, Z_NO_FLUSH);
    if (ret == Z_STREAM_END)
      break;
    if (ret != Z_OK) {
      inflateEnd(&d_stream);
      free(output);
      return false;
    }
    if (d_stream.avail_out == 0) {
      void* p = realloc(output, outputsz * 2);
      if (!p) {
        free(output);
        return false;
      }
      output = (unsigned char*)p;
      d_stream.avail_out = (uInt)outputsz;
      d_stream.next_out = output + outputsz;
      outputsz *= 2;
    }
  }
  ret = inflateEnd(&d_stream);
  if (ret != Z_OK) {
    free(output);
    return 0;
  }
  out.assign((char*)output, d_stream.total_out);
  free(output);
  return true;
}

inline bool do_deflate(const char* data, size_t bytes, bool gzip, std::string& out) {
  int err;
  size_t inputsz = bytes;
  size_t outsz   = inputsz * 2;
  unsigned char *output = (unsigned char*)malloc(outsz);
  const unsigned char *input = (unsigned char*)data;

  z_stream c_stream; /* compression stream */
  c_stream.zalloc = zmalloc;
  c_stream.zfree  = zfree;
  c_stream.opaque = (voidpf)0;
  err = deflateInit2(&c_stream,
    Z_BEST_SPEED,
    Z_DEFLATED,
    gzip ? MAX_WBITS + 16 : -MAX_WBITS,
    MAX_MEM_LEVEL - 1,
    Z_DEFAULT_STRATEGY);
  if (err != Z_OK) {
    deflateEnd(&c_stream);
    free(output);
    return false;
  }
  c_stream.next_out  = output;
  c_stream.avail_out = (uInt)outsz;
  c_stream.opaque = 0;
  c_stream.next_in   = input;
  c_stream.avail_in  = (uInt)inputsz;

  for (;;) {
    err = deflate(&c_stream, gzip ? Z_NO_FLUSH : Z_SYNC_FLUSH);
    if (err != Z_OK) {
      deflateEnd(&c_stream);
      free(output);
      return false;
    }
    if (c_stream.avail_out) {
      break;
    }
    void* p = realloc(output, outsz * 2);
    if (!p) {
      free(output);
      return false;
    }
    output = (unsigned char*)p;
    c_stream.avail_out = (uInt)outsz;
    c_stream.next_out = output + outsz;
    outsz *= 2;
  }
  for (;;) {
    err = deflate(&c_stream, Z_FINISH);
    if (err == Z_STREAM_END)
      break;
    if (err == Z_OK) {
      void* p = realloc(output, outsz * 2);
      if (!p) {
        free(output);
        return false;
      }
      output = (unsigned char*)p;
      c_stream.avail_out = (uInt)outsz;
      c_stream.next_out = output + outsz;
      outsz *= 2;
    }
    else {
      deflateEnd(&c_stream);
      free(output);
      return false;
    }
  }
  err = deflateEnd(&c_stream);
  if (err != Z_OK) {
    free(output);
    return false;
  }
  uLong rlen = c_stream.total_out;
  if (!gzip) {
    uLong i, j;
    for (i = c_stream.total_out - 4, j = 0; j < 6; i--, j++) {
      unsigned int* p = (unsigned int*)(output + i);
      if (*p == 0xffff0000) {
        rlen = i;
        break;
      }
    }
  }
  out.assign((char*)output, rlen);
  free(output);
  return true;
}

#else

inline bool do_inflate(const char* data, size_t bytes, bool gzip, std::string& out) {
  return false;
}

inline bool do_deflate(const char* data, size_t bytes, bool gzip, std::string& out) {
  return false;
}

#endif

/*******************************************************************************/

inline bool deflate(const char* data, size_t size, std::string& out) {
  return do_deflate(data, size, false, out);
}

inline bool deflate(const std::string& data, std::string& out) {
  return deflate(data.c_str(), data.size(), out);
}

inline bool inflate(const char* data, size_t size, std::string& out) {
  return do_inflate(data, size, false, out);
}

inline bool inflate(const std::string& data, std::string& out) {
  return inflate(data.c_str(), data.size(), out);
}

inline bool gzip(const char* data, size_t size, std::string& out) {
  return do_deflate(data, size, true, out);
}

inline bool gzip(const std::string& data, std::string& out) {
  return gzip(data.c_str(), data.size(), out);
}

inline bool ungzip(const char* data, size_t size, std::string& out) {
  return do_inflate(data, size, true, out);
}

inline bool ungzip(const std::string& data, std::string& out) {
  return ungzip(data.c_str(), data.size(), out);
}

/*******************************************************************************/
} //end of namespace zlib
} //end of namespace eport
/*******************************************************************************/
