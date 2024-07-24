

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local cluster = {};

--------------------------------------------------------------------------------

function cluster.join(host, port)
  if cluster.adapter then
    return false, "repeat joining";
  end
  local ok, adapter = os.pload("cluster.adapter", host, port);
  if ok then
    cluster.adapter = adapter;
    return true;
  end
  return false, adapter;
end

--------------------------------------------------------------------------------

return cluster;

--------------------------------------------------------------------------------
