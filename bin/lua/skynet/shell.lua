

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

local actives = {};
local service = {}

--------------------------------------------------------------------------------

--uri: /service/load?name=xxxx
function service.load(query)
  local name = query.name;
  if type(name) ~= "string" then
    return "name not found";
  end
  local result = {};
  local ok, job = os.pload(name, query);
  if not ok then  
    result.state  = "ERROR";
	result.encode = "base64";
    result.error  = base64.encode(job);
    return json.encode(result);
  end
  if not actives[name] then
    actives[name] = {};
  end
  table.insert(actives[name], job);    
  result.state = "OK";
  result.count = #actives[name];
  return json.encode(result);
end

--------------------------------------------------------------------------------

--uri: /service/stop?name=xxxx&id=xxx
function service.stop(query)
  local name = query.name;
  if type(name) ~= "string" then
    return "name not found";
  end
  local removed = {};
  local id = tonumber(query.id) or 0;
  for i, job in pairs(actives[name] or {}) do
    if id == 0 or id == job:id() then
      job:stop();
	  table.insert(removed, i);
    end
  end
  for _, i in pairs(removed) do
    table.remove(actives[name], i);
  end
  if actives[name] and #actives[name] == 0 then
    actives[name] = nil;
  end
  local result = {};
  result.state = "OK";
  return json.encode(result);
end

--------------------------------------------------------------------------------

--uri: /service/list?name=xxxx
function service.list(query)
  local name = query.name;
  if type(name) ~= "string" then
    return "name not found";
  end
  local result = {
    array = {},
    count = 0;
  };
  for name, item in pairs(actives) do
    result.count = result.count + 1;
    result.array[name] = {};
    for _, job in pairs(item) do
      local state = {};
      state.id = job:id();
      state.state = job:state();
	  table.insert(result.array[name], state);
    end
  end
  result.state = "OK";
  return json.encode(result);
end

--------------------------------------------------------------------------------

--uri: /service/exec?cmd=xxxx
function service.exec(query)
  local command = query.command;
  if type(command) ~= "string" then
    return "command not found";
  end
  local result  = {};
  local ok, err = os.shell(command);
  if not ok then
    result.state = "ERROR";
    result.error = err;
    return json.encode(result);
  end
  result.state  = "OK";
  result.encode = "base64";
  result.output = base64.encode(err);
  return json.encode(result);
end

--------------------------------------------------------------------------------

function service.init()
  os.declare("service:load", service.load);
  os.declare("service:stop", service.stop);
  os.declare("service:list", service.list);
  os.declare("service:exec", service.exec);
end

--------------------------------------------------------------------------------

local function makeurl(url, port)
  return string.format("%s/?type=http&port=%d", url, port);
end

--------------------------------------------------------------------------------

function main(port, httpurl)
  service.init();
  port = port or 80;
  local ok, job = os.pload("http.broker", port);
  if not ok then
    error(job);
	return;
  end
  while not os.stopped() do
    os.wait(10000);
    if httpurl then
      io.wwwget(makeurl(httpurl, port));
    end
  end
  job:stop();
end

--------------------------------------------------------------------------------
