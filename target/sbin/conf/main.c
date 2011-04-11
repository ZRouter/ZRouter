
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/cdefs.h>

//#include <signal.h>
//#include <sys/wait.h>
#include <ctype.h>
#include <unistd.h>
//#include <histedit.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <bsdxml.h>


#include <param.h>

#include <node.h>
#include <nodelist.h>
#include <conf.h>
#include <parsers.h>
#ifdef USE_XML_PARSER
#include <xml.h>
#endif



void
dump(Node *top, char * prepend)
{
	Node *node;
	int i;

	if (!top) return;

	printf("%s%s", prepend, top->name);
	if (top->value)
		printf("=%s\t\"%s\"", top->value, top->desc);

	if (top->firstAttr) {
		printf("\n");
		printf("%sAttrs: {\n", prepend);
		for (node = top->firstAttr; node; node = node->next ) {
			char *newpp = malloc(strlen(prepend)+5);
			sprintf(newpp, "    +%s", prepend);
			dump(node, newpp);
			free(newpp);
		}
		printf("%s}", prepend);
	}

	printf("\n");

	for (node = top->firstChild; node; node = node->next ) {
		char *newpp = malloc(strlen(prepend)+5);
		sprintf(newpp, "    +%s", prepend);
		dump(node, newpp);
		free(newpp);
	}
}


#if 0
Node *
FillTree()
{
	Node *root, *ifs, *ports, *iface, *node, *proto;
	Attr *pseudo, *cmd, *attr;

#ifdef DEBUG
	root = newNode("root", 0);
#else
	root = newNode("", 0);
#endif
	appendNewChild(root, "inttest", 0);
	ifs = appendNewChildDescr(root, "interfaces", "System interfaces", 0);
	    createConfValue(ifs, "IPADDR", "ipaddr", "192.168.0.254", "192.168.0.1", "IPv4 Address");
	ports = appendNewChildDescr(root, "switchports", "Ethernet Switch ports", 0);
	iface = appendNewChildDescr(ifs, "alc0", "Gigabit Ethernet card", 0);
	    proto = appendNewChildDescr(iface, "static", "Static configuration", 0);
		createConfValue(proto, "BOOL", "enable", "No", "No", "Enabled status");
		createConfValue(proto, "CIDR", "ipaddr", "192.168.0.2/24", "192.168.0.1/24", "IPv4 Address");
		createConfValue(proto, "IPADDR", "gwaddr", "192.168.0.253", "192.168.0.2", "IPv4 Gateway Address");
		createConfValue(proto, "IPADDR", "dns1addr", "192.168.100.1", "192.168.0.7", "IPv4 DNS1 Address");
		createConfValue(proto, "IPADDR", "dns2addr", "192.168.100.2", "192.168.0.8", "IPv4 DNS2 Address");
	    proto = appendNewChildDescr(iface, "DHCP", "Get network config from DHCP server", 0);
		createConfValue(proto, "BOOL", "enable", "No", "No", "Enabled status");
		createConfValue(proto, "CIDR", "ipaddr", "192.168.0.254/23", "0.0.0.0/0", "IPv4 Address");
		createConfValue(proto, "IPADDR", "gwaddr", "192.168.0.253", "0.0.0.0", "IPv4 Gateway Address");
		createConfValue(proto, "IPADDR", "dns1addr", "192.168.100.1", "0.0.0.0", "IPv4 DNS1 Address");
		createConfValue(proto, "IPADDR", "dns2addr", "192.168.100.2", "0.0.0.0", "IPv4 DNS2 Address");
	appendNewChildDescr(ifs, "run0", "802.11n card", 0);
	appendNewChildDescr(ifs, "lo0", "Local Loopback device", 0);
	appendNewChildDescr(ifs, "wlan0", "Wireless LAN interface", 0);
	appendNewChildDescr(ifs, "lagg0", "Link aggrigation interface LACP", 0);
	appendNewChild(ports, "port0", 0);

	/* Pseudo items */
	/*
	 * appendChild(appendNewChild(root,  "get", 0), root->firstChild);
	 * Looks nice, but STP required :))))
	 */
	pseudo = newAttr("type", "pseudo");
	cmd = newAttr("command", "pseudo");
	addAttr(appendNewChild(root,  "get", pseudo), cmd);
	addAttr(appendNewChild(root, "list", pseudo), cmd);
	addAttr(appendNewChild(root, "show", pseudo), cmd);

	return (root);
}
#else
Node *
FillTree()
{
	char *test = "<test default=\"1\" value=\"0\"><some>data</some></test>";
	Node *testroot;
	Node *root;

	load_xml(&root, "test.xml");


	if (load_xml_buf(&testroot, test, strlen(test)) == 0)
		appendChild(root, testroot);

//	dump(testroot, "   ");

	return (root);
}
#endif

Node *
findNode(Node *root, char *path)
{
	char * subpath, *name, *p;
	Node *node, *new;

	if (!root)
		return (0);

	printf("%s: path = %s\n", __func__, path);

	if (strcmp(root->name, path) == 0) {
		return (root);
	}

	p = strdup(path);

	subpath = strchr(p, '.');

	if (subpath)
		*(subpath++) = '\0';

	name = p;

	printf("%s: path = %s (%s and %s)\n", __func__, path, name, subpath);


	for (node = root; node; node = node->next ) {
		printf("%s: %s == %s\n", __func__, node->name, name);
		if (strcmp(node->name, name) == 0) {
			if (node->firstChild) {
				new = findNode(node->firstChild, subpath);
				if (new) {
					free(p);
					return (new);
				}
			}
			if (!subpath) {
				free(p);
				return (node);
			}
		}
	}
	if (root->firstAttr) {
		new = findNode(root->firstAttr, subpath);
		if (new) {
			free(p);
			return (new);
		}
	}

	free(p);
	return (0);
}

int
main(int argc, char **argv)
{
	int f, num, exitcode = 0;
	const char *buf;

	NodeList *cwd = NodeListInit();

	Node *root = FillTree();
	NodeListPush(cwd, root);
//	return (0);

	printf("TEXT_DUMP of root:\n");
	dump(root, "   ");
	printf("------------------------------------- TEXT_DUMP\n");
	printf("TEXT_DUMP of root:\n");
	dump(root, "-->");
	printf("------------------------------------- TEXT_DUMP\n");
	printf("XML_DUMP of root:\n");
	XMLdump(root);
	printf("------------------------------------- XML_DUMP\n");

	Node *ip = findNode(root, "root.interfaces.alc0.static.ipaddr");
	if (ip) {
		printf("node found: name=\"%s\"\n", ip->name);
		printf("XML_DUMP of root.interfaces.alc0.static.ipaddr:\n");
		XMLdump(ip);
		printf("------------------------------------- XML_DUMP\n");
		printf("TEXT_DUMP of root.interfaces.alc0.static.ipaddr:\n");
		dump(ip, ">  ");
		printf("------------------------------------- TEXT_DUMP\n");
	}

	freeNode(root);
	NodeListFree(cwd);
	return (exitcode);
}
