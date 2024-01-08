 **1. global functions** 
-   bind(arg1 [,...])
-   pack(arg1 [,...])
-   unpack(str)
-   pcall(f [, arg1, ...])
-   print(fmt [, ...])
-   trace(fmt [, ...])
-   error(fmt [, ...])
-   throw(fmt [, ...])

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
-   crypto.aes_encrypt
-   crypto.aes_decrypt
-   crypto.rsa_sign
-   crypto.rsa_verify
-   crypto.rsa_encrypt
-   crypto.rsa_decrypt
-   crypto.hash32
-   crypto.sha1
-   crypto.hmac_sha1
-   crypto.sha224
-   crypto.hmac_sha224
-   crypto.sha256
-   crypto.hmac_sha256
-   crypto.sha384
-   crypto.hmac_sha384
-   crypto.sha512
-   crypto.hmac_sha512
-   crypto.md5
-   crypto.hmac_md5
