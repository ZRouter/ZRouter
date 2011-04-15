#ifndef _CONF_STRBUF_H_
#define _CONF_STRBUF_H_


struct strbuf {
	char	*buf;
	char	*end;
	size_t	size;
};

typedef struct strbuf Strbuf;

void	strAppend(Strbuf *s, const char *str);
Strbuf 	*strInit();
char 	*strGet(Strbuf *s);
void	strFree(Strbuf *s);
void	strReset(Strbuf *s);

#endif /* _CONF_STRBUF_H_ */

