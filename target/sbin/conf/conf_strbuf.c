

//#include <sys/types.h>
//#include <strings.h>

#include <stdlib.h>
#include <string.h>
#include <conf_strbuf.h>


#define STRBUF_BLOCKSIZE 0x1000

void
strAppend(Strbuf *s, const char *str)
{
	int len;

	len = strlen(str);
	if ((s->end - s->buf + len + 1) > s->size) {
		s->size += STRBUF_BLOCKSIZE;
		s->buf = realloc(s->buf, s->size);

		if (!s->buf)
			return;
	}

	strncpy(s->end, str, len+1);
	s->end += len;
}

int
strReplaceOne(Strbuf *s, const char *old, const char *new)
{
	int len, nlen, olen;

	char *match = strstr(s->buf, old);
	if (!match) {
		return (0);
	}

	nlen = strlen(new);
	olen = strlen(old);
	len = strlen(s->buf);

	if ((s->end - s->buf + (nlen-olen) + 1) > s->size) {
		s->size += STRBUF_BLOCKSIZE;
		s->buf = realloc(s->buf, s->size);

		if (!s->buf) {
			return (-1);
		}
	}

	/* Same again, because realloc can move as to another location */
	match = strstr(s->buf, old);
	/* move tail */
	memmove(match+nlen, match+olen, len - (match - s->buf) - olen + 1);
	/* copy new w/o \0 */
	strncpy(match, new, nlen);

	s->end = len - olen + nlen;

	return (1);
}

int
strReplace(Strbuf *s, const char *old, const char *new)
{
	int ret, count = 0;

	while (1) {
		ret = strReplaceOne(s, old, new);
		if (ret == 0) {
			return (count);
		} else if (ret > 0) {
			count += ret;
		} else {
			return (ret);
		}
	}
}

Strbuf *
strInit()
{
	Strbuf *s;

	s = malloc(sizeof(Strbuf));
	s->buf = s->end = malloc(STRBUF_BLOCKSIZE);
	s->size = STRBUF_BLOCKSIZE;

	return (s);
}

void
strReset(Strbuf *s)
{

	s->end = s->buf;
	s->buf[0] = '\0';

	return;
}

char *
strGet(Strbuf *s)
{

	return (s->buf);
}

void
strFree(Strbuf *s)
{

	free(s->buf);
	free(s);
}


