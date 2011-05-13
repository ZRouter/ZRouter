-----------------------------------
----------- Node Class ------------
-----------------------------------
sock = {};
mt = {};

function sock:new(s)
    local s = setmetatable({ socket = s or require( "libhttpd" ) }, mt);
    return (s);
end

function sock:open(host, port)
    if type(port) ~= "number" then
	port = tonumber(port);
    end

    print(string.format("open: %s:%d", host, port));
    self.port = port;
    self.host = host;

    self.sock = self.socket.connect( host, port );
    if ( self.sock == nil ) then
	error( "Socket failed to connect" );
    end
end

function sock:write(msg)
    return self.socket.write( self.sock, msg );
end

function sock:read()
    local line     = "";
    local response = "";
    local len;
    local length = 0;

    repeat
	len,line = self.socket.read( self.sock );
	if len then length = length + len; end
	response = response .. line;
    until len <= 0;

    return response, length;
end

function sock:drain()
    local len;
    repeat
	len = self.socket.read( self.sock );
    until len <= 0;
end

function sock:close()
    return self.socket.close( self.sock );
end

function sock:refresh()
    self:close();
    self:open(self.host, self.port);
end

mt.__index = sock;
-----------------------------------
-------- End of Node Class --------
-----------------------------------

-- Socket Pool --
--[[
    function spadd
    args:
	s	- socket pool table
	h	- hostname for new socket
	p	- port for new socket
    return:
	s	- updated socket pool table
	name	- name of new socket ("host:port")
]]
function spadd(h, p, s)
    if not s then
	s = {};
    end

    local sock = sock:new(socket);
    local name = string.format("%s:%s", h, tostring(p));
    sock:open(h, p);
    s[name] = sock;

    return s, name;
end


