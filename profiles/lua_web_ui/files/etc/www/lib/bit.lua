function bit(p)
    return 2 ^ (p - 1);
    -- 1-based indexing
end
-- Typical call: if hasbit(x, bit(3)) then ...
function hasbit(x, p)
    return x % (p + p) >= p;
end

function setbit(x, p)
    return hasbit(x, p) and x or x + p;
end

function clearbit(x, p)
    return hasbit(x, p) and x - p or x;
end

-- &(0x12345678 & 0xfffff000) = 0x12345000
function andL(x, y)
    local o = 0;

    for i = 1,32,1 do
	if hasbit(x, bit(i)) and hasbit(y, bit(i)) then
	    o = o + bit(i);
	end
    end

    return (o);
end
