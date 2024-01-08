

#include <memory.h>
#include "luaf_crypto.h"

#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>

/***********************************************************************************/

static unsigned int hash32(const void* key, size_t len) {
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
    len -= 4;
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

static bool luaL_getboolean(lua_State* L, int index) {
  return lua_toboolean(L, index) ? true : false;
}

static bool luaL_optboolean(lua_State* L, int index, bool def = false) {
  return lua_isnoneornil(L, index) ? def : luaL_getboolean(L, index);
}

static std::string to_string(const std::string& data) {
  std::string ret;
  char buf[32];
  unsigned char* p = (unsigned char*)data.c_str();
  for (size_t i = 0; i < data.size(); i++) {
    snprintf(buf, sizeof(buf), "%02x", p[i]);
    ret.append(buf, 2);
  }
  return ret;
}

static int hmac_hash(lua_State* L, const EVP_MD* evp) {
  size_t len, klen;
  const char* cs  = luaL_checklstring(L, 1, &len);
  const char* key = luaL_checklstring(L, 2, &klen);
  bool raw = luaL_optboolean(L, 3, false);
  unsigned char bs[EVP_MAX_MD_SIZE];

  unsigned int n;
  HMAC(evp, key, (int)klen, (unsigned char*)cs, (int)len, bs, &n);
  if (raw) {
    lua_pushlstring(L, (char*)bs, n);
    return 1;
  }
  unsigned int hexn = n * 2 + 2, i;
  char* dst = (char*)malloc(hexn);
  for (i = 0; i < n; i++) {
    snprintf(dst + i * 2, hexn, "%02x", bs[i]);
  }
  lua_pushlstring(L, dst, hexn);
  free(dst);
  return 1;
}

static int hash_crc32(lua_State* L) {
  size_t size;
  const char* data = luaL_checklstring(L, 1, &size);
  auto hash = hash32(data, size);
  lua_pushinteger(L, hash);
  return 1;
}

static int base64_encode(lua_State* L) {
  size_t len;
  const char* bs = luaL_checklstring(L, 1, &len);
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO* bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  BIO_write(bio, bs, (int)len);
  BIO_flush(bio);
  BUF_MEM* p;
  BIO_get_mem_ptr(bio, &p);
  int n = (int)p->length;
  char* dst = (char*)malloc(n + 1);
  memcpy(dst, p->data, n);
  BIO_free_all(bio);

  lua_pushlstring(L, dst, n);
  free(dst);
  return 1;
}

static int base64_decode(lua_State* L) {
  size_t len;
  const char* cs = luaL_checklstring(L, 1, &len);
  size_t dn = len & 3;

  std::string tmp;
  if (dn > 0) {
    tmp.assign(cs, len);
    tmp.append("===", dn);
    cs  = tmp.c_str();
    len = tmp.size();
  }

  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO* bio = BIO_new_mem_buf((void*)cs, (int)len);
  bio = BIO_push(b64, bio);
  char* dst = (char*)malloc(len);
  int n = BIO_read(bio, dst, (int)len);
  BIO_free_all(bio);

  lua_pushlstring(L, dst, n);
  free(dst);
  return 1;
}

static int hash_md5(lua_State* L) {
  size_t size = 0;
  const char* data = luaL_checklstring(L, 1, &size);
  bool raw = luaL_optboolean(L, 2, false);
  unsigned char hash[MD5_DIGEST_LENGTH];
  MD5_CTX md5c;
  MD5_Init(&md5c);
  MD5_Update(&md5c, data, size);
  MD5_Final(hash, &md5c);
  if (raw) {
    lua_pushlstring(L, (char*)hash, sizeof(hash));
    return 1;
  }
  char hex[3];
  std::string result;
  for (size_t i = 0; i < sizeof(hash); i++) {
    sprintf(hex, "%02x", hash[i]);
    result += hex;
  }
  lua_pushlstring(L, result.c_str(), result.size());
  return 1;
}

static int hash_hmac_md5(lua_State* L) {
  return hmac_hash(L, EVP_md5());
}

static int hash_sha1(lua_State* L) {
  size_t size = 0;
  const char* data = luaL_checklstring(L, 1, &size);
  bool raw = luaL_optboolean(L, 2, false);
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA_CTX sha1;
  SHA1_Init(&sha1);
  SHA1_Update(&sha1, data, size);
  SHA1_Final(hash, &sha1);
  if (raw) {
    lua_pushlstring(L, (char*)hash, sizeof(hash));
    return 1;
  }
  char hex[3];
  std::string result;
  for (size_t i = 0; i < sizeof(hash); i++) {
    sprintf(hex, "%02x", hash[i]);
    result += hex;
  }
  lua_pushlstring(L, result.c_str(), result.size());
  return 1;
}

static int hash_hmac_sha1(lua_State* L) {
  return hmac_hash(L, EVP_sha1());
}

static int hash_sha224(lua_State* L) {
  size_t size = 0;
  const char* data = luaL_checklstring(L, 1, &size);
  bool raw = luaL_optboolean(L, 2, false);
  unsigned char hash[SHA224_DIGEST_LENGTH];
  SHA256_CTX sha1;
  SHA224_Init(&sha1);
  SHA224_Update(&sha1, data, size);
  SHA224_Final(hash, &sha1);
  if (raw) {
    lua_pushlstring(L, (char*)hash, sizeof(hash));
    return 1;
  }
  char hex[3];
  std::string result;
  for (size_t i = 0; i < sizeof(hash); i++) {
    sprintf(hex, "%02x", hash[i]);
    result += hex;
  }
  lua_pushlstring(L, result.c_str(), result.size());
  return 1;
}

static int hash_hmac_sha224(lua_State* L) {
  return hmac_hash(L, EVP_sha224());
}

static int hash_sha256(lua_State* L) {
  size_t size = 0;
  const char* data = luaL_checklstring(L, 1, &size);
  bool raw = luaL_optboolean(L, 2, false);
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, data, size);
  SHA256_Final(hash, &sha256);
  if (raw) {
    lua_pushlstring(L, (char*)hash, sizeof(hash));
    return 1;
  }
  char hex[3];
  std::string result;
  for (size_t i = 0; i < sizeof(hash); i++) {
    sprintf(hex, "%02x", hash[i]);
    result += hex;
  }
  lua_pushlstring(L, result.c_str(), result.size());
  return 1;
}

static int hash_hmac_sha256(lua_State* L) {
  return hmac_hash(L, EVP_sha256());
}

static int hash_sha384(lua_State* L) {
  size_t size = 0;
  const char* data = luaL_checklstring(L, 1, &size);
  bool raw = luaL_optboolean(L, 2, false);
  unsigned char hash[SHA384_DIGEST_LENGTH];
  SHA512_CTX sha1;
  SHA384_Init(&sha1);
  SHA384_Update(&sha1, data, size);
  SHA384_Final(hash, &sha1);
  if (raw) {
    lua_pushlstring(L, (char*)hash, sizeof(hash));
    return 1;
  }
  char hex[3];
  std::string result;
  for (size_t i = 0; i < sizeof(hash); i++) {
    sprintf(hex, "%02x", hash[i]);
    result += hex;
  }
  lua_pushlstring(L, result.c_str(), result.size());
  return 1;
}

static int hash_hmac_sha384(lua_State* L) {
  return hmac_hash(L, EVP_sha384());
}

static int hash_sha512(lua_State* L) {
  size_t size = 0;
  const char* data = luaL_checklstring(L, 1, &size);
  bool raw = luaL_optboolean(L, 2, false);
  unsigned char hash[SHA512_DIGEST_LENGTH];
  SHA512_CTX sha1;
  SHA512_Init(&sha1);
  SHA512_Update(&sha1, data, size);
  SHA512_Final(hash, &sha1);
  if (raw) {
    lua_pushlstring(L, (char*)hash, sizeof(hash));
    return 1;
  }
  char hex[3];
  std::string result;
  for (size_t i = 0; i < sizeof(hash); i++) {
    sprintf(hex, "%02x", hash[i]);
    result += hex;
  }
  lua_pushlstring(L, result.c_str(), result.size());
  return 1;
}

static int hash_hmac_sha512(lua_State* L) {
  return hmac_hash(L, EVP_sha512());
}

static int aes_encrypt(lua_State* L) {
  size_t len;
  const char* src = luaL_checklstring(L, 1, &len);
  const char* key = luaL_checkstring(L, 2);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_init(ctx);

  int ret = EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, (unsigned char*)key, NULL);
  if (ret != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return luaL_error(L, "EVP encrypt init error");
  }

  int dstn = (int)len + 256, n, wn;
  char* dst = (char*)malloc(dstn);
  memset(dst, 0, dstn);

  ret = EVP_EncryptUpdate(ctx, (unsigned char*)dst, &wn, (unsigned char*)src, (int)len);
  if (ret != 1)
  {
    free(dst);
    EVP_CIPHER_CTX_free(ctx);
    return luaL_error(L, "EVP encrypt update error");
  }
  n = wn;

  ret = EVP_EncryptFinal_ex(ctx, (unsigned char*)(dst + n), &wn);
  if (ret != 1)
  {
    free(dst);
    EVP_CIPHER_CTX_free(ctx);
    return luaL_error(L, "EVP encrypt final error");
  }
  EVP_CIPHER_CTX_free(ctx);
  n += wn;

  lua_pushlstring(L, dst, n);
  free(dst);
  return 1;
}

static int aes_decrypt(lua_State* L) {
  size_t len;
  const char* src = luaL_checklstring(L, 1, &len);
  const char* key = luaL_checkstring(L, 2);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_init(ctx);

  int ret = EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, (unsigned char*)key, NULL);
  if (ret != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return luaL_error(L, "EVP decrypt init error");
  }

  int n, wn;
  char* dst = (char*)malloc(len);
  memset(dst, 0, len);

  ret = EVP_DecryptUpdate(ctx, (unsigned char*)dst, &wn, (unsigned char*)src, (int)len);
  if (ret != 1)
  {
    free(dst);
    EVP_CIPHER_CTX_free(ctx);
    return luaL_error(L, "EVP decrypt update error");
  }
  n = wn;

  ret = EVP_DecryptFinal_ex(ctx, (unsigned char*)(dst + n), &wn);
  if (ret != 1)
  {
    free(dst);
    EVP_CIPHER_CTX_free(ctx);
    return luaL_error(L, "EVP decrypt final error");
  }
  EVP_CIPHER_CTX_free(ctx);
  n += wn;

  lua_pushlstring(L, dst, n);
  free(dst);
  return 1;
}

/*
** type=1£ºPEM   (-----BEGIN RSA PUBLIC KEY----)
** type=2£ºPKCS8 (-----BEGIN PUBLIC KEY-----)
*/
static RSA* rsa_create(lua_State* L, const char* key, bool pubkey) {
  BIO* bio = BIO_new_mem_buf((void*)key, -1);
  if (bio == nullptr) {
    return nullptr;
  }
  RSA* rsa = nullptr;
  if (pubkey) {
    std::string pem(key);
    auto pos = pem.find("BEGIN RSA PUBLIC KEY");
    if (pos != std::string::npos) {
      rsa = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);
    }
    else {
      rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
    }
  }
  else {
    rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
  }
  BIO_free_all(bio);
  return rsa;
}

static int rsa_public_encrypt(lua_State* L) {
  size_t len;
  const char* src = luaL_checklstring(L, 1, &len);
  const char* key = luaL_checkstring(L, 2);
  RSA* rsa = rsa_create(L, key, true);
  if (rsa == NULL) {
    return luaL_error(L, "RSA read public key error");
  }
  int n = RSA_size(rsa);
  char* dst = (char*)malloc(n);
  memset(dst, 0, n);

  int ret = RSA_public_encrypt((int)len, (unsigned char*)src, (unsigned char*)dst, rsa, RSA_PKCS1_PADDING);
  if (ret != n)
  {
    free(dst);
    RSA_free(rsa);
    return luaL_error(L, "RSA public encrypt error");
  }
  RSA_free(rsa);
  lua_pushlstring(L, dst, n);
  free(dst);
  return 1;
}

static int rsa_private_decrypt(lua_State* L) {
  const char* src = luaL_checkstring(L, 1);
  const char* key = luaL_checkstring(L, 2);
  RSA* rsa = rsa_create(L, key, false);
  if (rsa == NULL) {
    return luaL_error(L, "RSA read private key error");
  }
  int n = RSA_size(rsa);
  char* dst = (char*)malloc(n);
  memset(dst, 0, n);

  int ret = RSA_private_decrypt(n, (unsigned char*)src, (unsigned char*)dst, rsa, RSA_PKCS1_PADDING);
  if (ret <= 0)
  {
    free(dst);
    RSA_free(rsa);
    return luaL_error(L, "RSA private decrypt error");
  }
  RSA_free(rsa);
  lua_pushlstring(L, dst, ret);
  free(dst);
  return 1;
}

static int rsa_private_encrypt(lua_State* L) {
  size_t len;
  const char* src = luaL_checklstring(L, 1, &len);
  const char* key = luaL_checkstring(L, 2);
  RSA* rsa = rsa_create(L, key, false);
  if (rsa == NULL) {
    return luaL_error(L, "RSA read private key error");
  }
  int n = RSA_size(rsa);
  char* dst = (char*)malloc(n);
  memset(dst, 0, n);

  int ret = RSA_private_encrypt((int)len, (unsigned char*)src, (unsigned char*)dst, rsa, RSA_PKCS1_PADDING);
  if (ret != n)
  {
    free(dst);
    RSA_free(rsa);
    return luaL_error(L, "RSA private encrypt error");
  }
  RSA_free(rsa);
  lua_pushlstring(L, dst, n);
  free(dst);
  return 1;
}

static int rsa_public_decrypt(lua_State* L) {
  const char* src = luaL_checkstring(L, 1);
  const char* key = luaL_checkstring(L, 2);
  RSA* rsa = rsa_create(L, key, true);
  if (rsa == NULL) {
    return luaL_error(L, "RSA read private key error");
  }
  int n = RSA_size(rsa);
  char* dst = (char*)malloc(n);
  memset(dst, 0, n);

  int ret = RSA_public_decrypt(n, (unsigned char*)src, (unsigned char*)dst, rsa, RSA_PKCS1_PADDING);
  if (ret <= 0)
  {
    free(dst);
    RSA_free(rsa);
    return luaL_error(L, "RSA public decrypt error");
  }
  RSA_free(rsa);
  lua_pushlstring(L, dst, ret);
  free(dst);
  return 1;
}

static bool init_sha1_hash(const char* data, size_t size, unsigned char* sha) {
  SHA_CTX c;
  if (SHA1_Init(&c) != 1) {
    OPENSSL_cleanse(&c, sizeof(c));
    return false;
  }
  if (SHA1_Update(&c, data, size) != 1) {
    OPENSSL_cleanse(&c, sizeof(c));
    return false;
  }
  if (SHA1_Final(sha, &c) != 1) {
    OPENSSL_cleanse(&c, sizeof(c));
    return false;
  }
  OPENSSL_cleanse(&c, sizeof(c));
  return true;
}

static int rsa_encrypt(lua_State* L) {
  const char* key = luaL_checkstring(L, 2);
  if (strstr(key, "PRIVATE KEY")) {
    return rsa_private_encrypt(L);
  }
  if (strstr(key, "PUBLIC KEY")) {
    return rsa_public_encrypt(L);
  }
  return luaL_error(L, "RSA encrypt error: key type invalid");
}

static int rsa_decrypt(lua_State* L) {
  const char* key = luaL_checkstring(L, 2);
  if (strstr(key, "PRIVATE KEY")) {
    return rsa_private_decrypt(L);
  }
  if (strstr(key, "PUBLIC KEY")) {
    return rsa_public_decrypt(L);
  }
  return luaL_error(L, "RSA decrypt error: key type invalid");
}

static int rsa_sign(lua_State* L) {
  size_t len;
  const char* src = luaL_checklstring(L, 1, &len);
  const char* key = luaL_checkstring(L, 2);

  unsigned char sha[SHA_DIGEST_LENGTH];
  memset(sha, 0, SHA_DIGEST_LENGTH);
  init_sha1_hash(src, len, sha);

  RSA* rsa = rsa_create(L, key, false);
  if (rsa == NULL) {
    return luaL_error(L, "RSA read private key error");
  }
  int n = RSA_size(rsa), wn;
  char* dst = (char*)malloc(n);
  memset(dst, 0, n);

  int ret = RSA_sign(NID_sha1, (unsigned char*)sha, SHA_DIGEST_LENGTH, (unsigned char*)dst, (unsigned int*)&wn, rsa);
  if (ret != 1) {
    free(dst);
    RSA_free(rsa);
    return luaL_error(L, "RSA sign error");
  }
  RSA_free(rsa);
  lua_pushlstring(L, dst, wn);
  free(dst);
  return 1;
}

static int rsa_verify(lua_State* L) {
  size_t srclen, signlen;
  const char* src = luaL_checklstring(L, 1, &srclen);
  const char* sign = luaL_checklstring(L, 2, &signlen);
  const char* key = luaL_checkstring(L, 3);

  unsigned char sha[SHA_DIGEST_LENGTH];
  memset(sha, 0, SHA_DIGEST_LENGTH);
  init_sha1_hash(src, srclen, sha);

  RSA* rsa = rsa_create(L, key, true);
  if (rsa == NULL) {
    return luaL_error(L, "RSA read public key error");
  }
  int ret = RSA_verify(NID_sha1, sha, SHA_DIGEST_LENGTH, (unsigned char*)sign, (unsigned int)signlen, rsa);
  RSA_free(rsa);
  lua_pushboolean(L, ret);
  return 1;
}

/***********************************************************************************/

LUALIB_API int luaC_open_base64(lua_State* L) {
  luaL_checkversion(L);
  lua_getglobal(L, LUA_GNAME);
  lua_newtable(L);
  const luaL_Reg methods[] = {
    { "encode",       base64_encode     },
    { "decode",       base64_decode     },
    { NULL,		        NULL              }
  };
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "base64");
  lua_pop(L, 1);
  return 0;
}

/***********************************************************************************/

LUALIB_API int luaC_open_crypto(lua_State* L) {
  lua_getglobal(L, LUA_GNAME);
  lua_newtable(L);
  const luaL_Reg methods[] = {
    { "aes_encrypt",  aes_encrypt       },
    { "aes_decrypt",  aes_decrypt       },
    { "rsa_sign"   ,  rsa_sign          },
    { "rsa_verify" ,  rsa_verify        },
    { "rsa_encrypt",  rsa_encrypt       },
    { "rsa_decrypt",  rsa_decrypt       },
    { "hash32",       hash_crc32        },
    { "sha1",         hash_sha1         },
    { "hmac_sha1",    hash_hmac_sha1    },
    { "sha224",       hash_sha224       },
    { "hmac_sha224",  hash_hmac_sha224  },
    { "sha256",       hash_sha256       },
    { "hmac_sha256",  hash_hmac_sha256  },
    { "sha384",       hash_sha384       },
    { "hmac_sha384",  hash_hmac_sha384  },
    { "sha512",       hash_sha512       },
    { "hmac_sha512",  hash_hmac_sha512  },
    { "md5",          hash_md5          },
    { "hmac_md5",     hash_hmac_md5     },
    { NULL,		        NULL              }
  };
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "crypto");
  lua_pop(L, 1);
  return 0;
}

/***********************************************************************************/
