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
--package.cpath = "/usr/lib/?.so";
package.cpath = 
	"/lib/?.so;/usr/lib/?.so;/usr/lib/lua/?.so;" ..
	"/lib/?-core.so;/usr/lib/?-core.so;/usr/lib/lua/?-core.so;" ..
	"/lib/?/core.so;/usr/lib/?/core.so;/usr/lib/lua/?/core.so";

io.stdout = assert(io.open("/dev/console", "w"));
io.stderr = io.stdout;

socket = require( "libhttpd" );

-- Expat binding
dofile('lib/xml.lua');
-- XML entity handlers
dofile('lib/handler.lua');

-- read_file, tdump, xmldump
dofile('lib/utils.lua');
-- Conf object
dofile('lib/conf.lua');
-- Node object
dofile('lib/node.lua');
-- base64 decode/encode
dofile('lib/base64.lua');
-- Socket helper
dofile("lib/sock.lua");
-- MPD helper
dofile("lib/mpd.lua");
-- DHCPD helper
dofile("lib/dhcpd.lua");
-- HOSTAPD helper
dofile("lib/hostapd.lua");


print( "\n\nLoaded the socket library, version: \n  " .. socket.version );


--
--  A table of MIME types.  
--  These values will be loaded from /etc/mime.types if available, otherwise
-- a minimum set of defaults will be used.
--
mime = {};




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
    --
    config.listener = socket.bind( c:getNode("http.port"):value() );

    if ( config.listener == nil ) then
       error("Error in bind()");
    end

    --
    --   Print some status messages.
    -- 
    print( "\nListening upon:" );
    print( "  http://" .. c:getNode("http.host"):value() .. ":" .. c:getNode("http.port"):value() .. "/" );
    print( "\n\n");


    --
    --  Loop accepting requests.
    --
    while ( running == 1 ) do
        processConnection( c, config );
    end


    --
    -- Finished.
    --
    socket.close( config.listener );
end




--
--  Process a single incoming connection.
--
function processConnection( c, config ) 
    --
    --  Accept a new connection
    --
    client,ip = socket.accept( config.listener );

    found   = 0;  -- Found the end of the HTTP headers?
    chunk   = 0;  -- Count of data read from client socket
    size    = 0;  -- Total size of incoming request.
    code    = 0;  -- Status code we send to the client
    request = ""; -- Request body read from client.
    rq = {};
    rq.ip = ip;


    --
    --  Read in a response from the client, terminating at the first
    -- '\r\n\r\n' line - this is the end of the HTTP header request.
    --
    --  Also break out of this loop if we read ten packets of data
    -- from the client but didn't manage to find a HTTP header end.
    -- This should help protect us from DOSes.
    --
    while ( ( found == 0 ) and ( chunk < 10 ) ) do
        length, data = socket.read(client);
        if ( length < 1 ) then
            found = 1;
        end

        size    = size + length;
        request = request .. data;

        position,len = string.find( request, '\r\n\r\n' );
        if ( position ~= nil )  then
            rq.RequestBeader = string.sub(request, 0, position);
            rq.RequestBody = string.sub(request, position+4);
            rq.RequestBodyLength = len-4;
            found = 1;
        end

        chunk = chunk + 1;
    end


    --
    --  OK We now have a complete HTTP request of 'size' bytes long
    -- stored in 'request'.
    --
    
    --
    -- Find the requested path.
    --
    _, _, method, path, major, minor  = string.find(request, "([A-Z]+) (.+) HTTP/(%d).(%d)");

    --
    -- We only handle GET requests.
    --
    if ( method ~= "GET" ) and ( method ~= "POST" ) then
        error = "Method not implemented";

        if ( method == nil ) then
            error = error .. ".";
        else
            error = error .. ": " .. urlEncode( method );
        end

        err = sendError(501, error);
        size = string.len(err);
        socket.write(client, err)
        socket.close(client);
        return size, "501";
    end


    --
    -- Decode the requested path.
    --
    path = urlDecode( path );
--    print(request);


    --
    -- find the Virtual Host which we need for serving, and find the
    -- user agent and referer for logging purposes.
    --
    _, _, rq.Host           = string.find(request, "Host: ([^:\r\n]+)");
    _, _, rq.Agent          = string.find(request, "Agent: ([^\r\n]+)");
    _, _, rq.Referer        = string.find(request, "Referer: ([^\r\n]+)");

    _, _, rq.Accept         = string.find(request, "Accept: ([^:\r\n]+)");
    _, _, rq.AcceptLanguage = string.find(request, "Accept%-Language: ([^:\r\n]+)");
    _, _, rq.AcceptEncoding = string.find(request, "Accept%-Encoding: ([^:\r\n]+)");
    _, _, rq.AcceptCharset  = string.find(request, "Accept%-Charset: ([^:\r\n]+)");
    _, _, rq.ContentType    = string.find(request, "Content%-Type: ([^:\r\n]+)");
    _, _, rq.ContentLength  = string.find(request, "Content%-Length: ([^:\r\n]+)");
    _, _, rq.Authorization  = string.find(request, "Authorization: ([^:\r\n]+)");

    rq.UserName = nil;
    rq.Password = nil;
    rq.Group    = nil;
    if rq.Authorization then
        local BasicAuth = "";
        _, _, BasicAuth = string.find(request, "Basic ([^:\r\n]+)");
        if BasicAuth then
            local pair = b64dec(BasicAuth);
            _, _, _u, _p = string.find(pair, "(.-)%:(.*)");

            for usr = 1, 8, 1 do

                local u = c:getNode(string.format("http.users.user[%d]", usr));
                if (u:attr("username") == _u) and (u:attr("password") == _p) then
                    rq.UserName = _u;
                    rq.Password = _p;
                    rq.Group    = u:attr("group");
                end
            end
        end
    end

    if not rq.Group and rq.ip ~= "127.0.0.1" then
        local message = "HTTP/1.1 401 Authorization Required\r\n" ;
        message = message .. "Server: lua-httpd " .. socket.version .. "\r\n";
        message = message .. "WWW-Authenticate: Basic realm=\"Secure Area\"";
        message = message .. "Content-type: text/html\r\n";
        message = message .. "Connection: close\r\n\r\n" ;
        message = message .. "<html><head><title>Error</title></head>" ;
        message = message .. "<body><H1>401 Unauthorized.</H1></body></html>" ;
        --Content-Length: 311

        size = string.len(message);
        socket.write(client, message);
        socket.close(client);
        code = "401";
    else
        size, code = handleRequest( c, config, path, client, method, request, rq );
    end


    if ( rq.Agent   == nil ) then rq.Agent   = "-" end;
    if ( rq.Referer == nil ) then rq.Referer = "-" end;
    if ( code       == nil ) then code       = 0 ; end;

    --
    -- Log the access in something resembling the Apache common
    -- log format.
    -- 
    -- Note: The logging does not write the name of the virtual host.
    --
--    logAccess( rq.UserName or "-", method, rq.Host, ip, path, code, size, rq.Agent, rq.Referer, major, minor );


    if ( running == 0 ) then
        print( "Terminating as per request from " .. ip );
    end

    --
    --  Close the client connection.
    --
    socket.close( client );
end

--
--  Attempt to serve the given path to the given client
--
function handleRequest( c, config, path, client, method, request, rq )
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
        socket.write(client, err)
        socket.close(client);
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
    socket.write( client, "HTTP/1.0 200 OK\r\n" );
    socket.write( client, "Server: lua-httpd " .. socket.version .. "\r\n" );
    socket.write( client, "Content-type: " .. mimetype  .. "\r\n" );
    socket.write( client, "Connection: close\r\n\r\n" );


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
        t = string.gsub(t, "%$%$%$(.-)%$%$%$", function (s) if not s then return; end return evalembeded(s) end);

    -- Eval Lua code
    elseif (ext == "lua") or (ext == "xml") then
        code, err = assert(loadstring(t));
        if not code then
            t = sendError( 500, "Error \"".. err .."\" when parse " .. urlEncode(path));
            retcode = "500"
        else
            t = code();
        end
--        print(t or "(no output)", retcode or "");
    end
    socket.write( client, t );
    socket.close( client );

    return string.len(t), retcode;
end

function evalembeded(s)
    if not s then
        return "";
    end

    match, _, code = string.find(s, "^code%:(.*)$");
    if match then
--      print("Eval code \"" .. code .. "\"");
        local func, err = loadstring("return " .. code);
        if not func then
            return err;
        end
        return func();
    else
        return field(c:getNode(s):value());
    end
end

function exec_output(cmd)
        fp = io.popen(cmd, "r");
        data = fp:read("*a");
        fp:close();
        return data;
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
                                return exec_output("sysctl -n kern.ident");
                        end
                end
        end

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
    message = message .. "Server: lua-httpd " .. socket.version .. "\r\n";
    message = message .. "Content-type: text/html\r\n";
    message = message .. "Connection: close\r\n\r\n" ;
    message = message .. "<html><head><title>Error</title></head>" ;
    message = message .. "<body><h1>Error</h1>" ;
    message = message .. "<p>" .. str .. "</p></body></html>" ;

--    socket.write( client, message );
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
    print( "WARNING: /etc/mime.types could not be read." );
    print( "         Running with minimal MIME types." );

    -- TODO: lua scripts can be able to set content type
    mime[ "lua" ]  = "text/html";
    mime[ "xml" ]  = "application/xml";
    mime[ "html" ]  = "text/html";
    mime[ "txt"  ]  = "text/plain";
    mime[ "js"   ]  = "text/plain";
    mime[ "css"  ]  = "text/css";
    mime[ "jpg"  ]  = "image/jpeg";
    mime[ "jpeg" ]  = "image/jpeg";
    mime[ "gif"  ]  = "image/gif";
    mime[ "png"  ]  = "image/png";
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
	local s = sock:new(socket);
	s:open("127.0.0.1", 5005);
	mpd = MPD:new(c, s);
    end

    local bundle = "PPP";
    local path = "interfaces.wan0.PPP";

    mpd:config_bundle(path, bundle);
    mpd:config_link(path, link, bundle);

--    print(mpd:show_bundle(path, bundle));

--    print("--   --");

--    s:close();
end

function configure_wan(c)
    -- TODO: auto enumerate wan links
    local sub = "";
    for _, sub in ipairs({"Static", "PPPoE", "PPP"}) do
	print("sub=" .. sub);
	local path = "interfaces.wan0." .. sub;

	if c:getNode(path):attr("enable") == "true" then
	    local subtype = c:getNode(path):attr("type");
	    print("subtype=" .. subtype);
	    if
		(subtype == "l2tp") or
		(subtype == "pppoe") or
		(subtype == "pptp") then
		-- TODO: multilink have different link names
	    elseif (subtype == "modem") then
		os.execute("kldload umodem");
		os.execute("kldload u3g");
		os.execute("sleep 1");
		configure_mpd_link(c, path, sub, sub);
	    elseif (subtype == "hw") then
		local dhcp = c:getNode(path .. ".dhcp");
		local dev = c:getNode(path .. ".device"):value();

		-- Config static first
		local ip = c:getNode(path .. ".ipaddr"):value();
		print("Run: \"ifconfig " .. dev .. " " .. ip .. "\"");
		if os.execute(string.format("ifconfig %s %s", dev, ip)) ~= 0 then
		    print("\"ifconfig " .. dev .. " " .. ip .. "\" - fail");
		end

		if dhcp and dhcp:attr("enable") == "true" then
		    print(path .. ".dhcp:attr(enable)=" .. dhcp:attr("enable"));
		    -- Run dhclient
		    print("Run dhclient on " .. dev);
		    os.execute("mkdir /var/db/");
		    os.execute(string.format("/sbin/dhclient %s", dev));
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

--
--  Now start the server.
--
--

-- Globals 
config = {};	-- Unused now
c = {}; 	-- XML tree from config.xml
r = {};		-- Runtime varibles structure
r.routes = {};
r.routes["default"] = "127.0.0.1";
r.routes["224.0.0.0/4"] = "-iface bridge0"


print("Parse config ...");
c = Conf:new(load_file("config.xml"));

print("Run info collector ...");
-- Run it as background task
os.execute("/etc/www/collector.sh &");

print("Initialize board ...");

print("Init LAN");
configure_lan(c);

start_dhcpd(c);

print("Init AP");
ap = HOSTAPD:new(c, 1);
ap:run();

print("Init WAN links");
configure_wan(c);

print("Run server ...");
start_server( c, config );





