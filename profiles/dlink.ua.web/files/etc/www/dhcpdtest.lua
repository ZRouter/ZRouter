#!/usr/bin/lua


package.cpath = "/usr/lib/?.so";
package.path = "./?.lua;./lib/?.lua;/etc/www/lib/?.lua";

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

-- DHCPD helper
dofile("lib/dhcpd.lua");


c = Conf:new(load_file("config.xml"));
print(tostring(c));
print(xmldump(c));



d = dhcpd:new(c, "dhcpd.instances.instance[1]");

d:make_conf();

print(d.conf_data);

