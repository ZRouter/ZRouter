--
-- check(pidfilename)
-- check for pid file and runnig process
-- return ( 1) if process runnning
-- return (-1) if stale pid file
-- return ( 0) if no pid file
--

function checkpid(pidfilename)
    local pidfile = io.open(pidfilename, "r");
    if pidfile then
	local oldpid = assert(pidfile:read("*n"));
	pidfile:close();

	local ret = os.execute("ps -axo command= " .. oldpid);
	if ret == 0 then
	    return (1); -- Process running
	end
	return (-1); -- Stale pid file, no process
    end
    return (0); -- No pid file found
end

--
-- pidfile(pidfilename)
-- check for runnig process and create new pid file otherwise
-- if exit process if checked proccess running
--

function pidfile(pidfilename)
    local psx = require("posix");
    local pid = psx.getprocessid("pid");

    if checkpid(pidfilename) == 1 then
	print("pid file ".. pidfilename .. " locked")
	os.exit(1);
    end

    pidfile = assert(io.open(pidfilename, "w"));
    assert(pidfile:write(pid));
    pidfile:close();

    return (true);
end
