

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

---------------------------------------------------------------------------------

local original = pairs;

---------------------------------------------------------------------------------

local function lt(a, b)
  if type(a) == "number" and type(b) == "number" then
    if a < b then return true
    else return false end
  end
  if tostring(a) < tostring(b) then 
    return true
  end
end

---------------------------------------------------------------------------------

local function gt(a, b)
  if type(a) == "number" and type(b) == "number" then
    if a > b then return true
    else return false end
  end
  if tostring(a) > tostring(b) then 
    return true
  end
end

---------------------------------------------------------------------------------

local function __pairs(t, f)
  if f == nil then
    return original(t);
  end
  local a = {}
  for k in original(t) do table.insert(a, k) end
  if type(f) == "string" then
    if f == "<" then
      f = lt
    elseif f == ">" then
      f = gt
    end
  end
  if type(f) ~= "function" then
    assert("invalid option");
  end
  table.sort(a, f)
  local i = 0         -- iterator variable
  local iter = function ()  -- iterator function
     i = i + 1
     if a[i] == nil then return nil
     else return a[i], t[a[i]]
     end
  end
  return iter
end

---------------------------------------------------------------------------------

return __pairs;

---------------------------------------------------------------------------------
