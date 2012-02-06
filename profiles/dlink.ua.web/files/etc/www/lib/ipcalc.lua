require("bit");

function bits_to_hostmask(b)
    return (2 ^ (32-b) - 1);
end

function bits_to_netmask(b)
    return ((2 ^ 32 - 1) - bits_to_hostmask(b));
end


function cidr_to_net(cidr)
    local r = {};
    r.cidr	= cidr;

    local function dotted(i)
	local l1,l2,l3,r1,r2,r3,r4;
	r1 =  i % 256; l1 =  i / 256;
	r2 = l1 % 256; l2 = l1 / 256;
	r3 = l2 % 256; l3 = l2 / 256;
	r4 = l3 % 256;
	return string.format("%d.%d.%d.%d", r4, r3, r2, r1);
    end

    local function toint(i1, i2, i3, i4)
	return
	    ((tonumber(i1) * (2^24)) +
	     (tonumber(i2) * (2^16)) +
	     (tonumber(i3) * ( 2^8)) +
	     (tonumber(i4)         ));
    end

    local i1, i2, i3, i4;

    i1, i2, i3, i4, r.bits = string.match(cidr, "^(%d+)%.(%d+)%.(%d+)%.(%d+)%/(%d+)$");

    if r.bits then
	r.ip = toint(i1, i2, i3, i4);
    else
	local i1, i2, i3, i4, m1, m2, m3, m4 = string.match(cidr,
	    "^(%d+)%.(%d+)%.(%d+)%.(%d+)%/(%d+)%.(%d+)%.(%d+)%.(%d+)$");
	if m4 then
	    r.ip  = toint(i1, i2, i3, i4);
	    r.net = toint(m1, m2, m3, m4);
	else
	    return nil, "Failed to parse";
	end
    end

    r.mask 	= dotted(           bits_to_netmask (r.bits));
    r.net 	= dotted(andL(r.ip, bits_to_netmask (r.bits)));
    r.host 	= dotted(andL(r.ip, bits_to_hostmask(r.bits)));
    r.broadcast = dotted(andL(r.ip, bits_to_netmask (r.bits)) + (2 ^ (32-r.bits) - 1) );
    r.ip 	= dotted(     r.ip);

    return r;
end

