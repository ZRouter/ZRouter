
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/cdefs.h>

#include <ctype.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#include <conf_param.h>

#include <conf_node.h>
#include <conf_nodelist.h>
#include <conf_conf.h>
#include <conf_parsers.h>

#ifdef WITH_XML_PARSER
#include <bsdxml.h>
#include <conf_xml.h>
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

#ifdef WITH_XML_PARSER
	load_xml(&root, "test.xml");

	if (load_xml_buf(&root, test, strlen(test)) == 0) {}

#endif

	return (root);
}


int
main(int argc, char **argv)
{
	int f, num, exitcode = 0;
	const char *buf;

	NodeList *cwd = NodeListInit();

	Node *root = FillTree();
	Node *save = root;
	root = root->firstChild;
	NodeListPush(cwd, root);

#ifdef WITH_XML_PARSER
	printf("XML_DUMP of root:\n");
	XMLdump(root);
	printf("------------------------------------- XML_DUMP\n");
#else
	printf("TEXT_DUMP of root:\n");
	dump(root, "-->");
	printf("------------------------------------- TEXT_DUMP\n");
#endif


	char *path = (argc > 1)?argv[1]:"root.interfaces.alc0.static.ipaddr";
	Node *ip = findNodePath(root, path);
	if (ip) {
#ifdef WITH_XML_PARSER
		printf("XML_DUMP of %s:\n", path);
		XMLdump(ip);
		printf("------------------------------------- XML_DUMP\n");
#else
		printf("TEXT_DUMP of %s:\n",path);
		dump(ip, ">  ");
		printf("------------------------------------- TEXT_DUMP\n");
#endif
	} else {
		printf("Path \"%s\" not found\n", path);
	}

	freeNode(save);
	NodeListFree(cwd);
	return (exitcode);
}
