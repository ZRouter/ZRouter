
#include <stdio.h>
#include <stdlib.h> /* malloc/free */
#include <string.h> /* strdup */
#include <strings.h> /* bzero */

#include <conf_node.h>


Node *
newNode(const char *name, Node *firstAttr)
{
	Node *node;

	if (!name)
		return (0);
	node = (Node*)malloc(sizeof(Node));
	bzero(node, sizeof(Node));
	node->name = strdup(name);
	if (firstAttr)
		node->firstAttr = firstAttr;
	node->type = NODE_TYPE_NODE;
	return (node);
}

Node *
newNodeNVD(const char *name, const char *value, const char * desc)
{
	Node *node = newNode(name, 0);
	node->value = strdup(value);
	if (node->desc)
		node->desc = strdup(desc);
	return (node);
}

void
nodeSetAttr(Node *node, Node *attr)
{
	node->firstAttr = attr;
}

Node *
nodeGetAttr(Node *node)
{
	return (node->firstAttr);
}

Node *
newNodeDescr(const char *name, const char * desc, Node *firstAttr)
{
	if (!name || !desc)
		return (0);
	Node *node = newNode(name, firstAttr);
	node->desc = strdup(desc);
	return (node);
}

Node *
appendChild(Node *parent, Node *new)
{
	Node *node;
	if (!parent->firstChild) {
		parent->firstChild = new;
		new->parent = parent;
		return (new);
	}
	for (node = parent->firstChild; node; node = node->next)
		if (!node->next) 
			break;
	node->next = new;
	new->parent = parent;
	new->prev  = node;
	return (new);
}

Node *
appendNewChild(Node * parent, const char *name, Node *firstAttr)
{
	Node *new = newNode(name, firstAttr);
	return (appendChild(parent, new));
}

Node *
appendNewChildDescr(Node * parent, const char *name, const char *desc, Node *firstAttr)
{
	Node *new = appendNewChild(parent, name, firstAttr);
	new->desc = strdup(desc);
	return (new);
}

Node *
applyNode(Node *parent, const char *name)
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

void
freeNode(Node *node)
{
	if (node->firstChild)
		freeNode(node->firstChild);
	if (node->next)
		freeNode(node->next);
	free((void *)node->name);
	if (node->desc)
		free((void *)node->desc);
	if (node->value)
		free((void *)node->value);
	free(node);
}

Node *
findNodePath(Node *root, const char *path)
{
	char * subpath, *name, *p;
	Node *node, *new;

	if (!root || !path)
		return (0);

	if (strcmp(root->name, path) == 0)
		return (root);

	p = strdup(path);

	subpath = strchr(p, '.');

	if (subpath)
		*(subpath++) = '\0';

	name = p;

	for (node = root; node; node = node->next ) {
		if (strcmp(node->name, name) == 0) {
			if (node->firstChild) {
				new = findNodePath(node->firstChild, subpath);
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
		new = findNodePath(root->firstAttr, subpath);
		if (new) {
			free(p);
			return (new);
		}
	}

	free(p);
	return (0);
}

Attr *
newAttr(const char *name, const char *value)
{
	Attr *attr;
	if (!name)
		return (0);

	attr = newNode(name, 0);
	attr->type = NODE_TYPE_ATTRBUTE;
	if (value)
		attr->value = strdup(value);
	return (attr);
}

Node *
addAttr(Node *node, Attr *newAttr)
{
	Attr *topAttr;
	if (!node || !newAttr)
		return (0);
	topAttr = node->firstAttr;
	node->firstAttr = newAttr;
	newAttr->next = topAttr;
	if (topAttr)
		topAttr->prev = newAttr;

	return (node);
}

void
applyAttr(Node *parent, const char *name, const char *value)
{
	Node *node;

	if (!parent) return;

	/* Apply to matched attribute */
	if (parent->firstAttr)
		for (node = parent->firstAttr; node; node = node->next) {
			if (strcmp(node->name, name) == 0) {
				free((void *)node->value);
				node->value = value;
				return;
			}
		}

	/* Create if no match */
	addAttr(parent, newAttr(name, value));

	return;
}

int
NodeHasAttrVal(Node *node, const char *attrName, const char *attrValue)
{
	Attr *attr;
	int ret = 0;
	if (!node || !attrName)
		return (-1);
	if (!node->firstAttr)
		return (0);

	for (attr = node->firstAttr; attr; attr = attr->next) {
		if (!strcasecmp(attr->name, attrName)) {
			/* match attrName */
			if (attrValue)
				/* attrValue checked */
				if (!strcasecmp(attr->value, attrValue))
					/* match attrValue */
					ret++;
			else
				/* attrValue don`t checked, so matched */
				ret ++;
		}
	}
	printf("%s: return %d\n", __func__, ret);
	return (ret);
}












