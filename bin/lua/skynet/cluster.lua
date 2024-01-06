

--------------------------------------------------------------------------------

local format = string.format;
local type_what = require("skynet.protocol");

local sessions   = {};
local lua_bounds = {};
local def_ws_port<const> = 80;

--------------------------------------------------------------------------------

local function sendto_others(data)
  for id, session in pairs(sessions) do
    session.socket:send(data);
  end
end

--------------------------------------------------------------------------------

local function sendto_member(data, session)
  session.socket:send(data);
end

--------------------------------------------------------------------------------

local function ws_on_error(ec, peer, msg)
  local id = peer:id();
  local session = sessions[id];
  if session then
    sessions[id] = nil;
    peer:close();
  end
  --cancel bind for the session
  for caller, v in pairs(lua_bounds) do
    if caller & 0xffff == id then
	  for name, info in pairs(v) do
	    os.r_unbind(name, caller);
        trace(format("unbind %s by caller(%d)", name, caller));
	  end
	  lua_bounds[caller] = nil;
	end
  end
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

local function lua_unbind(info, caller)
  local name = info.name;
  lua_bounds[caller][name] = nil;
  trace(format("unbind %s by caller(%d)", name, caller));
end

--------------------------------------------------------------------------------

local function ws_on_receive(peer, ec, data)
  if ec > 0 then
    ws_on_error(ec, peer, "receive error");
	return;
  end
  
  local id   = peer:id();
  local info = unpack(data);
  local what = info.what;
  
  if what == type_what.deliver then
    local name   = info.name;
	local argv   = info.argv;
	local mask   = info.mask;
	local who    = info.who;
	local caller = info.caller << 16 | id;
	local rcf    = info.rcf;
	os.r_deliver(name, argv, mask, who, caller, rcf);
	return;
  end
  
  if what == type_what.response then
    local data   = info.data;
	local caller = info.caller;
	local rcf    = info.rcf;
	os.r_response(data, caller, rcf);
	return;
  end
  
  if what == type_what.bind then
    local name   = info.name;
	local rcb    = info.rcb;
	local caller = info.caller << 16 | id;
	lua_bind(info, caller);
	os.r_bind(name, caller, rcb);
	return;
  end
  
  if what == type_what.unbind then
    local name   = info.name;
	local caller = info.caller << 16 | id;
	lua_unbind(info, caller);
	os.r_unbind(name, caller);
	return;
  end
end

--------------------------------------------------------------------------------

local function on_lookout(info)
  local what = info.what;  
  if what == type_what.bind then
	lua_bind(info, info.caller);
    sendto_others(pack(info));
	return;
  end
  
  if what == type_what.unbind then
	lua_unbind(info, info.caller);
    sendto_others(pack(info));
	return;
  end
  
  local id;
  if what == type_what.deliver then
    id = info.who & 0xffff;
	info.who = info.who >> 16;
  end
  
  if what == type_what.response then
    id = info.caller & 0xffff;
	info.caller = info.caller >> 16;
  end
  
  local session = sessions[id];
  if session then
    sendto_member(pack(info), session);
  end
end

--------------------------------------------------------------------------------

local function new_session(protocol, peer)
  local session = {
    socket = peer,
  };
  local id = peer:id();
  sessions[id] = session;
  peer:receive(bind(ws_on_receive, peer));
end

--------------------------------------------------------------------------------

local function ws_on_accept(protocol, ec, peer)
  if ec > 0 then
    ws_on_error(ec, peer, "accept error");
	return;
  end
  new_session(protocol, peer);
  for caller, bounds in pairs(lua_bounds) do
    if caller <= 0xffff then
	  for name, info in pairs(bounds) do
	    peer:send(pack(info));
	  end
	end
  end
  return io.socket(protocol);
end

--------------------------------------------------------------------------------

local function new_connect(host, port, protocol)
  local peer = io.socket(protocol);
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
	local packet = unpack(data);
	if packet.what == type_what.ready then
	  break;
	end
	if packet.what ~= type_what.forword then
	  return false;
	end
	local host = packet.ip;
	local port = packet.port;
	if not new_connect(host, port, protocol) then
      error(format("socket connect to %s:%d error", host, port));
	  return false;
	end
  end
  return true;
end

--------------------------------------------------------------------------------

local function create_listen(port, protocol)
  local acceptor = io.acceptor();
  local ok = acceptor:listen(port or 0, "0.0.0.0", 16);
  if not ok then
	return;
  end
  local socket = io.socket(protocol);
  acceptor:accept(socket, bind(ws_on_accept, protocol));
  return acceptor:endpoint("local");
end

--------------------------------------------------------------------------------

function main(host, port)
  local protocol = "ws";
  local _, lport = create_listen(0, protocol);
  if not lport then
    error(format("socket listen error at port: %d", port));
    return;
  end
  
  local socket = io.socket(protocol);
  host = host or "127.0.0.1";
  port = port or def_ws_port;
  socket:setheader("xforword-port", lport);  
  
  if not socket:connect(host, port) then
    error(format("socket connect to %s:%d error", host, port));
	return;
  end
  
  if not connect_members(socket, protocol) then
    return;
  end
  socket:receive(function() end);
  
  os.lookout(on_lookout);
  print(format("%s works on port %d", os.name(), lport));
  while not os.stopped() do
    os.wait(60000);
	collectgarbage();
  end
  socket:close();
end

--------------------------------------------------------------------------------
