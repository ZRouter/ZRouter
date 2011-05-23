--[[
option domain-name "fugue.com";
option domain-name-servers toccata.fugue.com;

option subnet-mask 255.255.255.224;
default-lease-time 600;
max-lease-time 7200;


subnet 192.5.5.0 netmask 255.255.255.224 {
  range 192.5.5.26 192.5.5.30;
  option name-servers bb.home.vix.com, gw.home.vix.com;
  option domain-name "vix.com";
  option routers 192.5.5.1;
  option subnet-mask 255.255.255.224;
  option broadcast-address 192.5.5.31;
  default-lease-time 600;
  max-lease-time 7200;
}

host fantasia {
  hardware ethernet 08:00:07:26:c0:a5;
  fixed-address fantasia.fugue.com;
}
]]

require("ipcalc");

DHCPD = {};
local mt = {};

--DHCPD.instances.instance[1]


function DHCPD:new(c, path, debug)
    t = {};
    t.path = path;
    t.c = c;
    t.iface = c:getNode(path .. "." .. "interface"):value()
    t.configfile = "/tmp/dhcpd_" .. t.iface .. ".conf";
    t.pidfile = "/tmp/dhcpd_" .. t.iface .. ".pid";
    t.debug = debug;
    return setmetatable(t, mt);
end

function DHCPD:make_conf()

    self.get  = function (var      ) return self.c:getNode(self.path .. "." .. var):value()    end;
    self.attr = function (var, attr) return self.c:getNode(self.path .. "." .. var):attr(attr) end;
    self.conf_data = "";

    local function a(txt)
	if not txt then
	    return (nil);
	end
	self.conf_data = self.conf_data .. txt .. "\n";
	if self.debug then print(txt); end
    end




    local function perhost(name, macaddr, ip)
	return  "host " .. name .." {\n\thardware ethernet " .. macaddr ..
	    ";\n\tfixed-address " .. ip ..";\n}\n";
    end
--    for i = 1,16,1 do
--	-- TODO: static entrys
--    end

    local if_ipaddr = self.c:getNode("interfaces." .. self.iface .. ".ipaddr"):value();
    local r = cidr_to_net(if_ipaddr);
    a("option domain-name \"".. self.get("domain") .."\";");
    a("option domain-name-servers " .. r.ip ..";");
    a("default-lease-time " .. self.get("default-lease-time") ..";");
    a("max-lease-time ".. self.get("max-lease-time") ..";");
    a("subnet " .. r.net .." netmask " .. r.mask .." {");
    a("\trange " .. self.get("range.start") .. " " .. self.get("range.end") ..";");
    a("\toption routers " .. r.ip ..";");
    a("\toption broadcast-address " .. r.broadcast ..";");
    a("}");

end

function DHCPD:write()

    local f = assert(io.open(self.configfile, "w"));
    f:write(self.conf_data);
    f:close();
end

function DHCPD:run()

    os.execute("mkdir -p /var/run/");
    os.execute("mkdir -p /var/db/");
    os.execute("touch /var/db/dhcpd.leases");
    os.execute("/sbin/dhcpd -lf /var/db/dhcpd.leases -cf " .. self.configfile .. " " .. self.iface);
end

function DHCPD:stop()

    os.execute("kill `cat /var/run/dhcpd.pid`");
end

mt.__index = DHCPD;

