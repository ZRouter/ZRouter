#include <stdio.h>
#include <string.h>


#include <conf_node.h>
#include <conf_nodelist.h>
#include <conf_parsers.h>


Node *
addConfValue(Node *node, char *type, char *name, char *val, char *def)
{
	char *newval, *newdef;

	newdef = parseAsType(type, def);
	if (!newdef)
		return (node);
	newdef = strdup(newdef);
	if (!newdef)
		return (node);
		printf("default = %s\n", newdef);
	addAttr(node, newAttr("default", newdef));

	newval = parseAsType(type, val);
	if (!newval) {
		return (node);
	}
	newval = strdup(newval);
	if (!newval) {
		return (node);
	}
		printf("value = %s\n", newval);
	addAttr(node, newAttr("value", newval));

	addAttr(node, newAttr("type", type));

	return (node);
}

Node *
createConfValue(Node *node, char *type, char *name, char *val, char *def, char *descr)
{
	Node *confnode;

	confnode = appendNewChildDescr(node, name, descr, 0);
	addConfValue(confnode, type, name, val, def);

	return (confnode);
}

