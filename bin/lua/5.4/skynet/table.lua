

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

---------------------------------------------------------------------------------

local pairs = require("skynet.pairs");
local __table = table;
local insert  = table.insert;
local concat  = table.concat;

---------------------------------------------------------------------------------

local function depth_pairs(t, r, des)
  for k, v in pairs(t , des) do
    if type(v) == "table" then
      depth_pairs(v, r, des);
    else
      insert(r, tostring(k) .. tostring(v));
    end
  end
end

---------------------------------------------------------------------------------
---克隆 table
---@param object table
---@param deep boolean
---@retrun table

__table.clone = function(object)
  local lookup_table = {}
  local function _copy(object)
    if type(object) ~= "table" then
      return object;
    elseif lookup_table[object] then
      return lookup_table[object];
    end
    
    local new_table = {};
    lookup_table[object] = new_table;
    
    for key, value in pairs(object) do
      new_table[_copy(key)] = _copy(value);
    end
    return setmetatable(new_table, getmetatable(object));
  end
  return _copy(object);
end

---------------------------------------------------------------------------------
---创建只读 table
---@param it table
---@retrun table

local travelled = setmetatable({}, {__mode="k"});
__table.read_only = function(it)
  if not DEBUG then
    return it;
  end
  local function __read_only(tbl) 
    if not travelled[tbl] then
      local tbl_mt = getmetatable(tbl) 
      if not tbl_mt then 
        tbl_mt = {} 
        setmetatable(tbl, tbl_mt) 
      end
      local proxy = tbl_mt.__read_only_proxy 
      if not proxy then 
        proxy = {} 
        tbl_mt.__read_only_proxy = proxy 
        local proxy_mt = {
          __index = tbl, 
          __newindex  = function (t, k, v) throw("error write to a read-only table with key = " .. tostring(k)) end, 
          __pairs   = function (t) return pairs(tbl) end, 
          __len     = function (t) return #tbl end, 
          __read_only_proxy = proxy 
        } 
        setmetatable(proxy, proxy_mt) 
      end 
      travelled[tbl] = proxy 
      for k, v in pairs(tbl) do 
        if type(v) == "table" then 
          tbl[k] = __read_only(v) 
        end 
      end 
    end
    return travelled[tbl] 
  end
  return __read_only(it);
end

---------------------------------------------------------------------------------
---对table数据进行等于比较
---@param t1 table
---@param t2 table
---@return boolean

__table.equal = function(t1, t2)
  assert(type(t1) == "table")
  assert(type(t2) == "table")
  local r1, r2 = {}, {};
  depth_pairs(t1, r1, false);
  depth_pairs(t2, r2, false);
  return (concat(r1) == concat(r2));
end

---------------------------------------------------------------------------------
---对table数据进行签名
---@param data table
---@param key  string 密钥
---@param algo function(data:string, key:string)
---@return string

__table.sign = function(data, key, algo, des)
  assert(type(data) == "table")
  assert(type(key)  == "string")
  assert(type(algo) == "function")
  assert(des == nil or type(des)  == "boolean");
  
  if des == nil then
    des = false;
  end
  
  local result = {};
  depth_pairs(data, result, des);
  return algo(concat(result), key);
end

---------------------------------------------------------------------------------
---对table数据进行签名验证
---@param data table
---@param key  string 密钥
---@param sign string
---@param algo function(data:string, sign:string, key:string)
---@return string

__table.verify = function(data, key, sign, algo, des)
  assert(type(sign) == "string")
  assert(type(data) == "table")
  assert(type(key)  == "string")
  assert(type(algo) == "function")
  assert(des == nil or type(des)  == "boolean");
  
  if des == nil then
    des = false;
  end
  
  local result = {};
  depth_pairs(data, result, des);
  return algo(concat(result), sign, key);
end

---------------------------------------------------------------------------------

return __table;

---------------------------------------------------------------------------------
