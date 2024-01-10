**global functions**
-   bind(arg1 [,...])
-   pack(arg1 [,...])
-   unpack(str)
-   pcall(f [, arg1, ...])
-   print(fmt [, ...])
-   trace(fmt [, ...])
-   error(fmt [, ...])
-   throw(fmt [, ...])

 **os functions** 
-   os.deliver(name, mask, receiver [, ...])
-   os.caller()
-   os.bind(name, f [, <true/false>])
-   os.unbind(name)
-   os.rpcall([f, ] name [, arg1, ...])
-   os.pload(name [, arg1, ...])
-   os.name()
-   os.timer()
-   os.dirsep()
-   os.mkdir(name)
-   os.opendir([name])
-   os.id()
-   os.post(f [, arg1, ...])
-   os.wait([expires])
-   os.restart()
-   os.exit()
-   os.stop()
-   os.stopped()
-   os.debugging()
-   os.steady_clock()

 **io functions** 
-   io.wwwget(url)
-   io.socket([<tcp/ssl/ws/wss>])
-   io.acceptor()
-   io.http.request_parser(options)
-   io.http.response_parser(options)
-   io.http.parse_url(url)
-   io.http.escape(url)
-   io.http.unescape(url)

 **string functions**
-   string.split(s, seq)

 **gzip functions** 
-   gzip.deflate(str [,<true/false>])
-   gzip.inflate(str [,<true/false>])

 **json functions** 
-   json.encode(tab)
-   json.decode(str)

 **base64 functions** 
-   base64.encode(str)
-   base64.decode(str)

 **crypto functions** 
-   crypto.aes.encrypt(str, key)
-   crypto.aes.decrypt(str, key)
-   crypto.rsa.sign(str, key)
-   crypto.rsa.verify(src, sign, key)
-   crypto.rsa.encrypt(str, key)
-   crypto.rsa.decrypt(str, key)
-   crypto.hash32(str)
-   crypto.sha1(str)
-   crypto.sha224(str)
-   crypto.sha256(str)
-   crypto.sha384(str)
-   crypto.sha512(str)
-   crypto.md5(str)
-   crypto.hmac.sha1(str, key)
-   crypto.hmac.sha224(str, key)
-   crypto.hmac.sha256(str, key)
-   crypto.hmac.sha384(str, key)
-   crypto.hmac.sha512(str, key)
-   crypto.hmac.md5(str, key)
