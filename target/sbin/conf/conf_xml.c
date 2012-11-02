#include <stdio.h>
#include <bsdxml.h>
#include <string.h>

#include <conf_node.h>
#include <conf_nodelist.h>
#include <conf_conf.h>
#include <conf_xml.h>
#include <conf_param.h>

#define BUFFSIZE        1024


struct xml_state {
	int 	level;
	Node 	*root;
	Node 	*last;
	int	conf_id;
};

static void 
startElement(void *st, const char *el,const char **attr) 
{
	int i;
	Node *node = 0, *n;
	struct xml_state *xst = st;
	char attrname[16];

	if (xst->conf_id == 1) {
		/* conf_id is defaults, there we need only create */
		node = appendNewChild(xst->last, el, 0);
	} else {
		node = applyNode(xst->last, el);
	}

	if (!node) {
		printf("ERROR: no target node\n");
		return;
	}

	/* Set new node as last */
	xst->last = node;

	for (i = 0; attr[i] && *attr[i]; i+=2)
		if (xst->conf_id == 1)
			applyAttr(xst->last, attr[i], attr[i+1]);
		else {
			if (strcmp(attr[i], "value") == 0) {
				sprintf(attrname, "value%d", xst->conf_id);
				applyAttr(xst->last, attrname, attr[i+1]);
			}
		}

	xst->level++;

}

static void 
endElement(void *st, const char *el) 
{ 
	struct xml_state *xst = st;

	xst->level--;

	if (xst->last == xst->root) return;

	xst->last = xst->last->parent;
}

static void characters(void *st, const char *ch, int len) 
{
	/* Implement if need inner text */
}



int load_xml(Node **newroot, char *file, int conf_id)
{
	char buf[BUFFSIZE];
	FILE *fp;
	struct xml_state xst;

	xst.level = 0;
	xst.conf_id = conf_id;
	xst.last = xst.root = (*newroot)?*newroot:newNode("root", 0);

	printf("%s: filename %s\n", __func__, file);

	fp = fopen(file,"r");
	if (!fp)
		return (EIO);

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characters);
	XML_SetUserData(parser, &xst);

	for (;;) {
    		int len  = fread(buf, 1, BUFFSIZE, fp);
    		int done = feof(fp);
    		if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
    			printf("XML_Parse: %s, at %d:%d\n",
    			    XML_ErrorString(XML_GetErrorCode(parser)),
    			    XML_GetCurrentLineNumber(parser),
    			    XML_GetCurrentColumnNumber(parser));
    			return (ENXIO);
    		}

		if (done) break;
	}
	fclose(fp); 

	*newroot = xst.root;
	return (0);
}

int load_xml_buf(Node **newroot, char *buf, int len, int conf_id)
{
	struct xml_state xst;

	xst.level = 0;
	xst.conf_id = conf_id;
	xst.last = xst.root = (*newroot)?*newroot:newNode("root", 0);

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characters);
	XML_SetUserData(parser, &xst);

    	if (! XML_Parse(parser, buf, len, 1)) {
    		printf("XML_Parse: %s, at %d:%d\n",
    		    XML_ErrorString(XML_GetErrorCode(parser)),
    		    XML_GetCurrentLineNumber(parser),
    		    XML_GetCurrentColumnNumber(parser));
    		return (ENXIO);
    	}

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

	if (top->firstAttr) {
		XMLattrdump(top->firstAttr, l+1);
	}

	printf(">");

	if (top->firstChild)
		/* Break line only if have childs */
		printf("\n");

	for (node = top->firstChild; node; node = node->next ) {

			XMLdumpL(node, l+1);
	}

	if (top->firstChild)
		/* indent close tag only if have childs */
		print_indent(l); 

	printf("</%s>\n", top->name);
}

void
XMLdump(Node *first)
{
	XMLdumpL(first, 0);
}


