--[[


]]

ROUTE = {};
local mt = {};


function ROUTE:new(c, debug)
    t = {};
    t.c = c;

    t.ifaces = {};

    t.debug = debug;
    return setmetatable(t, mt);
end

--[[
NAME		STATUS	USEGW	USEDNS	FLAGS		COST	GROUP		LBWIDTH	LBGROUP
wan0		ACTIVE	--	--	KEEP		10000	""		0	--
wan0pppoe	ACTIVE	GW	DNS	LB|BACKUP|KEEP	10	"Internet"      100	WANLB
PPP0		DOWN	--	--	BACKUP		1000	"Internet"      0	--
PPP1		DOWN	--	--	LB|BACKUP|KEEP	10	"Internet"      5	WANLB
wan1		ACTIVE	--	--	NONE		10000	""		0	--
wan1pppoe	ACTIVE	--	--	BACKUP|KEEP	100	"Internet"      0	--
wan2		ACTIVE	--	--	BACKUP		10	"Foreign LAN"   0	--
IPSEC0		ACTIVE	--	--	BACKUP		100	"Foreign LAN"   0	--

BACKUP	- iface used as backup for iface group ${GROUP}
LB	- joined into LoadBalancing group

]]

--[[
Wan0 up
Wan0ppp down, disabled
Wan1 up

Event wan0 down
]]



function ROUTE:f(name, group, cost, state, linkstate)
	local iface = {};

	iface.name = name;
	if not group or group == "" then
		iface.group = "MAIN";
	else
		iface.group = group;
	end
	iface.cost = cost;
	iface.state = state;
	iface.keep = true; -- true for most cases, false for 3G link(default)
	if linkstate then
		iface.linkstate = linkstate;
	else
		iface.linkstate = "LINKDOWN";
	end

	iface.createtime = os.time();
	iface.enabletime = 0;

	iface.debug = {};
	iface.debug.updelay = 5;

	iface.isup =
		function (self)
			if self.state == "up" then
				return (true);
			end
			return (false);
		end;
	iface.islinkup =
		function (self)
			if self.enabletime == 0 then
				if self.linkstate == "LINKUP" then
					return (true);
				end
				return (false);
			end
			if os.time() < (self.enabletime + self.debug.updelay) then
				return (false);
			end
			return (true);
		end;
	iface.enable =
		function (self)
			print("Enabling iface " .. self.name);
			-- Init
			-- Start
			-- Apply config
			self.enabletime = os.time();
			self.state = "up";
			-- linkstate must be updated by LINKUP event
		end;
	iface.disable =
		function (self)
			print("Disabling iface " .. self.name);
			-- Stop
			-- Clear
			self.state = "down";
			self.linkstate = "LINKDOWN";
			self.enabletime = 0;
		end;
	iface.show =
		function (self)
			print(self.name .. " (" .. self.group .. ", " ..  self.cost .. "), ",
			    "State: " .. self.state, " Linked: " .. tostring(self:islinkup()));
		end;
	iface.updatelinkstate =
		function (self, lstate)
			-- TODO: Filter lstates

			-- Ignore linkup on disabled ifaces
			if self.state ~= "up" and lstate == "LINKUP" then
				return;
			end;

			self.linkstate = lstate;
			-- Handle link state update
		end
	iface.wait =
		function (self, state, max)
			local count = 0;
			if state == "LINKUP" then
				-- Rewrite to match return value of islinkup
				state = true;
			elseif state == "LINKDOWN" then
				state = false;
			else
				return nil;
			end
			while true do
				if self:islinkup() == state then
					break;
				end
				if max and count > max then
					print("Timout");
					return (nil);
				end
				os.execute("sleep 1");
				count = count + 1;
			end
			return (true);
		end

	table.insert(self.ifaces, iface);
	return (0);
end

function ROUTE:dump()
	print("----------------------------------------------------------------------------");

	for k, i in ipairs(self.ifaces) do
		i:show();
	end
	print("");
end

function ROUTE:find_iface(name)
	for k,i in ipairs(self.ifaces) do
		print("Find", i.name, name);
		if i.name == name then
			return (i);
		end
	end
	return (nil);
end

function ROUTE:group_list(group)
	local list = {};
	local k,i;

	for k,i in ipairs(self.ifaces) do
		if i.group == group then
			table.insert(list, i);
		end
	end

	return (list);
end

function ROUTE:group_sort(list)

	function link2cost(a)
		-- iface in link goes before other with equal cost
		if (a.linkstate == "LINKUP") then
			return (a.cost * 2);
		end
		return ((a.cost * 2) + 1);
	end

	table.sort(list,
	        function (a1, a2)
	                return (link2cost(a1) < link2cost(a2));
	        end);
end

function ROUTE:maintain_ifgroup(iface)
	local list, selected = false;

	list = self:group_list(iface.group);
	self:group_sort(list);

	for n,i in pairs(list) do
		print(i.name, i.cost, i.linkstate)
		-- If we have already best iface
		if selected then
			-- If not best and not keep
			if i:isup() and not i.keep then
				-- disable iface
				i:disable();
			end
		end
		if i:isup() and i:islinkup() and not selected then
			print(i.name .. " is selected");
			selected = i;
		else
			if not selected and not i:isup() then
				-- Enable PPP(3G) link
				i:enable();
				-- Don't touch selected until PPP went up
				-- Maybe we have running iface with greater cost
			end

		end
	end
end

function ROUTE:event(iface, t)
	local i = self:find_iface(iface);
	if not i then
		return (nil);
	end

	if t == "up" then
		i:updatelinkstate("LINKUP");
	end
	if t == "down" then
		i:updatelinkstate("LINKDOWN");
	end
	--[[ other events? ]]

	self:maintain_ifgroup(i);
end

mt.__index = ROUTE;
