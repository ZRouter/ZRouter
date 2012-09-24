print_native = print;
cons = io.open("/dev/console", "w");

-- XXX We need to find a way how to close it on signal
-- cons:close();

-- Write to stdout and console at the same time
function console(...)
	ret = 0;
	out = string.gsub(string.format(...), "\n", "\r\n");
	if cons then
		ret = cons:write(out);
	end
	-- check if native print moved
	if print_native then
		print_native(out);
	else
		print(out);
	end
	return ret;
end

function progress(...)
	console(...);
end

print = function (...)
	-- "string.format" need raw string of "print" as a single string
	console(table.concat(arg, "\t"):gsub("%%","%%%%") .. "\n");
end
