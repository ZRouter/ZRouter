
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

#include "util/typed_mem.h"
#include "io/boundary_fp.h"

#define MEM_TYPE		"boundary_fp"

#define MAX_BOUNDARY		255		/* must be less than 256 */

/* Internal state per stream */
struct boundary_fp {
	FILE	*fp;
	char	bdry[MAX_BOUNDARY];		/* boundary string */
	u_char	fail[MAX_BOUNDARY];		/* failure function */
	u_char	buf[MAX_BOUNDARY];		/* input buffer data */
	int	closeit;			/* close underlying stream */
	int	blen;				/* boundary string length */
	int	nbuf;				/* number chars in buffer */
	int	nmatch;				/* length of the longest suffix
						   of buffer data that matches
						   a prefix of the boundary */
};

/*
 * Internal functions
 */
static int	boundary_read(void *cookie, char *buf, int len);
static int	boundary_close(void *cookie);

/*
 * Create a FILE * that reads from another FILE *, stopping
 * just after it reads in the supplied boundary string.
 */
FILE *
boundary_fopen(FILE *fp, const char *boundary, int closeit)
{
	struct boundary_fp *m;
	int blen;
	int i, j;

	/* Sanity check */
	if ((blen = strlen(boundary)) > MAX_BOUNDARY) {
		errno = EINVAL;
		return (NULL);
	}

	/* Create state structure */
	if ((m = MALLOC(MEM_TYPE, sizeof(*m))) == NULL)
		return (NULL);
	memset(m, 0, sizeof(*m));
	m->fp = fp;
	m->blen = blen;
	m->closeit = closeit;
	memcpy(m->bdry, boundary, m->blen);

	/* Generate failure function (linear algorithms exist; this isn't) */
	for (i = 2; i < m->blen; i++) {
		for (j = 1; j < i && strncmp(m->bdry, m->bdry + j, i - j); j++);
		m->fail[i] = i - j;
	}

	/* Create new stream using methods below */
	if ((fp = funopen(m, boundary_read,
	    NULL, NULL, boundary_close)) == NULL) {
		FREE(MEM_TYPE, m);
		return (NULL);
	}

	/* Done */
	return (fp);
}

/*
 * Read method.
 */
static int
boundary_read(void *cookie, char *buf, int len)
{
	struct boundary_fp *const m = cookie;
	int nr;
	int i;

	/* Sanity */
	assert(m->nmatch <= m->nbuf);

	/* Have we got a complete boundary sitting in the buffer? */
	if (m->nmatch == m->blen)
		return (0);

	/* Return available data, if any (and shift buffer) */
	if (m->nbuf > m->nmatch) {
		len = MIN(len, m->nbuf - m->nmatch);
		memcpy(buf, m->buf, len);
		m->nbuf -= len;
		memmove(m->buf, m->buf + len, m->nbuf);
		return (len);
	}

	/*
	 * Try to fill up the buffer from underlying stream. It is not
	 * possible to read past the end of a boundary at this point.
	 */
	nr = fread(m->buf + m->nbuf, 1, m->blen - m->nbuf, m->fp);

	/* Check for EOF/error on underlying stream */
	if (nr == 0) {
		if (!ferror(m->fp))
			errno = EFTYPE;		/* no final boundary seen */
		return (-1);
	}

	/* Recompute match counter using failure links and new data */
	for (i = m->nbuf; i < m->nbuf + nr; i++) {
		while (m->nmatch > 0 && m->buf[i] != m->bdry[m->nmatch])
			m->nmatch = m->fail[m->nmatch];
		if (m->buf[i] == m->bdry[m->nmatch])
			m->nmatch++;
	}
	m->nbuf += nr;

	/* Try again */
	return (boundary_read(cookie, buf, len));
}

/*
 * Close method.
 */
static int
boundary_close(void *cookie)
{
	struct boundary_fp *const m = cookie;

	if (m->closeit)
		fclose(m->fp);
	FREE(MEM_TYPE, m);
	return (0);
}

