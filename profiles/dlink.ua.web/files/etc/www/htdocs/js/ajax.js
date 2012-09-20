
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

	if (handler != "ignore") {
		xmlhttptimeout = setTimeout( function(){ xmlhttp.abort(); alert("Timeout") }, 10000);
	}

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

function getValuesAsArray(formRef)
{
	var ret = "r=" + encodeURIComponent(Math.random());
	var els = formRef.elements;
	for(var no=0; no < els.length; no++)
	{
		if(els[no].disabled)
		    continue;
		var tag = els[no].tagName.toLowerCase();

		switch(tag){
			case "input":
				var type = els[no].type.toLowerCase();

				if(!type)
				    type='text';

				switch(type){
					case "text":
					case "image":
					case "hidden":
					case "password":
						ret += '&' + encodeURIComponent(els[no].name) + '=' + encodeURIComponent(els[no].value);
						break;
					case "checkbox":
						if(els[no].checked)
						    ret += '&' + encodeURIComponent(els[no].name) + '=true';
						else
						    ret += '&' + encodeURIComponent(els[no].name) + '=false';
						break;
					case "radio":
						if(els[no].checked)
						    ret += '&' + encodeURIComponent(els[no].name) + '=' + encodeURIComponent(els[no].value);
						break;
				}
				break;
			case "select":
				var string = '';
				var mult = els[no].getAttribute('multiple');
				if(mult || mult===''){
					ops = els[no].options;

					for(var no2=0;no2<ops.length;no2++)
					{
						var index = retArray[els[no].name].length;
						if(ops[no2].selected)
						    ret += '&' + encodeURIComponent(els[no].name + index) + '=' + encodeURIComponent(ops[no2].value);
					}
				}else{
					ret += '&' + encodeURIComponent(els[no].name) + '=' + encodeURIComponent(ops[els[no].selectedIndex].value);
				}
				break;
			case "textarea":
				ret += '&' + encodeURIComponent(els[no].name) + '=' + encodeURIComponent(els[no].value);
				break;
		}
	}
	return ret;
}

