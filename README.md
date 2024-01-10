 **skynet-lua usage**
-   skynet name [arguments...]
-   skynet cluster.leader [port] [host]

 **global functions**
-   bind(func, [,...])
-   pack(...)
-   unpack(str)
-   pcall(func [, ...])
-   print(fmt [, ...])
-   trace(fmt [, ...])
-   error(fmt [, ...])
-   throw(fmt [, ...])

 **os functions** 
-   os.deliver(name, mask, receiver [, ...])
-   os.caller()
-   os.bind(name, func [, <true/false>])
-   os.unbind(name)
-   os.rpcall([func, ] name [, ...])
-   os.pload(name [, ...]) *
-   os.name()
-   os.timer() *
-   os.dirsep()
-   os.mkdir(name)
-   os.opendir([name])
-   os.id()
-   os.post(func [, ...])
-   os.wait([expires])
-   os.restart()
-   os.exit()
-   os.stop()
-   os.stopped()
-   os.debugging()
-   os.steady_clock()

 **job functions**
-   job:free()
-   job:id()

 **io functions** 
-   io.wwwget(url)
-   io.socket([<tcp/ssl/ws/wss>], [ca], [key], [pwd]]) *
-   io.acceptor() *
-   io.http.request_parser(options)
-   io.http.response_parser(options)
-   io.http.parse_url(url)
-   io.http.escape(url)
-   io.http.unescape(url)

 **socket functions**
-   socket:connect(host, port [, func])
-   socket:id()
-   socket:read()
-   socket:write(data)
-   socket:send(data)
-   socket:receive(func)
-   socket:endpoint()
-   socket:close()
-   socket:geturi()
-   socket:getheader(name)
-   socket:seturi(uri)
-   socket:setheader(name, value)

 **acceptor functions**
-   acceptor:listen(port [, host, backlog])
-   acceptor:id()
-   acceptor:accept(s [, func])
-   acceptor:close();

 **timer functions**
-   timer:expires(ms, func)
-   timer:close()
-   timer:cancel()

 **string functions**
-   string.split(s, seq)
-   string.isalpha(s)
-   string.isalnum(s)

 **storage functions**
-   storage.exist(key)
-   storage.set(key, value [, ...])
-   storage.set_if(key, func, value [, ...])
-   storage.get(key)
-   storage.erase(key)
-   storage.size()
-   storage.clear()

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

The function ending in (*) returns the userdata object