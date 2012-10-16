#!/usr/bin/lua

package.path = "./?.lua;/etc/www/lib/?.lua;./lib/?.lua";
package.cpath =
	"/lib/?.so;/usr/lib/?.so;/usr/lib/lua/?.so;" ..
	"/lib/lua?.so;/usr/lib/lua?.so;/usr/lib/lua/lua?.so;" ..
	"/lib/?-core.so;/usr/lib/?-core.so;/usr/lib/lua/?-core.so;" ..
	"/lib/?/core.so;/usr/lib/?/core.so;/usr/lib/lua/?/core.so;";

serverhost = serverhost or "127.0.0.1";
serverport = serverport or "80";

-- redirect print to syslog
dofile("lib/lsyslog.lua");
syslog_init("devd.lua");

-- urlEncode/urlDecode
dofile("lib/urlXxcode.lua");

-- read_file, tdump, xmldump, exec_output
dofile('lib/utils.lua');

local eventrelay = exec_output("kenv -q EVENTRELAY");

function tab_to_query(t)
    local ret = "event=devd";

    for k,v in pairs(t) do
	ret = ret .. "&" .. urlEncode(k) .. "=" .. urlEncode(v);
    end

    return (ret);
end

function call_server(config, q)
    local query = eventrelay .. "?" .. q;

    local body, code, headers = config.http.request(query);

    if code == 200 then
	return (true);
    else
	return (nil);
    end

end

function system_event(config, msg)
    -- !system=IFNET subsystem=rt0 type=ATTACH
    -- !system=DEVFS subsystem=CDEV type=CREATE cdev=usb/0.1.1
    -- !system=USB subsystem=DEVICE type=ATTACH ugen=ugen0.1 cdev=ugen0.1 \
    --		vendor=0x0000 product=0x0000 devclass=0x09 devsubclass=0x00 \
    --		sernum= release=0x0100 mode=host port=1 parent=dotg0
    -- !system=GPIO subsystem=pin14 type=PIN_LOW bus=gpiobus0 period=0
    -- !system=GPIO subsystem=pin14 type=PIN_HIGH bus=gpiobus0 period=0

    function generic_event(m, msg)
	-- Relay to main control logic
	local query = "cmd=event" ..
	    "&system=" .. m.system ..
	    "&subsystem=" .. m.subsystem ..
	    "&type=" .. m.type ..
	    "&data=" .. urlEncode(msg);
	-- cmd=event&iface=wan0&state=linkup;
	print("devd.lua: query master with \"" .. query .. "\"");
	if call_server(config, query) == false then
	    -- XXX error handling
	end
    end

    local m = {};

    if not msg then
	return (nil);
    end

    for k, v in msg:gmatch("(%w+)=(%S+)") do
        m[k] = v;
        -- print(k,v);
    end

    if m.system and m.subsystem and m.type then
	if     m.system == "IFNET" then
	    -- Interfaces
	    -- system=IFNET subsystem=lan0 type=LINK_UP
	    local linkstate;
	    if m.type == "LINK_UP" then
		if m.subsystem == "lan0" then
		    -- XXX: bug workaround, we have to check why UP does not unplumb iface
		    os.execute("ifconfig lan0 down");
		    os.execute("ifconfig lan0 up");
		end
		linkstate = "linkup";
	    elseif m.type == "LINK_DOWN" then
		linkstate = "linkdown";
	    else
		generic_event(m, msg);
		return (nil);
	    end
	    local query = "cmd=event" ..
		"&iface=" .. urlEncode(m.subsystem) ..
		"&state=" .. linkstate;
	    -- cmd=event&iface=wan0&state=linkup;
	    if call_server(config, query) == false then
		-- XXX error handling
	    end
	elseif m.system == "DEVFS" then
	    -- Device nodes
	    generic_event(m, msg);

	elseif m.system == "USB" then
	    -- USB messages
	    generic_event(m, msg);
	    -- !system=USB subsystem=INTERFACE type=ATTACH ugen=ugen0.2 cdev=ugen0.2 vendor=0x1f28 product=0x0021
	    -- devclass=0x00 devsubclass=0x00 sernum="216219360300" release=0x0000
	    -- mode=host interface=0 endpoints=2 intclass=0x08 intsubclass=0x06 intprotocol=0x50

	    -- system=USB subsystem=DEVICE type=ATTACH ugen=ugen0.2 cdev=ugen0.2 vendor=0x1f28 product=0x0021
	    -- devclass=0x00 devsubclass=0x00 sernum=%22216219360300%22 release=0x0000 mode=host port=1 parent=ugen0.1

	    -- Our usb_modeswitch :)
	    if m.subsystem == "DEVICE" and m.type == "ATTACH" then
		-- XXX: get that from config
		m.vendor = m.vendor - 0;
		m.product = m.product - 0;
		-- ugen0.2 -> 0.2
		devid = m.ugen:gsub("ugen", "");
		if m.vendor == 0x1f28 and m.product == 0x0021 then
	    	    os.execute("hex2bin " ..
	    		"55534243b82e238c24000000800108df200000000000000000000000000000" ..
	    		" > /dev/usb/" .. devid .. ".8"); -- EndPoint 8
		end
	    end

	elseif m.system == "GPIO" then
	    -- GPIO messages
	    generic_event(m, msg);

	    -- XXX: should not be hardcoded
	    if m.subsystem == reset_pin and m.bus == "gpiobus0" then
		if     m.type == "PIN_LOW" then
		    -- Pin return to normal state
		    time = tonumber(m.period);
		    -- if hold time between 10 and 15 sec, call httpd to reset to default
		    -- XXX better to send event to httpd, then httpd will decide what to do
		    -- XXX2 but if httpd have wrong config, then he can't start
		    if 10 < time and time < 15 then
			-- XXX: always do restore config here, because wrong config break httpd yet
			-- if call_server(config, "restore=config") == false then
			    -- If we can't get success from httpd, then we restore default manualy
			    os.execute("mv /tmp/etc/www/config.xml /tmp/etc/www/config.xml.bak");
			    os.execute("/etc/rc.save_config");
			    os.execute("reboot");
			-- end
			os.execute("echo \"devd.lua: User request Reset to Default\" > /dev/console");
		    end
		elseif m.type == "PIN_HIGH" then
		    -- User push the reset button
		    -- Nothing to do yet
		else
		    -- Unknown type
		end
	    end
	else
	    -- Unknown system
	end
    end
end

function unknown_device(config, msg)
-- "? at pins=?  on gpiobus0"
end

function device_attached(config, msg)
-- "+nvram2env0 at   on nexus0"
end

function device_detached(config, msg)
-- "-nvram2env0 at   on nexus0"
end



local run = true;
local config = {};
config.http = require("socket.http");

reset_pin = exec_output("kenv -q RESET_PIN");
if not reset_pin then
    reset_pin = 10;
end
reset_pin = string.format("pin%03d", reset_pin);

while run do
    if not config.d then
	config.d = io.open("/dev/devctl", "r");
    end
    local line = config.d:read("*l");
    if line then
	print("devd.lua:DEBUG: got \"" .. line .. "\"");
	local m, _, t, msg = line:find("^(.)(.*)");
	if m and t and msg then
		if     t == "!" then
		    system_event(config, msg);
		elseif t == "?" then
		    unknown_device(config, msg);
		elseif t == "+" then
		    device_attached(config, msg);
		elseif t == "-" then
		    device_detached(config, msg);
		else
			os.execute("echo \" Message with unknown type received " .. line .. "\" > /dev/console");
		end
	end
    end
end


--[[
!system=IFNET subsystem=rt0 type=ATTACH
!system=IFNET subsystem=usbus0 type=ATTACH
!system=IFNET subsystem=rt28600 type=ATTACH
!system=IFNET subsystem=lo0 type=ATTACH

+nvram2env0 at   on nexus0
+clock0 at   on nexus0
+rt0 at   on nexus0
+rt305x_sysctl0 at   on obio0
+rt305x_ic0 at   on obio0
+gpioc0 at pins=?  on gpiobus0
+gpioreset0 at pins=?  on gpiobus0
? at pins=?  on gpiobus0
+gpioled1 at pins=?  on gpiobus0
+gpiobus0 at   on gpio0
+gpio0 at   on obio0
+uart1 at   on obio0
+cfid0 at   on cfi0
+cfi0 at   on obio0
+usbus0 at   on dotg0
+dotg0 at   on obio0
+rt28600 at   on nexus0
+nexus0 at   on root0

-- New device node
!system=DEVFS subsystem=CDEV type=CREATE cdev=usbctl
!system=DEVFS subsystem=CDEV type=CREATE cdev=usb/0.1.0
!system=DEVFS subsystem=CDEV type=CREATE cdev=ugen0.1
!system=DEVFS subsystem=CDEV type=CREATE cdev=usb/0.1.1

!system=USB subsystem=DEVICE type=ATTACH ugen=ugen0.1 cdev=ugen0.1 vendor=0x0000 product=0x0000 devclass=0x09 devsubclass=0x00 sernum= release=0x0100 mode=host port=1 parent=dotg0
!system=USB subsystem=INTERFACE type=ATTACH ugen=ugen0.1 cdev=ugen0.1 vendor=0x0000 product=0x0000 devclass=0x09 devsubclass=0x00 sernum= release=0x0100 mode=host interface=0 endpoints=1 intclass=0x09 intsubclass=0x00 intprotocol=0x00
!system=DEVFS subsystem=CDEV type=CREATE cdev=cfid0
!system=DEVFS subsystem=CDEV type=CREATE cdev=map/bootloader

-- Reported GPIO pins
!system=GPIO subsystem=pin14 type=PIN_LOW bus=gpiobus0 period=0
!system=GPIO subsystem=pin14 type=PIN_HIGH bus=gpiobus0 period=0
]]
