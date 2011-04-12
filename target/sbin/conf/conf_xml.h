#ifndef _XML_H_
#define _XML_H_

int load_xml(Node **newroot, char *file, int conf_id);
int load_xml_buf(Node **newroot, char *buf, int len, int conf_id);
void print_indent(int l);
void XMLattrdump(Node *first, int l);
void XMLdumpL(Node *top, int l);
void XMLdump(Node *first);


#endif /* _XML_H_ */

