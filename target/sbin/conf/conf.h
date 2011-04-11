#ifndef _CONF_H_
#define _CONF_H_

#include <node.h>
#include <nodelist.h>

Node *addConfValue(Node *node, char *type, char *name, char *val, char *def);
Node *createConfValue(Node *node, char *type, char *name, char *val, char *def, char *descr);

#endif /* _CONF_H_ */

