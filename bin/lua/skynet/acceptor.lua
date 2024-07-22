

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

local function on_callback(handler, ec, peer, data)
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

local function new_socket(context)
  local protocol = context.protocol;
  local ca       = context.ca;
  local key      = context.key;
  local pwd      = context.pwd;
  return io.socket(protocol, ca, key, pwd);
end

--------------------------------------------------------------------------------

local function on_receive(session, ec, data)
  local handler = session.handler;
  local peer = session.socket;
  if ec > 0 then
    if handler and session.active then
      on_callback(handler, ec, peer, data);
    end
    peer:close();
    return;
  end
  if not session.active then
    session.active = true;
  end
  if handler then
    on_callback(handler, 0, peer, data);
  end
end

--------------------------------------------------------------------------------

local function on_accept(context, ec, peer)
  if context.accept_handler then
    local handler = context.accept_handler;
    local ok, err = pcall(handler, ec, peer);
    if not ok then
      error(err);
    end
  end
  if ec > 0 or not peer:valid() then
    peer:close();
  else
    local handler = context.receive_handler;
    local session = new_session(peer, handler);
    peer:receive(bind(on_receive, session));
  end
  return new_socket(context);
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

function ws_class:on(receive, accept)
  assert(type(receive) == "function");
  assert(accept == nil or type(accept) == "function");
  self.accept_handler  = accept;
  self.receive_handler = receive;
end

--------------------------------------------------------------------------------

function ws_class:listen(port, host, backlog)
  host = host or "0.0.0.0";
  port = port or (self.ca and 443 or 80);
  local ok = self.acceptor:listen(port, host, backlog or 64);
  if not ok then
    return false;
  end
  local socket = new_socket(self);
  self.acceptor:accept(socket, bind(on_accept, self));
  return true;
end

--------------------------------------------------------------------------------

return ws_class;

--------------------------------------------------------------------------------
