--[[
ieee80211d=1
country_code=UA
interface=wlan0
macaddr_acl=0
auth_algs=1
debug=1
hw_mode=g
ctrl_interface=/var/run/hostapd
ctrl_interface_group=wheel
ssid=freebsdap
wpa=2
wpa_passphrase=freebsdmall
wpa_key_mgmt=WPA-PSK
wpa_pairwise=CCMP TKIP

<hostapd>
    <instance id="0" enable="true">
        <ieee80211d>1</ieee80211d>
        <country_code>UA</country_code>
        <interface>wlan0</interface>
        <macaddr_acl>0</macaddr_acl>
        <auth_algs>1</auth_algs>
        <debug>0</debug>
        <hw_mode>g</hw_mode>
        <ctrl_interface>/var/run/hostapd</ctrl_interface>
        <ctrl_interface_group>wheel</ctrl_interface_group>
        <ssid>DIR-620</ssid>
        <!-- Open -->
        <wpa>0</wpa>
        <!-- WPA -->
        <!-- <wpa>1</wpa> -->
        <!-- RSN/WPA2 -->
        <!-- <wpa>2</wpa> -->
    </instance>
    <instance id="1" enable="false">
        <ieee80211d>1</ieee80211d>
        <country_code>UA</country_code>
        <interface>wlan0</interface>
        <macaddr_acl>0</macaddr_acl>
        <auth_algs>1</auth_algs>
        <debug>0</debug>
        <hw_mode>g</hw_mode>
        <ctrl_interface>/var/run/hostapd</ctrl_interface>
        <ctrl_interface_group>wheel</ctrl_interface_group>
        <ssid>DIR-620</ssid>
        <!-- WPA -->
        <!-- <wpa>1</wpa> -->
        <!-- RSN/WPA2 -->
        <wpa>2</wpa>
        <wpa_passphrase>freebsdmall</wpa_passphrase>
        <wpa_key_mgmt>WPA-PSK</wpa_key_mgmt>
        <wpa_pairwise>CCMP TKIP</wpa_pairwise>
    </instance>
</hostapd>

]]

HOSTAPD = {};
local mt = {};

--hostapd.instance[1]


function HOSTAPD:new(c, debug)
    t = {};
    t.c = c;
    t.debug = debug;
    t.configfile = {};
    t.conf_data = {};
    t.pidfile = "/var/run/hostapd.pid";
    return setmetatable(t, mt);
end

function HOSTAPD:make_conf(i)

    if self.debug == 1 then print("HOSTAPD:make_conf(".. tostring(i) .. ")"); end

    local path = "hostapd.instance[" .. i .. "]";

    self.conf_data[i] = "";

    local function a(txt)
	if not txt then
	    return (nil);
	end
	self.conf_data[i] = self.conf_data[i] .. txt .. "\n";
	if self.debug then print(txt); end
    end

	local iface = self.c:getNode(path .. ".interface"):value();
	self.configfile[i] = "/tmp/hostapd_" .. iface .. ".conf";

	a("ieee80211d=1");
	a("country_code=UA");
	a("interface=" .. iface);
	a("macaddr_acl=0");
	a("auth_algs=1");
	a("debug=0");
	a("hw_mode=g");
	a("ctrl_interface=/var/run/hostapd");
	a("ctrl_interface_group=wheel");
	a("ssid=" .. self.c:getNode(path .. ".ssid"):value() .. "");
	-- TODO: crypto support
	a("wpa=2");
	a("wpa_passphrase=freebsdmall");
	a("wpa_key_mgmt=WPA-PSK");
	a("wpa_pairwise=CCMP");

end

function HOSTAPD:write(i)

    local f = assert(io.open(self.configfile[i], "w"));
    f:write(self.conf_data[i]);
    f:close();
end

function HOSTAPD:run()
    local configs = "";
    local i;
    for i = 1,8,1 do
	local vap = self.c:getNode("hostapd.instance[" .. i .. "]");
	if vap and vap:attr("enable") == "true" then
	    if self.debug == 1 then print("CALL self.make_conf(".. tostring(i) .. ")"); end
	    self:make_conf(i);
	    if self.debug == 1 then print("CALL self.write(".. tostring(i) .. ")"); end
	    self:write(i);
	    configs = configs .. " " .. self.configfile[i];
	end
    end

    os.execute("mkdir -p /var/run/");
    os.execute("/usr/sbin/hostapd -B -P " .. self.pidfile .. " " .. configs);
end

function HOSTAPD:stop()

    os.execute("kill `cat " .. self.pidfile .."`");
end

mt.__index = HOSTAPD;

