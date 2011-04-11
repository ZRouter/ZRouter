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
startElement(void *st, const char *el,const char **attr) 
{
	int i;
	Node *node;
	struct xml_state *xst = st;

	/* Create new node */
	node = newNode(el, 0);

	if (xst->last) {
		/* Attach new node to upper */
		appendChild(xst->last, node);
	}

	/* Set new node as last */
	xst->last = node;

	if (!xst->root)
		xst->root = xst->last;

	print_indent(xst->level);
	printf("<%s", el);
	for (i = 0; attr[i] && *attr[i]; i+=2) {
		printf("%s=\"%s\" ", attr[i], attr[i+1]);
		addAttr(xst->last, newAttr(attr[i], attr[i+1]));
	}
	printf(">\n");
	xst->level++;

}

static void 
endElement(void *st, const char *el) 
{ 
	struct xml_state *xst = st;

	xst->level--;
	print_indent(xst->level);
	printf("</%s>\n", el);

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
	char Buff[BUFFSIZE];
	FILE *fp;
	struct xml_state xst;

	xst.level = 0;
	xst.root = 0;
	xst.last = 0;

	fp = fopen(file,"r");

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characters);
	XML_SetUserData(parser, &xst);

	for (;;) {
    		int len  = fread(Buff, 1, BUFFSIZE, fp);
    		int done = feof(fp);
    		if (! XML_Parse(parser, Buff, len, done))

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
	xst.root = 0;
	xst.last = 0;

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

	for (node = top->firstChild; node; node = node->next ) {

		{
			/* Break line only if have childs */
			printf("\n");
			XMLdumpL(node, l+1);
			/* indent close tag only if have childs */
			print_indent(l); 
		}

	}
	printf("</%s>\n", top->name);
}

void
XMLdump(Node *first)
{
	XMLdumpL(first, 0);
}


