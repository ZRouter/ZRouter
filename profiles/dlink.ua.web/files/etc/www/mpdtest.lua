#!/usr/bin/lua


package.cpath = "/usr/lib/?.so";
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
-- Node object
dofile('lib/base64.lua');

-- Socket helper
dofile("lib/sock.lua");

-- MPD helper
dofile("lib/mpd.lua");


c = Conf:new(load_file("config.xml"));
print(tostring(c));
print(xmldump(c));

print( c:getNode("interfaces.wan0.PPPoE"):attr("type") );



local s = sock:new(socket);
s:open("127.0.0.1", 5005);


local mpd = MPD:new(c, s);

local bundle = "PPP";
local path = "interfaces.wan0.PPP";

mpd:config_bundle(path, bundle);
mpd:config_link(path, "PPP", bundle);

print("--   --");

s:close();


