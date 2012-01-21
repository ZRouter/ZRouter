#ifndef __NODELIST_H__
#define __NODELIST_H__

#include <conf_node.h>

struct NodeList {
	Node * nodes[256];
	int count;
};

typedef struct NodeList NodeList;

NodeList *	NodeListInit();
NodeList *	NodeListClone(NodeList *old); /* return new NodeList* */
void 		NodeListPush(NodeList *nl, Node *node);
Node * 		NodeListPop(NodeList *nl);
int 		NodeListCount(NodeList * nl);
Node * 		NodeListItem(NodeList * nl, int i);
void 		NodeListFree(NodeList * nl);

#endif /* __NODELIST_H__ */
