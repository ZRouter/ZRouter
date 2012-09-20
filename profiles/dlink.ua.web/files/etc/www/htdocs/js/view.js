function tryget(msg)
{
	if (msg == 'system=reboot') {

		ajax("POST", "/cmd.xml", "system=reboot&r=" + encodeURIComponent(Math.random()), true, "ignore");
		reboottimeout = setTimeout( function(){ clearTimeout(reboottimeout); window.location = "/";}, 80000);
		return false;
	}

	ajax("POST", "/cmd.xml", msg + "&r=" + encodeURIComponent(Math.random()), true,
	    function (x) {
		window.alert(x.responseXML.getElementsByTagName("data")[0].firstChild.nodeValue);
	    }
	);
	return false;
}


