#include <stdio.h>
#include <bsdxml.h>
#include <string.h>

#include <node.h>
#include <nodelist.h>
#include <conf.h>
#include <xml.h>
#include <param.h>

#define BUFFSIZE        1024


struct xml_state {
	int level;
	Node *root;
	Node *last;
};

static void
applyAttrs(Node *parent, char *name, char *value)
{
	Node *node;

	if (!parent) return;

	/* Apply to matched attribute */
	if (parent->firstAttr)
		for (node = parent->firstAttr; node; node = node->next) {
			if (strcmp(node->name, name) == 0) {
				free(node->value);
				node->value = value;
				return;
			}
		}

	/* Create if no match */
	addAttr(parent, newAttr(name, value));

	return;
}

static Node *
applyNode(Node *parent, char *name)
{
	Node *node;

	if (!parent) return 0;

	/* Return matched node */
	if (parent->firstChild)
		for (node = parent->firstChild; node; node = node->next) {
			if (strcmp(node->name, name) == 0) {
				return (node);
			}
		}

	/* Create if no match */
	node = newNode(name, 0);
	appendChild(parent, node);
	return (node);
}

static void 
startElement(void *st, const char *el,const char **attr) 
{
	int i;
	Node *node = 0, *n;
	struct xml_state *xst = st;

	node = applyNode(xst->last, el);
	if (!node) {
		printf("ERROR: no target node\n");
		return;
	}

	/* Set new node as last */
	xst->last = node;

//	print_indent(xst->level);
//	printf("<%s", el);
	for (i = 0; attr[i] && *attr[i]; i+=2) {
//		printf(" %s=\"%s\"", attr[i], attr[i+1]);
//		addAttr(xst->last, newAttr(attr[i], attr[i+1]));
		applyAttrs(xst->last, attr[i], attr[i+1]);
	}
//	printf(">\n");
	xst->level++;

}

static void 
endElement(void *st, const char *el) 
{ 
	struct xml_state *xst = st;

	xst->level--;
//	print_indent(xst->level);
//	printf("</%s>\n", el);

	if (xst->last == xst->root) return;

	xst->last = xst->last->parent;
}

static void characters(void *st, const char *ch, int len) 
{
//	struct xml_state *xst = st;
//	int i;

//	print_indent(xst->level);
//	printf("chars: \"");
//	for (i=0; i < len; i++)
//		putchar(ch[i]);
//	printf("\"\n");
}



int load_xml(Node **newroot, char *file) 
{
	char buf[BUFFSIZE];
	FILE *fp;
	struct xml_state xst;

	xst.level = 0;

	xst.last = xst.root = (*newroot)?*newroot:newNode("root", 0);

	fp = fopen(file,"r");

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characters);
	XML_SetUserData(parser, &xst);

	for (;;) {
    		int len  = fread(buf, 1, BUFFSIZE, fp);
    		int done = feof(fp);
    		if (! XML_Parse(parser, buf, len, done))
    			return (-1);

		if (done) break;
	}
	fclose(fp); 
	printf("\n");

	*newroot = xst.root;
	return (0);
}

int load_xml_buf(Node **newroot, char *buf, int len)
{
	struct xml_state xst;

	xst.level = 0;
	xst.last = xst.root = (*newroot)?*newroot:newNode("root", 0);

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characters);
	XML_SetUserData(parser, &xst);

    	if (! XML_Parse(parser, buf, len, 1))
    		return (-1);
	printf("\n");

	*newroot = xst.root;
	return (0);
}

void
print_indent(int l)
{
	int i;
	for (i = 0; i < l; i ++)
		printf(INDENT);
}

void
XMLattrdump(Node *first, int l)
{
	Node *node;
	for (node = first; node; node = node->next ) {
		if (node->name) {
			if (node->value)
				printf(" %s=\"%s\"", node->name, node->value);
			else
				printf(" %s=\"true\"", node->name);
		}
	}
}


void
XMLdumpL(Node *top, int l)
{
	Node *node;

	print_indent(l); printf("<%s", top->name);

	if (top->value)
		printf(" value=\"%s\"", top->value);
	if (top->desc)
		printf(" descriptions=\"%s\"", top->desc);

	if (top->firstAttr) {
		XMLattrdump(top->firstAttr, l+1);
	}

	printf(">");

	if (top->firstChild)
		/* Break line only if have childs */
		printf("\n");

	for (node = top->firstChild; node; node = node->next ) {

			XMLdumpL(node, l+1);
			/* indent close tag only if have childs */
			print_indent(l); 
	}
	printf("</%s>\n", top->name);
}

void
XMLdump(Node *first)
{
	XMLdumpL(first, 0);
}


