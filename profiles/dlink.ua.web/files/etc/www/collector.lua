#!/usr/bin/lua

package.path = "./?.lua;/etc/www/lib/?.lua;./lib/?.lua";
package.cpath = 
	"/lib/?.so;/usr/lib/?.so;/usr/lib/lua/?.so;" ..
	"/lib/lua?.so;/usr/lib/lua?.so;/usr/lib/lua/lua?.so;" ..
	"/lib/?-core.so;/usr/lib/?-core.so;/usr/lib/lua/?-core.so;" ..
	"/lib/?/core.so;/usr/lib/?/core.so;/usr/lib/lua/?/core.so;";

host = host or "127.0.0.1";
port = port or "8";

serverhost = serverhost or "127.0.0.1";
serverport = serverport or "80";

-- Globals 
r = {};		-- Runtime varibles structure
rquery = {};
queue = {};

--
-- Utility function:  URL encoding function
--
function urlEncode(str)
    if (str) then
        str = string.gsub (str, "\n", "\r\n") 
        str = string.gsub (str, "([^%w ])",
            function (c) return string.format ("%%%02X", string.byte(c)) end) 
        str = string.gsub (str, " ", "+") 
    end 
    return str 
end


--
-- Utility function:  URL decode function
--
function urlDecode(str)
    str = string.gsub (str, "+", " ") 
    str = string.gsub (str, "%%(%x%x)", function(h) return string.char(tonumber(h,16)) end) 
    str = string.gsub (str, "\r\n", "\n") 
    return str 
end

-- convert name1=value1&name2=val+ue%2F2
-- to table {"name1"="value1", "name2"="val ue/2"}
function parse_query(query)
    local parsed = {};
    local pos = 0;

    query = string.gsub(query, "&amp;", "&");
    query = string.gsub(query, "&lt;", "<");
    query = string.gsub(query, "&gt;", ">");

    local function ginsert(qstr)
        local first, last = string.find(qstr, "=");
        if first then
            local key = urlDecode(string.sub(qstr, 0, first-1));
            local value = urlDecode(string.sub(qstr, first+1));
            parsed[key] = value;
        end
    end

    while true do
        local first, last = string.find(query, "&", pos);
        if first then
            ginsert(string.sub(query, pos, first-1));
            pos = last+1;
        else
            ginsert(string.sub(query, pos));
            break;
        end
    end
    return parsed;
end

function process(q, queryline)

    if q["cmd"] == "event" then

	    local iface = q["iface"];

	    if not iface then
		return ("collector.lua: ERROR: Interface not defined");
	    end

	    if type(r[iface]) ~= "table" then r[iface] = {}; end
--	    rquery[iface] = queryline;
	    table.insert(queue, {handled=false, query=queryline, qt=q});

	    for k,v in pairs(q) do
		if (k ~= "iface") and (k ~= "cmd") then
		    r[iface][k] = v;
		end
	    end
	    if q["eventtype"] == "linkup" then
		-- XXX: should not be here
		-- XXX: must check exit code
		exitcode = os.execute(
		    "route change default -iface " .. iface .." > /dev/null 2>&1 || " ..
		    "route add default -iface " .. iface .." > /dev/null 2>&1"
		);
		local dns = {};
		if q["dns1"] and q["dns1"]:len() >= 7 then
		    table.insert(dns, q["dns1"]);
		end
		if q["dns2"] and q["dns2"]:len() >= 7 then
		    table.insert(dns, q["dns2"]);
		end
		if table.getn(dns) > 0 then
		    resolv_conf = io.open("/etc/resolv.conf", "w");
		    for i,v in ipairs(dns) do
			-- print("nameserver	" .. v);
			resolv_conf:write("nameserver	" .. v .. "\n");
		    end
		    resolv_conf:close();
		end
	    end

	return "OK";

    elseif q["cmd"] == "revent" then

	iface = q["iface"];

	if r[iface] then
	    local ret = nil;

	    for k,v in pairs(r[iface]) do
		if ret then
		    ret = ret .. "&" .. urlEncode(k) .. "=" .. urlEncode(v);
		else
		    ret = urlEncode(k) .. "=" .. urlEncode(v);
		end
	    end

	    return (ret);
	else
	    return ("ERROR: no data for interface " .. iface);
	end
    else
	return ("ERROR: unknown command");
    end
end

function call_server(http, q)
    local query = "http://127.0.0.1:80/event.xml?" .. q;

    local body, code, headers = http.request(query);

    --[[
    print("http.request(" .. query ..") ",
	body or "(no body)",
	headers or "(no headers)",
	code or "(no code)"
	);
    ]]

    if code == 200 then
	return (true);
    else
	return (nil);
    end

end

function getopt(args, opts)
    i=1;
    while i < table.getn(arg) do 
	if arg[i]:match("^-") then
	    opts[arg[i]] = arg[i+1];
	    i = i + 1;
	end
	i = i + 1;
    end
    return (opts);
end



opts = {};
opts["-P"] = "/var/run/collector.pid";

if arg then
    opts = getopt(arg, opts);
end


-- Check pidfile
dofile("lib/pidfile.lua");
pidfile(opts["-P"]);

socket = require("socket");
http = require("socket.http");
server = assert(socket.bind(host, port));
server:settimeout(5);


while 1 do
    local method, path, query, major, minor;

    local control = server:accept();

    if control then
	while 1 do 
    	    local data, err = control:receive();
    	    if not err then
    		local q;
	        _, _, method, path, q, major, minor  = string.find(data, "([A-Z]+) (.-)%?(.-) HTTP/(%d).(%d)");
		if not query and q then
		    query = q;
		    break;
		end
    	    else
    		break;
    	    end
	end

	if query then
	    local q = parse_query(query);
	    if q and q.cmd then
		assert(control:send(
		    "HTTP/1.0 200 OK\r\n" ..
		    "Server: simple-lua\r\n" ..
		    "Content-type: text/xml\r\n" ..
		    "Connection: close\r\n" ..
		    "\r\n" ..
		    process(q, query)
		));
    	    end
        end

	assert(control:close());
    end

    local n = table.getn(queue);
    if n > 0 then
	for i = n,1,-1 do
	    if queue[i].handled ~= true then
		-- print("queue[" .. i .. "] Will send " .. queue[i].query .. " to main server");
		if (call_server(http, queue[i].query) == true) then
		    -- print("queue[" .. i .. "] Will delete " .. queue[i].query .. " from queue");
		    queue[i].handled = true;
		    table.remove(queue, i);
		end
	    end
	end
    end

end

os.exit(0);





