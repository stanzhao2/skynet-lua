

--------------------------------------------------------------------------------

local format = string.format;
local insert = table.insert;
local concat = table.concat;

--------------------------------------------------------------------------------

local http_status_text = {
  [100] = "Continue",
  [101] = "Switching Protocols",
  [200] = "OK",
  [201] = "Created",
  [202] = "Accepted",
  [203] = "Non-Authoritative Information",
  [204] = "No Content",
  [205] = "Reset Content",
  [206] = "Partial Content",
  [300] = "Multiple Choices",
  [301] = "Moved Permanently",
  [302] = "Found",
  [303] = "See Other",
  [304] = "Not Modified",
  [305] = "Use Proxy",
  [307] = "Temporary Redirect",
  [400] = "Bad Request",
  [401] = "Unauthorized",
  [402] = "Payment Required",
  [403] = "Forbidden",
  [404] = "File Not Found",
  [405] = "Method Not Allowed",
  [406] = "Not Acceptable",
  [407] = "Proxy Authentication Required",
  [408] = "Request Time-out",
  [409] = "Conflict",
  [410] = "Gone",
  [411] = "Length Required",
  [412] = "Precondition Failed",
  [413] = "Request Entity Too Large",
  [414] = "Request-URI Too Large",
  [415] = "Unsupported Media Type",
  [416] = "Requested range not satisfiable",
  [417] = "Expectation Failed",
  [500] = "Internal Server Error",
  [501] = "Not Implemented",
  [502] = "Bad Gateway",
  [503] = "Service Unavailable",
  [504] = "Gateway Time-out",
  [505] = "HTTP Version not supported",
};

--------------------------------------------------------------------------------

local http_mime_type = {
    cod   = "image/cis-cod",
    ras   = "image/cmu-raster",
    fif   = "image/fif",
    gif   = "image/gif",
    ief   = "image/ief",
    jpeg  = "image/jpeg",
    jpg   = "image/jpeg",
    jpe   = "image/jpeg",
    png   = "image/png",
    tiff  = "image/tiff",
    tif   = "image/tiff",
    mcf   = "image/vasa",
    wbmp  = "image/vnd.wap.wbmp",
    fh4   = "image/x-freehand",
    fh5   = "image/x-freehand",
    fhc   = "image/x-freehand",
    pnm   = "image/x-portable-anymap",
    pbm   = "image/x-portable-bitmap",
    pgm   = "image/x-portable-graymap",
    ppm   = "image/x-portable-pixmap",
    rgb   = "image/x-rgb",
    ico   = "image/x-icon",
    xwd   = "image/x-windowdump",
    xbm   = "image/x-xbitmap",
    xpm   = "image/x-xpixmap",
    js    = "text/javascript",
    css   = "text/css",
    htm   = "text/html",
    html  = "text/html",
    shtml = "text/html",
    txt   = "text/plain",
    pdf   = "application/pdf",
    xml   = "application/xml",
    json  = "application/json",
};

--------------------------------------------------------------------------------

local function index_default()
  return "skynet-lua/3.0.1";
end

--------------------------------------------------------------------------------

local function http_response(peer, code, body)
  local status  = format("HTTP/1.1 %d %s\r\n", code, http_status_text[code]);
  local headers = {
    ["Server"]         = index_default(),
    ["Connection"]     = "keep-alive",
    ["Cache-Control"]  = "max-age=0",
    ["Connection"]     = "keep-alive",
    ["Content-Length"] = format("%d", #(body or "")),
    ["Content-Type"]   = http_mime_type.html .. ";charset=UTF-8",
  };
  if code ~= 200 then
    headers["Connection"] = "Close";
  end
  local response = {};
  insert(response, status);
  for k, v in pairs(headers) do
    insert(response, k);
    insert(response, ": ");
    insert(response, v);
    insert(response, "\r\n");
  end
  insert(response, "\r\n");
  insert(response, body or "");
  peer:send(concat(response));
end

--------------------------------------------------------------------------------

local function co_on_request(method, session)
  local url     = session.url;
  local body    = concat(session.body);
  local context = io.http.parse_url(url);
  local fname   = context.path:sub(2);
  if not fname or #fname == 0 then
    fname = "index";
  end
  local query   = context.query;
  if query and #query > 0 then
    local t = string.split(query, "&");
    query = {};
    for k, v in pairs(t) do
      local s = string.split(v, "=");
      query[s[1]] = s[2];
    end
  end
  local status = 200;
  local ok, result = os.rpcall(fname, query, body);
  if not ok then
    status = 500;
  end
  http_response(session.socket, status, result);
end

--------------------------------------------------------------------------------

local function rp_on_request(session)
  local peer   = session.socket;
  local co     = session.co;
  local parser = session.parser;
  local method = parser:method();
  if method ~= "GET" and method ~= "POST" then
    http_response(session.socket, 405, "Method Not Allowed");
    return;
  end
  if not coroutine.resume(co, method, session) then
    peer:close();
  end
end

--------------------------------------------------------------------------------

local function http_on_request(session)
  local ok, err = pcall(rp_on_request, session);
  if not ok then
    error(err);
    local peer = session.socket;
    peer:close();
  end
end

--------------------------------------------------------------------------------

local function http_on_receive(session, ec, data)
  local peer = session.socket;
  if ec > 0 then
    peer:close();
    coroutine.close(session.co);
    return;
  end
  local parser = session.parser;
  while #data > 0 do
    local parsed = parser:execute(data);
    if parser:error() > 0 then
      peer:close();
      return;
    end
    if parsed == #data then
      break;
    end
    data = data:sub(parsed + 1);
  end
end

--------------------------------------------------------------------------------

local function http_on_url(session, url)
  session.url = url;
end

--------------------------------------------------------------------------------

local function http_on_body(session, data)
  insert(session.body, data);
end

--------------------------------------------------------------------------------

local function http_on_begin(session)
  session.url  = "";
  session.body = {};
end

--------------------------------------------------------------------------------

local function http_on_accept(protocol, ca, key, pwd, ec, peer)
  if ec > 0 then
    peer:close();
	return;
  end
  local session = {
    socket = peer,
    url  = "",
    body = {},
    co = coroutine.create(co_on_request)
  };
  local options = {
    on_message_begin    = bind(http_on_begin,   session),
    on_body             = bind(http_on_body,    session),
    on_url              = bind(http_on_url,     session),
    on_message_complete = bind(http_on_request, session)
  };
  session.parser = io.http.request_parser(options);
  peer:receive(bind(http_on_receive, session));
  if not ca then
    return io.socket(protocol);
  end
  return io.socket(protocol, ca, key, pwd);
end

--------------------------------------------------------------------------------

function main(port, host, ca, key, pwd)
  port = port or (ca and 443 or 80);
  local acceptor = io.acceptor();
  local ok = acceptor:listen(port, host or "0.0.0.0", 64);
  if not ok then
    error(format("socket listen error at port: %d", port));
	return;
  end

  local protocol = "tcp";
  if ca and key then
    protocol = "ssl";
  end
  local socket = io.socket(protocol, ca, key, pwd);
  acceptor:accept(socket, bind(http_on_accept, protocol, ca, key, pwd));
  
  os.bind("index", index_default);
  print(format("%s works on port %d", os.name(), port));
  while not os.stopped() do
    os.wait();
  end
  acceptor:close();
end

--------------------------------------------------------------------------------
