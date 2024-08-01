

/***********************************************************************************/

#ifndef __CIRCULAR_BUFFER_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdlib.h>
#include <string.h>

#define cbminx(a, b) ((a) < (b) ? (a) : (b))
#define cbused(w, r) ((w) - (r))
#define cbfree(w, r) (_size - cbused(w, r))
#define is_power_of_2(n) (n && ((n & (n - 1)) == 0))

class circular_buffer final {
  circular_buffer(const circular_buffer&);
  unsigned char *_buff;
  size_t _size, _wi, _ri, _limit;

public:
  virtual ~circular_buffer() {
    free(_buff);
  }
  inline circular_buffer(size_t size = 1024)
    : _buff(0)
    , _size(roundup_power_of_2(size))
    , _wi(0)
    , _ri(0)
    , _limit(0) {
    _buff = realloc(_size);
  }
  inline void limit(size_t nmax) {
    _limit = roundup_power_of_2(nmax);
  }
  inline size_t read(char* buff, size_t size) {
    return _read(buff, size);
  }
  inline size_t write(const void* buff, size_t size) {
    return _write((char*)buff, size);
  }
  inline size_t size() const {
    return _size;
  }
  inline size_t used() const {
    return cbused(_wi, _ri);
  }
  inline bool empty() const {
    return cbused(_wi, _ri) == 0;
  }

private:
  static size_t roundup_power_of_2(size_t n) {
    if (is_power_of_2(n)) {
      return n;
    }
    size_t maxv = ~0;
    size_t andv = ~(maxv & (maxv >> 1));
    while ((andv & n) == 0) andv = andv >> 1;
    return andv << 1;
  }
  size_t _read(char* buff, size_t size) {
    size_t nmax  = cbminx(size, cbused(_wi, _ri));
    size_t ridx  = _ri & (_size - 1);
    size_t first = cbminx(nmax, _size - ridx);
    memcpy(buff, _buff + ridx, first);
    memcpy(buff + first, _buff, nmax - first);
    return (_ri += nmax), nmax;
  }
  size_t _write(const char* data, size_t size) {
    size_t nmax = cbminx(size, cbfree(_wi, _ri));
    if (_limit && nmax < size) {
      size_t n = cbused(_wi, _ri);
      if (resize(n, size)) {
        return _write(data, size);
      }
    }
    size_t widx = _wi & (_size - 1);
    size_t first = cbminx(nmax, _size - widx);
    memcpy(_buff + widx, data, first);
    memcpy(_buff, data + first, nmax - first);
    return (_wi += nmax), nmax;
  }
  bool resize(size_t original, size_t growing) {
    size_t nsize = original + growing;
    nsize = roundup_power_of_2(nsize);
    if (nsize > _limit) {
      return false;
    }
    unsigned char* nbuff = realloc(nsize);
    if (!nbuff) {
      return false;
    }
    _wi &= (_size - 1);
    _ri &= (_size - 1);
    if (original > 0) { //not empty
      if (_wi <= _ri) {
        memcpy(nbuff + _size, nbuff, _wi);
        _wi = _wi + _size;
      }
    }
    _buff = nbuff;
    _size = nsize;
    return true;
  }
  unsigned char* realloc(size_t nsize) {
    return (unsigned char*)::realloc(_buff, nsize);
  }
};

/***********************************************************************************/

#endif //__CIRCULAR_BUFFER_H_

/***********************************************************************************/
