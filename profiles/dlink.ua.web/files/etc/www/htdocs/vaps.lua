

template = ([[
<div id="$$$ifaceid$$$" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>Station $$$n$$$ Name: $$$ssid$$$</h4>
    </div>
    <div class="yui3-bd">
	<form method="POST" target="/test.lua">
	<table width="100%">
	    <tr >
		<td class=""><label for="$$$iface$$$SSID">Wireless Network Name&nbsp;(SSID)&nbsp;:</label></td>
		<td><input id="$$$iface$$$SSID" size="20" maxlength="32"   type="text" value="$$$ssid$$$"></td>
	    </tr>
	    <tr>
		<td class="">Visibility Status&nbsp;:</td>
		<td> 
			<input id="$$$iface$$$visibility" name="visibility"  type="checkbox" "$$$visibility_checked$$$">
			<label for="$$$iface$$$visibility">Visible</label> 
		</td>
	    </tr>
<!-- Only OPEN security mode now supported  -->
	
<!--
	    <tr>
		<td class="form_label">Security Mode&nbsp;:</td>
		<td> 
		    <select class="security_mode"  onChange="updateSecurityMode(this);">
		        <option value="none">None</option>
		        <option value="wep">WEP</option>
		        <option value="wpa">WPA</option>
			<option value="wpa-radius">WPA+RADIUS</option>
		    </select>
		</td>
	    </tr>
	    <tr class="security_mode_wpa"> 
		<td class="form_label">WPA Mode&nbsp;:</td>
		<td> 
			<select >
				<option value="2">Auto (WPA or WPA2)</option>

				<option value="3">WPA2 Only</option>
				<option value="1">WPA Only</option>
			</select>
		</td>
	    </tr>
	    <tr class="security_mode_wpa"> 
		<td class="form_label"><label for="$$$iface$$$wpa_rekey_time">Group Key Update Interval&nbsp;:</label></td>
		<td><input id="$$$iface$$$wpa_rekey_time" size="10"  type="text"> (seconds)</td>
	    </tr>
	    -- Same for wpa-radius  --
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label">WPA Mode&nbsp;:</td>
		<td> 
			<select >
				<option value="2">Auto (WPA or WPA2)</option>
				<option value="3">WPA2 Only</option>
				<option value="1">WPA Only</option>
			</select>
		</td>
	    </tr>
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label"><label for="$$$iface$$$wpa_rekey_time">Group Key Update Interval&nbsp;:</label></td>
		<td><input id="$$$iface$$$wpa_rekey_time" size="10"  type="text">	(seconds) </td>
	    </tr>
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label"><label for="$$$iface$$$reauth_time">Authentication Timeout&nbsp;:</label></td>
		<td><input id="$$$iface$$$reauth_time" size="10"  type="text">(minutes)</td>
	    </tr>
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label"><label for="$$$iface$$$radius_server_address">RADIUS server IP Address&nbsp;:</label></td>
		<td><input id="$$$iface$$$radius_server_address" size="20"  type="text"></td>
	    </tr>
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label"><label for="$$$iface$$$radius_server_port">RADIUS server Port&nbsp;:</label></td>
		<td><input id="$$$iface$$$radius_server_port" size="10"  type="text"></td>
	    </tr>
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label"><label for="$$$iface$$$radius_shared_secret">RADIUS server Shared Secret&nbsp;:</label></td>
		<td><input id="$$$iface$$$radius_shared_secret" size="20" maxlength="64"  type="text"></td>
	    </tr>
	    <tr class="security_mode_wpa-radius"> 
		<td class="form_label"><label for="$$$iface$$$radius_auth_mac">MAC Address Authentication&nbsp;:</label></td>
		<td><input id="$$$iface$$$radius_auth_mac" type="checkbox"></td>

	    </tr>
-->
	</table>
	</form>
    </div>
</div>
]]);

local out = "";

function onetemplate(c, tamplate, i)
    local out = "";
    local repl = {};
    local instance = "hostapd.instance[" .. tostring(i) .. "]";

    vap = c:getNode(instance);

    if (not vap) or (c:getNode(instance .. ".ssid"):value() == nil) then
	return ("");
    end

    repl["n"]			= tostring(i);
    repl["ifaceid"]		= "wlan" .. tostring(i);
    repl["iface"]		= repl["ifaceid"] .. ":";
    repl["auth_algs"]		= c:getNode(instance .. ".auth_algs"):value();
    repl["ssid"]		= c:getNode(instance .. ".ssid"):value();
    repl["wpa"]			= c:getNode(instance .. ".wpa"):value();
    repl["wpa_passphrase"]	= c:getNode(instance .. ".wpa_passphrase"):value();
    repl["wpa_key_mgmt"]	= c:getNode(instance .. ".wpa_key_mgmt"):value();

    local visible = c:getNode(instance .. ".wpa_key_mgmt"):value();
    if (visible ~= "false") and (visible ~= "0") then
	repl["visibility_checked"] = "checked";
    end

    out = string.gsub(template, "%$%$%$(.-)%$%$%$", repl);

    return (out);
end

-- TODO foreach wlan if
-- for i = 1,8,1 do
for i = 1,1,1 do
    out = out .. onetemplate(c, tamplate, i);
end

return (out);


