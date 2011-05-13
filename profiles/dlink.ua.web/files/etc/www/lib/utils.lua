--[[
    tdump(table_to_dump)
    return text dump of table
]]

function tdump(t)
    local function dmp(t, l, k)
	local ret = "";
	if type(t) == "table" then
    	    ret = ret .. string.format("%s%s:", string.rep(" ", l*2), tostring(k)) .. "\n";
    	    for k, v in pairs(t) do
    		if k ~= "_parent" then
    		    ret = ret .. dmp(v, l+1, k);
    		end
    	    end
	else
    	    ret = ret .. string.format("%s%s:%s", string.rep(" ", l*2), tostring(k), tostring(t)) .. "\n";
	end
	return (ret);
    end
    return dmp(t, 1, "root");
end

--[[
    xmldump(table_to_dump)
    return XML subtree of table
]]

function xmldump(t)

    local function xatrdmp(t)
	local ret = '';

        for k, v in pairs(t) do
    	    ret = ret .. string.format(" %s=\"%s\"", tostring(k), tostring(v));
        end

        return ret;
    end

    local function xdmp(t, l)
	local ret = '';
	local enum = 0;
	local name = t._name or '';
	local attrs = '';
	local childs = '';
	local brk = nil;

    	if t._attr then
	    attrs = attrs .. xatrdmp(t._attr);
	end


	if t._children then
    	    for k, v in ipairs(t._children) do
    		childs = childs .. xdmp(v, l+1);
    	    end

    	    if table.maxn (t._children) > 1 then
		brk = 1;
	    elseif table.maxn (t._children) == 1 and
	      t._children[1]._type ~= "TEXT" then
		brk = 1;
	    end
    	end

	if t._type == "ELEMENT" then
	    if brk then
    		ret =   ret ..
    		    string.format("%s<%s%s>\n", string.rep(" ", l*4), name, attrs) ..
    		    childs ..
    		    string.format("%s</%s>\n", string.rep(" ", l*4), name);
    	    else
    		ret =   ret ..
    		    string.format("%s<%s%s>", string.rep(" ", l*4), name, attrs) ..
    		    childs ..
    		    string.format("</%s>\n", name);
    	    end
    	elseif t._type == "TEXT" then
    	    ret = ret .. t._text;
    	elseif t._type == "ROOT" then
    	    ret = childs;
    	elseif t._type == "COMMENT" then
    	    ret = ret ..
    		string.format("%s<!-- %s -->\n", string.rep(" ", l*4), t._text);
--    	else
--    	    print("Parse Error Unknow type \"" .. (t._type or "(nil)") .. "\"");
    	end

	-- print(ret);
	return ret;
    end

    return xdmp(t, -1);
end



--[[
    read_file(_file)
    return data from file
]]
function read_file(_file)
    local xml;

    if (_debug) then
        io.write ( "File: ".._file.."\n" )
    end

    local f, e = io.open(_file, "r")
    if f then
      xml = f:read("*a")
    else 
      error(e)
    end

    return xml;
end

function load_file(_file)
    h = domHandler();
    x = xmlParser(h);
    index = 1

    x.options.expandEntities = 1; -- nil;
    x:parse(read_file(_file));

    return (h.root);
end

function save_file(_file, _data)
    local out, e = io.open(_file, "w");
    if out then
        out:write(_data);
	assert(out:close());
    else 
        print(e);
        return (nil);
    end

    return (true);
end

function checked(v)
    if (v and v ~= "" and v ~= "false") then
        return "checked";
    end
    return "";
end

function field(v)
    if (v and v ~= "") then
        return v;
    end
    return "";
end

