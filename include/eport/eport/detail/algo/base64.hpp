

#pragma once

////////////////////////////////////////////////////////////////////////////////
const static char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
////////////////////////////////////////////////////////////////////////////////
namespace eport {
namespace base64 {
////////////////////////////////////////////////////////////////////////////////

inline char* encode(const unsigned char *data, char *out, int len) {
  int i, j;
  unsigned char current;
  for (i = 0, j = 0; i < len; i += 3) {
    current = (data[i] >> 2);
    current &= (unsigned char)0x3F;
    out[j++] = B64[(int)current];

    current = ((unsigned char)(data[i] << 4)) & ((unsigned char)0x30);
    if (i + 1 >= len) {
        out[j++] = B64[(int)current];
        out[j++] = '=';
        out[j++] = '=';
        break;
    }
    current |= ((unsigned char)(data[i + 1] >> 4)) & ((unsigned char)0x0F);
    out[j++] = B64[(int)current];
    current = ((unsigned char)(data[i + 1] << 2)) & ((unsigned char)0x3C);
    if (i + 2 >= len) {
        out[j++] = B64[(int)current];
        out[j++] = '=';
        break;
    }
    current |= ((unsigned char)(data[i + 2] >> 6)) & ((unsigned char)0x03);
    out[j++] = B64[(int)current];
    current = ((unsigned char)data[i + 2]) & ((unsigned char)0x3F);
    out[j++] = B64[(int)current];
  }
  out[j] = '\0';
  return out;
}

////////////////////////////////////////////////////////////////////////////////

inline int decode(const char *out, unsigned char *data) {
  int i, j;
  unsigned char k;
  unsigned char temp[4];
  for (i = 0, j = 0; out[i] != '\0'; i += 4) {
    memset(temp, 0xFF, sizeof(temp));
    for (k = 0; k < 64; k++) {
      if (B64[k] == out[i])
          temp[0] = k;
    }
    for (k = 0; k < 64; k++) {
      if (B64[k] == out[i + 1])
          temp[1] = k;
    }
    for (k = 0; k < 64; k++) {
      if (B64[k] == out[i + 2])
          temp[2] = k;
    }
    for (k = 0; k < 64; k++) {
      if (B64[k] == out[i + 3])
          temp[3] = k;
    }
    data[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2)) & 0xFC)) | ((unsigned char)((unsigned char)(temp[1] >> 4) & 0x03));
    if (out[i + 2] == '=') {
      break;
    }
    data[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4)) & 0xF0)) | ((unsigned char)((unsigned char)(temp[2] >> 2) & 0x0F));
    if (out[i + 3] == '=') {
      break;
    }
    data[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6)) & 0xF0)) | ((unsigned char)(temp[3] & 0x3F));
  }
  return j;
}

////////////////////////////////////////////////////////////////////////////////
} // end of namespace base64
} // end of namespace eport
////////////////////////////////////////////////////////////////////////////////
