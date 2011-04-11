
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
//#include <sys/cdefs.h>

#include <ctype.h>
#include <unistd.h>

//#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <parsers.h>


static char obuf[32];

struct confTypes conftypes[] = {
	CONFTYPE(BOOL),
	CONFTYPE(IPADDR),
	CONFTYPE(IPMASK),
	CONFTYPE(CIDR),
	{0, 0}
};



/*
 * Configuration values stored always as string
 */

char *
IPADDR_parser(char *in)
{
	struct in_addr addr;

	if (!in)
		return (0);

	if (inet_aton(in, &addr) == 0) {
		/* parse error */
		return (0);
	}

	return(inet_ntoa(addr));
}
char *
IPMASK_parser(char *in)
{
	unsigned int num, i, len;
	char * endptr;

	if (!in)
		return (0);

	len = strlen(in);
	num = strtoul(in, &endptr, 0);
	if ((in + len) == endptr) {
		/* dec/oct/hex number, no other chars */
		/* maybe bit len, maybe binary mask */
		if (num & 0x80000000) {
			/* first bit is 1, so binary mask */
			return (IPADDR_parser(in));
		} else if (num <= 32) {
			/* length in bits */
			char buf[16];

			for (i = 0, len = 0; i < num; i ++)
				len |= 1 << (31 - i);

			sprintf(buf, "0x%08x", len);

			return (IPADDR_parser(buf));
		} else {
			/* parse error */
			printf("parse error num = 0x%08x\n", num);
			return (0);
		}
	} else {

		for (i = 0, num = 0; i < len; i++) {
			if (in[i] == '.') {
				num ++;
				continue;
			}
			if (isdigit(in[i]))
				continue;
			/* parse error */
			printf("parse error in = \"%s\", illegal characters\n", in);
			return (0);
		}
		if (len > 15 || num != 3) {
			/* parse error */
			printf("parse error in = \"%s\", wrong format\n", in);
			return (0);
		}
		return (IPADDR_parser(in));
	}
}

char *
CIDR_parser(char *in)
{
	char *mask, *ip, *i, *cidr;

	if (!in)
		return (0);

	ip = i = strdup(in);
	mask = strchr(ip, '/');
	if (mask) {
		*(mask++) = '\0';
		ip = IPADDR_parser(ip);
		mask = IPMASK_parser(mask);
		free(i);
		if (!ip || !mask)
			return (0);
		cidr = obuf;
		sprintf(cidr, "%s/%s", ip, mask);
		printf("%s: %s\n", __func__, cidr);
		return (cidr);
	} else {
		printf("parse error in = \"%s\", CIDR must contain '/' char\n", in);
		free(i);
		return (0);
	}
}

char *
BOOL_parser(char *in)
{
	char *mask, *ip, *i, *cidr;

	if (!in)
		return (0);

	if (strlen(in) == 0)
		return (0);

	if (strlen(in) == 1) {
		switch (in[0]) {
		case 'T':
		case 'Y':
		case 't':
		case 'y':
		case '1':
			return ("TRUE");
		case 'F':
		case 'N':
		case 'f':
		case 'n':
		case '0':
			return ("FALSE");
		default:
			return (0);
		}
	}
	if (strcasecmp(in, "true") == 0 || strcasecmp(in, "yes") == 0 ) {
		return ("TRUE");
	} else if (strcasecmp(in, "false") == 0 || strcasecmp(in, "no") == 0 ) {
		return ("FALSE");
	}
	return (0);
}

int
cidr2net_host(char * cidr, char *net, char *host)
{
	char *vcidr;

	if (!cidr || !net || !host)
		return (1);

	vcidr = CIDR_parser(cidr);
}

char *
parseAsType(char *type, char *val)
{
	int i;

	for (i = 0; conftypes[i].name; i ++) {
		if (strcmp(type, conftypes[i].name) == 0)
			break;
	}

	if (!conftypes[i].name) {
		printf("Unknown type \"%s\"\n", type);
		return (0);
	}

	printf("%s: parse as type %s\n", __func__, conftypes[i].name);
	return (conftypes[i].parser(val));
}
