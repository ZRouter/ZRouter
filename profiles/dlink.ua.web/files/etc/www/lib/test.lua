
require("ipcalc");


print(string.format("hasbit(%08x, %08x) = %s", 0x12345678, 0x00001000, tostring(hasbit(0x12345678, 0x00004000))));

print(string.format("(%08x & %08x) = %08x", 0x12345678, 0xfffff000, andL(0x12345678, 0x000fffff)));

m =13
print(string.format("%d  netmask=%08x hostmask=%08x", m, bits_to_netmask(m), bits_to_hostmask(m)));

require("ipcalc");

local r = cidr_to_net("213.130.11.129/23");


--[[
r.host	0.0.1.129
r.bits	23
r.broadcast	213.130.11.255
r.net	213.130.10.0
r.mask	255.255.254.0
r.ip	213.130.11.129
r.cidr	213.130.11.129/23
]]






