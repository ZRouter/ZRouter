#ifndef _PARSER_H_
#define _PARSER_H_

#define CONFTYPE(t)	{#t, t ## _parser }

char *IPADDR_parser(char *in);
char *IPMASK_parser(char *in);
char *CIDR_parser(char *in);
char *BOOL_parser(char *in);
char *parseAsType(char *type, char *val);


struct confTypes {
	char *name;
	char *(*parser)(char *);
};

#endif /* _PARSER_H_ */

