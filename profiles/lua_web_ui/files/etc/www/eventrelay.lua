#!/usr/bin/lua

package.path = "./?.lua;/etc/www/lib/?.lua;./lib/?.lua";
package.cpath =
	"/lib/?.so;/usr/lib/?.so;/usr/lib/lua/?.so;" ..
	"/lib/lua?.so;/usr/lib/lua?.so;/usr/lib/lua/lua?.so;" ..
	"/lib/?-core.so;/usr/lib/?-core.so;/usr/lib/lua/?-core.so;" ..
	"/lib/?/core.so;/usr/lib/?/core.so;/usr/lib/lua/?/core.so;";

-- URL encode/devcode funtions
dofile("lib/urlXxcode.lua");

-- redirect print to syslog
dofile("lib/lsyslog.lua");

-- read_file, tdump, xmldump, exec_output
dofile('lib/utils.lua');

function call_listener(http, listener, q)
	local query = listener .. "?" .. q;

	-- Pass query to syslog
	info(query);

	local body, code, headers = http.request(query);

	if code == 200 then
		return (true);
	end

	return (nil);
end

function getopt(args, opts)
    i=1;
    while i < table.getn(arg) do
	if arg[i]:match("^-") then
	    if arg[i] == "-l" then
		table.insert(listeners, arg[i+1]);
	    else
		    opts[arg[i]] = arg[i+1];
	    end
	    i = i + 1;
	end
	i = i + 1;
    end
    return (opts);
end

-- main()

syslog_init("eventrelay.lua");

host = os.getenv("EVENT_RELAY_HOST") or "127.0.0.1";
port = os.getenv("EVENT_RELAY_PORT") or "1";

listeners = {
	"http://127.0.0.1:80/event.xml",
	"http://127.0.0.1:8/event.xml",
};

opts = {};
opts["-P"] = "/var/run/event-relay.pid";

if arg then
    opts = getopt(arg, opts);
end

for k,v in ipairs(listeners) do
	print("Listener: " .. v);
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
	    			_, _, method, path, q, major, minor  =
	    			    string.find(data,
	    				"([A-Z]+) (.-)/%?(.-) HTTP/(%d).(%d)");
				if not query and q then
					query = q;
					break;
				end
    			else
    				break;
    			end
		end


		if query then
			assert(control:send(
			    "HTTP/1.0 200 OK\r\n" ..
			    "Server: simple-lua\r\n" ..
			    "Content-type: text/plain\r\n" ..
			    "Connection: close\r\n" ..
			    "\r\n" ..
			    "OK"
			));
		else
			assert(control:send(
			    "HTTP/1.0 400 Bad Request\r\n" ..
			    "Server: simple-lua\r\n" ..
			    "Content-type: text/plain\r\n" ..
			    "Connection: close\r\n" ..
			    "\r\n" ..
			    "FAIL"
			));
			err("Wrong event message received \"" .. tostring(data) .. "\"");
    		end

		assert(control:close());

		if query then
			for k,v in ipairs(listeners) do
				call_listener(http, v, query);
			end
		end
	end
end

os.exit(0);
