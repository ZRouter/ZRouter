
function readpid(pidfilename)
    local pidfile = io.open(pidfilename, "r");
    if pidfile then
	local pid = assert(pidfile:read("*n"));
	pidfile:close();
	return (pid);
    end
    -- TODO: rise error on read/open failures
    return (nil);
end


--
-- check(pidfilename)
-- check for pid file and runnig process
-- return ( 1) if process runnning
-- return (-1) if stale pid file
-- return ( 0) if no pid file
--

function checkpid(pidfilename)
    local oldpid = readpid(pidfilename);
    if oldpid and oldpid > 0 then
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


function pidfile_signal(pidfilename, signal)
    return (false);
end

--
--
-- Terminate running holder of pidfile, and write new pidfile
--

function pidfile_replace_stop()
end


