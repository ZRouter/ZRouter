#!/usr/bin/lua

-- Can be invoked:
-- echo "1c:af:f7:6f:21:9cONLY_CORRECT_LENGHT_REQUIRED" | lua base_mac.lua 2
-- dd if=/dev/map/MAC bs=64k count=1 2>/dev/null | lua base_mac.lua 7
--
-- Last argument is number, count of MAC addresses printed with device part++

count = arg[1];
vendor = 0;
device = 0;

function tohex(ar, f, t)
        x = 0;
        for i=f,t do
                x = x * 0x100; x = x + ar:byte(i);
        end
        return x;

end

function print_mac(vendor, device)
        print(string.format("%02x:%02x:%02x:%02x:%02x:%02x", 
            ((vendor / 0x10000) % 0x100),
            ((vendor / 0x00100) % 0x100),
            ((vendor / 0x00001) % 0x100),
            ((device / 0x10000) % 0x100),
            ((device / 0x00100) % 0x100),
            ((device / 0x00001) % 0x100)));
end

-- main --
ar = io.read("*all");
n = 1;

if count then
        n = count + 0;
end

if ar:sub(0, 17):match("%x%x:%x%x:%x%x:%x%x:%x%x:%x%x") then
	-- Parse MAC in string format
        local mac = ar:sub(0, 17):gsub(":", "");
        vendor = '0x' .. mac:sub(0, 6);
        device = '0x' .. mac:sub(7, 12);
else
	-- Parse MAC in binary format
        vendor = tohex(ar, 1, 3);
        device = tohex(ar, 4, 6);
end

-- Print N MAC addresses starting from base
i = 0;
while i < n do
        print_mac(vendor, device + i);
        i = i + 1;
end

