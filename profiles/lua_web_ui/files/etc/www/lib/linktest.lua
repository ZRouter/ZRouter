LinkTest = {};
mtLinkTest = {};

function LinkTest:new(name)
    ltest = {};

    ltest.name = name;

    return (setmetatable(ltest, mtLinkTest));
end

function LinkTest:NXDomainInit()
	self.dns = require "dns";
	self.ns = {};
	self.iface = "wan0"; -- XXX: test only
--	table.insert(self.ns, "192.168.10.10");
	table.insert(self.ns, "192.168.10.15");

	function setup_ns_route(ns, iface)
		local routecmd = string.format(
		    "/sbin/route add %s -iface %s", ns, iface);
		print("LinkTest: exec", routecmd);
		os.execute(routecmd);
	end

	for i,ns in ipairs(self.ns) do
		setup_ns_route(ns, self.iface);
	end
end

function LinkTest:NXDomain()
	local function query(dns, server, host)
		local e = dns.encode {
  			id       = math.random(),
  			query    = true,
  			rd       = true,
  			opcode   = 'query',
  			question = {
  				name  = host,
  				type  = 'a',
  				class = 'in'
  			}
  		}

		local r,err = dns.query(server, e);

		if r == nil then
			print("error:",err)
			return nil
		end

		return dns.decode(r)
	end
	for i,ns in ipairs(self.ns) do
		local nxhost = os.time() .. "nxdom.com.";
		print("LinkTest: Ask " .. ns .. " for \"" .. nxhost ..
		    "\" via " .. self.iface);
		local result = query(self.dns, ns, nxhost);
		if (result.rcode == 3) then
			return (true);
		end
	end

	return (false);
end

mtLinkTest.__index = LinkTest;

--[[
How to test:

lt = LinkTest:new("test");
lt:NXDomainInit();
lt:NXDomain();
]]
