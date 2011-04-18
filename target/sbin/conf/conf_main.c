
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

#if 0
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
	load_xml(&root, "test.xml", 0);

	if (load_xml_buf(&root, test, strlen(test), 0) == 0);

#endif

	return (root);
}
#endif

#define MAX_CMD_PATH 8

struct config {
	Node 	*tree;
	Node	*root;
	Node	*events;
	Node	*commands;
	char	*indent;
	char	*cwd[MAX_CMD_PATH];
};

typedef struct config Config;

int
load_default(Config *c, int argc, char ** argv)
{
	/*
	 * handle "load default xml filename.xml"
	 *        "load default json filename.json"
	 */
	if (argc < 2)
		return (EINVAL);

#ifdef WITH_XML_PARSER
	if (strcmp(argv[0], "xml") == 0)
		return (load_xml(&c->tree, argv[1], 1));

#endif
#ifdef WITH_JSON_PARSER
	if (strcmp(argv[0], "json") == 0)
		return (load_json(&c->tree, argv[1], 1));

#endif

	return (EINVAL);
}

int
load_config(Config *c, int argc, char ** argv)
{
	int conf_id = 2;
	/*
	 * handle "load config 1 xml filename.xml"
	 *        "load config 2 json filename.json"
	 */
	if (argc < 2)
		return (EINVAL);

	char * end;
	if (argc == 3) {
		conf_id = strtol(argv[0], &end, 0);
		if (*end != '\0')
			return (EINVAL);
		argv++;
	}

	printf("%s %d %s %s\n", __func__, conf_id, argv[0], argv[1]);
#ifdef WITH_XML_PARSER
	if (strcmp(argv[0], "xml") == 0)
		return (load_xml(&c->tree, argv[1], conf_id));

#endif
#ifdef WITH_JSON_PARSER
	if (strcmp(argv[0], "json") == 0)
		return (load_json(&c->tree, argv[1], conf_id));

#endif

	return (EINVAL);
}

void
dump_nodes_attr(Node *root, char *name)
{
	Node *node;
	if (!root || !name)
		return;

	if (nodeHasAttr(root, name)) {
		print_indent(nodeGetLevel(root));
		printf("%s = %s\n", root->name, nodeGetAttr(root, name));
	}
	for (node = root; node; node = node->next) {
		dump_nodes_attr(node->firstChild, name);
	}
}

int
dump_text(Node *root, int conf_id)
{
	char name[16];

	if (conf_id > 1)
		sprintf(name, "value%d", conf_id);

	dump_nodes_attr(root, (conf_id > 1)?name:"value");
}

void
dump_xml_nodes_attr(Strbuf *s, Node *root, char *name)
{
	Node *node;
	int i;

	if (!root || !name)
		return;

	for (i = 0; i < nodeGetLevel(root); i ++)
		strAppend(s, INDENT);
	strAppend(s, "<");
	strAppend(s, root->name);

	if (nodeHasAttr(root, name)) {
		strAppend(s, " value=\"");
		strAppend(s, nodeGetAttr(root, name));
		strAppend(s, "\"");
	}
	strAppend(s, ">");

	if (root->firstChild) {
		strAppend(s, "\n");

		for (node = root->firstChild; node; node = node->next) {
			dump_xml_nodes_attr(s, node, name);
		}

		/* Add indent If have childs */
		for (i = 0; i < nodeGetLevel(root); i ++)
			strAppend(s, INDENT);
	}

	strAppend(s, "</");
	strAppend(s, root->name);
	strAppend(s, ">\n");
}

int
dump_xml(Node *root, int conf_id)
{
	char name[16];

	if (conf_id > 1)
		sprintf(name, "value%d", conf_id);

	Strbuf *s = strInit();
	dump_xml_nodes_attr(s, root, (conf_id > 1)?name:"value");
	printf("dump_xml(\"\n%s\n)\n", strGet(s));

	return (0);
}

int
event(Config *c, int argc, char ** argv)
{
	int i, ret = 0;

	printf("%s: args: ", __func__);
	for (i = 0; i < argc; i ++) {
		printf("%s ", argv[i]);
	}
	printf("\n");

	Node *cmds = c->events;

	if (!cmds) {
		ret = 1;
		return (ret);
	}

	if (!c->events->firstChild) {
		/* No handlers */
		return (1);
	}

	Node *node = 0;
	for (node = c->events->firstChild; node; node = node->next) {
		int matched = 0;
		printf("%s: Event name=%s\n", __func__, node->name);
		if (node->firstChild && (strcmp(node->firstChild->name, "match") == 0)) {
			Node *match = node->firstChild;
			printf("%s: match node=%s\n", __func__, match->name);
			Node *m;
			for (m = match->firstChild; m; m = m->next) {
				if (nodeHasAttr(m, "value")) {
					//printf("%s: m->name=%s value=\"%s\"\n", __func__, m->name, nodeGetAttr(m, "value"));
					matched = 1;
				}
			}
		}
		if (matched) {
//			printf("Matched event:\n");
//			dump(node, ">  ");
//			printf("-------------------\n");
			Node *cmds = findNodePath(node, "event.do");
			if (cmds && cmds->firstChild) {
				Node *m;
				for (m = cmds->firstChild; m; m = m->next) {
					if (nodeHasAttr(m, "value")) {
						printf("Run command \"%s\"\n", nodeGetAttr(m, "value"));
						docmd(c, nodeGetAttr(m, "value"));
					}
				}
			}
		}
	}


	return (ret);
}

int
show_config(Config *c, int argc, char ** argv)
{
	int conf_id = 0;
	/*
	 * handle "show config 1 xml"
	 *        "show config 2 text"
	 *        "show config 2 json"
	 *        "show config 2" - show config 2 in text format
	 *        "show config" - show current in text format
	 */

	char * end;
	if (argc > 0) {
		conf_id = strtol(argv[0], &end, 0);
		if (*end != '\0')
			return (EINVAL);
		argv++;
		argc--;
	}

	printf("%s %d %s\n", __func__, conf_id, (argc)?argv[0]:"");

	if (strcmp(argv[0], "text") == 0 || argc == 0)
		return (dump_text(c->tree, conf_id));

#ifdef WITH_XML_PARSER
	if (strcmp(argv[0], "xml") == 0)
		return (dump_xml(c->tree, conf_id));

#endif
#ifdef WITH_JSON_PARSER
	if (strcmp(argv[0], "json") == 0)
		return (dump_json(c->tree, conf_id));

#endif

	return (EINVAL);
}

int
docmd(Config *c, char *cmdline)
{
	struct commands {
		int (*handler)(Config *, int, char **);
		char *name;
	} commands[] = {
		{&load_default, "load_default"},
		{&load_config,  "load_config"},
		{&show_config,  "show_config"},
		{&event,        "event"},
		{0,0}
	};

	char **argv = malloc(sizeof(char *) * MAX_CMD_PATH);
	char *cmd = strdup(cmdline);
	char *p = cmd;
	int argc, i, ret = 0;
	int (*handler)(Config *, int, char **);

	printf("cmd=%s\n", cmd);

	for (i = 0; (i < MAX_CMD_PATH) && *p; i++) {
		argv[i] = p;
		p = strchr(p, ' ');

		if (!p) break;

		*p++ = '\0';
		while (isspace(*p)) p++;
	}

	argc = i+1;

	printf("%s: args: ", __func__);
	for (i = 0; i < argc; i ++) {
		printf("\"%s\", ", argv[i]);
	}
	printf("\n");

	/* Now we have argc, argv */
	int error;

	Node *cmds = c->commands;

	if (!cmds) {
		ret = 1;
		goto docmd_free_cmd;
	}

	Strbuf *s = strInit();
	Node *node = 0;

	strAppend(s, "commands");
	for (i = 0; i < argc; i ++)
	{
		strAppend(s, ".");
		strAppend(s, argv[i]);

		node = findNodePath(cmds, strGet(s));
		if (node) {
			printf("Path = %s, Node *node =  %p (name=%s)\n", strGet(s), node, node->name);
			printf("\n\n\n");
			if (nodeHasAttrVal(node, "command", "pseudo"))
				break;
		}
	}
	strFree(s);


	if (node) {
		printf("Node %s\n", node->name);
		int level = i+1;
		const char *hname = nodeGetAttr(node, "handler");
		printf("hname=%s\n", hname);
		if (hname) {
			for (i = 0; commands[i].name; i ++)
				if (strcmp(commands[i].name, hname) == 0) {
					handler = commands[i].handler;
					handler(c, argc-level, argv+level);
				}
		}
	}

docmd_free_cmd:
	free(cmd);
	free(argv);

	return (ret);

}
#if 0
#define CONF_ETC_DIR "/etc/"
#else
#define CONF_ETC_DIR "./"
#endif

int
init_defaults(Config **config, int argc, char **argv)
{
	int error = 0;
	Config *c = malloc(sizeof(Config));
	char *ar[2] = {"xml", CONF_ETC_DIR "conf_defaults.xml"};

	c->tree = newNode("root", 0);
	c->indent = INDENT;

	int ch;

	while ((ch = getopt(argc, argv, "f:")) != -1)
		switch (ch) {
		case 'f': /* location of conf_defaults.xml file */
			ar[1] = optarg;
			break;
//		default:
//			usage();
		}
	argv += optind;



	/* Load defaults to have commands */
	error = load_default(c, 2, ar);
	if (error) {
		printf("ERROR: %s\n", strerror(error));
		free(c);
		return (error);
	}

	c->root = c->tree->firstChild;
	c->events = findNodePath(c->root, "root.events");
	c->commands = findNodePath(c->root, "root.commands");

	*config = c;

	return (0);
}

int
event_handler(Config *c, char *args)
{
	Strbuf *s;

	s = strInit();
	strAppend(s, "event ");
	strAppend(s, args);
	printf("%s\n", strGet(s));
	docmd(c, strGet(s));
	strFree(s);
}


int
main(int argc, char **argv)
{
	int f, num, exitcode = 0, i;
	int error = 0;
	const char *buf;
	Config *c;

	if (init_defaults(&c, argc, argv) != 0) {
		exit(1);
	}

	NodeList *cwd = NodeListInit();

	char *test_cmds[] = {
//		"load default xml test.xml",
//		"load config 3 xml test.xml",
//		"load config 2 xml test_config.xml",
//		"show config",
//		"show config 2",
//		"show config 2 xml",
		0
	};




	for (i = 0; test_cmds[i]; i++) {
		docmd(c, test_cmds[i]);
	}

	/* Call STARTUP event */
	event_handler(c, "STARTUP");

/*

	char *co[3] = {"2", "xml","test_config.xml"};
	error = load_config(c, 3, co);
	if (error)
		printf("ERROR: %s\n", strerror(error));

	char *dt[3] = {""};
	error = show_config(c, 0, dt);
	if (error)
		printf("ERROR: %s\n", strerror(error));

	char *dt1[3] = {"2"};
	error = show_config(c, 1, dt1);
	if (error)
		printf("ERROR: %s\n", strerror(error));

	char *dt2[3] = {"2", "xml"};
	error = show_config(c, 2, dt2);
	if (error)
		printf("ERROR: %s\n", strerror(error));
*/

	NodeListPush(cwd, c->root);
/* * /
#ifdef WITH_XML_PARSER
	printf("XML_DUMP of root:\n");
	XMLdump(c->root);
	printf("------------------------------------- XML_DUMP\n");
#else
	printf("TEXT_DUMP of root:\n");
	dump(c->root, "-->");
	printf("------------------------------------- TEXT_DUMP\n");
#endif

/ *
	char *path = (argc > 1)?argv[1]:"root.interfaces.alc0.static.ipaddr";
	Node *ip = findNodePath(c->root, path);
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
*/
	if (c->tree)
		freeNode(c->tree);
	NodeListFree(cwd);

	return (exitcode);
}
