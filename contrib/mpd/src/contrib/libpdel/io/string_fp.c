
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
#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "io/string_fp.h"
#include "util/typed_mem.h"

#define ROUNDUP2(x, y)		(((x) + ((y) - 1)) & ~((y) - 1))

#ifndef __linux__
#define GET_COOKIE(fp)		((fp)->_cookie)
#else
#define GET_COOKIE(fp)		(((void **)(fp + 1))[1])
#endif

/***********************************************************************
		    STRING BUFFER OUTPUT STREAMS
***********************************************************************/

#define STRING_BUF_COOKIE	0x3f8209ec

/* Internal state for a string_buf_output() stream */
struct output_buf {
	u_int32_t	cookie;				/* sanity check id */
	char		*buf;				/* output buffer */
	size_t		length;				/* chars in buf */
	size_t		alloc;				/* amount allocated */
	int		error;				/* write error */
	char		*mtype;				/* memory type */
	char		mtype_buf[TYPED_MEM_TYPELEN];
};

/* Stream methods */
static int	string_buf_output_write(void *arg, const char *data, int len);
static int	string_buf_output_close(void *arg);

/*
 * Create a new output string buffer stream.
 */
FILE *
string_buf_output(const char *mtype)
{
	struct output_buf *sbuf;
	int esave;
	FILE *fp;

	if ((sbuf = MALLOC(mtype, sizeof(*sbuf))) == NULL)
		return (NULL);
	memset(sbuf, 0, sizeof(*sbuf));
	sbuf->cookie = STRING_BUF_COOKIE;
	if (mtype != NULL) {
		strlcpy(sbuf->mtype_buf, mtype, sizeof(sbuf->mtype_buf));
		sbuf->mtype = sbuf->mtype_buf;
	}
	if ((fp = funopen(sbuf, NULL, string_buf_output_write,
	    NULL, string_buf_output_close)) == NULL) {
		esave = errno;
		FREE(mtype, sbuf);
		errno = esave;
		return (NULL);
	}
	return (fp);
}

/*
 * Get current buffer contents.
 */
char *
string_buf_content(FILE *fp, int reset)
{
	struct output_buf *const sbuf = GET_COOKIE(fp);
	char *s;

	/* Sanity check */
	assert((size_t)sbuf >= 2048 && sbuf->cookie == STRING_BUF_COOKIE);

	/* Check for previous error */
	if (sbuf->error != 0) {
		errno = sbuf->error;
		return (NULL);
	}

	/* Flush all output to buffer */
	fflush(fp);

	/* If no data, initialize with empty string */
	if ((s = sbuf->buf) == NULL) {
		if (string_buf_output_write(sbuf, NULL, 0) == -1)
			return (NULL);
		s = sbuf->buf;
	}

	/* Reset buffer if desired */
	if (reset) {
		sbuf->buf = NULL;
		sbuf->length = 0;
		sbuf->alloc = 0;
	}

	/* Done */
	return (s);
}

/*
 * Get length of buffer.
 */
int
string_buf_length(FILE *fp)
{
	struct output_buf *const sbuf = GET_COOKIE(fp);

	/* Sanity check */
	assert((size_t)sbuf >= 2048 && sbuf->cookie == STRING_BUF_COOKIE);

	/* Flush output */
	fflush(fp);

	/* Return length of current buffer */
	return (sbuf->length);
}

/*
 * Output stream write function.
 */
static int
string_buf_output_write(void *arg, const char *data, int len)
{
	struct output_buf *const sbuf = arg;
	int esave;

	/* Was there a previous error? */
	if (sbuf->error) {
		errno = sbuf->error;
		return (-1);
	}

	/* Expand buffer */
	if (sbuf->length + len + 1 > sbuf->alloc) {
		const size_t new_size = ROUNDUP2(sbuf->length + len + 1, 256);
		void *mem;

		if ((mem = REALLOC(sbuf->mtype, sbuf->buf, new_size)) == NULL) {
			esave = errno;
			FREE(sbuf->mtype, sbuf->buf);
			sbuf->buf = NULL;
			sbuf->length = 0;
			sbuf->alloc = 0;
			sbuf->error = errno = esave;
			return (-1);
		}
		sbuf->buf = mem;
		sbuf->alloc = new_size;
	}

	/* Done */
	memcpy(sbuf->buf + sbuf->length, data, len);
	sbuf->length += len;
	sbuf->buf[sbuf->length] = '\0';
	return (len);
}

/*
 * Output stream close function.
 */
static int
string_buf_output_close(void *arg)
{
	struct output_buf *const sbuf = arg;

	sbuf->error = EINVAL;
	FREE(sbuf->mtype, sbuf->buf);
	FREE(sbuf->mtype, sbuf);
	return (0);
}

/***********************************************************************
		    STRING BUFFER INPUT STREAMS
***********************************************************************/

/* Internally used memory type for input streams */
#define INPUT_MEM_TYPE		"string_buf_input"

/* Internal state for a string_buf_input() stream */
struct input_buf {
	char		*data;				/* input data */
	int		copied;				/* data was copied */
#ifndef __linux__
	fpos_t		len;				/* input length */
	fpos_t		pos;				/* read position */
#else
	_IO_off64_t	len;				/* input length */
	_IO_off64_t	pos;				/* read position */
#endif
};

/* Stream methods */
static int	string_buf_input_read(void *cookie, char *buf, int len);
#ifndef __linux__
static fpos_t	string_buf_input_seek(void *cookie, fpos_t pos, int whence);
#else
static int	string_buf_input_seek(void *cookie,
			_IO_off64_t *posp, int whence);
#endif
static int	string_buf_input_close(void *arg);

/*
 * Create a new input string buffer stream.
 */
FILE *
string_buf_input(const void *buf, size_t len, int copy)
{
	struct input_buf *r;
	int esave;
	FILE *fp;

	if ((r = MALLOC(INPUT_MEM_TYPE, sizeof(*r))) == NULL)
		return (NULL);
	memset(r, 0, sizeof(*r));
	if (copy) {
		if ((r->data = MALLOC(INPUT_MEM_TYPE, len)) == NULL) {
			FREE(INPUT_MEM_TYPE, r);
			return (NULL);
		}
		memcpy(r->data, buf, len);
		r->copied = 1;
	} else
		r->data = (char *)buf;
	r->pos = 0;
	r->len = len;
	if ((fp = funopen(r, string_buf_input_read,
	    NULL, string_buf_input_seek, string_buf_input_close)) == NULL) {
		esave = errno;
		if (r->copied)
			FREE(INPUT_MEM_TYPE, r->data);
		FREE(INPUT_MEM_TYPE, r);
		errno = esave;
	}
	return (fp);
}

/*
 * Seeker for input streams.
 */
#ifndef __linux__
static fpos_t
string_buf_input_seek(void *cookie, fpos_t pos, int whence)
{
	struct input_buf *const r = cookie;

	switch (whence) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		pos += r->pos;
		break;
	case SEEK_END:
		pos += r->len;
		break;
	default:
		errno = EINVAL;
		return (-1);
	}
	if (pos < 0) {
		errno = EINVAL;
		return (-1);
	}
	if (pos > r->len) {
		errno = EINVAL;
		return (-1);
	}
	r->pos = pos;
	return (pos);
}
#else
static int
string_buf_input_seek(void *cookie, _IO_off64_t *posp, int whence)
{
	struct input_buf *const r = cookie;
	_IO_off64_t newpos = *posp;

	switch (whence) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		newpos += r->pos;
		break;
	case SEEK_END:
		newpos += r->len;
		break;
	default:
		errno = EINVAL;
		return (-1);
	}
	if (newpos < 0) {
		errno = EINVAL;
		return (-1);
	}
	if (newpos > r->len) {
		errno = EINVAL;
		return (-1);
	}
	*posp = r->pos = newpos;
	return (0);
}
#endif

/*
 * Reader for input streams.
 */
static int
string_buf_input_read(void *cookie, char *buf, int len)
{
	struct input_buf *const r = cookie;

	if (len > r->len - r->pos)
		len = r->len - r->pos;
	memcpy(buf, r->data + r->pos, len);
	r->pos += len;
	return (len);
}

/*
 * Closer for input streams.
 */
static int
string_buf_input_close(void *cookie)
{
	struct input_buf *const r = cookie;

	if (r->copied)
		FREE(INPUT_MEM_TYPE, r->data);
	FREE(INPUT_MEM_TYPE, r);
	return (0);
}

