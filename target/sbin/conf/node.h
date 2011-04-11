#ifndef __NODE_H__
#define __NODE_H__

struct node;
typedef struct node Node;
typedef struct node Attr;

enum NodeTypes_e {
	NODE_TYPE_NONE = 0,
	NODE_TYPE_NODE = 1,
	NODE_TYPE_ATTRBUTE,
};

struct node {
	int type;
	char * name;
	char * value;
	char * desc;
	Attr *firstAttr;
	Node *parent;
	Node *prev;
	Node *next;
	Node *firstChild;
};

Node *	newNode(const char *nodeName, Attr *firstAttr);
Node *	newNodeDescr(const char *nodeName, const char * nodeDesc, Attr *firstAttr);
Node *	newNodeNVD(const char *nodeName, const char *nodeValue, const char * nodeDesc);
Node *	appendChild(Node *parent, Node *newChild);
Node *	appendNewChild(Node * parent, const char *childName, Attr *childFirstAttr);
Node *	appendNewChildDescr(Node * parent, const char *childName, const char *childDescr, Attr *childFirstAttr);
void	freeNode(Node *node);



Node *	newAttr(const char *attrName, const char *attrValue);
Node *  addAttr(Node *node, Attr *newAttr);
void 	nodeSetAttr(Node *node, Attr *firstAttr);
Attr * 	nodeGetAttr(Node *node); /* return firstAttr node */
int	NodeHasAttrVal(Node *node, const char *attrName, const char *attrValue);

#endif /* __NODE_H__ */
