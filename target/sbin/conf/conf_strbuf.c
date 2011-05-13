

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

/* replace using posix regular expressions */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
int regreplace(regex_t *re, char *buf, int size, char *regex, char *replace);
int regreplace_single(char *string, int size, char *regex, char *replace);

int regreplace_single(char *string, int size, char *regex, char *replace)
{
	regex_t re1, *re;
	int ret;

	re = &re1;

	if (regcomp(re, regex, REG_EXTENDED))
		return (0);


	ret = regreplace(re, string, 1024, regex, replace);

	return (ret);
}

int regreplace(regex_t *re, char *buf, int size, char *regex, char *replace)
{
	char *pos, b[1024];
	int sub, so, n, p;
	regmatch_t pmatch [10]; /* regoff_t is int so size is int */

	int repllen = strlen(replace);

	if ((n = regexec(re, buf, 10, pmatch, 0))) {
		regerror(n, re, b, 1024);
		printf("ERROR: %s\n", b);
		return (0);
	}

	for (pos = replace; *pos; pos++) {
		if (*pos == '$' && *(pos + 1) > '0' && *(pos + 1) <= '9') {
			p = pos[1] - '0';
			so = (int)pmatch[p].rm_so;
			n = (int)pmatch[p].rm_eo - so;

			if (so < 0 || repllen + n - 1 > size)
				return (1);

			memmove(pos + n, pos + 2, strlen(pos) - 1);
			memmove(pos, buf + so, n);
			pos = pos + n - 2;
		}
	}

	sub = (int)pmatch[1].rm_so; /* no repeated replace when sub >= 0 */

	for (pos = buf; !regexec(re, pos, 1, pmatch, 0); ) {
		n = (int)pmatch[0].rm_eo - (int)pmatch[0].rm_so;
		pos += (int)pmatch[0].rm_so;

		if (strlen(buf) - n + repllen + 1 > size)
			return (1);

		memmove(pos + repllen, pos + n, strlen(pos) - n + 1);
		memmove(pos, replace, repllen);
		pos += repllen;

		if (sub >= 0)
			break;
	}

	return 0;
}

