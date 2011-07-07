#!/usr/bin/lua

package.path = "./?.lua;/etc/www/lib/?.lua;./lib/?.lua";
package.cpath = 
	"/lib/?.so;/usr/lib/?.so;/usr/lib/lua/?.so;" ..
	"/lib/lua?.so;/usr/lib/lua?.so;/usr/lib/lua/lua?.so;" ..
	"/lib/?-core.so;/usr/lib/?-core.so;/usr/lib/lua/?-core.so;" ..
	"/lib/?/core.so;/usr/lib/?/core.so;/usr/lib/lua/?/core.so;";

serverhost = serverhost or "127.0.0.1";
serverport = serverport or "80";

--
-- Utility function:  URL encoding function
--
function urlEncode(str)
    if (str) then
        str = string.gsub (str, "\n", "\r\n") 
        str = string.gsub (str, "([^%w ])",
            function (c) return string.format ("%%%02X", string.byte(c)) end) 
        str = string.gsub (str, " ", "+") 
    end 
    return str 
end


--
-- Utility function:  URL decode function
--
function urlDecode(str)
    str = string.gsub (str, "+", " ") 
    str = string.gsub (str, "%%(%x%x)", function(h) return string.char(tonumber(h,16)) end) 
    str = string.gsub (str, "\r\n", "\n") 
    return str 
end

function tab_to_query(t)
    local ret = "event=devd";

    for k,v in pairs(t) do
	ret = ret .. "&" .. urlEncode(k) .. "=" .. urlEncode(v);
    end

    return (ret);
end

function call_server(config, q)
    local query = "http://127.0.0.1:80/event.xml?" .. q;

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
	elseif m.system == "DEVFS" then
	    -- Device nodes
	elseif m.system == "USB" then
	    -- USB messages
	elseif m.system == "GPIO" then
	    -- GPIO messages
	    -- XXX: should not be hardcoded
	    if m.subsystem == "pin10" and m.bus == "gpiobus0" then
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
			    os.execute("/etc/save_etc");
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



while run do
    if not config.d then
	config.d = io.open("/dev/devctl", "r");
    end
    local line = config.d:read("*l");
    if line then
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
