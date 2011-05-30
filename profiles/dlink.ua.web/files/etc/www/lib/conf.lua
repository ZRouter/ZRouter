-----------------------------------
----------- Conf Class ------------
-----------------------------------
Conf = {};
local mt = {};

function Conf:new(s)
    return setmetatable({ tree = s or '' }, mt)
end

function Conf:gettree()
    return (self.tree);
end

function Conf:getNode(path)
    local function getNodeL(node, lpath)
        local match, name, rpath, id = nil;
	local matched = 0;

	if not node then return (nil) end
	if (not lpath) or (string.len(lpath) == 0) then
	    return (node);
	end

	match, _, name, _, rpath = string.find(lpath, "^([%w%_%-%[%]]+)(%.*)(%S*)$");
	if not match then
	    print("Wrong lpath " .. lpath);
	    return (nil);
	end

	rmatch, _, idname, id = string.find(name, "^([%w%_%-]+)%[(%d+)%]$");
	if rmatch then
	    name = idname;
	end

	if not name then
	    print("Empty name");
	    return (nil);
	end

	if node._children then
    	    for _, v in ipairs(node._children) do
    		if v._type == "ELEMENT" and v._name == name then
    		    matched = matched + 1;
    		    if (not id or (matched == tonumber(id))) then
    			if string.len(rpath) > 0 then
    			    return getNodeL(v, rpath);
    			end
    			return (v);
    		    end
    		end
    	    end
    	end
	return (nil);
    end

    local ret = getNodeL(self.tree, path);
    return (Node:new(ret, path));
end

function Conf:get(path)
    print(tostring(self.tree), path);
    return (path);
end

mt.__index = Conf;
-----------------------------------
-------- End of Conf Class --------
-----------------------------------
