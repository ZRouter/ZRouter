-- posix.lua
-- support code for posix library
-- usage lua -lposix ...

local function so(x)
	local SOPATH= os.getenv"LUA_SOPATH" or "./"
	assert(package.loadlib(SOPATH.."l"..x..".so","luaopen_"..x))()
end

so"posix"

return posix
