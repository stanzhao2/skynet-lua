

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local format = string.format;
local proto_type = require("cluster.protocol");

local default_port = 80;
local active_sessions = {};

--------------------------------------------------------------------------------

local function sendto_member(session)
  for peer, v in pairs(active_sessions) do
    if peer ~= session.socket then
	  local packet = {
	    what = proto_type.forword,
		id   = v.socket:id(),
		ip   = v.ip,
		port = v.port,
	  };
	  session.socket:send(wrap(packet));
	end
  end
  local peer = session.socket;
  peer:send(wrap({what = proto_type.ready}));
end

--------------------------------------------------------------------------------

local function ws_on_error(ec, peer, msg)
  local session = active_sessions[peer];
  if session then
    active_sessions[peer] = nil;
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
  active_sessions[peer] = session;
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

function main(port, host)
  port = port or default_port;
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
    os.wait();
  end
  acceptor:close();
end

--------------------------------------------------------------------------------
