
function pidfile(pidfilename)
    local psx = require("posix");
    local pid = psx.getprocessid("pid");

    local pidfile = io.open(pidfilename, "r");
    if pidfile then
	local oldpid = assert(pidfile:read("*n"));
	pidfile:close();

	local ret = os.execute("ps -axo command= " .. oldpid);
	if ret == 0 then
	    print("pid file ".. pidfilename .. " locked")
	    os.exit(1);
	end
    end

    pidfile = assert(io.open(pidfilename, "w"));
    assert(pidfile:write(pid));
    pidfile:close();

    return (true);
end
