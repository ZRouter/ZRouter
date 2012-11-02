
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

<div id="Main Log" class="yui3-module boxitem">
    <div class="yui3-hd">
        <h4>Main Log</h4>
    </div>
    <a href="#" target="_blank">Open log in new window</a>
    <div class="yui3-bd"><pre>
]]
    .. exec_output("cat /var/log/all.log") ..
    [[</pre></div>
</div>

</body>
</html>
]]);

