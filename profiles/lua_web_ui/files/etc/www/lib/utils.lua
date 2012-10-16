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

function xmldump(t, level)
    if type(t) ~= "table" then
	return (nil);
    end

    local function xatrdmp(t)
	local ret = '';

        for k, v in pairs(t) do
    	    ret = ret .. string.format(" %s=\"%s\"", tostring(k), tostring(v));
        end

        return (ret);
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
    	end

	return (ret);
    end

    return xdmp(t, level or -1);
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
      return ("Can't open file " .. _file);
    end

    return xml;
end

function inc(_file)
    return read_file(_file);
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

function conf_table(id, header, rootpath, fields)

	local ret =
		"<div id=\"" .. id .. "\" class=\"yui3-module boxitem\">\n" ..
		"    <div class=\"yui3-hd\">\n" ..
		"        <h4>" .. header .."</h4>\n" ..
		"    </div>\n" ..
		"    <div class=\"yui3-bd\">\n" ..
		"	<form method=\"POST\" target=\"/cmd.xml\" onchange=\"document.getElementById('" .. id .. "_update_button').disabled = false; return true;\">\n" ..
		"	    <input type=\"hidden\" name=\"path\" value=\"" .. rootpath .."\" />\n" ..
		"	    <input type=\"hidden\" name=\"cmd\" value=\"setmany\" />\n" ..
		"	<table>\n";


	for i,v in ipairs(fields) do
	    if v.type == "attr" then
		local _, _, nodestr, attrstr = string.find(v.node, "(.-)%:(.*)");

		print(string.format("node='%s' attr='%s'", nodestr or "(no node)", attrstr or "(no attr)"));

		if not nodestr then
		    nodestr = rootpath;
		else
		    nodestr = rootpath .. "." .. nodestr;
		end

		if not attrstr then
		    ret = ret .. "Error parsing attr fileld at path " .. v.node;
		else

		attrvalue = c:getNode(nodestr):attr(attrstr) or
		    "Error getting attribute value of " .. nodestr .. ":attr(" .. attrstr .. ")";
		local inserted = "";
		if v.htmltype == "checkbox" then
		    inserted = checked(attrvalue);
		elseif v.htmltype == "text" then
		    inserted = " value=\"" .. attrvalue .. "\"";
		else
		    inserted = "Unknown htmltype " .. v.htmltype;
		end
		ret = ret ..
		"            <tr>\n" ..
		"        	<td>" .. v.label .. ":</td>\n" ..
		"        	<td><input name=\"smattr:" .. v.node .. "\" " ..
				    "type=\"" .. v.htmltype .. "\" " ..
				    inserted ..
				    " onchange=\"" ..
					"document.getElementById(" .. 
					    "'" .. id .. "_update_button'" ..
					").disabled = false; return true;\"/></td>\n" ..
		"    	     </tr>\n";
		end
	    elseif v.type == "node" then
		local nodestr = v.node;
		if not nodestr then
		    nodestr = rootpath;
		else
		    nodestr = rootpath .. "." .. nodestr;
		end

		local n = c:getNode(nodestr);
		if n then
		    nodevalue = n:value() or "";
		else
		    nodevalue = "Error: node " .. nodestr .. " not found";
		end

		nodevalue = nodevalue:gsub("\"","&quot;");

		ret = ret ..
		"            <tr>\n" ..
		"        	<td>" .. v.label .. ":</td>\n" ..
		"    		<td><input type=\"" .. v.htmltype .. "\" name=\"sm:" .. v.node .. "\" value=\"" ..
				    nodevalue ..
				    "\" onchange=\"document.getElementById(" ..
					"'" .. id .. "_update_button'" ..
				    ").disabled = false; return true;\"/></td>\n" ..
		"            </tr>\n";
	    end
	end

	ret = ret ..
		"            <tr>\n" ..
		"        	<td>&nbsp;</td>\n" ..
		"        	<td><input id=\"" .. id .. "_update_button\" type=\"button\" name=\"Update\" value=\"Update\" disabled onclick=\"" ..
					"send_update(this.form, function(x) { document.getElementById('" .. id .. "_update_button').disabled = true; })" ..
				    "\"/></td>\n" ..
		"            </tr>\n" ..
		"	</table>\n" ..
		"	</form>\n" ..
		"    </div>\n" ..
		"</div>\n";

	return (ret);
end

function peritem(item, sub)
	if type(item) == "table" then

    		for k, v in pairs(item) do
    			sub(v);
    		end
	else
    		sub(item);
	end
end

function kldload(module)
	function kldload_one(module)
		if debug then
			print("kldstat -q -m " .. module .. " || kldload " .. module);
		end
		os.execute("kldstat -q -m " .. module .. " || kldload " .. module);
	end
	peritem(module, kldload_one);
end

function strip_quote (str)
	str = str:gsub("^\"(.*)\"$", "%1");
	str = str:gsub("^\'(.*)\'$", "%1");
	return str;
end

function parse_kv_file (ar, file)
	local err = nil;
	if not file then
		return "Filename required";
	end

	if type(ar) ~= "table" then
		return "First argument must be table";
	end

	local f = io.open(file);
	if not f then
		return "Can't open \"" .. file .. "\"\n";
	end

	local count = 0;
	while true do
		local line = f:read('*line');
		if line == nil then break end -- EOF
		if count > 100 then
			-- to much lines
			err = "Lines count more than 100 \"" .. file .. "\n";
			break;
		end

		line = line:gsub("#.*$", ""); -- Strip comments
		line = line:gsub("%s*$", ""); -- Strip whitespaces

		if line ~= "" then -- Do only if line not empty
			match, _, name, value = line:find("^(.+)=(.+)%s*$");
			if not match then
				err = "Wrong format in \"" .. file .. "\n";
				break;
			end
			ar[strip_quote(name)] = strip_quote(value);
		end

		count = count + 1;
	end

	f:close();
	return err;
end

function exec_output(cmd)
	fp = io.popen(cmd, "r");
	data = fp:read("*a");
	fp:close();
	-- remove CR and LF, but only at the end of data
	return data:gsub("[\n\r]$", "");
end
