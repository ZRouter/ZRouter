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
set iface up-script /etc/mpd5-up.sh
set iface down-script /etc/mpd5-down.sh

set ipcp ranges 0.0.0.0/0 0.0.0.0/0
set ipcp enable req-pri-dns
set ipcp enable req-sec-dns

open iface



create link static L1 modem
create link static L1 pppoe
create link static L1 pptp
create link static L1 ltp

set auth authname adbrd****@dsl.ukrtel.net
set auth password *******

set modem device /dev/cuaU0.0
set modem var $DialPrefix "DT"
set modem var $Telephone "#777"
set modem var $TryPPPEarly "yes"
set modem script DialPeer
set modem idle-script Ringback
set modem watch -cd

set pppoe iface nfe0
set pppoe service ""

set pptp self 10.1.3.3
set pptp peer 10.1.0.1
set pptp disable windowing

set l2tp peer 10.10.10.10

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

--require("sleep");

MPD = {};
mt = {};

function MPD:new(tree, host, port, debug)
	local t = {};
	if not host or not port then
		print("No host or port");
		return (nil);
	end

--	t.socket = socket.connect(host, port);
--	if not t.socket then
--		print("Can't connect to " .. host .. ":" .. port);
--		return (nil);
--	end
	t.host = host;
	t.port = port;
	t.c = tree;
	t.debug = 1;
	return setmetatable(t, mt);
end

function MPD:msg(s, wait)

    if self.debug then print("DEBUG MPD:msg: " .. s); end
    self.socket:settimeout(0.1);
    self.socket:send(s .. "\n");

    local err = nil;
    local ret = "";
--    while not err do
	local data, err = self.socket:receive("*a");
	ret = ret .. (data or "");
--    end
    if self.debug then print("DEBUG MPD:msg:return = [[" .. ret .. "]]"); end

    return (ret);
end

function MPD:config_bundle(path, bundle)
    local node;

    self.socket = socket.connect(self.host, self.port);
	if not self.socket then
		print("Can't connect to " .. self.host .. ":" .. self.port);
		return (nil);
	end
    -- Create or select bundle
    self:msg("create bundle static " .. bundle);
    self:msg("bundle " .. bundle);

    -- Multilink only
--    self:msg("set bundle links L1");

    -- TODO
--    local ip = c:getNode(path .. "." .. "ipaddr")
--    if ip then
--	string.match(ip:value(), "%d+%.%d+%.%d+%.%d+%/(%d+)")
--    end

    self:msg("set iface addrs 10.254.254.1 10.254.254.2");
    self:msg("set iface up-script /etc/mpd-linkup");
    self:msg("set iface down-script /etc/mpd-linkdown");
    self:msg("set ipcp ranges 0.0.0.0/0 0.0.0.0/0");

    -- We should apply default on a up-script run
--     self:msg("set iface route default");

    local dod = self.c:getNode(path .. "." .. "on-demand")
    if dod and dod:attr("enable") == "true" then
	self:msg("set iface enable on-demand");
        self:msg("set iface idle " .. self.c:getNode(path .. "." .. "idle-time"):value());
    end

    self:msg("set iface name " .. bundle);

    node = self.c:getNode(path .. "." .. "nat")
    if node and node:attr("enable") == "true" then
	self:msg("set iface enable nat");
    end

    self:msg("set ipcp enable req-pri-dns");
    self:msg("set ipcp enable req-sec-dns");

    self:msg("open iface");

    self:msg("exit");
    self.socket:close();

    return (true);
end

function MPD:config_link(path, link, bundle)

    self.socket = socket.connect(self.host, self.port);
	if not self.socket then
		print("Can't connect to " .. self.host .. ":" .. self.port);
		return (nil);
	end

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
	self:msg("set modem var $TryPPPEarly \"yes\"");
	self:msg("set modem var $Telephone \"" .. self.c:getNode(path .. "." .. "phone"):value() .. "\"");
	self:msg("set modem script DialPeer");
	self:msg("set modem idle-script Ringback");
	self:msg("set modem watch -cd");
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
    self:msg("exit");
    self.socket:close();



    return (true);
end

function MPD:show_bundle(path, bundle)
--    self.socket:refresh();
    self.socket = socket.connect(self.host, self.port);

    self:msg("bundle " .. bundle);
    local info = self:msg("show bundle " .. bundle, 500);
    self:msg("exit");
    self.socket:close();

    return (true);
end

function MPD:close()
--    self.socket:close();
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
