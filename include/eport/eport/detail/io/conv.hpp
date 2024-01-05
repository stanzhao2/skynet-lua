
#pragma once

#include <locale>
#include <string>
#include <memory>

/***********************************************************************************/
namespace eport {
/***********************************************************************************/

inline bool is_utf8(const char* p, size_t n = 0)
{
  unsigned char chr;
  size_t i = 0, follow = 0;
  for (; n ? (i < n) : (p[i] != 0); i++) {
    chr = p[i];
    if (follow == 0) {
      if (chr > 0x80) {
        if (chr == 0xfc || chr == 0xfd) {
          follow = 5;
        }
        else if (chr >= 0xf8) {
          follow = 4;
        }
        else if (chr >= 0xf0) {
          follow = 3;
        }
        else if (chr >= 0xe0) {
          follow = 2;
        }
        else if (chr >= 0xc0) {
          follow = 1;
        }
        else {
          return false;
        }
      }
    } else {
      follow--;
      if ((chr & 0xC0) != 0x80) {
        return false;
      }
    }
  }
  return (follow == 0);
}

/***********************************************************************************/

inline std::string wcs_to_mbs(const std::wstring& wstr, const char* locale = 0)
{
  setlocale(LC_CTYPE, locale ? locale : "");
  std::string ret;
  std::mbstate_t state = {};
  const wchar_t* src = wstr.c_str();
  size_t len = std::wcsrtombs(nullptr, &src, 0, &state);
  if (static_cast<size_t>(-1) != len)
  {
    std::unique_ptr<char[]> buff(new char[len + 1]);
    len = std::wcsrtombs(buff.get(), &src, len, &state);
    if (static_cast<size_t>(-1) != len) {
      ret.assign(buff.get(), len);
    }
  }
  setlocale(LC_CTYPE, "C");
  return ret;
}

/***********************************************************************************/

inline std::wstring mbs_to_wcs(const std::string& str, const char* locale = 0)
{
  setlocale(LC_CTYPE, locale ? locale : "");
  std::wstring ret;
  std::mbstate_t state = {};
  const char* src = str.c_str();
  size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
  if (static_cast<size_t>(-1) != len)
  {
    std::unique_ptr<wchar_t[]> buff(new wchar_t[len + 1]);
    len = std::mbsrtowcs(buff.get(), &src, len, &state);
    if (static_cast<size_t>(-1) != len) {
      ret.assign(buff.get(), len);
    }
  }
  setlocale(LC_CTYPE, "C");
  return ret;
}

/***********************************************************************************/

inline std::wstring utf8_to_wcs(const std::string& str)
{
  std::wstring wstr;
  unsigned char chr;
  wchar_t wchr = 0;
  wchar_t* pwb = new wchar_t[str.size() + 1];
  wchar_t* pwc = pwb;
  const char* p = str.c_str();
  if (!pwb) {
    return wstr;
  }
  size_t follow = 0;
  for (size_t i = 0; p[i] != 0; i++) {
    chr = p[i];
    if (follow == 0) {
      wchr = 0;
      if (chr > 0x80) {
        if (chr == 0xfc || chr == 0xfd) {
          follow = 5;
          wchr = chr & 0x01;
        }
        else if (chr >= 0xf8) {
          follow = 4;
          wchr = chr & 0x03;
        }
        else if (chr >= 0xf0) {
          follow = 3;
          wchr = chr & 0x07;
        }
        else if (chr >= 0xe0) {
          follow = 2;
          wchr = chr & 0x0f;
        }
        else if (chr >= 0xc0) {
          follow = 1;
          wchr = chr & 0x1f;
        }
        else {
          follow = -1;
          break;
        }
      }
      else {
        *pwc++ = chr;
      }
    }
    else {
      if ((chr & 0xC0) != 0x80) {
        return L"";
      }
      wchr = (wchr << 6) | (chr & 0x3f);
      if (--follow == 0) {
        *pwc++ = wchr;
      }
    }
  }
  if (follow == 0) {
    wstr.append(pwb, pwc - pwb);
  }
  delete[] pwb;
  return std::move(wstr);
}

/***********************************************************************************/

inline std::string wcs_to_utf8(const std::wstring& wstr)
{
  typedef unsigned char uchr;
  typedef unsigned int  wchr;
  if (wstr.empty()) {
    return std::string();
  }
  std::string str;
  size_t wlen = wstr.size();
  uchr* obuff = new uchr[wlen * 3 + 1];
  if (!obuff) {
    return std::string();
  }
  uchr* t = obuff;
  wchar_t* us = (wchar_t*)wstr.c_str();
  size_t lwchr = sizeof(wchar_t);
  wchr unmax = (lwchr >= 4 ? 0x110000 : 0x10000);
  for (size_t i = 0; i < wlen; i++) {
    wchr w = us[i];
    if (w <= 0x7f) {
      *t++ = (uchr)w;
      continue;
    }
    if (w <= 0x7ff) {
      *t++ = 0xc0 | (uchr)(w >> 6);
      *t++ = 0x80 | (uchr)(w & 0x3f);
      continue;
    }
    if (w <= 0xffff) {
      *t++ = 0xe0 | (uchr)(w >> 12);
      *t++ = 0x80 | (uchr)((w >> 6) & 0x3f);
      *t++ = 0x80 | (uchr)(w & 0x3f);
      continue;
    }
    if (w < unmax) {
      *t++ = 0xf0 | (uchr)((w >> 18) & 0x07);
      *t++ = 0x80 | (uchr)((w >> 12) & 0x3f);
      *t++ = 0x80 | (uchr)((w >> 6) & 0x3f);
      *t++ = 0x80 | (uchr)(w & 0x3f);
      continue;
    }
  }
  str.append((char*)obuff, t - obuff);
  delete[]obuff;
  return std::move(str);
}

/***********************************************************************************/

inline std::string mbs_to_utf8(const std::string& str)
{
  std::wstring wstr = mbs_to_wcs(str);
  return wcs_to_utf8(wstr);
}

/***********************************************************************************/

inline std::string utf8_to_mbs(const std::string& str)
{
  std::wstring wstr = utf8_to_wcs(str);
  return wcs_to_mbs(wstr);
}

/***********************************************************************************/

//全角转半角
inline std::string sbc_to_dbc(const char* s)
{
  std::string output;
  for (; *s; ++s) {
    if ((*s & 0xff) == 0xa1 && (*(s + 1) & 0xff) == 0xa1) { //全角空格
      output += 0x20;
      ++s;
    }
    else if ((*s & 0xff) == 0xa3 && (*(s + 1) & 0xff) >= 0xa1 && (*(s + 1) & 0xff) <= 0xfe) { //ascii码中其它可显示字符
      output += (*++s - 0x80);
    }
    else {
      if (*s < 0) //如果是中文字符，则拷贝两个字节
        output += (*s++);
      output += (*s);
    }
  }
  return std::move(output);
}

/***********************************************************************************/

//全角转半角
inline std::string sbc_to_dbc(const std::string& str)
{ 
  return sbc_to_dbc(str.c_str());
}

/***********************************************************************************/

//半角转全角
inline std::string dbc_to_sbc(const char* s)
{
  std::string output;
  for (; *s; ++s) {
    if ((*s & 0xff) == 0x20) { //半角空格
      output += (char)0xa1;
      output += (char)0xa1;
    }
    else if ((*s & 0xff) >= 0x21 && (*s & 0xff) <= 0x7e) {
      output += (char)0xa3;
      output += (*s + (char)0x80);
    }
    else {
      if (*s < 0)    //如果是中文字符，则拷贝两个字节
        output += (*s++);
      output += (*s);
    }
  }
  return std::move(output);
}

/***********************************************************************************/

//半角转全角
inline std::string dbc_to_sbc(const std::string& str)
{ 
  return dbc_to_sbc(str.c_str());
}

/***********************************************************************************/

inline std::string utf8_to_dbc(const char* s, char placeholder = 0)
{
  std::string output;
  for (size_t j = 0; s[j] != 0; j++)
  {
    if (((s[j] & 0xf0) ^ 0xe0) != 0) {
      output += s[j];
      continue;
    }
    int c = (s[j] & 0xf) << 12 | ((s[j + 1] & 0x3f) << 6 | (s[j + 2] & 0x3f));
    if (c == 0x3000) { // blank
      if (placeholder) output += placeholder;
      output += ' ';
    }
    else if (c >= 0xff01 && c <= 0xff5e) { // full char
      if (placeholder) output += placeholder;
      char new_char = c - 0xfee0;
      output += new_char;
    }
    else { // other 3 bytes char
      output += s[j];
      output += s[j + 1];
      output += s[j + 2];
    }
    j = j + 2;
  }
  return std::move(output);
}

/***********************************************************************************/
} //end of namespace eport
/***********************************************************************************/
