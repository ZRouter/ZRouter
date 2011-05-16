-----------------------------------
----------- MPD Class ------------
-----------------------------------
--[[

create bundle static B1
bundle B1

set bundle links L1

set iface addrs 1.1.1.1 2.2.2.2
set iface route default
set iface enable on-demand
set iface idle 900
set iface enable nat

set ipcp ranges 0.0.0.0/0 0.0.0.0/0
set ipcp enable req-pri-dns
set ipcp enable req-sec-dns

open iface



create link static L1 modem
create link static L1 pppoe
create link static L1 pptp

set auth authname adbrd****@dsl.ukrtel.net
set auth password *******

set modem device /dev/cuaU0.0
set modem var $DialPrefix "DT"
set modem var $Telephone "#777"
set modem script DialPeer
set modem idle-script Ringback

set pppoe iface nfe0
set pppoe service ""

set pptp self 10.1.3.3
set pptp peer 10.1.0.1
set pptp disable windowing

set link disable chap pap
set link accept chap pap
set link action bundle B1
set link enable incoming
set link max-redial 0
set link mtu 1460
set link keep-alive 10 60
set link bandwidth 14194304

open


]]


MPD = {};
mt = {};

function MPD:new(tree, s)
    return setmetatable({ c = tree, socket = s }, mt)
end

function MPD:msg(s, timeout)
--    print(tostring(self.socket));
    print("DEBUG MPD:msg: " .. s);
    self.socket:write(s .. "\n");

--    return (true);
--    return print(self.socket:read());
    return (self.socket:read(timeout));
end

function MPD:config_bundle(path, bundle)
    local node;
    -- Create or select bundle
    self:msg("create bundle static " .. bundle);
    self:msg("bundle " .. bundle);

--    self:msg("set bundle links L1");

    -- TODO
--    local ip = c:getNode(path .. "." .. "ipaddr")
--    if ip then 
--	string.match(ip:value(), "%d+%.%d+%.%d+%.%d+%/(%d+)")
--    end

    self:msg("set iface addrs 10.254.254.1 10.254.254.2");
    self:msg("set ipcp ranges 0.0.0.0/0 0.0.0.0/0");

    -- We should apply default on a up-script run
--     self:msg("set iface route default");

    local dod = self.c:getNode(path .. "." .. "on-demand")
    if dod and dod:value() then
	self:msg("set iface enable on-demand");
        self:msg("set iface idle " .. self.c:getNode(path .. "." .. "idle-time"):value());
    end


    node = self.c:getNode(path .. "." .. "nat")
    if node and node:value() == "true" then
	self:msg("set iface enable nat");
    end

    self:msg("set ipcp enable req-pri-dns");
    self:msg("set ipcp enable req-sec-dns");

    self:msg("open iface");


    return (true);
end

function MPD:config_link(path, link, bundle)

    local mpdtype = self.c:getNode(path);
    if mpdtype and mpdtype:attr("type") then
	mpdtype = mpdtype:attr("type");
    else
	return (nil);
    end

    self:msg("create link static " .. link .. " " .. mpdtype);
    self:msg("link " .. link);

    local user = self.c:getNode(path .. "." .. "username");
    if user and user:value() and string.len(user:value()) > 0 then
	self:msg("set auth authname " .. user:value());
	local passwd = self.c:getNode(path .. "." .. "username");
	if passwd and passwd:value() then
	    self:msg("set auth password \"" .. passwd:value() .. "\"");
	else
	    self:msg("set auth password \"\"");
	end
    end

    if mpdtype == "modem" then
	self:msg("set modem device " .. self.c:getNode(path .. "." .. "device"):value() .. "");
	self:msg("set modem var $DialPrefix \"DT\"");
	self:msg("set modem var $Telephone \"" .. self.c:getNode(path .. "." .. "phone"):value() .. "\"");
	self:msg("set modem script DialPeer");
	self:msg("set modem idle-script Ringback");
    elseif mpdtype == "pppoe" then
	self:msg("set pppoe iface " .. self.c:getNode(path .. "." .. "interface"):value() .. "");
	self:msg("set pppoe service \"" .. self.c:getNode(path .. "." .. "service"):value() .. "\"");
    elseif mpdtype == "pptp" then
	-- TODO: 
	self:msg("set pptp self 10.1.3.3");
	self:msg("set pptp peer 10.1.0.1");
	self:msg("set pptp disable windowing");
    end


    -- TODO
    self:msg("set link disable chap pap");
    self:msg("set link accept chap pap");
    self:msg("set link action bundle " .. bundle);
    self:msg("set link enable incoming");
    self:msg("set link max-redial 0");
    self:msg("set link mtu 1460");
    self:msg("set link keep-alive 10 60");
--    self:msg("set link bandwidth 14194304");

    -- Open link
    self:msg("open");

    self:msg("bundle " .. bundle);
    print(self:msg("show bundle " .. bundle, 1));


    return (true);
end

mt.__index = MPD;
-----------------------------------
-------- End of MPD Class --------
-----------------------------------

--[[
Bundle 'PPP' (static):
	Links          : PPP[Opened/UP] 
	Status         : OPEN
	Session time   : 1119 seconds
	MultiSession Id: 249-PPP
	Total bandwidth: 28800 bits/sec
	Avail bandwidth: 28800 bits/sec
	Peer authname  : ""
Configuration:
	Retry timeout  : 2 seconds
	BW-manage:
	  Period       : 60 seconds
	  Low mark     : 20%
	  High mark    : 80%
	  Min conn     : 30 seconds
	  Min disc     : 90 seconds
	  Links        :                 
Bundle level options:
	ipcp      	enable
	ipv6cp    	disable
	compression	disable
	encryption	disable
	crypt-reqd	disable
	bw-manage 	disable
	round-robin	disable
Multilink PPP:
	Status         : Inactive
Traffic stats:
	Input octets   : 1322
	Input frames   : 19
	Output octets  : 1270
	Output frames  : 18
	Bad protocols  : 0
	Runts          : 0
	Dup fragments  : 0
	Drop fragments : 0
]]
