
/*
 * Copyright (c) 2001-2002 Packet Design, LLC.
 * All rights reserved.
 * 
 * Subject to the following obligations and disclaimer of warranty,
 * use and redistribution of this software, in source or object code
 * forms, with or without modifications are expressly permitted by
 * Packet Design; provided, however, that:
 * 
 *    (i)  Any and all reproductions of the source or object code
 *         must include the copyright notice above and the following
 *         disclaimer of warranties; and
 *    (ii) No rights are granted, in any manner or form, to use
 *         Packet Design trademarks, including the mark "PACKET DESIGN"
 *         on advertising, endorsements, or otherwise except as such
 *         appears in the above copyright notice or in the software.
 * 
 * THIS SOFTWARE IS BEING PROVIDED BY PACKET DESIGN "AS IS", AND
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, PACKET DESIGN MAKES NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, REGARDING
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY AND ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT.  PACKET DESIGN DOES NOT WARRANT, GUARANTEE,
 * OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF, OR THE RESULTS
 * OF THE USE OF THIS SOFTWARE IN TERMS OF ITS CORRECTNESS, ACCURACY,
 * RELIABILITY OR OTHERWISE.  IN NO EVENT SHALL PACKET DESIGN BE
 * LIABLE FOR ANY DAMAGES RESULTING FROM OR ARISING OUT OF ANY USE
 * OF THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE, OR CONSEQUENTIAL
 * DAMAGES, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, LOSS OF
 * USE, DATA OR PROFITS, HOWEVER CAUSED AND UNDER ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF PACKET DESIGN IS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Archie Cobbs <archie@freebsd.org>
 */

#include <sys/types.h>
#include <sys/queue.h>

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>

#include <openssl/ssl.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "util/typed_mem.h"
#include "http/http_defs.h"
#include "http/http_server.h"
#include "http/http_internal.h"

#define CR		'\r'
#define LF		'\n'

#define MAX_STRING	(64 * 1024)

#define MEM_TYPE_HEAD	"http_head"
#define MEM_TYPE_HDRS	"http_head.hdrs"
#define MEM_TYPE_NAME	"http_head.name"
#define MEM_TYPE_VALUE	"http_head.value"

#define issep(ch)	(strchr("()<>@,;:\\\"/[]?={} \t", ch) != NULL)

struct http_header {
	char	*name;
	char	*value;
};

struct http_head {
	char			*words[3];	/* first line stuff */
	int			num_hdrs;	/* number of headers */
	struct http_header	*hdrs;		/* headers, sorted */
};

/*
 * Internal variables
 */
static const char *header_sort[] = {
	HTTP_HEADER_DATE,
	HTTP_HEADER_SERVER,
	HTTP_HEADER_CONNECTION,
	HTTP_HEADER_PROXY_CONNECTION,
	HTTP_HEADER_CACHE_CONTROL,
	HTTP_HEADER_PRAGMA,
	HTTP_HEADER_LAST_MODIFIED,
	HTTP_HEADER_WWW_AUTHENTICATE,
	HTTP_HEADER_CONTENT_TYPE,
	HTTP_HEADER_CONTENT_LENGTH,
	HTTP_HEADER_CONTENT_ENCODING,
};
#define NUM_HEADER_SORT	(sizeof(header_sort) / sizeof(*header_sort))

/*
 * Internal functions
 */
static int	http_head_special(const char *name);
static int	http_header_cmp(const void *v1, const void *v2);
static char	*read_hval(FILE *fp);
static char	*read_line(FILE *fp, const char *mtype);
static char	*read_token(FILE *fp, int liberal, const char *mtype);
static void	read_whitespace(FILE *fp);
static int	addch(char **sp, int *slen, int ch, const char *mtype);

/*
 * Create new header object.
 */
struct http_head *
_http_head_new(void)
{
	struct http_head *head;

	/* Create structure */
	if ((head = MALLOC(MEM_TYPE_HEAD, sizeof(*head))) == NULL)
		return (NULL);
	memset(head, 0, sizeof(*head));
	return (head);
}

/*
 * Free a header object.
 */
void
_http_head_free(struct http_head **headp)
{
	struct http_head *const head = *headp;
	int i;

	if (head == NULL)
		return;
	for (i = 0; i < 3; i++)
		FREE(MEM_TYPE_VALUE, head->words[i]);
	for (i = 0; i < head->num_hdrs; i++) {
		struct http_header *const hdr = &head->hdrs[i];

		FREE(MEM_TYPE_NAME, hdr->name);
		FREE(MEM_TYPE_VALUE, hdr->value);
	}
	FREE(MEM_TYPE_HDRS, head->hdrs);
	FREE(MEM_TYPE_HEAD, head);
	*headp = NULL;
}

/*
 * Copy headers
 */
struct http_head *
_http_head_copy(struct http_head *head0)
{
	struct http_head *head;
	int i;

	/* Get new header struct */
	if ((head = _http_head_new()) == NULL)
		goto fail;

	/* Copy first line words */
	for (i = 0; i < 3; i++) {
		if (head0->words[i] == NULL)
			continue;
		if ((head->words[i] = STRDUP(MEM_TYPE_VALUE,
		    head0->words[i])) == NULL)
			goto fail;
	}

	/* Copy other headers */
	for (i = 0; i < head0->num_hdrs; i++) {
		const char *name;
		const char *value;

		if (_http_head_get_by_index(head0, i, &name, &value) == -1)
			goto fail;
		if (_http_head_set(head, 0, name, "%s", value) == -1)
			goto fail;
	}

	/* Done */
	return (head);

fail:
	_http_head_free(&head);
	return (NULL);
}

/*
 * Get a header field value.
 *
 * For headers listed multiple times, this only gets the first instance.
 */
const char *
_http_head_get(struct http_head *head, const char *name)
{
	struct http_header key;
	struct http_header *hdr;
	int i;

	/* First line stuff */
	if ((i = http_head_special(name)) != -1)
		return (head->words[i]);

	/* Normal headers */
	key.name = (char *)name;
	if ((hdr = bsearch(&key, head->hdrs, head->num_hdrs,
	    sizeof(*head->hdrs), http_header_cmp)) == NULL) {
		errno = ENOENT;
		return (NULL);
	}
	return (hdr->value);
}

/*
 * Get the number of headers
 */
int
_http_head_num_headers(struct http_head *head)
{
	return (head->num_hdrs);
}

/*
 * Get a header by index.
 */
int
_http_head_get_by_index(struct http_head *head, u_int index,
	const char **namep, const char **valuep)
{
	if (index >= head->num_hdrs) {
		errno = EINVAL;
		return (-1);
	}
	if (namep != NULL)
		*namep = head->hdrs[index].name;
	if (valuep != NULL)
		*valuep = head->hdrs[index].value;
	return (0);
}

/*
 * Get the names of all headers.
 */
int
_http_head_get_headers(struct http_head *head,
	const char **names, size_t max_names)
{
	int i;

	if (max_names > head->num_hdrs)
		max_names = head->num_hdrs;
	for (i = 0; i < max_names; i++)
		names[i] = head->hdrs[i].name;
	return (i);
}

/*
 * Set header value, in either append or replace mode.
 */
int
_http_head_set(struct http_head *head, int append,
	const char *name, const char *valfmt, ...)
{
	va_list args;
	int ret;

	va_start(args, valfmt);
	ret = _http_head_vset(head, append, name, valfmt, args);
	va_end(args);
	return (ret);
}

/*
 * Set header value, in either append or replace mode.
 */
int
_http_head_vset(struct http_head *head, int append,
	const char *name, const char *valfmt, va_list args)
{
	struct http_header key;
	struct http_header *hdr;
	char *value;
	void *mem;
	int i;

	/* Generate value */
	VASPRINTF(MEM_TYPE_VALUE, &value, valfmt, args);
	if (value == NULL)
		return (-1);

	/* First line stuff */
	if ((i = http_head_special(name)) != -1) {
		FREE(MEM_TYPE_VALUE, head->words[i]);
		head->words[i] = value;
		return (0);
	}

	/* If header doesn't already exist, add it (unless special) */
	key.name = (char *)name;
	if (strcasecmp(name, HTTP_HEADER_SET_COOKIE) == 0	/* XXX blech */
	    || (hdr = bsearch(&key, head->hdrs, head->num_hdrs,
	      sizeof(*head->hdrs), http_header_cmp)) == NULL) {

		/* Extend headers array */
		if ((mem = REALLOC(MEM_TYPE_HDRS, head->hdrs,
		    (head->num_hdrs + 1) * sizeof(*head->hdrs))) == NULL) {
			FREE(MEM_TYPE_VALUE, value);
			return (-1);
		}
		head->hdrs = mem;
		hdr = &head->hdrs[head->num_hdrs];

		/* Copy name */
		if ((hdr->name = STRDUP(MEM_TYPE_NAME, name)) == NULL) {
			FREE(MEM_TYPE_VALUE, value);
			return (-1);
		}

		/* Add new header */
		hdr->value = value;
		head->num_hdrs++;

		/* Keep array sorted */
		(void)mergesort(head->hdrs, head->num_hdrs,
		    sizeof(*head->hdrs), http_header_cmp);
		return (0);
	}

	/* Append or replace */
	if (append) {
		const int plen = strlen(hdr->value);

		if ((mem = REALLOC(MEM_TYPE_VALUE,
		    hdr->value, plen + strlen(value) + 3)) == NULL) {
			FREE(MEM_TYPE_VALUE, value);
			return (-1);
		}
		hdr->value = mem;
		sprintf(hdr->value + plen, ", %s", value);
		FREE(MEM_TYPE_VALUE, value);
	} else {
		FREE(MEM_TYPE_VALUE, hdr->value);
		hdr->value = value;
	}
	return (0);
}

/*
 * Remove a header.
 */
int
_http_head_remove(struct http_head *head, const char *name)
{
	struct http_header key;
	struct http_header *hdr;
	int i;

	/* First line stuff */
	if ((i = http_head_special(name)) != -1) {
		if (head->words[i] != NULL) {
			FREE(MEM_TYPE_VALUE, head->words[i]);
			head->words[i] = NULL;
			return (1);
		}
		return (0);
	}

	/* Does header exist? */
	key.name = (char *)name;
	if ((hdr = bsearch(&key, head->hdrs, head->num_hdrs,
	    sizeof(*head->hdrs), http_header_cmp)) == NULL)
		return (0);

	/* Remove header */
	i = hdr - head->hdrs;
	FREE(MEM_TYPE_NAME, hdr->name);
	FREE(MEM_TYPE_VALUE, hdr->value);
	memmove(head->hdrs + i, head->hdrs + i + 1,
	    (--head->num_hdrs - i) * sizeof(*head->hdrs));
	return (1);
}

/*
 * Return index for one of the 'special' headers representing
 * one of the three parts of the first line of the HTTP request
 * or response.
 */
static int
http_head_special(const char *name)
{
	if (name == HDR_REQUEST_METHOD)		/* also HDR_REPLY_VERSION */
		return (0);
	if (name == HDR_REQUEST_URI)		/* also HDR_REPLY_STATUS */
		return (1);
	if (name == HDR_REQUEST_VERSION)	/* also HDR_REPLY_REASON */
		return (2);
	return (-1);
}

/*
 * Compare two headers by name.
 *
 * XXX this could be faster
 */
static int
http_header_cmp(const void *v1, const void *v2)
{
	const struct http_header *const hdrs[] = { v1, v2 };
	int sortval[2];
	int i, j;

	/* Check assigned ordering list */
	for (i = 0; i < 2; i++) {
		const char *hdr = hdrs[i]->name;

		sortval[i] = INT_MAX;
		for (j = 0; j < NUM_HEADER_SORT; j++) {
			if (strcasecmp(hdr, header_sort[j]) == 0) {
				sortval[i] = j;
				break;
			}
		}
	}
	if (sortval[0] == INT_MAX && sortval[1] == INT_MAX)
		return (strcasecmp(hdrs[0]->name, hdrs[1]->name));
	return (sortval[0] - sortval[1]);
}

/*
 * Read an entire HTTP request or response header.
 */
int
_http_head_read(struct http_head *head, FILE *fp, int req)
{
	int ch;

	/* Read any initial whitespace including CR, LF */
	while ((ch = getc(fp)) != EOF) {
		if (!isspace(ch)) {
			ungetc(ch, fp);
			break;
		}
	}

	/* Read first word */
	FREE(MEM_TYPE_VALUE, head->words[0]);
	if ((head->words[0] = read_token(fp, 1, MEM_TYPE_VALUE)) == NULL)
		return (-1);

	/* Get whitespace */
	read_whitespace(fp);

	/* Read second word */
	FREE(MEM_TYPE_VALUE, head->words[1]);
	if ((head->words[1] = read_token(fp, 1, MEM_TYPE_VALUE)) == NULL)
		return (-1);

	/* Get whitespace */
	read_whitespace(fp);

	/* Read third and remaining words on the line including CR-LF */
	FREE(MEM_TYPE_VALUE, head->words[2]);
	if ((head->words[2] = read_line(fp, MEM_TYPE_VALUE)) == NULL)
		return (-1);

	/* Special format for HTTP 0.9 request (no protocol or headers) */
	if (req && *head->words[2] == '\0') {
		FREE(MEM_TYPE_VALUE, head->words[2]);
		if ((head->words[2] = STRDUP(MEM_TYPE_VALUE,
		    HTTP_PROTO_0_9)) == NULL)
			return (-1);
		return (0);
	}

	/* Read headers */
	if (_http_head_read_headers(head, fp) == -1)
		return (-1);

	/* Done */
	return (0);
}

/*
 * Read HTTP headers
 */
int
_http_head_read_headers(struct http_head *head, FILE *fp)
{
	char *name;
	char *value;
	int ret;
	int ch;

	while (1) {

		/* Check for CR-LF */
		if ((ch = getc(fp)) == EOF) {
			if (ferror(fp))
				goto fail;
			goto fail_invalid;
		}
		if (ch == CR) {
			if ((ch = getc(fp)) != LF) {
				if (ch == EOF && ferror(fp))
					goto fail;
				goto fail_invalid;
			}
			return (0);
		}
		ungetc(ch, fp);

		/* Get header name */
		if ((name = read_token(fp, 0, MEM_TYPE_NAME)) == NULL)
			goto fail;

		/* Get separator */
		if ((ch = getc(fp)) != ':') {
			FREE(MEM_TYPE_NAME, name);
			goto fail_invalid;
		}

		/* Get whitespace */
		read_whitespace(fp);

		/* Get header value including final CR-LF */
		if ((value = read_hval(fp)) == NULL) {
			FREE(MEM_TYPE_NAME, name);
			goto fail;
		}

		/* Append to header value */
		ret = _http_head_set(head, 1, name, "%s", value);
		FREE(MEM_TYPE_NAME, name);
		FREE(MEM_TYPE_VALUE, value);
		if (ret == -1)
			goto fail;
	}

fail_invalid:
	errno = EINVAL;
fail:
	return (-1);
}

/*
 * Write out an HTTP header.
 */
int
_http_head_write(struct http_head *head, FILE *fp)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (head->words[i] == NULL) {
			errno = EINVAL;
			return (-1);
		}
	}
	fprintf(fp, "%s %s %s\r\n",
	    head->words[0], head->words[1], head->words[2]);
	for (i = 0; i < head->num_hdrs; i++) {
		struct http_header *const hdr = &head->hdrs[i];

		fprintf(fp, "%s: %s\r\n", hdr->name, hdr->value);
	}
	fprintf(fp, "\r\n");
#if 0
	fflush(fp);
#endif
	return (0);
}

/*
 * Figure out whether there is anything in the head or not.
 */
int
_http_head_has_anything(struct http_head *head)
{
	return (head->words[0] != NULL);
}

/*
 * Determine if keep alive is requested.
 */
int
_http_head_want_keepalive(struct http_head *head)
{
	const char *hval;

	return (((hval = _http_head_get(head, HTTP_HEADER_CONNECTION)) != NULL
	      || (hval = _http_head_get(head,
	       HTTP_HEADER_PROXY_CONNECTION)) != NULL)
	    && strcasecmp(hval, "Keep-Alive") == 0);
}

/*
 * Read an HTTP header value.
 */
static char *
read_hval(FILE *fp)
{
	char *s = NULL;
	int inquote = 0;
	int bslash = 0;
	int slen = 0;
	int ch;

	/* Read characters */
	while (1) {
		if ((ch = getc(fp)) == EOF) {
			if (ferror(fp))
				goto fail;
			goto fail_invalid;
		}
		if (bslash) {				/* implies inquote */
			if (!addch(&s, &slen, ch, MEM_TYPE_VALUE))
				goto fail;
			bslash = 0;
			continue;
		}
		switch (ch) {
		case '"':
			inquote = !inquote;
			break;
		case '\\':
			if (inquote) {
				bslash = 1;
				continue;
			}
		case CR:
			if ((ch = getc(fp)) != LF) {	/* read linefeed */
				if (ch == EOF && ferror(fp))
					goto fail;
				goto fail_invalid;
			}
			if ((ch = getc(fp)) == EOF) {	/* get next char */
				if (ferror(fp))
					goto fail;
				goto fail_invalid;
			}
			if (ch == ' ' || ch == '\t') {	/* line continuation */
				read_whitespace(fp);
				ch = ' ';
				break;
			}
			ungetc(ch, fp);
			goto done;
		default:
			if (iscntrl(ch) && ch != ' ' && ch != '\t')
				goto fail_invalid;
			break;
		}
		if (!addch(&s, &slen, ch, MEM_TYPE_VALUE))
			goto fail;
	}

done:
	/* Terminate and return string */
	if (!addch(&s, &slen, '\0', MEM_TYPE_VALUE))
		goto fail;
	return (s);

fail_invalid:
	errno = EINVAL;
fail:
	FREE(MEM_TYPE_VALUE, s);
	return (NULL);
}

/*
 * Read up through end of line and return value, not including CR-LF.
 */
static char *
read_line(FILE *fp, const char *mtype)
{
	char *s = NULL;
	int slen = 0;
	int ch;

	/* Read characters */
	while (1) {
		if ((ch = getc(fp)) == EOF) {
			if (ferror(fp))
				goto fail;
			goto fail_invalid;
		}
		if (ch == CR) {
			if ((ch = getc(fp)) != LF) {
				if (ch == EOF && ferror(fp))
					goto fail;
				goto fail_invalid;
			}
			goto done;
		}
		if (ch == LF)			/* handle broken clients */
			break;
		if (!addch(&s, &slen, ch, mtype))
			goto fail;
	}

done:
	/* Terminate and return string */
	if (!addch(&s, &slen, '\0', mtype))
		goto fail;
	return (s);

fail_invalid:
	errno = EINVAL;
fail:
	FREE(mtype, s);
	return (NULL);
}

/*
 * Read an HTTP header token.
 */
static char *
read_token(FILE *fp, int liberal, const char *mtype)
{
	char *s = NULL;
	int slen = 0;
	int ch;

	/* Read characters */
	while (1) {
		if ((ch = getc(fp)) == EOF) {
			if (ferror(fp))
				goto fail;
			goto fail_invalid;
		}
		if (!isprint(ch)
		    || (ch == ' ' || ch == '\t')
		    || (!liberal && issep(ch))) {
			ungetc(ch, fp);
			break;
		}
		if (!addch(&s, &slen, ch, mtype))
			goto fail;
	}

	/* Terminate and return string */
	if (!addch(&s, &slen, '\0', mtype))
		goto fail;
	return (s);

fail_invalid:
	errno = EINVAL;
fail:
	FREE(mtype, s);
	return (NULL);
}

/*
 * Read any amount of whitespace.
 */
static void
read_whitespace(FILE *fp)
{
	int ch;

	while (1) {
		if ((ch = getc(fp)) == EOF)
			return;
		if (ch != ' ' && ch != '\t') {
			ungetc(ch, fp);
			break;
		}
	}
}

/*
 * Add a character to a malloc'd string.
 */
static int
addch(char **sp, int *slen, int ch, const char *mtype)
{
	void *mem;

	if (*slen >= MAX_STRING) {
		errno = E2BIG;
		return (0);
	}
	if (*slen % 128 == 0) {
		if ((mem = REALLOC(mtype, *sp, *slen + 128)) == NULL)
			return (0);
		*sp = mem;
	}
	(*sp)[(*slen)++] = ch;
	return (1);
}

