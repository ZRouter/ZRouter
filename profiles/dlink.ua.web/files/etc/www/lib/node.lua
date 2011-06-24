-----------------------------------
----------- Node Class ------------
-----------------------------------
Node = {};
mtNode = {};

function Node:new(s, path)

    if type(s) ~= "table" then return (nil); end
    local node = setmetatable({ node = s or '' }, mtNode);
    node.path = path;
    return (node);
end

function Node:dump(f, v)
    if self.node then
	if (not f) or f == "txt" then
	    if v then print("TEXT dump of \"" .. self.path .. "\"") end
	    return tdump(self.node);
	elseif f == "xml" then
	    if v then print("XML dump of \"" .. self.path .. "\"") end
	    return xmldump(self.node);
	else
	    print("Unknow format " .. f);
	end
    end
end

function Node:attr(attr, value)
    if self.node._attr and type(self.node._attr) == "table" then
	if value then
	    self.node._attr[attr] = value;
	    return (true);
	else
	    return (self.node._attr[attr] or "");
	end
    end
    return (nil);
end

function Node:value(_value)
    if self.node._children and 
	self.node._children[1] and
	self.node._children[1]._text then

	if _value then
	    self.node._children[1]._text = _value;
	    return (true);
	else
	    return (self.node._children[1]._text or "");
	end

    end

    if _value then
	if not self.node._children then
	    self.node._children = {};
	end
	table.insert(self.node._children, { _text = _value, _type = "TEXT" });
    end

    return (nil);
end

mtNode.__index = Node;
-----------------------------------
-------- End of Node Class --------
-----------------------------------
