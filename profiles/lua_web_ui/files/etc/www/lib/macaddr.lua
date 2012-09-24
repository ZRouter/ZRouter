function tohex(pref, ar, f, t)
        x = 0;
        for i=f,t do
    		x = x * 0x100; x = x + ar:byte(i);
    	end
    	io.write(string.format(pref .. '=0x%06x\n', x));
end

ar = io.read("*all");
tohex('VENDOR', ar, 1, 3);
tohex('DEVICE', ar, 4, 6);


