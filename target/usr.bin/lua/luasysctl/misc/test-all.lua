#!/usr/bin/env lua

--[[
--  usage: sysctl -Na | lua misc/test-all.lua
--
--  note that some value can't be returned (like "S,proc" struct proc).
]]
require('sysctl')

function show()
    local sctl, stype = sysctl.get(key)
    if (type(sctl) == 'table') then
        local k,v
        print("{")
        for k,v in pairs(sctl) do
            print("  " .. k .. "=" .. v)
        end
        print("}\t" .. stype)
    else
        print(sctl .. "\t" .. stype)
    end
end



key = io.read()
while key do
    print(key)
    ok, err = pcall(show);
    if (not ok) then
        print("*** FAILED ***")
        print(err);
    end
    print("------")
    key = io.read()
end
