/* You can find instructions for this file here:
/ http://www.geocities.com/marcelino_martins/ftv2instructions.html 
*/

// Decide if the names are links or just the icons
var USETEXTLINKS = 1;  //replace 0 with 1 for hyperlinks

// Decide if the tree is to start all open or just showing the root folders
var STARTALLOPEN = 0; //replace 0 with 1 to show the whole tree

var ICONPATH = 'img/'; 

var deviceType="DAP-3520";

var  band=0;


if(band==0)
{
	var x="CfgWLanParam.html?0";
}
else
{
	var x="CfgWLanParam.html?1";
}


var foldersTree = gHeader(deviceType, "?page=home_sys.html");

aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Basic Settings </font>", ""));
insDoc(aux1, gLnk("R","Wireless", "?page=bsc_wlan.html"));
insDoc(aux1, gLnk("R","&nbsp;LAN", "?page=bsc_lan.html"));

aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Advanced Settings </font>", ""));
insDoc(aux1, gLnk("R","&nbsp;Performance", "?page=adv_perf.html"));
insDoc(aux1, gLnk("R","&nbsp;Multi-SSID", "?page=adv_mssid.html"));
insDoc(aux1, gLnk("R","&nbsp;VLAN ", "?page=adv_8021q.html"));
insDoc(aux1, gLnk("R","&nbsp;Instrusion", "?page=adv_rogue.html"));
insDoc(aux1, gLnk("R","&nbsp;Schedule", "?page=adv_schedule.html"));
insDoc(aux1, gLnk("R","&nbsp;QOS", "?page=adv_qos.html"));

aux2 = insFld(aux1, gFld("<font face=Tahoma size=2>&nbsp;DHCP Server</font>", ""));
insDoc(aux2, gLnk("R","&nbsp;Dynamic Pool Setting", "?page=adv_dhcpd.html"));
insDoc(aux2, gLnk("R","&nbsp;Static Pool Setting", "?page=adv_dhcps.html"));
insDoc(aux2, gLnk("R","&nbsp;Current IP Mapping List", "?page=adv_dhcpl.html"));

aux2 = insFld(aux1, gFld("<font face=Tahoma size=2>&nbsp;Filters</font>", ""));
insDoc(aux2, gLnk("R","&nbsp;Wireless MAC ACL", "?page=adv_acl.html"));
insDoc(aux2, gLnk("R","&nbsp;WLAN Partition", "?page=adv_partition.html"));

aux1 = insFld(foldersTree, gFld("<font face=Tahoma size=2> Status</font>", ""));
insDoc(aux1, gLnk("R","&nbsp;Device Information", "?page=st_device.html"));
insDoc(aux1, gLnk("R","&nbsp;Client Information", "?page=st_info.html"));
insDoc(aux1, gLnk("R","&nbsp;WDS Information", "?page=st_wds_info.html"));

aux2 = insFld(aux1, gFld("<font face=Tahoma size=2> Stats</font>", ""));
insDoc(aux2, gLnk("R","&nbsp;Ethernet", "?page=st_stats_lan.html"));
insDoc(aux2, gLnk("R","&nbsp;WLAN", "?page=st_stats_wl.html"));

aux2 = insFld(aux1, gFld("<font face=Tahoma size=2>Log</font>", ""));
insDoc(aux2, gLnk("R","&nbsp;View Log", "?page=st_log.html"));
insDoc(aux2, gLnk("R","&nbsp;Log Settings", "?page=st_logs.html"));

