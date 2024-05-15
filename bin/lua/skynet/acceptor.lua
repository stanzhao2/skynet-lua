

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local format   = string.format;
local class    = require("skynet.classy");
local ws_class = class("ws_class");

--------------------------------------------------------------------------------

local function ws_callback(handler, ec, peer, data)
  local ok, err = pcall(handler, ec, peer, data);
  if not ok then
    error(err);
  end
end

--------------------------------------------------------------------------------

local function new_session(peer, handler)
  return { socket = peer, handler = handler };
end

--------------------------------------------------------------------------------

local function new_socket(protocol, ca, key, pwd)
  return io.socket(protocol, ca, key, pwd);
end

--------------------------------------------------------------------------------

local function on_ws_receive(session, ec, data)
  local handler = session.handler;
  local peer = session.socket;

  if ec > 0 then
    if session.active then
      ws_callback(handler, ec, peer, data);
    end
    peer:close();
    return;
  end

  if not session.active then
    session.active = true;
  end
  ws_callback(handler, 0, peer, data);
end

--------------------------------------------------------------------------------

local function on_ws_accept(context, handler, ec, peer)
  if ec > 0 then
    peer:close();
  else
    local session = new_session(peer, handler);
    peer:receive(bind(on_ws_receive, session));
  end

  local protocol = context.protocol;
  local ca  = context.ca;
  local key = context.key;
  local pwd = context.pwd;
  return new_socket(protocol, ca, key, pwd);
end

--------------------------------------------------------------------------------

function ws_class:__init(protocol, ca, key, pwd)
  self.protocol = protocol or "tcp";
  self.ca  = ca;
  self.key = key;
  self.pwd = pwd;
  self.acceptor = io.acceptor();
end

--------------------------------------------------------------------------------

function ws_class:close()
  self.acceptor:close();
end

--------------------------------------------------------------------------------

function ws_class:listen(handler, port, host, backlog)
  assert(type(handler) == "function");
  port = port or (self.ca and 443 or 80);

  local ok = self.acceptor:listen(port, host or "0.0.0.0", backlog or 64);
  if not ok then
    return false;
  end

  local protocol = self.protocol;
  local ca       = self.ca;
  local key      = self.key;
  local pwd      = self.pwd;
  local socket   = new_socket(protocol, ca, key, pwd);
  self.acceptor:accept(socket, bind(on_ws_accept, self, handler));
  return true;
end

--------------------------------------------------------------------------------

return ws_class;

--------------------------------------------------------------------------------
