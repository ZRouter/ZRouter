function pppoe_status()
    if c:getNode("interfaces.wan0.PPPoE"):attr("enable") == "false" then
	return "PPPoE disabled";
    else
	return "PPPoE enabled";
    end
end

function getrq()
    if type(rq) == "table" then
	return string.gsub(string.gsub(tdump(rq), "\n", "<br>\n"), " ", "&nbsp;");
    end
    return "rq not defined";
end

return ([[
<html>
<head>
  <title>Hello World</title>
</head>
<body>
  <strong>Hello World!</strong>
]] ..
getrq() .. "<br>" ..
pppoe_status() .."<br>" ..
c:getNode("interfaces.wan0.PPPoE"):attr("enable") ..
[[
</body>
</html>
]]);
