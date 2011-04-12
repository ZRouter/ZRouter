#ifndef _XML_H_
#define _XML_H_

int load_xml(Node **newroot, char *file);
int load_xml_buf(Node **newroot, char *buf, int len);
void print_indent(int l);
void XMLattrdump(Node *first, int l);
void XMLdumpL(Node *top, int l);
void XMLdump(Node *first);


#endif /* _XML_H_ */

