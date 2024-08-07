

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local format = string.format;
local proto_type = require("cluster.protocol");

local active_sessions = {};
local lua_bounds      = {};
local default_port    = 80;

--------------------------------------------------------------------------------

local function sendto_others(data)
  for id, session in pairs(active_sessions) do
    session.socket:send(data);
  end
end

--------------------------------------------------------------------------------

local function sendto_member(data, session)
  session.socket:send(data);
end

--------------------------------------------------------------------------------

local function lua_bind(info, caller)
  local name = info.name;
  if not lua_bounds[caller] then
    lua_bounds[caller] = {};
  end
  lua_bounds[caller][name] = info;
  trace(format("bind %s by caller(%d)", name, caller));
end

--------------------------------------------------------------------------------

local function lua_unbind(name, caller)
  if caller > 0xffff then    
	os.r_unbind(name, caller);
  end
  lua_bounds[caller][name] = nil;
  trace(format("unbind %s by caller(%d)", name, caller));
end

--------------------------------------------------------------------------------

local function ws_on_error(ec, peer, msg)
  local id = peer:id();
  local session = active_sessions[id];
  if session then
    active_sessions[id] = nil;
    peer:close();
  end

  --cancel bind for the session
  for caller, v in pairs(lua_bounds) do
    if caller > 0xffff and caller & 0xffff == id then
	  for name, info in pairs(v) do
	    lua_unbind(name, caller);
	  end
	  lua_bounds[caller] = nil;
	end
  end
end

--------------------------------------------------------------------------------

local function ws_on_receive(peer, ec, data)
  if ec > 0 then
    ws_on_error(ec, peer, "receive error");
	return;
  end
  
  local id   = peer:id();
  local info = unwrap(data);
  local what = info.what;
  
  if what == proto_type.deliver then
    local name   = info.name;
	local argv   = info.argv;
	local mask   = info.mask;
	local who    = info.who;
	local caller = info.caller << 16 | id;
	local rcf    = info.rcf;
	os.r_deliver(name, argv, mask, who, caller, rcf);
	return;
  end
  
  if what == proto_type.response then
    local data   = info.data;
	local caller = info.caller;
	local rcf    = info.rcf;
	os.r_response(data, caller, rcf);
	return;
  end
  
  if what == proto_type.bind then
    local name   = info.name;
	local rcb    = info.rcb;
	local caller = info.caller << 16 | id;
	lua_bind(info, caller);
	os.r_bind(name, caller, rcb);
	return;
  end
  
  if what == proto_type.unbind then
    local name   = info.name;
	local caller = info.caller << 16 | id;
	lua_unbind(name, caller);
	return;
  end
end

--------------------------------------------------------------------------------

local function on_lookout(info)
  local what = info.what;  
  if what == proto_type.bind then
	lua_bind(info, info.caller);
    sendto_others(wrap(info));
	return;
  end
  
  if what == proto_type.unbind then
	lua_unbind(info.name, info.caller);
    sendto_others(wrap(info));
	return;
  end
  
  local id;
  if what == proto_type.deliver then
    id = info.who & 0xffff;
	info.who = info.who >> 16;
  end
  
  if what == proto_type.response then
    id = info.caller & 0xffff;
	info.caller = info.caller >> 16;
  end
  
  local session = active_sessions[id];
  if session then
    sendto_member(wrap(info), session);
  end
end

--------------------------------------------------------------------------------

local function new_session(protocol, peer)
  local what = peer:getheader("xforword-join");
  if what ~= "skynet-lua" then
    return false;
  end

  local ip, port = peer:endpoint();
  local session = {
    socket = peer,
	ip     = ip,
	port   = port,
  };

  local id = peer:id();
  active_sessions[id] = session;
  peer:receive(bind(ws_on_receive, peer));
  return true;
end

--------------------------------------------------------------------------------

local function peer_exist(host, port)
  for id, session in pairs(active_sessions) do
    if session.ip == host and session.port == port then
	  return true;
	end
  end
  return false;
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

  for caller, bounds in pairs(lua_bounds) do
    if caller <= 0xffff then
	  for name, info in pairs(bounds) do
	    peer:send(wrap(info));
	  end
	end
  end
  return io.socket(protocol);
end

--------------------------------------------------------------------------------

local function new_connect(host, port, protocol)
  if peer_exist(host, port) then
    return true;
  end

  local peer = io.socket(protocol);
  peer:setheader("xforword-join", "skynet-lua");
  if peer:connect(host, port) then
    new_session(protocol, peer);
	return true;
  end
  return false;
end

--------------------------------------------------------------------------------

local function connect_members(socket, protocol)
  while true do
    local data = socket:read();
	if not data then
	  return false;
	end
    
	local packet = unwrap(data);
	if packet.what == proto_type.ready then
	  break;
	end
    
	if packet.what ~= proto_type.forword then
	  return false;
	end
    
	local host = packet.ip;
	local port = packet.port;
	if not new_connect(host, port, protocol) then
      error(format("socket connect to %s:%d error", host, port));
	  return false;
	end
    os.wait(0);
  end
  return true;
end

--------------------------------------------------------------------------------

local function listen_on_local(port, protocol)
  local acceptor = io.acceptor();
  local ok = acceptor:listen(port or 0, "0.0.0.0", 16);
  if not ok then
	return;
  end

  local socket = io.socket(protocol);
  acceptor:accept(socket, bind(ws_on_accept, protocol));
  local host, port = acceptor:endpoint("local");
  return acceptor, port;
end

--------------------------------------------------------------------------------

local function keepalive(socket, ec, data)
  if ec > 0 then
    socket:close();
    error("connection error with leader");
  end
end

--------------------------------------------------------------------------------

function main(host, port)
  local protocol = "ws";
  local acceptor, lport = listen_on_local(0, protocol);
  if not lport then
    error(format("socket listen error at port: %d", port));
    return;
  end
  
  host = host or "127.0.0.1";
  port = port or default_port;
  local socket = io.socket(protocol);
  socket:setheader("xforword-port", lport);
  
  if not socket:connect(host, port) then
    error(format("socket connect to %s:%d error", host, port));
	return;
  end  

  if not connect_members(socket, protocol) then
    return;
  end
  
  os.lookout(on_lookout);
  socket:receive(bind(keepalive, socket));  
  print(format("%s works on port %d", os.name(), lport));
  
  while not os.stopped() do
    os.wait();
  end
  
  os.lookout(nil);
  socket:close();
  acceptor:close();
end

--------------------------------------------------------------------------------
