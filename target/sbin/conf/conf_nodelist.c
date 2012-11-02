#include <stdlib.h> /* malloc/free */
#include <string.h> /* memcpy */

#include <conf_nodelist.h>

NodeList *
NodeListInit()
{
	NodeList *nl = (NodeList *) malloc(sizeof(NodeList));
	nl->count = 0;
	return (nl);
}

NodeList *
NodeListClone(NodeList *old)
{
        NodeList * new = NodeListInit();
        memcpy(new, old, sizeof(NodeList));
        return (new);
}

void
NodeListPush(NodeList *nl, Node *node)
{
	nl->nodes[nl->count++] = node;
}

Node *
NodeListPop(NodeList *nl)
{
	return (nl->nodes[nl->count--]);
}

int
NodeListCount(NodeList * nl)
{
	return (nl->count);
}

Node *
NodeListItem(NodeList * nl, int i)
{
	return (nl->nodes[i]);
}

void
NodeListFree(NodeList * nl)
{
	free(nl);
}



