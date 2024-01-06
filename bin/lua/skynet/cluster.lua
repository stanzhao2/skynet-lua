

--------------------------------------------------------------------------------

local format = string.format;

local sessions = {};

local def_ws_port<const> = 80;

--------------------------------------------------------------------------------

local function ws_on_error(ec, peer, msg)
  local session = sessions[peer];
  if not session then
    return;
  end
  sessions[peer] = nil;
  peer:close();
  error(format("error(%d): %s", ec, msg));
end

--------------------------------------------------------------------------------

local function ws_on_receive(peer, ec, data)
  if ec > 0 then
    ws_on_error(ec, peer, "receive error");
	return;
  end
end

--------------------------------------------------------------------------------

local function new_session(protocol, peer)
  local _, ip, port = peer:endpoint();
  local session = {
    socket = peer,
  };
  sessions[peer] = session;
  peer:receive(bind(ws_on_receive, peer));
  print(format("%s accept(%s:%d)", protocol, ip, port));
end

--------------------------------------------------------------------------------

local function ws_on_accept(protocol, ec, peer)
  if ec > 0 then
    ws_on_error(ec, peer, "accept error");
	return;
  end
  new_session(protocol, peer);
  return io.socket(protocol);
end

--------------------------------------------------------------------------------

local function start_listen(port, protocol)
  port = port or 0;
  local acceptor = io.acceptor();
  local ok = acceptor:listen(port, nil, 16);
  if not ok then
	return false;
  end
  local socket = io.socket(protocol);
  acceptor:accept(socket, bind(ws_on_accept, protocol));
  return acceptor:endpoint("local");
end

--------------------------------------------------------------------------------

local function new_connect(host, port, protocol)
  local peer = io.socket(protocol);
  local ok = peer:connect(host, port);
  if ok then
    new_session(protocol, peer);
  end
  return ok;
end

--------------------------------------------------------------------------------

local function connect_others(socket, protocol)
  while true do
    local ok, data = socket:read();
	if not ok then
	  return false;
	end
	local packet = unpack(data);
	if packet.what == "ready" then
	  break;
	end
	if packet.what ~= "member" then
	  return false;
	end
	local host = packet.ip;
	local port = packet.port;
	ok   = new_connect(host, port, protocol);
	if not ok then
      error(format("socket connect to %s:%d error", host, port));
	  return false;
	end
  end
  return true;
end

--------------------------------------------------------------------------------

function main(host, port)
  local protocol = "ws";
  local ok, _, lport = start_listen(0, protocol);
  if not ok then
    error(format("socket listen error at port: %d", port));
    return;
  end
  
  local socket = io.socket(protocol);
  host = host or "127.0.0.1";
  port = port or def_ws_port;
  socket:setheader("cluster-port", lport);  
  
  if not socket:connect(host, port) then
    error(format("socket connect to %s:%d error", host, port));
	return;
  end
  if not connect_others(socket, protocol) then
    return;
  end
  socket:receive(bind(ws_on_receive, socket));
  
  print(format("%s works on port %d", os.name(), lport));
  while not os.stopped() do
    os.wait(60000);
	collectgarbage();
  end
  socket:close();
end
