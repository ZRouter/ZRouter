--[[
Usage:

----------------------------------------
dofile("lib/lsyslog.lua");

print("hi");
info("info");

function dtest()
        dbg("xxxxx");
end

dbg("first");
dtest();
----------------------------------------
Output:

lua syslog: hi
lua syslog: info
lua syslog: test.lua:19:<none>(): first
lua syslog: test.lua:16:dtest(): xxxxx


]]

-- XXX: how to do syslog.closelog() ?

function emerg(line)
        syslog.syslog("LOG_EMERG", line);
end

function alert(line)
        syslog.syslog("LOG_ALERT", line);
end

function crit(line)
        syslog.syslog("LOG_CRIT", line);
end

function err(line)
        syslog.syslog("LOG_ERR", line);
end

function warn(line)
        syslog.syslog("LOG_WARNING", line);
end

function notice(line)
        syslog.syslog("LOG_NOTICE", line);
end

function info(line)
        syslog.syslog("LOG_INFO", line);
end

function dbg(line)
        local caller, str;

        caller = debug.getinfo(2);
        str = string.format("%s:%d:%s(): %s", caller.short_src,
            caller.currentline, caller.name or "<none>", line or "");

        syslog.syslog("LOG_DEBUG", str);

end

function syslog_init(name, flags, facility)
	_G.syslog = require "syslog";

	-- Defaulting
	name = name or "lua";
	flags = flags or syslog.LOG_PID + syslog.LOG_PERROR +
	    syslog.LOG_ODELAY;
	facility = facility or "LOG_USER";

	if type(syslog) ~= "table" then
		error("Can't use syslog module");
	end

	syslog.openlog(name, flags, facility);

	_G.print_native = print;

	_G.print = function (...)
                -- "string.format" need raw string of "print" as a single string
                -- info(table.concat(arg, "\t"):gsub("%%","%%%%") .. "\n");
	        -- XXX: return it to info
                -- dbg(table.concat(arg, "\t"):gsub("%%","%%%%") .. "\n");

                local caller, str;

	        caller = debug.getinfo(2);
                str = string.format("%s:%d:%s(): %s", caller.short_src,
	            caller.currentline, caller.name or "<none>",
	            table.concat(arg, "\t"):gsub("%%","%%%%"));
	        syslog.syslog("LOG_DEBUG", str);
	end

end
