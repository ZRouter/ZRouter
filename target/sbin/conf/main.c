
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


Node *
FillTree()
{
	char *test = 
	"<root>"
	    "<test default=\"1\" value=\"0\"><some>data</some></test>"
	    "<interfaces>"
		"<wlan0>"
		    "<DHCP descriptions=\"Get network config from DHCP server\">"
			"<enable default=\"FALSE\" value=\"TRUE\" type=\"BOOL\" descriptions=\"Enabled status\"></enable>"
		    "</DHCP>"

		"</wlan0>"
		"<alc0>"
		    "<DHCP>"
			"<enable value=\"XXX\" value1=\"TRUE\"></enable>"
		    "</DHCP>"

		"</alc0>"
	    "</interfaces>"
	"</root>";
	Node *testroot = 0;
	Node *root = 0;

	load_xml(&root, "test.xml");


	if (load_xml_buf(&root, test, strlen(test)) == 0) {}

	XMLdump(root);

	return (root);
}

Node *
findNode(Node *root, char *path)
{
	char * subpath, *name, *p;
	Node *node, *new;

	if (!root || !path)
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

//	printf("TEXT_DUMP of root:\n");
//	dump(root, "   ");
//	printf("------------------------------------- TEXT_DUMP\n");
//	printf("TEXT_DUMP of root:\n");
//	dump(root, "-->");
//	printf("------------------------------------- TEXT_DUMP\n");
//	printf("XML_DUMP of root:\n");
//	XMLdump(root);
//	printf("------------------------------------- XML_DUMP\n");

	Node *ip = findNode(root, (argc > 1)?argv[1]:"root.interfaces.alc0.static.ipaddr");
	if (ip) {
		printf("node found: name=\"%s\"\n", ip->name);
		printf("XML_DUMP of root.interfaces.alc0.static.ipaddr:\n");
		XMLdump(ip);
		printf("------------------------------------- XML_DUMP\n");
//		printf("TEXT_DUMP of root.interfaces.alc0.static.ipaddr:\n");
//		dump(ip, ">  ");
//		printf("------------------------------------- TEXT_DUMP\n");
	}

	freeNode(root);
	NodeListFree(cwd);
	return (exitcode);
}
