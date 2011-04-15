

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


