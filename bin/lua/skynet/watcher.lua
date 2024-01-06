
--skynet watcher service

--------------------------------------------------------------------------------

local format = string.format;

local sessions = {};

local def_ws_port<const> = 80;

--------------------------------------------------------------------------------

local function sendto_others(data, session)
  for peer, v in pairs(sessions) do
    if peer ~= session.socket then
	  peer:send(data);
	end
  end
end

--------------------------------------------------------------------------------

local function member_change(what, session)
  local packet = {
    what   = what,
	id     = session.socket:id(),
	ip     = session.ip,
	port   = session.port,
  };
  sendto_others(pack(packet), session);
end

--------------------------------------------------------------------------------

local function ws_on_error(ec, peer, msg)
  local session = sessions[peer];
  if not session then
    return;
  end
  member_change("leave", session);
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
  peer:send(data);
end

--------------------------------------------------------------------------------

local function new_session(protocol, peer)
  local _, raddr = peer:getheader("cluster-addr");
  local _, rport = peer:getheader("cluster-port");
  local session = {
    socket = peer,
	ip     = raddr,
	port   = rport,
  };
  local _, ip, port = peer:endpoint();
  if not session.ip then
    session.ip = ip;
  end
  if not session.port then
    session.port = def_ws_port;
  end
  sessions[peer] = session;
  peer:receive(bind(ws_on_receive, peer));
  member_change("join", session);
  print(format("%s accept(%s:%d)", protocol, session.ip, session.port));
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

function main(port, host)
  port = port or def_ws_port;
  local acceptor = io.acceptor();
  local ok = acceptor:listen(port, host, 16);
  if not ok then
    error(format("socket listen error at port: %d", port));
	return;
  end
  
  local protocol = "ws";
  local socket = io.socket(protocol);
  acceptor:accept(socket, bind(ws_on_accept, protocol));
  
  print(format("%s works on port %d", os.name(), port));
  while not os.stopped() do
    os.wait(60000);
	collectgarbage();
  end
  acceptor:close();
end

--------------------------------------------------------------------------------
