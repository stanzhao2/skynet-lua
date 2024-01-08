 **1. global functions** 
```
-   bind(arg1 [,...])
-   pack(arg1 [,...])
-   unpack(str)
-   pcall(f [, arg1, ...])
-   print(fmt [, ...])
-   trace(fmt [, ...])
-   error(fmt [, ...])
-   throw(fmt [, ...])
```


 **2. os functions** 
-   os.deliver(name, mask, receiver [, ...])
-   os.bind(name, f [, cluster])
-   os.unbind(name)
-   os.rpcall(name [, arg1, ...])
-   os.pload(name [, arg1, ...])
-   os.name()
-   os.dirsep()
-   os.timer()
-   os.id()
-   os.post(f [, arg1, ...])
-   os.wait([expires])
-   os.restart()
-   os.exit()
-   os.stop()
-   os.stopped()
-   os.debugging()
-   os.steady_clock()

 **3. io functions** 
-   io.wwwget(url)
-   io.socket(protocol)
-   io.acceptor()

 **4. gzip functions** 
-   gzip.deflate(str)
-   gzip.inflate(str)

 **5. json functions** 
-   json.encode(tab)
-   json.decode(str)

 **6. base64 functions** 
-   base64.encode(str)
-   base64.decode(str)

 **7. crypto functions** 
-   crypto.aes_encrypt(str, key)
-   crypto.aes_decrypt(str, key)
-   crypto.rsa_sign(str, key)
-   crypto.rsa_verify(src, sign, key)
-   crypto.rsa_encrypt(str, key)
-   crypto.rsa_decrypt(str, key)
-   crypto.hash32(str)
-   crypto.sha1(str)
-   crypto.hmac_sha1(str, key)
-   crypto.sha224(str)
-   crypto.hmac_sha224(str, key)
-   crypto.sha256(str)
-   crypto.hmac_sha256(str, key)
-   crypto.sha384(str)
-   crypto.hmac_sha384(str, key)
-   crypto.sha512(str)
-   crypto.hmac_sha512(str, key)
-   crypto.md5(str)
-   crypto.hmac_md5(str, key)
