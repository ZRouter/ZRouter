
function getXmlHttp()
{
	var xmlhttp;
	try { xmlhttp = new ActiveXObject("Msxml2.XMLHTTP"); } 
	catch (e) {
		try {  xmlhttp = new ActiveXObject("Microsoft.XMLHTTP.6.0"); } 
		catch (E) { }
		try {  xmlhttp = new ActiveXObject("Microsoft.XMLHTTP.3.0"); } 
		catch (E) { }
		try {  xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); } 
		catch (E) { xmlhttp = false; }
	}
	if (!xmlhttp && typeof XMLHttpRequest!='undefined') {
		xmlhttp = new XMLHttpRequest();
	}
	return xmlhttp;
}

var xmlhttp = getXmlHttp()
var xmlhttptimeout = 0;

function ajax(method, target, msg, async, handler)
{
	xmlhttptimeout = setTimeout( function(){ xmlhttp.abort(); alert("Timeout") }, 10000);
	xmlhttp.onreadystatechange = function()
	{
		if (xmlhttp.readyState != 4) return;

		clearTimeout(xmlhttptimeout);

		if (xmlhttp.status == 200) {
	    		handler(xmlhttp);
		} else {
	    		alert("Error: " + xmlhttp.statusText);
		}
	}
	xmlhttp.open(method, target, async);
	xmlhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded')
	xmlhttp.send(msg);
}



