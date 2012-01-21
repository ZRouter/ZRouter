
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
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include <openssl/ssl.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "io/boundary_fp.h"
#include "io/string_fp.h"
#include "util/typed_mem.h"
#include "http/http_defs.h"
#include "http/http_server.h"
#include "http/http_internal.h"

#define CR		'\r'
#define LF		'\n'

#define MIME_MEM_TYPE		"mime_multipart"

struct mime_multipart {
	u_int			nparts;
	u_int			nalloc;
	struct mime_part	*parts;
};

struct mime_part {
	struct http_head	*head;			/* mime headers */
	u_char			*data;			/* part's data */
	u_int			dlen;			/* length of data */
};

/* Internal functions */
static http_mime_handler_t _http_request_read_mime_handler;

/*
 * Read in multi-part MIME data all at once into memory.
 */
struct mime_multipart *
http_request_read_mime_multipart(struct http_request *req)
{
	struct mime_multipart *mp;

	/* Allocate multipart structure */
	if ((mp = MALLOC(MIME_MEM_TYPE, sizeof(*mp))) == NULL)
		return (NULL);
	memset(mp, 0, sizeof(*mp));

	/* Read in parts */
	if (http_request_get_mime_multiparts(req,
	    _http_request_read_mime_handler, mp) < 0) {
		http_mime_multipart_free(&mp);
		return (NULL);
	}

	/* Done */
	return (mp);
}

/*
 * Handler used by http_request_read_mime_multipart().
 */
static int
_http_request_read_mime_handler(void *arg, struct mime_part *part0, FILE *fp)
{
	struct mime_multipart *const mp = arg;
	struct mime_part *part;
	FILE *sb = NULL;
	char buf[256];
	int nr;

	/* Allocate new part structure */
	if (mp->nalloc < mp->nparts + 1) {
		const u_int new_alloc = (mp->nalloc + 1) * 2;
		struct mime_part *new_parts;

		if ((new_parts = REALLOC(MIME_MEM_TYPE,
		    mp->parts, new_alloc * sizeof(*mp->parts))) == NULL)
			return (-1);
		mp->parts = new_parts;
		mp->nalloc = new_alloc;
	}
	part = &mp->parts[mp->nparts++];
	memset(part, 0, sizeof(*part));

	/* Copy headers for this part */
	if ((part->head = _http_head_copy(part0->head)) == NULL)
		return (-1);

	/* Slurp part's data into memory buffer */
	if ((sb = string_buf_output(MIME_MEM_TYPE)) == NULL)
		return (-1);
	while ((nr = fread(buf, 1, sizeof(buf), fp)) != 0) {
		if (fwrite(buf, 1, nr, sb) != nr) {
			fclose(sb);
			return (-1);
		}
	}
	if (ferror(fp)) {
		fclose(sb);
		return (-1);
	}

	/* Extract data from string buffer stream */
	part->dlen = string_buf_length(sb);
	if ((part->data = (u_char *)string_buf_content(sb, 1)) == NULL) {
		part->dlen = 0;
		fclose(sb);
		return (-1);
	}
	fclose(sb);

	/* Done */
	return (0);
}

/*
 * Read in multi-part MIME data, and call the handler for each part.
 *
 * Returns the number of parts successfully read, or the ones
 * complement of that number if the handler aborted.
 */
int
http_request_get_mime_multiparts(struct http_request *req,
	http_mime_handler_t *handler, void *arg)
{
	const char *hval;
	FILE *fp = NULL;
	char boundary[256];
	char buf[256];
	char *tokctx;
	FILE *input;
	int nparts;
	char *s;

	/* Get POST input stream */
	if ((input = http_request_get_input(req)) == NULL)
		return (~0);

	/* Get boundary string */
	if ((hval = http_request_get_header(req,
	      HTTP_HEADER_CONTENT_TYPE)) == NULL
	    || strlen(hval) > sizeof(buf) - 1)
		goto bogus;
	strlcpy(buf, hval, sizeof(buf));
	if ((s = strchr(buf, ';')) == NULL)
		goto bogus;
	*s++ = '\0';
	if (strcasecmp(buf, HTTP_CTYPE_MULTIPART_FORMDATA) != 0)
		goto bogus;
	if ((s = strtok_r(s, " \t;=", &tokctx)) == NULL
	    || strcasecmp(s, "boundary") != 0
	    || (s = strtok_r(NULL, " \t;=", &tokctx)) == NULL) {
bogus:		errno = EINVAL;
		return (~0);
	}
	snprintf(boundary, sizeof(boundary), "\r\n--%s", s);

	/* Read up through the initial boundary string */
	if ((fp = boundary_fopen(input, boundary + 2, 0)) == NULL)
		return (~0);
	while (fgets(buf, sizeof(buf), fp) != NULL)
		;
	if (ferror(fp)) {
		fclose(fp);
		return (~0);
	}
	fclose(fp);

	/* Read in each part */
	for (nparts = 0; 1; nparts++) {
		struct mime_part part;
		int ch;
		int r;

		/* We just saw a boundary; see if it was the last one */
		if ((ch = getc(input)) == '-')
			return (nparts);
		if (ch != '\r' || getc(input) != '\n') {
			errno = EFTYPE;
			break;
		}

		/* Get stream for the next part only */
		if ((fp = boundary_fopen(input, boundary, 0)) == NULL)
			break;

		/* Read in the next part's headers */
		memset(&part, 0, sizeof(part));
		if ((part.head = _http_head_new()) == NULL) {
			fclose(fp);
			break;
		}
		if (_http_head_read_headers(part.head, fp) == -1) {
			_http_head_free(&part.head);
			fclose(fp);
			break;
		}

		/* Invoke the handler */
		r = (*handler)(arg, &part, fp);

		/* Read any data not read by handler */
		while (fgets(buf, sizeof(buf), fp) != NULL)
			;

		/* Clean up */
		_http_head_free(&part.head);
		fclose(fp);

		/* If handler aborted, stop */
		if (r != 0)
			break;
	}

	/* There was an error */
	return (~nparts);
}

u_int
http_mime_multipart_get_count(struct mime_multipart *mp)
{
	return (mp->nparts);
}

struct mime_part *
http_mime_multipart_get_part(struct mime_multipart *mp, u_int index)
{
	if (index >= mp->nparts) {
		errno = EINVAL;
		return (NULL);
	}
	return (&mp->parts[index]);
}

void
http_mime_multipart_free(struct mime_multipart **mpp)
{
	struct mime_multipart *const mp = *mpp;
	int i;

	if (mp == NULL)
		return;
	for (i = 0; i < mp->nparts; i++) {
		struct mime_part *const part = &mp->parts[i];

		_http_head_free(&part->head);
		FREE(MIME_MEM_TYPE, part->data);
	}
	FREE(MIME_MEM_TYPE, mp->parts);
	FREE(MIME_MEM_TYPE, mp);
	*mpp = NULL;
}

const char *
http_mime_part_get_header(struct mime_part *part, const char *name)
{
	return (_http_head_get(part->head, name));
}

u_int
http_mime_part_get_length(struct mime_part *part)
{
	return (part->dlen);
}

u_char *
http_mime_part_get_data(struct mime_part *part)
{
	return (part->data);
}


