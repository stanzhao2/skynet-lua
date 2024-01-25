

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local format = string.format;
local type_what = require("cluster.protocol");

local sessions = {};
local def_ws_port<const> = 80;

--------------------------------------------------------------------------------

local function sendto_member(session)
  for peer, v in pairs(sessions) do
    if peer ~= session.socket then
	  local packet = {
	    what = type_what.forword,
		id   = v.socket:id(),
		ip   = v.ip,
		port = v.port,
	  };
	  session.socket:send(pack(packet));
	end
  end
  local peer = session.socket;
  peer:send(pack({what = type_what.ready}));
end

--------------------------------------------------------------------------------

local function ws_on_error(ec, peer, msg)
  local session = sessions[peer];
  if session then
    sessions[peer] = nil;
    peer:close();
  end
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
  local port = peer:getheader("xforword-port");
  if not port then
    return false;
  end

  local host = peer:getheader("xforword-host");
  if not host then
    host = peer:endpoint();
  end

  local session = {
    socket = peer,
	ip     = host,
	port   = port,
  };
  sessions[peer] = session;
  sendto_member(session);
  return true;
end

--------------------------------------------------------------------------------

local function ws_on_accept(protocol, ec, peer)
  if ec > 0 then
    ws_on_error(ec, peer, "accept error");
	return;
  end

  if not new_session(protocol, peer) then
    peer:close();
	return;
  end

  peer:receive(bind(ws_on_receive, peer));
  return io.socket(protocol);
end

--------------------------------------------------------------------------------

function main(host, port)
  port = port or def_ws_port;
  local acceptor = io.acceptor();
  local ok = acceptor:listen(port, host or "0.0.0.0", 16);

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
