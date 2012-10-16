function runtime(iface)
    local ret = "";
    if r[iface] then
	for k,v in pairs(r[iface]) do
	    ret = ret .. k .. "=" .. v .. "<br/>\n";
	end

	return (ret);
    end
    return ("");
end

return ([[
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
    <head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8">
	<title>Status</title>
	<style type="text/css"> body {	margin:0; padding:0; } 	</style>
	<link href="css/anim.css" rel="stylesheet" type="text/css">
    </head>
    <body class="yui3-skin-sam  yui-skin-sam">
    <h1>LAN configuration</h1>

<div id="Routes" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>Routes</h4>
    </div>
    <div class="yui3-bd">
    ]] ..
    "<pre>\n" ..
    exec_output("netstat -rn") ..
    "</pre>\n" ..
    [[
    </div>
</div>
<div id="Static" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>Static/DHCP</h4>
    </div>
    <div class="yui3-bd">
    ]]
    .. runtime("wan0") ..
    [[
    </div>
</div>
<div id="PPPoE" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>PPPoE</h4>
    </div>
    <div class="yui3-bd">
    ]]
    .. runtime("PPPoE") ..
    [[
    </div>
</div>
<div id="PPP" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>PPP</h4>
    </div>
    <div class="yui3-bd">
    ]]
    .. runtime("PPP") ..
    [[
    </div>
</div>
<div id="IPSec0" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>IPSec0</h4>
    </div>
    <div class="yui3-bd">
    ]]
    .. runtime("IPSec0") ..
    [[
    </div>
</div>
<div id="Memory" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>Memory Usage</h4>
    </div>
    <div class="yui3-bd"><pre>
    ]]
    .. exec_output("sysctl -n vm.vmtotal") ..
    [[
    </pre></div>
</div>
<div id="ifconfig" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>ifconfig output</h4>
    </div>
    <div class="yui3-bd"><pre>
    ]]
    .. exec_output("ifconfig") ..
    [[
    </pre></div>
</div>
<script type="text/javascript" src="js/ajax.js"></script>
<script>
function send_update(form, proc)
{
    var query = getValuesAsArray(form);
    if (!proc) {
	proc = function (x) { };
    }
    ajax("POST", "/cmd.xml", query, true, proc);

    return false;
}
</script>



</body>
</html>
]]);

