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

function Conf:getNodeValue(path)
    local node = self:getNode(path);
    if type(node) == "table" then
	return (node:value());
    end
    return (nil);
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

function Conf:getNodeValueSafe(path, default)
	node = self:getNode(path);
	if not node then
		if default then
			return default;
		else
			return ("");
		end
	end
	return node:value();
end

--      node = { _name = <Element Name>,
--              _type = ROOT|ELEMENT|TEXT|COMMENT|PI|DECL|DTD,
--              _attr = { Node attributes - see callback API },
--              _parent = <Parent Node>
--              _children = { List of child nodes - ROOT/NODE only }
--            }

function createNode(_name, _type, _attr, _children, _parent)
    -- TODO: check for "^(ROOT|ELEMENT|TEXT|COMMENT|PI|DECL|DTD)$"
    if not _name or not _type then
	print(_name or "(no name)", _type or "(no type)");

	return (nil);
    end

    local t = {};
    t._name = _name;
    t._type = _type;
    t._attr = _attr or {};
    t._parent = _parent;
    t._children = _children or {};
    return (t);
end

function attachNode(parent, node)
    if not parent or not node then
	return (nil);
    end

    if not parent._children then
	parent._children = {};
    end

    table.insert(parent._children, node);
    node._parent = parent;

    return (node);
end

function Conf:getOrCreateNode(path)
    local node = self:getNode(path);
    if node then
	return (node);
    end

    node = self.tree;
    for part in string.gmatch(path, "([^\.]+)%.?") do
	local rmatch, _, idname, id = string.find(part, "^([%w%_%-]+)%[(%d+)%]$");
	if rmatch then
	    part = idname;
	end

	local matched = 0;
	if type(node) == "table" and type(node._children) == "table" then
    	    for _, v in ipairs(node._children) do

    		if v._type == "ELEMENT" and v._name == part then
		    matched = matched + 1;

		    if id then
			if id == matched then
			    node = v;
			    break;
			end
		    else
			node = v;
			break;
		    end
    		end
    	    end
    	end

	if matched == 0 or (id and matched < tonumber(id)) then
	    local new = createNode(part, "ELEMENT");
	    attachNode(node, new);
	    node = new;
	end
    end
    return (Node:new(node, path));
end

function Conf:get(path)
    print(tostring(self.tree), path);
    return (path);
end

mt.__index = Conf;
-----------------------------------
-------- End of Conf Class --------
-----------------------------------
