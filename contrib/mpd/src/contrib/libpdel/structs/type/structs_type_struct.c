
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "structs/structs.h"
#include "structs/type/array.h"
#include "structs/type/struct.h"
#include "util/typed_mem.h"

#define NUM_BYTES(x)	(((x) + 7) / 8)

/*********************************************************************
			STRUCTURE TYPES
*********************************************************************/

int
structs_struct_init(const struct structs_type *type, void *data)
{
	const struct structs_field *field;

	/* Make sure it's really a structure type */
	if (type->tclass != STRUCTS_TYPE_STRUCTURE) {
		errno = EINVAL;
		return (-1);
	}

	/* Initialize each field */
	memset(data, 0, type->size);
	for (field = type->args[0].v; field->name != NULL; field++) {
		assert(field->size == field->type->size);    /* safety check */
		if ((*field->type->init)(field->type,
		    (char *)data + field->offset) == -1)
			break;
	}

	/* If there was a failure, clean up */
	if (field->name != NULL) {
		while (field-- != type->args[0].v) {
			(*field->type->uninit)(field->type,
			    (char *)data + field->offset);
		}
		memset((char *)data, 0, type->size);
		return (-1);
	}
	return (0);
}

int
structs_struct_copy(const struct structs_type *type,
	const void *from, void *to)
{
	const struct structs_field *field;

	/* Make sure it's really a structure type */
	if (type->tclass != STRUCTS_TYPE_STRUCTURE) {
		errno = EINVAL;
		return (-1);
	}

	/* Copy each field */
	memset(to, 0, type->size);
	for (field = type->args[0].v; field->name != NULL; field++) {
		const void *const fdata = (char *)from + field->offset;
		void *const tdata = (char *)to + field->offset;

		if ((*field->type->copy)(field->type, fdata, tdata) == -1)
			break;
	}

	/* If there was a failure, clean up */
	if (field->name != NULL) {
		while (field-- != type->args[0].v) {
			(*field->type->uninit)(field->type,
			    (char *)to + field->offset);
		}
		memset((char *)to, 0, type->size);
	}
	return (0);
}

int
structs_struct_equal(const struct structs_type *type,
	const void *v1, const void *v2)
{
	const struct structs_field *field;

	/* Make sure it's really a structure type */
	if (type->tclass != STRUCTS_TYPE_STRUCTURE)
		return (0);

	/* Compare all fields */
	for (field = type->args[0].v; field->name != NULL; field++) {
		const void *const data1 = (char *)v1 + field->offset;
		const void *const data2 = (char *)v2 + field->offset;

		if (!(*field->type->equal)(field->type, data1, data2))
			return (0);
	}
	return (1);
}

int
structs_struct_encode(const struct structs_type *type, const char *mtype,
	struct structs_data *code, const void *data)
{
	struct structs_data *fcodes;
	u_int nfields;
	u_int bitslen;
	u_char *bits;
	int r = -1;
	u_int tlen;
	u_int i;

	/* Count number of fields */
	for (nfields = 0;
	    ((struct structs_field *)type->args[0].v)[nfields].name != NULL;
	    nfields++);

	/* Create bit array. Each bit indicates a field as being present. */
	bitslen = NUM_BYTES(nfields);
	if ((bits = MALLOC(TYPED_MEM_TEMP, bitslen)) == NULL)
		return (-1);
	memset(bits, 0, bitslen);
	tlen = bitslen;

	/* Create array of individual encodings, one per field */
	if ((fcodes = MALLOC(TYPED_MEM_TEMP,
	    nfields * sizeof(*fcodes))) == NULL)
		goto fail1;
	for (i = 0; i < nfields; i++) {
		const struct structs_field *const field
		    = (struct structs_field *)type->args[0].v + i;
		const void *const fdata = (char *)data + field->offset;
		struct structs_data *const fcode = &fcodes[i];
		void *dval;
		int equal;

		/* Compare this field to the default value */
		if ((dval = MALLOC(TYPED_MEM_TEMP, field->type->size)) == NULL)
			goto fail2;
		if (structs_init(field->type, NULL, dval) == -1) {
			FREE(TYPED_MEM_TEMP, dval);
			goto fail2;
		}
		equal = (*field->type->equal)(field->type, fdata, dval);
		structs_free(field->type, NULL, dval);
		FREE(TYPED_MEM_TEMP, dval);

		/* Omit field if value equals default value */
		if (equal == 1) {
			memset(fcode, 0, sizeof(*fcode));
			continue;
		}
		bits[i / 8] |= (1 << (i % 8));

		/* Encode this field */
		if ((*field->type->encode)(field->type, TYPED_MEM_TEMP,
		    fcode, (char *)data + field->offset) == -1)
			goto fail2;
		tlen += fcode->length;
	}

	/* Allocate final encoded region */
	if ((code->data = MALLOC(mtype, tlen)) == NULL)
		goto done;

	/* Copy bits */
	memcpy(code->data, bits, bitslen);
	code->length = bitslen;

	/* Copy encoded fields */
	for (i = 0; i < nfields; i++) {
		struct structs_data *const fcode = &fcodes[i];

		memcpy(code->data + code->length, fcode->data, fcode->length);
		code->length += fcode->length;
	}

	/* OK */
	r = 0;

done:
	/* Clean up and exit */
fail2:	while (i-- > 0)
		FREE(TYPED_MEM_TEMP, fcodes[i].data);
	FREE(TYPED_MEM_TEMP, fcodes);
fail1:	FREE(TYPED_MEM_TEMP, bits);
	return (r);
}

int
structs_struct_decode(const struct structs_type *type, const u_char *code,
	size_t cmax, void *data, char *ebuf, size_t emax)
{
	const u_char *bits;
	u_int nfields;
	u_int bitslen;
	u_int clen;
	u_int i;

	/* Count number of fields */
	for (nfields = 0;
	    ((struct structs_field *)type->args[0].v)[nfields].name != NULL;
	    nfields++);

	/* Get bits array */
	bitslen = NUM_BYTES(nfields);
	if (cmax < bitslen) {
		strlcpy(ebuf, "encoded structure is truncated", emax);
		errno = EINVAL;
		return (-1);
	}
	bits = code;
	code += bitslen;
	cmax -= bitslen;
	clen = bitslen;

	/* Decode fields */
	for (i = 0; i < nfields; i++) {
		const struct structs_field *const field
		    = (struct structs_field *)type->args[0].v + i;
		void *const fdata = (char *)data + field->offset;
		int fclen;

		/* If field not present, assign it the default value */
		if ((bits[i / 8] & (1 << (i % 8))) == 0) {
			if (structs_init(field->type, NULL, fdata) == -1)
				goto fail;
			continue;
		}

		/* Decode field */
		if ((fclen = (*field->type->decode)(field->type,
		    code, cmax, fdata, ebuf, emax)) == -1)
			goto fail;

		/* Go to next encoded field */
		code += fclen;
		cmax -= fclen;
		clen += fclen;
		continue;

		/* Un-do work done so far */
fail:		while (i-- > 0) {
			const struct structs_field *const field2
			    = (struct structs_field *)type->args[0].v + i;
			void *const fdata2 = (char *)data + field2->offset;

			structs_free(field2->type, NULL, fdata2);
		}
		return (-1);
	}

	/* Done */
	return (clen);
}

void
structs_struct_free(const struct structs_type *type, void *data)
{
	const struct structs_field *field;

	/* Make sure it's really a structure type */
	if (type->tclass != STRUCTS_TYPE_STRUCTURE)
		return;

	/* Free all fields */
	for (field = type->args[0].v; field->name != NULL; field++) {
		(*field->type->uninit)(field->type,
		    (char *)data + field->offset);
	}
	memset((char *)data, 0, type->size);
}

