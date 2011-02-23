function apply(fn)
{
	if(fn=="")
		document.write("<input type='submit' name='apply' value=\"\">");
	else
		document.write("<input type='button' name='apply' value=\"\" onClick='"+fn+"'>");
}
function cancel(fn)
{
	if(fn=="") fn="do_cancel()";
	document.write("<input type='button' name='cancel' value=\"\" onClick='"+fn+"'>");
}

// button for wizard ---------------------------------------------------------
function prev(fn)
{
	if(fn=="") fn="go_prev()";
	document.write("<input type='button' name='prev' value=\"\" onClick='"+fn+"'>&nbsp;");
}
function next(fn)
{
	if(fn=="")
		document.write("<input type='submit' name='next' value=\"\">&nbsp;");
	else
		document.write("<input type='button' name='next' value=\"\" onClick='return "+fn+"'>&nbsp;");
}

function exit()
{
	document.write("<input type='button' name='exit' value=\"\" onClick='exit_confirm()'>&nbsp;");
}
function exit_confirm()
{
	self.location.href="?page=bsc_wizard.html";
}
function wiz_save(fn)
{
	if(fn=="") fn="do_save()";
	document.write("<input type='submit' name='save' value=\"\" onClick='"+fn+"'>&nbsp;");

}
function wiz_connect(fn)
{
	if(fn=="") fn="do_save()";
	document.write("<input type='button' name='save' value=\"\" onClick='"+fn+"'>&nbsp;");

}
