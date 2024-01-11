

--------------------------------------------------------------------------------

local class = require("skynet.classy");
local co_class = class("co_class");

--------------------------------------------------------------------------------

local function print_if_error(ok, err)
  if ok ~= true then error(err) end
end

--------------------------------------------------------------------------------

function co_class:__init()
  self.tasklist = std.list();
  self.closed = false;
  self.co = coroutine.create(bind(self.comain, self));
end

--------------------------------------------------------------------------------

function co_class:__gc()
  coroutine.close(self.co);
end

--------------------------------------------------------------------------------

function co_class:close(func)
  assert(func == nil or type(func) == "function");
  self.closed = func;
  if self.tasklist.empty() then
    os.post(coroutine.resume, self.co);
  end
end

--------------------------------------------------------------------------------

function co_class:comain()
  while true do
    if self.tasklist:empty() then
      if not self.closed then
        coroutine.yield();
      else
        break;
      end
    else
      print_if_error(pcall(self.tasklist:front()));
      self.tasklist:pop_front();
    end
  end
  if self.closed then
    os.post(self.closed, self);
  end
end

--------------------------------------------------------------------------------

function co_class:append(task)
  assert(type(task) == "function");
  if not self.closed then
    self.tasklist:push_back(task);
    if self.tasklist:size() == 1 then
      os.post(coroutine.resume, self.co);
    end
  end
end

--------------------------------------------------------------------------------

return co_class;

--------------------------------------------------------------------------------
