-----------------------------------
----------- Led Class ------------
-----------------------------------
Led = {};
mtLed = {};

function Led:new(name, invert)
    led = {};

    if type(name) ~= "string" then return nil; end
    if not invert or invert == "0" or invert == 0 then
        invert = false;
    else
        invert = true;
    end

    led.name = name;
    led.path = "/dev/led/" .. name;
    led.invert = invert;

    return (setmetatable(led, mtLed));
end

function Led:new_from_env(name)

    local led = os.getenv(name);
    local invert = os.getenv(name .. "_INVERT");
    return (Led:new(led, invert));
end

function Led:set(mode)
    local f = io.open(self.path, "w");

    if not f then return nil; end

    if mode == 0 and self.invert then
        mode = 1;
    elseif mode == 1 and self.invert then
        mode = 0;
    end

    f:write(mode);

    f:close();

    return (true);
end

mtLed.__index = Led;
-----------------------------------
-------- End of Led Class --------
-----------------------------------
