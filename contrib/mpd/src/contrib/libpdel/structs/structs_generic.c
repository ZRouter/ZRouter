
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
#ifdef __linux__
#include <endian.h>
#else
#include <machine/endian.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "structs/structs.h"
#include "structs/type/array.h"
#include "util/typed_mem.h"

#ifndef BYTE_ORDER
#error BYTE_ORDER is undefined
#endif

/*********************************************************************
			GENERIC FUNCTIONS
*********************************************************************/

int
structs_region_init(const struct structs_type *type, void *data)
{
	memset(data, 0, type->size);
	return (0);
}

int
structs_region_copy(const struct structs_type *type, const void *from, void *to)
{
	memcpy(to, from, type->size);
	return (0);
}

int
structs_region_equal(const struct structs_type *type,
	const void *v1, const void *v2)
{
	return (memcmp(v1, v2, type->size) == 0);
}

int
structs_region_encode(const struct structs_type *type, const char *mtype,
	struct structs_data *code, const void *data)
{
	if ((code->data = MALLOC(mtype, type->size)) == NULL)
		return (-1);
	memcpy(code->data, data, type->size);
	code->length = type->size;
	return (0);
}

int
structs_region_decode(const struct structs_type *type,
	const u_char *code, size_t cmax, void *data, char *ebuf, size_t emax)
{
	if (cmax < type->size) {
		strlcpy(ebuf, "encoded data is truncated", emax);
		errno = EINVAL;
		return (-1);
	}
	memcpy(data, code, type->size);
	return (type->size);
}

int
structs_region_encode_netorder(const struct structs_type *type,
	const char *mtype, struct structs_data *code, const void *data)
{
	if (structs_region_encode(type, mtype, code, data) == -1)
		return (-1);
#if BYTE_ORDER == LITTLE_ENDIAN
    {
	u_char temp;
	u_int i;

	for (i = 0; i < code->length / 2; i++) {
		temp = code->data[i];
		code->data[i] = code->data[code->length - 1 - i];
		code->data[code->length - 1 - i] = temp;
	}
    }
#endif
	return (0);
}

int
structs_region_decode_netorder(const struct structs_type *type,
	const u_char *code, size_t cmax, void *data, char *ebuf, size_t emax)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char buf[16];
	u_char temp;
	u_int i;

	if (type->size > sizeof(buf)) {
		errno = ERANGE;		/* XXX oops fixed buffer is too small */
		return (-1);
	}
	if (cmax > type->size)
		cmax = type->size;
	memcpy(buf, code, cmax);
	for (i = 0; i < type->size / 2; i++) {
		temp = buf[i];
		buf[i] = buf[type->size - 1 - i];
		buf[type->size - 1 - i] = temp;
	}
	code = buf;
#endif
	return (structs_region_decode(type, code, cmax, data, ebuf, emax));
}

char *
structs_notsupp_ascify(const struct structs_type *type,
	const char *mtype, const void *data)
{
	errno = EOPNOTSUPP;
	return (NULL);
}

int
structs_notsupp_init(const struct structs_type *type, void *data)
{
	errno = EOPNOTSUPP;
	return (-1);
}

int
structs_notsupp_copy(const struct structs_type *type,
	const void *from, void *to)
{
	errno = EOPNOTSUPP;
	return (-1);
}

int
structs_notsupp_equal(const struct structs_type *type,
	const void *v1, const void *v2)
{
	errno = EOPNOTSUPP;
	return (-1);
}


int
structs_notsupp_binify(const struct structs_type *type,
	const char *ascii, void *data, char *ebuf, size_t emax)
{
	strlcpy(ebuf,
	    "parsing from ASCII is not supported by this structs type", emax);
	errno = EOPNOTSUPP;
	return (-1);
}

int
structs_notsupp_encode(const struct structs_type *type, const char *mtype,
	struct structs_data *code, const void *data)
{
	errno = EOPNOTSUPP;
	return (-1);
}

int
structs_notsupp_decode(const struct structs_type *type,
	const u_char *code, size_t cmax, void *data, char *ebuf, size_t emax)
{
	strlcpy(ebuf,
	    "binary decoding is not supported by this structs type", emax);
	errno = EOPNOTSUPP;
	return (-1);
}

void
structs_nothing_free(const struct structs_type *type, void *data)
{
	return;
}

int
structs_ascii_copy(const struct structs_type *type, const void *from, void *to)
{
	char *ascii;
	int rtn;

	if ((ascii = (*type->ascify)(type, TYPED_MEM_TEMP, from)) == NULL)
		return (-1);
	rtn = (*type->binify)(type, ascii, to, NULL, 0);
	FREE(TYPED_MEM_TEMP, ascii);
	return (rtn);
}

