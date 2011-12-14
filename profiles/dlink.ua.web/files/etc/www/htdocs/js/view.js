/* page init functoin */
function init()
{

}

/* parameter checking */
function check()
{

}

var tool_list = new Array();
tool_list[0] = ["Reboot",				"#", "tryget('system=reboot')"];
//tool_list[1] = ["Configuration File", 				"?page=tool_config.html"];
//tool_list[1] = ["Firmware and SSL Certification Upload", 	"?page=tool_fw.html"];
//tool_list[3] = ["Time and Date", 				"?page=tool_sntp.html"];

var config_list = new Array();
config_list[0] = ["Save Configuration",				"#", "tryget('save=config')"];
config_list[1] = ["Load Configuration (-)",				"#", "tryget('load=config')"];
config_list[1] = ["Restore Configuration",				"#", "tryget('restore=config')"];

function tryget(msg)
{
	ajax("POST", "/cmd.xml", msg + "&r=" + encodeURIComponent(Math.random()), true, 
	    function (x) {
		window.alert(x.responseXML.getElementsByTagName("data")[0].firstChild.nodeValue);
	    }
	);
	return false;
}

function genVerticalMenu(x, y, list, id)
{
	var str="";
	x = get_obj("MainTable").offsetLeft + x;

	str+="<div id='"+id+"' class='menu' style='display:none;position:absolute;top:"+y+"px;left:"+x+"px;'>";
	str+="  <ul>\n";
	for(var i=0; i<list.length; i++)
	{
		if (list[i][2])
		    str+="    <li><a target='ifrMain' onclick=\"" + list[i][2] + "\">"+list[i][0]+"</a></li>\n";
		else
		    str+="    <li><a href='"+list[i][1]+"' target='ifrMain'>"+list[i][0]+"</a></li>\n";
	}
	str+="  </ul>\n";
	str+="</div>";
	document.write(str);
}


function genLogOutMenu(x, y, msg, id)
{
	var str="";
	x = get_obj("MainTable").offsetLeft + x;
	str+="<div id='"+id+"' class='menu'  style='display:none;position:absolute;top:"+y+"px;left:"+x+"px;'>\n";
	str+="  <a href='?page=logout.html' class='logout'>"+msg+"</a>\n";
	str+="</div>\n";
	document.write(str);
}

function HideFrame()
{
	showlist("tool", 	"");
	showlist("config",	"");
	showlist("logout",	"");
}

function showlist(name, flag)
{
	var obj = document.getElementById(name);
	if (!obj) {
		return 0;
	}

	if(flag == "yes")
		obj.style.display = "";
	else
		obj.style.display = "none";

	return 1;
}

function gen_banner_td(name, flag)
{
	var str="";
	var menu_name="";

	if(name == "home")
		menu_name = "Home";
	else if(name == "tool")
		menu_name = "Maintenance";
	else if(name == "sys")
		menu_name = "System";
	else if(name == "config")
		menu_name = "Configuration";
//	else if(name == "logout")
//		menu_name = "Logout";
//	else if(name == "help")
//		menu_name = "Help";

	if(flag == "a_href")
	{
		str+="<td class='banner'>";
//		if(name == "sys")
//			str+="<a href='?page=sys_setting.html' target='ifrMain'>";
//		else if(name == "help")
//			str+="<a href='?page=help.html' target='_blank'>";
//		else
			str+="<a href='home_sys.html' target='ifrMain'>";
	}
	else
		str+="<td class='banner' onclick=\"showlist('"+name+"','yes')\">";

	if(name == "logout")
		str+="<img src='img/tool_bar_v.jpg' height='18' border='0' hspace='5'>";

	str+="<img src='img/"+name+".gif' width='16' border='0' hspace='10'>";

	str+="<span id='banner_"+name+"' class='word'>"+menu_name+"</span>";

	if(flag == "" && name != "logout")
		str+="<span class='img'><img src='img/triangle.gif' hspace='8'></span>";

	if(name != "help")
		str+="<img src='img/tool_bar_v.jpg' height='18' border='0' hspace='10'>";

	if(flag == "a_href")
		str+="</a>";
	str+="</td>";

	document.write(str);

}
