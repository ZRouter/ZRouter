/* You can find instructions for this file here:
/ http://www.geocities.com/marcelino_martins/ftv2instructions.html 
*/

// Decide if the names are links or just the icons
var USETEXTLINKS = 1;  //replace 0 with 1 for hyperlinks

// Decide if the tree is to start all open or just showing the root folders
var STARTALLOPEN = 0; //replace 0 with 1 to show the whole tree

var ICONPATH = 'img/'; 

var deviceType="Router";

var  band=0;


if(band==0)
{
	var x="CfgWLanParam.html?0";
}
else
{
	var x="CfgWLanParam.html?1";
}


var foldersTree = gHeader(deviceType, "/home_sys.html");
var aux1, aux2;

aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Basic Settings </font>", ""));
insDoc(aux1, gLnk("R","&nbsp;Internet", "/wan.html"));
insDoc(aux1, gLnk("R","&nbsp;Wireless", "/wlan.html"));
insDoc(aux1, gLnk("R","&nbsp;LAN", "/lan.html"));

aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Administration </font>", ""));
insDoc(aux1, gLnk("R","&nbsp;Users", "/users.html"));

//aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Advanced Settings </font>", ""));
//insDoc(aux1, gLnk("R","&nbsp;Performance", "/home_sys.html?page=adv_perf.html"));
//insDoc(aux1, gLnk("R","&nbsp;Multi-SSID", "/home_sys.html?page=adv_mssid.html"));
//insDoc(aux1, gLnk("R","&nbsp;VLAN ", "/home_sys.html?page=adv_8021q.html"));
//insDoc(aux1, gLnk("R","&nbsp;Instrusion", "/home_sys.html?page=adv_rogue.html"));
//insDoc(aux1, gLnk("R","&nbsp;Schedule", "/home_sys.html?page=adv_schedule.html"));
//insDoc(aux1, gLnk("R","&nbsp;QOS", "/home_sys.html?page=adv_qos.html"));

//aux2 = insFld(aux1, gFld("<font face=Tahoma size=2>&nbsp;DHCP Server</font>", ""));
//insDoc(aux2, gLnk("R","&nbsp;Dynamic Pool Setting", "/home_sys.html?page=adv_dhcpd.html"));
//insDoc(aux2, gLnk("R","&nbsp;Static Pool Setting", "/home_sys.html?page=adv_dhcps.html"));
//insDoc(aux2, gLnk("R","&nbsp;Current IP Mapping List", "/home_sys.html?page=adv_dhcpl.html"));

//aux2 = insFld(aux1, gFld("<font face=Tahoma size=2>&nbsp;Filters</font>", ""));
//insDoc(aux2, gLnk("R","&nbsp;Wireless MAC ACL", "/home_sys.html?page=adv_acl.html"));
//insDoc(aux2, gLnk("R","&nbsp;WLAN Partition", "/home_sys.html?page=adv_partition.html"));

aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Status</font>", ""));
insDoc(aux1, gLnk("R","&nbsp;Device Information", "/status.lua"));
//insDoc(aux1, gLnk("R","&nbsp;Client Information", "/home_sys.html?page=st_info.html"));
//insDoc(aux1, gLnk("R","&nbsp;WDS Information", "/home_sys.html?page=st_wds_info.html"));

//aux2 = insFld(aux1, gFld("<font face=Tahoma size=2> Stats</font>", ""));
//insDoc(aux2, gLnk("R","&nbsp;Ethernet", "/home_sys.html?page=st_stats_lan.html"));
//insDoc(aux2, gLnk("R","&nbsp;WLAN", "/home_sys.html?page=st_stats_wl.html"));

//aux2 = insFld(aux1, gFld("<font face=Tahoma size=2>Log</font>", ""));
//insDoc(aux2, gLnk("R","&nbsp;View Log", "/home_sys.html?page=st_log.html"));
//insDoc(aux2, gLnk("R","&nbsp;Log Settings", "/home_sys.html?page=st_logs.html"));

