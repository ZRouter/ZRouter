#!/usr/bin/lua
--  -*-mode: C++; style: K&R; c-basic-offset: 4 ; -*- */

--
--  A simple HTTP server written in Lua, using the socket primitives
-- in 'libhttpd.so'.
--
--
-- Steve Kemp
-- --
-- http://www.steve.org.uk/
--
-- $Id: httpd.lua,v 1.31 2005/10/31 17:08:22 steve Exp $


--
-- load the socket library
--
package.path = "./?.lua;/etc/www/lib/?.lua;./lib/?.lua";
package.cpath =
	"/lib/?.so;/usr/lib/?.so;/usr/lib/lua/?.so;" ..
	"/lib/lua?.so;/usr/lib/lua?.so;/usr/lib/lua/lua?.so;" ..
	"/lib/?-core.so;/usr/lib/?-core.so;/usr/lib/lua/?-core.so;" ..
	"/lib/?/core.so;/usr/lib/?/core.so;/usr/lib/lua/?/core.so;";

--io.stdout = assert(io.open("/dev/console", "w"));
--io.stderr = io.stdout;

socket = require("socket");
require('sysctl.core');

-- Expat binding
dofile('lib/xml.lua');
-- XML entity handlers
dofile('lib/handler.lua');

-- read_file, tdump, xmldump, exec_output
dofile('lib/utils.lua');
-- Conf object
dofile('lib/conf.lua');
-- Node object
dofile('lib/node.lua');
-- base64 decode/encode
dofile('lib/base64.lua');
-- Socket helper
--dofile("lib/sock.lua");
-- Route select logic
dofile("lib/route.lua");
-- MPD helper
dofile("lib/mpd.lua");
-- RACOON helper
dofile("lib/racoon.lua");
-- DHCPD helper
-- dofile("lib/dhcpd.lua");
-- HOSTAPD helper
--dofile("lib/hostapd.lua");

-- redirect print to syslog
dofile("lib/lsyslog.lua");
syslog_init("httpd.lua");

-- urlEncode/urlDecode
dofile("lib/urlXxcode.lua");

httpd_debug = true;

--
--  A table of MIME types.
--  These values will be loaded from /etc/mime.types if available, otherwise
-- a minimum set of defaults will be used.
--
mime = {};

-- # lua -e 'package.cpath ="/usr/lib/lua/lua?.so"; require("sysctl.core"); s,e,v,t = pcall(function () return sysctl.get("hw.device.model") end); print(s,e,v,t);'
-- true    DIR-825 A       nil
-- # lua -e 'package.cpath ="/usr/lib/lua/lua?.so"; require("sysctl.core"); s,e,v,t = pcall(function () return sysctl.get("hw.device.mode") end); print(s,e,v,t);'
-- false   (command line):1: unknown iod 'hw.device.mode'  nil     nil

-- save sysctl obj
sysctl_obj = sysctl;
function sysctl(oid, value)
	if type(oid) == "table" then
		print("sysctl(table) not implemented\n");
	else
		if value then
			print("sysctl(%s, %s)\n", oid, tostring(value));
			local s,v,t = pcall(function () return sysctl_obj.set(oid, value) end);
			if s then return (v); end
			return (nil);
		else
			print("sysctl(%s)\n", oid);
			local s,v,t = pcall(function () return sysctl_obj.get(oid) end);
			if s then return (v); end
			return (nil);
		end
	end
end

--
--  Start a server upon the given port, using the given
-- root directory path.
--
--  The root path is designed to contain sub-directories for
-- each given virtual host.
--
function start_server( c, config )
	running = 1;

	--
	--  Bind a socket to the given port
	-- c:getNode("http.host"):value();
	s = socket.tcp();
	assert(s:setoption("reuseaddr", true));
	assert(s:bind("*", c:getNodeValueSafe("http.port", 80)));
	assert(s:listen(8));
	assert(s:settimeout(5));
	config.listener = s;

	--
	--   Print some status messages.
	--
	print( "\nListening upon:" );
	print( "  http://" .. c:getNodeValueSafe("http.host", "*") .. ":" ..
			c:getNodeValueSafe("http.port", 80) .. "/" );
	print( "\n\n");
	--[[
	table.insert(r.tasks.countdown, { count=25, task=
			function()
			print("task 0 ok");
			return (true);
			end
			});
	table.insert(r.tasks.countdown, { count=10, task=
			function()
			print("task 1 ok");
			return (true);
			end
			});
	table.insert(r.tasks.countdown, { count=15, task=
			function()
			print("task 2 ok");
			return (true);
			end
			});
	table.insert(r.tasks.countdown, { count=5, task=
			function()
			print("task 3 ok");
			return (true);
			end
			});
	]]
	--
	--  Loop accepting requests.
	--
	while ( running == 1 ) do
	processConnection( c, config );
	periodic_tasks(c);
	end

	--
	-- Finished.
	--
	config.listener:close();
end

function periodic_tasks(c)
	-- fetch collector info
	local i;
	local n = #r.tasks.countdown;

	for i = n,1,-1 do
		r.tasks.countdown[i].count = r.tasks.countdown[i].count -
		    r.tasks.step;

		if (r.tasks.countdown[i].count <= 0) then
			-- Task must return true, else run forever
			if r.tasks.countdown[i].task() then
				table.remove(r.tasks.countdown, i);
			end
		end
	end
end



--
--  Process a single incoming connection.
--
function processConnection( c, config )
	--
	--  Accept a new connection
	--
	config.listener:settimeout(r.tasks.step);
	local client = config.listener:accept();
	if (not client) then return (nil) end;
	local ip, port = client:getpeername();

	if not client then
		return (nil);
	end

	found   = 0;  -- Found the end of the HTTP headers?
	chunk   = 0;  -- Count of data read from client socket
	size    = 0;  -- Total size of incoming request.
	code    = 0;  -- Status code we send to the client
	request = ""; -- Request body read from client.
	rq = {};
	rq.ip = ip;
	rq.port = port;


	--
	--  Read in a response from the client, terminating at the first
	-- '\r\n\r\n' line - this is the end of the HTTP header request.
	--
	--  Also break out of this loop if we read ten packets of data
	-- from the client but didn't manage to find a HTTP header end.
	-- This should help protect us from DOSes.
	--
	-- client:settimeout(1);
	--    while ( true ) do
	--    	local data, err = client:receive("*l");
	--
	--	if not data then break end
	--
	--        request = request .. data .. "\r\n";
	--
	--	if data:len() <= 0 then break end
	--    end

	--    local request, err = client:receive("*a");

	--    size = request:len();
	--
	--    print("Rq: \"" .. request .. "\"");
	--
	--    position,len = request:find("\r\n\r\n");
	--    if ( position ~= nil )  then
	--        rq.RequestHeader = request:sub(0, position);
	--        rq.RequestBody   = request:sub(position+4);
	--        rq.RequestBodyLength = len-4;
	--    else
	--	-- Maybe we need say ERROR!!!!
	--	return (nil);
	--    end

	rq.ContentLength = -1;
	rq.RequestHeader = "";
	i = 1;
	while ( true ) do
		local data, err = client:receive("*l");
		--print(i, data or "(no data)", err or " . ");
		i = i + 1;

		if not data then break end

		local match, _, len = data:find("Content%-Length: ([^:\r\n]+)");
		if match then
			rq.ContentLength = tonumber(len);
		end

		rq.RequestHeader = rq.RequestHeader .. data .. "\r\n";

		if data:len() <= 0 then break end
	end

	if rq.ContentLength >= 0 then
		rq.RequestBodyLength = rq.ContentLength;
		rq.RequestBody, err = client:receive(rq.RequestBodyLength);
	end

	if err then
		rq.RequestBodyLength = nil;
		rq.RequestBody = nil;
	end

	--
	--  OK We now have a complete HTTP request of 'size' bytes long
	-- stored in 'rq.RequestHeader'.
	--

	--
	-- Find the requested path.
	--
	_, _, method, path, major, minor  =
	    string.find(rq.RequestHeader, "([A-Z]+) (.+) HTTP/(%d).(%d)");

	--
	-- We only handle GET requests.
	--
	if ( method ~= "GET" ) and ( method ~= "POST" ) and
	    ( method ~= "PUT" ) and ( method ~= "DELETE" ) then
		error = "Method not implemented";

		if ( method == nil ) then
			error = error .. ".";
		else
			error = error .. ": " .. urlEncode( method );
		end

		err = sendError(501, error);
		client:send(err);
		client:close();
		return err:len(), "501";
	end


	--
	-- Decode the requested path.
	--
	path = urlDecode( path );
	rq.path = path;

	--
	-- find the Virtual Host which we need for serving, and find the
	-- user agent and referer for logging purposes.
	--
	_, _, rq.Host           = rq.RequestHeader:find( "Host: (.-)\r?\n");
	_, _, rq.Agent          = rq.RequestHeader:find( "Agent: (.-)\r?\n");
	_, _, rq.Referer        = rq.RequestHeader:find( "Referer: (.-)\r?\n");

	_, _, rq.Accept         = rq.RequestHeader:find( "Accept: (.-)\r?\n");
	_, _, rq.AcceptLanguage = rq.RequestHeader:find( "Accept%-Language: ([^:\r\n]+)");
	_, _, rq.AcceptEncoding = rq.RequestHeader:find( "Accept%-Encoding: ([^:\r\n]+)");
	_, _, rq.AcceptCharset  = rq.RequestHeader:find( "Accept%-Charset: ([^:\r\n]+)");
	_, _, rq.ContentType    = rq.RequestHeader:find( "Content%-Type: ([^:\r\n]+)");
	_, _, rq.ContentLength  = rq.RequestHeader:find( "Content%-Length: ([^:\r\n]+)");
	_, _, rq.Authorization  = rq.RequestHeader:find( "Authorization: ([^:\r\n]+)");

	rq.UserName = nil;
	rq.Password = nil;
	rq.Group    = nil;
	if rq.Authorization then
		local BasicAuth = "";
		_, _, BasicAuth = string.find(rq.RequestHeader,
		    "Basic ([^:\r\n]+)");
		if BasicAuth then
			local pair = b64dec(BasicAuth);
			_, _, _u, _p = string.find(pair, "(.-)%:(.*)");

			for usr = 1, 8, 1 do
				local u = c:getNode(string.format(
				    "http.users.user[%d]",
				    usr));
				if u and (u:attr("username") == _u) and
				    (u:attr("password") == _p) then
					rq.UserName = _u;
					rq.Password = _p;
					rq.Group    = u:attr("group");
				end
			end
		end
	end

	if not rq.Group and rq.ip ~= "127.0.0.1" then
		local message = "HTTP/1.1 401 Authorization Required\r\n" ;
			message = message .. "Server: httpd\r\n";
			message = message .. "WWW-Authenticate: Basic " ..
			    "realm=\"Secure Area\"\n";
			message = message .. "Content-type: text/html\r\n";
			message = message .. "Connection: close\r\n\r\n" ;
			message = message ..
			    "<html>" ..
				"<head><title>Error</title></head>" ..
				"<body><H1>401 Unauthorized.</H1></body>" ..
			    "</html>" ;
		--Content-Length: 311

		size = string.len(message);
		client:send(message);
		client:close();
		code = "401";
	else
		size, code = handleRequest(c, config, path, client, method, rq);
	end

	if ( rq.Agent   == nil ) then rq.Agent   = "-" end;
	if ( rq.Referer == nil ) then rq.Referer = "-" end;
	if ( code       == nil ) then code       = 0 ; end;

	if ( running == 0 ) then
		print( "Terminating as per request from " .. ip );
	end

	--
	--  Close the client connection.
	--
	client:close();
end

--
--  Attempt to serve the given path to the given client
--
function handleRequest( c, config, path, client, method, rq )
	--
	-- Local file
	--
	local file = string.gsub(path, "%?.*", "");
	local query_string = "";
	if file ~= path then
		query_string = string.gsub(path, ".*%?", "");
	end

	rq.ScriptName = file;
	rq.QueryString = query_string;
	rq.GET = parse_query(query_string);
	-- TODO check request content type
	if rq.RequestBody then
		rq.POST = parse_query(rq.RequestBody);
	end

	--
	--  File must be beneath the vhost root.
	--
	file = "htdocs/" .. file ;

	--
	--  Attempt to sanitize the input Virtual Host + requested path.
	--
	--    file = string.strip( file );
	file = string.gsub (file, "//", "/")

	--
	-- Add a trailing "index.html" to paths ending in / if such
	-- a file exists.
	--
	-- Otherwise if it is a directory then serve it.
	--
	if ( string.endsWith( file, "/" ) ) then
		tmp = file .. "index.html";
		if ( fileExists( tmp ) ) then
			file = tmp;
		end
	end

	--
	-- Open the file and return an error if it fails.
	--

	if ( fileExists(file) == false ) then
		err = sendError(404, "File not found " .. urlEncode(path));
		size = string.len(err);
		client:send(err)
		client:close();
		return size, "404";
	end;

	--
	-- Find the suffix to get the mime.type.
	--
	_, _, ext  = string.find( file, "\.([^\.]+)$" );
	if ( ext == nil ) then
		ext = "html";   -- HACK
	end

	mimetype = mime[ext];
	if ( mimetype == nil ) then mimetype = 'text/plain' ; end;

	--
	-- Send out the header.
	--
	client:send(
			"HTTP/1.0 200 OK\r\n" ..
			"Server: httpd\r\n" ..
			"Content-type: " .. mimetype  .. "\r\n" ..
			"Connection: close\r\n\r\n" );


	--
	-- Read the file, and then serve it.
	--
	f       = io.open( file, "rb" );
	size    = fileSize( f );
	local t = f:read("*all")
	f:close();

	local retcode = "200";

	-- Replace $$$ name $$$ variables
	if (ext == "html") or (ext == "js") then
		t = string.gsub(t, "%$%$%$(.-)%$%$%$",
		    function (s)
			if not s then return; end
			return evalembeded(s)
			end);

	-- Eval Lua code
	elseif (ext == "lua") or (ext == "xml") or (ext == "dat") then
		code, err = assert(loadstring(t));
		if not code then
			t = sendError(500, "Error \""..err.."\" when parse"..
			    " " .. urlEncode(path));
			retcode = "500";
		else
			st, t = pcall(code);
		end
	end
	client:send(t);
	client:close();

	return string.len(t), retcode;
end

function evalembeded(s)
	if not s then
		return "";
	end

	match, _, code = string.find(s, "^code%:(.*)$");
	if match then
		local func, err = assert(loadstring("return " .. code));
		if not func then
			return err;
		end
		local status, ret = pcall(func);
		if status == false then
			print(code, " failed");
			return "";
		end
		return (ret);
	else
		return (field(c:getNodeValueSafe(s, "")));
	end
end

function queryData(rq)
	if rq.POST["cmd"] then
		cmd = rq.POST["cmd"];
		if cmd == "get" then
			key = rq.POST["key"];
			if key == "datetime" then
				return os.date("%Y-%m-%d %H:%M:%S");
			end
			if key == "uptime" then
				return sysctl("kern.ident");
			end
		else
			return ("Unknown command \"" .. cmd .. "\"");
		end
	end
	return ("");
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

--
--  Send the given error message to the client.
--  Return the length of data sent to the client so we know what to log.
--
function sendError( status, str )
    message = "HTTP/1.0 " .. status .. " OK\r\n" ;
    message = message .. "Server: httpd\r\n";
    message = message .. "Content-type: text/html\r\n";
    message = message .. "Connection: close\r\n\r\n" ;
    message = message .. "<html><head><title>Error</title></head>" ;
    message = message .. "<body><h1>Error</h1>" ;
    message = message .. "<p>" .. str .. "</p></body></html>" ;

    return (message);
end



--
--  Utility function:  Determine the size of an open file.
--
function fileSize (file)
    local current = file:seek()      -- get current position
    local size = file:seek("end")    -- get file size
    file:seek("set", current)        -- restore position
    return size
end


--
--  Utility function:  Determine whether the given file exists.
--
function fileExists (file)
    local f = io.open(file, "rb");
    if f == nil then
        return false;
    else
        f:close();
        return true;
    end
end

function read_zrouter_version (ar)
	ar.zrouter_version = {};
	return parse_kv_file(ar.zrouter_version, "/etc/zrouter_version");
end

function read_rc_conf (ar)
	ar.rc_conf = {};
	return parse_kv_file(ar.rc_conf, "/etc/rc.conf");
end

--
--  Read the mime file and setup mime types
--
function loadMimeFile(file, table)
     local f = io.open( file, "r" );
     while true do
         local line = f:read()
         if line == nil then break end
         _, _, mimetype, name = string.find( line, "^(.*)\t+([^\t]+)$" );
         if ( mimetype ~= nil ) then
             for w in string.gfind(name, "([^%s]+)") do
                  table[w] = mimetype;
             end
         end
     end
end

--
--  Utility function:   Does the string end with the given suffix?
--
function string.endsWith(String,End)
    return End=='' or string.sub(String,-string.len(End))==End
end

--
--  Strip path traversal requests.
--
function string.strip( str )
    return( string.gsub( str, "/../", "" ) );
end

--
-- Log an access request.
--
function logAccess( user, method, host, ip, request, status, size, agent, referer, major, minor )
    date = os.date("%m/%b/%Y:%H:%M:%S +0000");

    --
    -- Format the logging line.
    --
    log  = string.format( '%s %s - - [%s] "%s %s HTTP/%s.%s" %s %s "%s" "%s"',
        user, method, ip, date, request, major, minor, status, size, referer, agent );

    logfile = "logs/access.log"

    --
    -- Open logfile for appending.
    --
    file = io.open( logfile , "a" );
    if ( file ~= nil ) then
        file:write( log .. "\n" );
        file:close();
    else
        print( "WARNING : Unable to open logfile for writing : " .. logfile );
    end

    print( log );
end

--
-----
---
--
--  This is the start of the real code.  Now that our functions have been
-- defined we actually execute from this point onwards.
--
---
----
--

--
--  Setup the MIME types our server will use for serving files.
--
if ( fileExists( "/etc/mime.types" ) ) then
    loadMimeFile( "/etc/mime.types",  mime );
else
    --
    --  The global MIME types file does not exist.
    --  Setup minimal defaults.
    --
--    print( "WARNING: /etc/mime.types could not be read." );
--    print( "         Running with minimal MIME types." );

    -- TODO: lua scripts can be able to set content type
    mime[ "lua" ]  = "text/html";
    mime[ "xml" ]  = "application/xml";
    mime[ "html"]  = "text/html";
    mime[ "txt" ]  = "text/plain";
    mime[ "js"  ]  = "text/plain";
    mime[ "css" ]  = "text/css";
    mime[ "jpg" ]  = "image/jpeg";
    mime[ "jpeg"]  = "image/jpeg";
    mime[ "gif" ]  = "image/gif";
    mime[ "png" ]  = "image/png";
    mime[ "dat" ]  = "application/octet-stream";
end

function checkIface(name)
	local ret = os.execute("ifconfig " .. name .. " > /dev/null");
	if ret ~= 0 then
	    return (nil);
	end
	return (true);
end
--[[
        <create>
            <exec order="1">ifconfig wlan0 ether `kenv WAN_MAC_ADDR`</exec>
            <exec order="2">ifconfig lan0 ether `kenv WAN_MAC_ADDR`</exec>
            <exec order="3">ifconfig bridge0 create addm lan0 addm wlan0 up</exec>
        </create>
        <init>
            <exec order="1">ifconfig bridge0</exec>
        </init>
]]

function doexec(c, path)
    if ((not c) or (not path)) then
	return (nil);
    end

    local block = c:getNode(path);

    if not block then
	return (nil);
    end

    local function execone(c, path)
	local exec = c:getNode(path);
	if exec then
	    exec = exec:value();
	    if exec then
		print("Run:" .. exec)
		print(exec_output(exec));
	    end
	end
    end

    for i = 1,16,1 do
	execone(c, path .. ".exec[" .. i .. "]");
    end

    return (true);

end


function configure_lan(c)

    local function config_iface(c, name)
        if not checkIface(name) then
	    doexec(c, "interfaces." .. name ..".create");
	end
	doexec(c, "interfaces." .. name ..".init");

	ipnode = c:getNode("interfaces." .. name .. ".ipaddr");
	if (ipnode) then
	    local ip = ipnode:value();
	    if ip then
		local cmd = "ifconfig " .. name .. " " .. " inet " .. ip;
		print("Run:" .. cmd)
		print(exec_output(cmd));
	    end
	end
    end

    config_iface(c, "lan0");
    config_iface(c, "wlan0");
    config_iface(c, "bridge0");

end

mpd = 0;

function configure_mpd_link(c, path, bundle, link)
    if mpd == 0 then
	mpd = MPD:new(c, "127.0.0.1", 5005);
    end

--    local bundle = "PPP";
--    local path = "interfaces.wan0.PPP";

    mpd:config_bundle(path, bundle);
    mpd:config_link(path, link, bundle);
end

function run_dhclient(c, iface)
    local path = "interfaces." .. iface .. ".Static";

    -- return if no iface.Static node.
    if not c:getNode(path) then
	return (nil);
    end

    -- return if Static mode not enabled.
    if c:getNode(path):attr("enable") ~= "true" then
	return (nil);
    end

    -- return if no DHCP enabled.
    local dhcp = c:getNode(path .. ".dhcp");
    if not dhcp or dhcp:attr("enable") ~= "true" then
	return (nil);
    end

    -- Run dhclient
    print("Run dhclient on " .. iface);
    os.execute(string.format("kill `cat /var/run/dhclient.%s.pid`", iface));

    -- Do not execute dhclient if interface is down
    if exec_output("ifconfig -v " .. iface .." | grep 'status: active'") ~= "" then
	os.execute(string.format("/sbin/dhclient -b %s", iface));
	-- XXX, hack to "unplumb" iface if it was not up on boot
	os.execute(string.format("ifconfig %s `ifconfig %s | grep 'ether '`", iface, iface));
    end
end


function configure_wan(c)
    -- TODO: auto enumerate wan links
    --[[
kldload ng_nat
kldload ng_ipfw

ngctl mkpeer ipfw: nat 60 out
ngctl name ipfw:60 nat
ngctl connect ipfw: nat: 61 in
ngctl msg nat: setaliasaddr 192.168.101.204

ipfw add 98 netgraph 61 all from any to any in via wan0
ipfw add 99 netgraph 60 all from any to any out via wan0

sysctl net.inet.ip.fw.one_pass=0
    ]]

    kldload({"ng_nat", "ng_ipfw"});
    sysctl("net.inet.ip.fw.one_pass", 0);

    local sub = "";
    for _, sub in ipairs({"Static", "PPPoE", "PPP", "PTPP", "L2TP"}) do
	print("sub=" .. sub);
	local path = "interfaces.wan0." .. sub;
	local ifnode = c:getNode(path);

	if ifnode and ifnode:attr("enable") == "true" then
	    local subtype = c:getNode(path):attr("type");
	    print("subtype=" .. subtype);
	    if (subtype == "l2tp") then
		-- TODO: multilink have different link names
	    elseif (subtype == "pppoe") then
		kldload({"ng_pppoe"});
		os.execute("sleep 1");
		configure_mpd_link(c, path, sub, sub);
		-- Add interface to route select logic
		r.route:f(sub,
		    c:getNodeValueSafe(path .. ".group", "WAN"),
		    c:getNodeValueSafe(path .. ".cost", "1000") - 0,
		    "down",
		    "LINKDOWN")
	    elseif (subtype == "pptp") then
		-- TODO: multilink have different link names
	    elseif (subtype == "modem") then
		kldload({"umodem", "u3g"});
		os.execute("sleep 1");
		configure_mpd_link(c, path, sub, sub);
		r.route:f(sub,
		    c:getNodeValueSafe(path .. ".group", "WAN"),
		    c:getNodeValueSafe(path .. ".cost", "1000") - 0,
		    "down",
		    "LINKDOWN")
	    elseif (subtype == "hw") then
		local dev = c:getNodeValueSafe(path .. ".device", "wan0");
		local dhcp = c:getNode(path .. ".dhcp");
		local dhcpenabled = 0;
		if dhcp and dhcp:attr("enable") == "true" then
			dhcpenabled = true;
		end

		os.execute(string.format("ifconfig %s up", dev));
		r.route:f(sub,
		    c:getNodeValueSafe(path .. ".group", "WAN"),
		    c:getNodeValueSafe(path .. ".cost", "1000") - 0,
		    "up",
		    "LINKDOWN")

    		local query = "cmd=event";
        	query = query .. "&state=up";
        	query = query .. "&iface=" .. 	urlEncode(dev);
        	query = query .. "&gw=" .. 	urlEncode(c:getNode(path .. ".gateway"):value());
        	query = query .. "&ip=" .. 	urlEncode(c:getNode(path .. ".ipaddr"):value());
        	query = query .. "&netmask=" .. urlEncode(c:getNode(path .. ".ipaddr"):value());
    		query = query .. "&dns1=" .. 	urlEncode(c:getNode(path .. ".dns1"):value());
    		query = query .. "&dns2=" .. 	urlEncode(c:getNode(path .. ".dns2"):value());

		-- Call collector, to let him know about static config, and assign route+dns's
		cmdline = "fetch -qo - \"" .. eventrelay .. "?" .. query .. "\"";
    		print("Exec: " .. cmdline);
    		os.execute(cmdline);

		-- Config static first
		local ip = c:getNode(path .. ".ipaddr"):value();
		local dhcp = c:getNode(path .. ".dhcp");
		print("Run: \"ifconfig " .. dev .. " " .. ip .. "\"");
		if os.execute(string.format("ifconfig %s %s", dev, ip)) ~= 0 then
		    print("\"ifconfig " .. dev .. " " .. ip .. "\" - failed");
		end

		run_dhclient(c, dev); -- if DHCP enabled.

		-- XXX temporary FIX for wan0 with static address
		-- if not dhcpenabled and dev == "wan0" then
		    os.execute(string.format("route add default %s",
			c:getNode(path .. ".gateway"):value()));
		-- end


		local nat = c:getNode(path .. ".nat");
		if nat and nat:attr("enable") == "true" then
		    local ip = c:getNode(path .. ".ipaddr"):value();
		    ip = ip:gsub("/%d+", "");
		    os.execute("ngctl mkpeer ipfw: nat 60 out");
		    os.execute("ngctl name ipfw:60 wan0nat");
		    os.execute("ngctl connect ipfw: wan0nat: 61 in");
		    os.execute("ngctl msg wan0nat: setaliasaddr " .. ip);

		    if dhcpenabled then
			-- pass DHCP via iface
			os.execute("ipfw add 96 allow all from any 67 to any 68 out via " .. dev);
			os.execute("ipfw add 97 allow all from any 68 to any 67 out via " .. dev);
		    end
		    os.execute("ipfw add 98 netgraph 61 all from any to any in via " .. dev);
		    os.execute("ipfw add 99 netgraph 60 all from any to any out via " .. dev);
		else
		    if dhcpenabled then
			os.execute("ipfw delete 96");
			os.execute("ipfw delete 97");
		    end
		    os.execute("ipfw delete 98");
		    os.execute("ipfw delete 99");
		end
	    else
		print("Unsupported interface type " .. subtype);
	    end
	else
	    print("Skiping disabled iface " .. path);
	end
    end
end

dhcpd = 0;
function start_dhcpd(c)
    if c:getNode("dhcpd.instances.instance[1]"):attr("enable") == "true" then
	if dhcpd == 0 then
	    dhcpd = DHCPD:new(c, "dhcpd.instances.instance[1]");
	end
	dhcpd:make_conf();
	dhcpd:write();
	print("Start DHCPD");
	dhcpd:run();
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

function update_board_info(c)
    for _, oid in pairs({"device.vendor", "device.model", "device.revision", "soc.vendor", "soc.model"}) do
	n = c:getOrCreateNode("info." .. oid);
	n:value(sysctl("hw." .. oid));
	if not n:value() then
	    n:value("__NO_DATA__");
	end
	print(tostring(oid) .. "=" .. n:value());
    end
end

function safeValue(c, name, default)
	node = c:getNode(name);
	if not node then
		if default then
			return default;
		else
			return "";
		end
	end
	return node:value();
end

function start_dnsmasq(c)
	-- TODO: if DNS-Relay enabled
	-- XXX: dnsmasq able to do DHCPD also
	-- dhcp-range=[interface:<inter-face>,][tag:<tag>[,tag:<tag>],][set:<tag],]<start-addr>,<end-addr>[,<netmask>[,<broadcast>]][,<lease time>]
	-- # --domain=zrouter,192.168.0.100,192.168.0.200
	-- # --dhcp-range=192.168.0.100,192.168.0.200,1h
	-- # --dhcp-authoritative - don't wait for other DHCPDs
	-- # --bogus-priv - NXDOMAIN for private nets
	local_domain = "";
	local_dhcp_range = "";
	if c:getNode("dhcpd.instances.instance[1]"):attr("enable") == "true" then

		dhcproot = "dhcpd.instances.instance[1].";
		domain = c:getNodeValueSafe(dhcproot .. "domain", "zrouter");
		dltime = c:getNodeValueSafe(dhcproot .. "default-lease-time", 3600);
		mltime = c:getNodeValueSafe(dhcproot .. "max-lease-time");
		ranges = c:getNodeValueSafe(dhcproot .. "range.start");
		rangee = c:getNodeValueSafe(dhcproot .. "range.end");

		local_domain = " --domain=" .. domain .."," .. ranges .. "," .. rangee;
		local_dhcp_range = " --dhcp-range=" .. ranges .. "," .. rangee .. "," .. dltime;

	end
	dnsmasq_cmd =
	    "/sbin/dnsmasq" ..
	    " -i bridge0" ..
	    local_domain ..
	    local_dhcp_range ..
	    " --dhcp-authoritative" ..
	    " --bogus-priv";

	print("Start " .. dnsmasq_cmd);
	os.execute(dnsmasq_cmd);
end

function start_hostapd(c)
	if c:getNode("hostapd.instance[1]"):attr("enable") == "true" then
		kldload({"wlan_xauth", "wlan_tkip", "wlan_ccmp"});
		aproot = "hostapd.instance[1].";
		driver = "bsd";
		--        <ieee80211d>1</ieee80211d>
		--        <country_code>UA</country_code>
		channel = c:getNodeValueSafe(aproot .. "channel", 6);
		country_code = c:getNodeValueSafe(aproot .. "country_code", "UA");
		--        <interface>wlan0</interface>
		interface = c:getNodeValueSafe(aproot .. "interface", "wlan0");

		commandline = string.format("ifconfig %s down",interface);
		if os.execute(commandline) ~= 0 then
		    print("Exec: " .. commandline .. " - FAILED");
		end
		commandline = string.format(
		    "ifconfig %s country %s channel %s up",
		    interface, country_code, channel);
		if os.execute(commandline) ~= 0 then
		    print("Exec: " .. commandline .. " - FAILED");
		end
		--        <macaddr_acl>0</macaddr_acl>
		--        <auth_algs>1</auth_algs>
		--        <debug>0</debug>
		--        <hw_mode>g</hw_mode>
		--        <ctrl_interface>/var/run/hostapd</ctrl_interface>
		--        <ctrl_interface_group>wheel</ctrl_interface_group>
		--        <ssid>zrouter</ssid>
		ssid = c:getNodeValueSafe(aproot .. "ssid", "zrouter");
		--        <!-- Open -->
		--        <wpa>0</wpa>
		wpa = c:getNodeValueSafe(aproot .. "wpa", 3);
		--        <!-- WPA -->
		--        <!-- <wpa>1</wpa> -->
		--        <!-- RSN/WPA2 -->
		-- <!-- <wpa>2</wpa> -->
    		-- <wpa_pairwise>CCMP TKIP</wpa_pairwise>
		wpa_key_mgmt = c:getNodeValueSafe(aproot .. "wpa_key_mgmt", "WPA-PSK");
		wpa_passphrase = c:getNodeValueSafe(aproot .. "wpa_passphrase", "freebsdmall");
		wpa_pairwise = c:getNodeValueSafe(aproot .. "wpa_pairwise", "CCMP");
		ctrl_interface = "/var/run/hostapd";

		-- # TARGET
		-- interface=wlan0
		-- driver=bsd
		-- ssid=CACHEBOY_11N_1
		-- wpa=3
		-- wpa_key_mgmt=WPA-PSK
		-- wpa_passphrase=Sysinit891234
		-- wpa_pairwise=CCMP
		-- ctrl_interface=/var/run/hostapd
		hostapd_conf = "/tmp/hostapd." .. interface ..".conf";

		hostapd_conf_data = 
		    "interface=" ..		interface .. "\n" ..
		    "driver=" ..		driver .. "\n" ..
		    "country_code=" .. 		country_code .. "\n" ..
		    "channel=" ..		channel .. "\n" ..
		    "ssid=" ..			ssid .. "\n" ..
		    "wpa=" ..			wpa .. "\n" ..
		    "wpa_key_mgmt=" ..		wpa_key_mgmt .. "\n" ..
		    "wpa_passphrase=" ..	wpa_passphrase .. "\n" ..
		    "wpa_pairwise=" ..		wpa_pairwise .. "\n" ..
		    "ctrl_interface=" ..	ctrl_interface .. "\n";

		local f = assert(io.open(hostapd_conf, "w"));
		f:write(hostapd_conf_data);
		f:close();

		hostapd_cmd = "/usr/sbin/hostapd -B " .. hostapd_conf;

		print("Start " .. hostapd_cmd);
		os.execute(hostapd_cmd);

	end
end

function start_igmp_fwd(c)
	if not c:getNode("igmp.instance[1]") then
		return;
	end
	if c:getNode("igmp.instance[1]"):attr("enable") == "true" then
		upif   = c:getNodeValueSafe(aproot .. "up", "wan0");
		downif = c:getNodeValueSafe(aproot .. "down", "lan0");

		cmd = "/etc/rc.d/ng_igmpproxy start " .. upif .. " " .. downif;

		print("Start " .. cmd);
		os.execute(cmd);
	end
end

-- Globals
sys = {};	-- Full tree of all params
config = {};	-- Hold HTTPD server params
c = {}; 	-- XML tree from config.xml
r = {};		-- Runtime varibles structure

sys.r = r;
sys.c = c;
sys.config = config;

r.routes = {};
r.routes["default"] = "127.0.0.1";
r.routes["224.0.0.0/4"] = "-iface bridge0"
r.tasks = {};
r.tasks.step = 5; -- Seconds
r.tasks.periodic = {};
r.tasks.onetime  = {}; -- At some time
r.tasks.countdown= {}; -- when counter expired
r.ver = {};
eventrelay = exec_output("kenv -q EVENTRELAY");


opts = {};
opts["-P"] = "/var/run/httpd.pid";

if arg then
    opts = getopt(arg, opts);
end

-- Check pidfile
dofile("lib/pidfile.lua");
pidfile(opts["-P"]);


print("Parse config ...");
c = Conf:new(load_file("config.xml"));

update_board_info(c);
read_zrouter_version(r);
read_rc_conf(r);

print("Initialize board ...");

print("Initialize route select logic ...");
r.route = ROUTE:new(c, 0);

print("Enable LAN ports ...");
os.execute("/etc/rc.d/switchctl enablelan");
print("Init LAN ...");
configure_lan(c);

print("Start DHCP/DNS Relay ...");
start_dnsmasq(c);
-- start_dhcpd(c);

-- print("Init AP");
-- ap = HOSTAPD:new(c, 1);
-- ap:run();

print("Init AP ...");
start_hostapd(c);

print("Init IGMP/Multicast forwarding ...");
start_igmp_fwd(c);

os.execute("ipfw add 100 allow ip from any to any via lo0");
-- os.execute("ipfw add 200 deny ip from any to 127.0.0.0/8");
-- os.execute("ipfw add 300 deny ip from 127.0.0.0/8 to any");

-- Hide Web-UI from WAN links
os.execute("ipfw add 400 allow tcp from any to me 80 via lan0");
os.execute("ipfw add 500 allow tcp from any to me 80 via bridge0");
-- TODO: If not enabled WAN administration
os.execute("ipfw add 600 deny tcp from any to me 80");
-- check w/ ipfw NAT-ed packets
sysctl("net.inet.ip.forwarding", 1);
sysctl("net.inet.ip.fastforwarding", 1);
sysctl("net.inet.tcp.blackhole", 2);
sysctl("net.inet.udp.blackhole", 0);
sysctl("net.inet.icmp.drop_redirect", 1);
sysctl("net.inet.icmp.log_redirect", 0);
sysctl("net.inet.ip.redirect", 0);
sysctl("net.inet.ip.sourceroute", 0);
sysctl("net.inet.ip.accept_sourceroute", 0);
sysctl("net.inet.icmp.bmcastecho", 0);
sysctl("net.inet.icmp.maskrepl", 0);
sysctl("net.link.ether.inet.max_age", 30);
sysctl("net.inet.ip.ttl", 226);
sysctl("net.inet.tcp.drop_synfin", 1);
sysctl("net.inet.tcp.syncookies", 1);
-- sysctl("kern.ipc.somaxconn", 32768);
-- sysctl("kern.maxfiles", 204800);
-- sysctl("kern.maxfilesperproc", 200000);
-- ??? -- sysctl("kern.ipc.nmbclusters", 524288);
-- sysctl("kern.ipc.maxsockbuf", 2097152);
sysctl("kern.random.sys.harvest.ethernet", 0);
sysctl("kern.random.sys.harvest.interrupt", 0);
-- sysctl("net.inet.ip.dummynet.io_fast", 1);
-- sysctl("net.inet.ip.dummynet.max_chain_len", 2048);
-- sysctl("net.inet.ip.dummynet.hash_size", 65535);
-- sysctl("net.inet.ip.dummynet.pipe_slot_limit", 2048);
-- ?? -- sysctl("net.inet.carp.preempt", 1);
-- ?? -- sysctl("net.inet.carp.log", 2);
-- ?? -- sysctl("kern.ipc.shmmax", 67108864);
sysctl("net.inet.ip.intr_queue_maxlen", 8192);
sysctl("net.inet.ip.fw.one_pass", 0);
sysctl("net.inet.ip.portrange.randomized", 0);
sysctl("net.inet.tcp.nolocaltimewait", 1);

racoon = 0;
if racoon == 0 then
	racoon = RACOON:new(c);
end

-- WAN links tasks
table.insert(r.tasks.countdown, { count=2, task=
	function()
	    print("Init WAN links ...");
	    configure_wan(c);
	    return (true);
	end
    });
-- IPSec links tasks
table.insert(r.tasks.countdown, { count=10, task=
	function()
	    print("Run racoon ...");
	    racoon:run();
	    return (true);
	end
    });

print("Run server ...");
start_server( c, config );





