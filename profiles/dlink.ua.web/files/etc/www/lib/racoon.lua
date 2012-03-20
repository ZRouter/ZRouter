--[[

file /etc/racoon/racoon.conf:
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
path    pidfile  "/var/run/racoon.pid";
path    pre_shared_key  "/var/db/racoon/racoon_psk.txt"; #location of pre-shared key file
log     notify;  #log verbosity setting: set to 'notify' when testing and debugging is complete

padding # options are not to be changed
{
        maximum_length  20;
        randomize       off;
        strict_check    off;
        exclusive_tail  off;
}

timer   # timing options. change as needed
{
        counter         5;
        interval        20 sec;
        persend         1;
#       natt_keepalive  15 sec;
        phase1          30 sec;
        phase2          15 sec;
}

listen  # address [port] that racoon will listening on
{
        isakmp          192.168.90.1 [500];
        isakmp_natt     192.168.90.1 [4500];
}

include /var/run/racoon.links.conf
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

Generated /var/run/racoon.links.conf:
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include /var/run/racoon.10.0.0.2:500.conf
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

Generated /var/run/racoon.10.0.0.2:500.conf:
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

remote  10.0.0.2 [500]
{
        exchange_mode   	main,aggressive;
        doi             	ipsec_doi;
        situation       	identity_only;
        my_identifier   	address 10.0.0.1;
        peers_identifier        address 10.0.0.2;
        lifetime        	time 28800 sec;
	initial_contact 	on;
        passive         	off;
        proposal_check  	obey;
        generate_policy 	off;

                        proposal {
                                encryption_algorithm    3des;
                                hash_algorithm          md5;
                                authentication_method   pre_shared_key;
                                dh_group                5;
			}
}
sainfo  (address 192.168.0.0/24 any address 192.168.2.0/24 any)
{
	pfs_group			5;
	lifetime			time 3600 sec;
	encryption_algorithm		3des;
	authentication_algorithm	hmac_md5;
	compression_algorithm		deflate;
}
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

usage: racoon [-BdFv] [-a (port)] [-f (file)] [-l (file)] [-p (port)]
   -B: install SA to the kernel from the file specified by the configuration file.
   -d: debug level, more -d will generate more debug message.
   -C: dump parsed config file.
   -L: include location in debug messages
   -F: run in foreground, do not become daemon.
   -v: be more verbose
   -a: port number for admin port.
   -f: pathname for configuration file.
   -l: pathname for log file.
   -p: port number for isakmp (default: 500).
   -P: port number for NAT-T (default: 4500).

<ipsec>
    <remote id="0" enable="true">
	<gateway>10.0.0.2:500</gateway>
        <exchange_mode>main,aggressive</exchange_mode>
        <my_identifier>address 10.0.0.1</my_identifier>
        <peers_identifier>address 10.0.0.2</peers_identifier>
        <lifetime>time 28800 sec</lifetime>
	<initial_contact>on</initial_contact>
        <passive>off</passive>
        <proposal_check>obey</proposal_check>
	<nat_traversal>off</nat_traversal>
        <generate_policy>off</generate_policy>
        <proposal>
            <encryption_algorithm>3des</encryption_algorithm>
            <hash_algorithm>md5</hash_algorithm>
            <authentication_method>pre_shared_key</authentication_method>
            <dh_group>5</dh_group>
            <psk>pskpskpsk</psk>
	</proposal>
    </remote>
    <sainfo id="0" enable="true">
	<src>address 192.168.0.0/24 any</src>
	<dst>address 192.168.2.0/24 any</dst>
	<pfs_group>5</pfs_group>
	<lifetime>time 3600 sec</lifetime>
	<encryption_algorithm>3des</encryption_algorithm>
	<authentication_algorithm>hmac_md5</authentication_algorithm>
	<compression_algorithm>deflate</compression_algorithm>
    </sainfo>
    <setkey>
	<line>spdadd 192.168.0.0/24 192.168.2.0/24 any -P out ipsec esp/tunnel/10.0.0.1-10.0.0.2/use</line>
	<line>spdadd 192.168.2.0/24 192.168.0.0/24 any -P in ipsec esp/tunnel/10.0.0.2-10.0.0.1/use</line>
	<line></line>
	<line></line>
    </setkey>
</ipsec>


]]

-- require("ipcalc");

RACOON = {};
local mt = {};


function RACOON:new(c, debug)
    t = {};
    t.c = c;
    t.remote_count = 0;
    t.pidfile = "/var/run/racoon.pid";
    t.logfile = "/var/log/racoon.log";
    t.configfile = "/etc/racoon/racoon.conf";
    t.linksfile = "/var/run/racoon.links.conf";
    t.psk = "/var/db/racoon/racoon_psk.txt";

    t.debug = debug;
    return setmetatable(t, mt);
end

function RACOON:make_conf()

    -- Generate linksfile
    local f = assert(io.open(self.linksfile, "w"));
    for i = 1,16,1 do
	local ppath = "ipsec.remote[" ..i.. "]";
	local node = self.c:getNode(ppath);
	if node and node:attr("enable") == "true" then
	    local gw = self.c:getNode(ppath .. ".gateway"):value();
	    if gw then
		f:write("include \"/var/run/racoon." .. gw .. ".conf\";\n");
		t.remote_count = t.remote_count + 1;
	    else
		print("Can't get " .. ppath .. ".gateway value");
		return (nil);
	    end
	end
    end
    if self.remote_count == 0 then
	-- No remote targets, so nothing todo
	print("No remote targets, so nothing todo");
	return (nil);
    end

    for i = 1,16,1 do
	local ppath = "ipsec.sainfo[" ..i.. "]";
	local node = self.c:getNode(ppath);
	if node and node:attr("enable") == "true" then
	    f:write("include \"/var/run/racoon.sainfo" .. i .. ".conf\";\n");
	end
    end
    f:close();



    local function addline(ppath, item, prep, confitem)
	local ret = "";
	    local val = self.c:getNode(ppath .. "." .. (confitem or item)):value();
	    if val then
		ret = (prep or "") .. item .. "\t" .. val .. ";\n";
	    else
		print("Can't get " .. ppath .. "." .. (confitem or item) .." value");
	    end
	return (ret);
    end

    for i = 1,16,1 do
	local ppath = "ipsec.remote[" ..i.. "]";
	local node = self.c:getNode(ppath);
	if node and node:attr("enable") == "true" then
	    local gw = self.c:getNode(ppath .. ".gateway"):value();
	    local file = "/var/run/racoon." .. gw .. ".conf";
	    local remote = gw:gsub(":", " [") .. "]";

	    local conf = "remote\t" .. remote .. "\n";

	    conf = conf .. "{\n";
	    conf = conf .. addline(ppath, "exchange_mode"	, "\t");
	    conf = conf .. "\tdoi	ipsec_doi;\n";
	    conf = conf .. "\tsituation	identity_only;\n";
	    conf = conf .. "\tscript \"/etc/racoon/phase1-up.sh\" phase1_up;\n";
	    conf = conf .. "\tscript \"/etc/racoon/phase1-down.sh\" phase1_down;\n";

	    conf = conf .. addline(ppath, "my_identifier"	, "\t");
	    conf = conf .. addline(ppath, "peers_identifier"	, "\t");
	    conf = conf .. addline(ppath, "lifetime"		, "\t");
	    conf = conf .. addline(ppath, "initial_contact"	, "\t");
	    conf = conf .. addline(ppath, "passive"		, "\t");
	    conf = conf .. addline(ppath, "proposal_check"	, "\t");
	    conf = conf .. addline(ppath, "generate_policy"	, "\t");
	    conf = conf .. "\tproposal {\n";
            conf = conf .. addline(ppath, "encryption_algorithm", "\t\t", "proposal.encryption_algorithm");
            conf = conf .. addline(ppath, "hash_algorithm"	, "\t\t", "proposal.hash_algorithm");
            conf = conf .. addline(ppath, "authentication_method","\t\t", "proposal.authentication_method");
            conf = conf .. addline(ppath, "dh_group"		, "\t\t", "proposal.dh_group");
	    conf = conf .. "\t}\n";
	    conf = conf .. "}\n";

	    print("IPsec Remote config file " .. file .. ":\n" ..
		conf ..
		"=======================================\n");

	    local f = assert(io.open(file, "w"));
	    f:write(conf);
	    f:close();

	end
    end
    for i = 1,16,1 do
	local ppath = "ipsec.sainfo[" ..i.. "]";
	local node = self.c:getNode(ppath);
	if node and node:attr("enable") == "true" then
	    local file = "/var/run/racoon.sainfo" .. i .. ".conf";
	    local src = self.c:getNode(ppath .. ".src"):value();
	    local dst = self.c:getNode(ppath .. ".dst"):value();

	    dst_cidr = dst:gsub("^%S+%s+(%S+)%s+%S+", "%1");
	    if dst_cidr:match("%d+%.%d+%.%d+%.%d+%/%d+") then
		-- Preinstall routes of secured remote nets
		-- "already in table" ignored
		print("/sbin/route add " .. dst_cidr .. " -blackhole");
		os.execute("/sbin/route add " .. dst_cidr .. " -blackhole");
	    end

	    local conf = "sainfo (" .. src .. " " .. dst .. ")\n";
	    conf = conf .. "{\n";
	    conf = conf .. addline(ppath, "pfs_group"			, "\t");
	    conf = conf .. addline(ppath, "lifetime"			, "\t");
	    conf = conf .. addline(ppath, "encryption_algorithm"	, "\t");
	    conf = conf .. addline(ppath, "authentication_algorithm"	, "\t");
	    conf = conf .. addline(ppath, "compression_algorithm"	, "\t");
	    conf = conf .. "}\n";

	    print("IPsec SA info config file " .. file .. ":\n" ..
		conf ..
		"=======================================\n");

	    local f = assert(io.open(file, "w"));
	    f:write(conf);
	    f:close();
	end
    end

end

function RACOON:make_psk()

    -- Generate linksfile
    local f = assert(io.open(self.psk, "w"));
    for i = 1,16,1 do
	local ppath = "ipsec.remote[" ..i.. "]";
	local node = self.c:getNode(ppath);
	if node and node:attr("enable") == "true" then
	    local gw = self.c:getNode(ppath .. ".gateway"):value();
	    if gw then
		gw = gw:gsub(":.*", "");
		local psk = self.c:getNode(ppath .. ".proposal.psk");
		if psk then
		    f:write(gw .. "\t" .. psk:value() .. "\n");
		end;
	    else
		print("Can't get " .. ppath .. ".gateway value");
		return (nil);
	    end
	end
    end
    f:close();
    os.execute("/bin/chmod 700 " .. self.psk);
end

function RACOON:setkey()
    local setkey = "";

    for i = 1,16,1 do
	local ppath = "ipsec.setkey.line[" ..i.. "]";
	local node = self.c:getNode(ppath);
	if node then
	    local val  = node:value();
	    if val and string.len(val) > 0 then
		setkey = setkey .. val .. ";\n";
	    end
	end
    end

    if string.len(setkey) > 0 then
	local sk = io.popen("/sbin/setkey -c ", "w");
        sk:write(setkey);
	sk:close();
    end
end

function RACOON:run()
    -- FIXME install routes to remote nets via his IPSec gates
    --os.execute("route add 192.168.2.0/24 10.0.0.1");
    os.execute("mkdir -p /var/run/");
    os.execute("mkdir -p /var/db/racoon");
    self:setkey();
    self:make_psk();
    self:make_conf();
    if self.remote_count > 0 then
--	os.execute("echo 'WARNING: RACOON run in DEBUG mode' > /dev/console");
--	os.execute("/sbin/racoon -ddddf " .. self.configfile .. " -l " .. self.logfile);
	os.execute("/sbin/racoon -f " .. self.configfile .. " -l " .. self.logfile);
    end
end

function RACOON:stop()

    os.execute("kill `cat /var/run/racoon.pid`");
end

mt.__index = RACOON;

