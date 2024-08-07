 **skynet-lua usage**
-   skynet name [arguments...]

 **skynet cluster**
-   skynet cluster.leader [port] [host]

 **skynet http broker**
-   skynet http.broker [port] [host] [ca] [key] [pwd] 

 **skynet shell**
-   skynet skynet.shell [port] [callback]

 **global functions**
-   bind(func, [, ...])
-   pcall(func [, ...])
-   print(fmt [, ...])
-   trace(fmt [, ...])
-   error(fmt [, ...])
-   throw(fmt [, ...])
-   wrap(...)
-   unwrap(s)
-   unwrap_rest(s [, offset])
-   unwrap_one(s [, offset])
-   unwrap_limit(s, n [, offset])

 **os functions** 
-   os.version()
-   os.pload(name [, ...]) #1
-   os.declare(name, func [, <true/false>])
-   os.undeclare(name)
-   os.rpcall([func, ] name [, ...])
-   os.deliver(name, mask, receiver [, ...])
-   os.caller()
-   os.compile(fname [, oname])
-   os.name()
-   os.timer([name]) #2
-   os.dirsep()
-   os.mkdir(name)
-   os.opendir([name])
-   os.processors()
-   os.memory()
-   os.id()
-   os.post(func [, ...])
-   os.wait([expires])
-   os.restart()
-   os.exit()
-   os.stop()
-   os.stopped()
-   os.debugging()
-   os.snapshot()

 **std functions**
-   std.list() #3

 **list functions**
-   list:empty()
-   list:size()
-   list:clear()
-   list:erase(iter)
-   list:front()
-   list:back()
-   list:reverse()
-   list:push_back(v)
-   list:pop_back()
-   list:push_front(v)
-   list:pop_front()

 **job functions**
-   job:memory()
-   job:stop()
-   job:id()
-   job:state()

 **io functions** 
-   io.wwwget(url)
-   io.socket([<tcp/ssl/ws/wss>], [ca], [key], [pwd]]) #4
-   io.acceptor() #5
-   io.http.request_parser(options)
-   io.http.response_parser(options)
-   io.http.parse_url(url)
-   io.http.escape(url)
-   io.http.unescape(url)

 **socket functions**
-   socket:connect(host, port [, func])
-   socket:valid()
-   socket:close()
-   socket:id()
-   socket:read()
-   socket:write(data)
-   socket:send(data)
-   socket:receive(func)
-   socket:endpoint([<"local"/"remote">])
-   socket:geturi()
-   socket:getheader(name)
-   socket:seturi(uri)
-   socket:setheader(name, value)

 **acceptor functions**
-   acceptor:listen(port [, host, backlog])
-   acceptor:id()
-   acceptor:close();
-   acceptor:endpoint()
-   acceptor:accept(s [, func])

 **timer functions**
-   timer:expires(ms, func)
-   timer:close()
-   timer:cancel()

 **string functions**
-   string.split(s, seq)
-   string.trim(s)
-   string.icmp(a, b)
-   string.isalpha(s)
-   string.isalnum(s)

 **storage functions**
-   storage.exist(key)
-   storage.set(key, value [, ...])
-   storage.set_if(key, func, value [, ...])
-   storage.get(key)
-   storage.erase(key)
-   storage.empty()
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

 **usage remarks**
-  _#1: return job object_
-  _#2: return timer object_
-  _#3: return list object_
-  _#4: return socket object_
-  _#5: return acceptor object_
