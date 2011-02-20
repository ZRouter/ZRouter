
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
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "structs/structs.h"
#include "structs/type/array.h"
#include "util/typed_mem.h"

#define NUM_BYTES(x)	(((x) + 7) / 8)

/*********************************************************************
			ARRAY TYPES
*********************************************************************/

int
structs_array_copy(const struct structs_type *type,
	const void *from, void *to)
{
	const struct structs_type *const etype = type->args[0].v;
	const char *mtype = type->args[1].v;
	const struct structs_array *const fary = from;
	struct structs_array *const tary = to;
	int errno_save;
	u_int i;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Allocate a new array */
	memset(tary, 0, sizeof(*tary));
	if ((tary->elems = MALLOC(mtype,
	    fary->length * etype->size)) == NULL)
		return (-1);

	/* Copy elements into it */
	for (i = 0; i < fary->length; i++) {
		const void *const from_elem
		    = (char *)fary->elems + (i * etype->size);
		void *const to_elem = (char *)tary->elems + (i * etype->size);

		if ((*etype->copy)(etype, from_elem, to_elem) == -1)
			break;
		tary->length++;
	}

	/* If there was a failure, undo the half completed job */
	if (i < fary->length) {
		errno_save = errno;
		while (i-- > 0) {
			(*etype->uninit)(etype,
			    (char *)tary->elems + (i * etype->size));
		}
		FREE(mtype, tary->elems);
		memset(tary, 0, sizeof(*tary));
		errno = errno_save;
		return (-1);
	}

	/* Done */
	return (0);
}

int
structs_array_equal(const struct structs_type *type,
	const void *v1, const void *v2)
{
	const struct structs_type *const etype = type->args[0].v;
	const struct structs_array *const ary1 = v1;
	const struct structs_array *const ary2 = v2;
	u_int i;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY)
		return (0);

	/* Check array lengths first */
	if (ary1->length != ary2->length)
		return (0);

	/* Now compare individual elements */
	for (i = 0;
	    i < ary1->length && (*etype->equal)(etype,
		(char *)ary1->elems + (i * etype->size),
		(char *)ary2->elems + (i * etype->size));
	    i++);
	return (i == ary1->length);
}

int
structs_array_encode(const struct structs_type *type, const char *mtype,
	struct structs_data *code, const void *data)
{
	const struct structs_type *const etype = type->args[0].v;
	const struct structs_array *const ary = data;
	const u_int bitslen = NUM_BYTES(ary->length);
	struct structs_data *ecodes;
	u_int32_t elength;
	u_char *bits;
	void *delem;
	u_int tlen;
	int r = -1;
	u_int i;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Get the default value for an element */
	if ((delem = MALLOC(TYPED_MEM_TEMP, etype->size)) == NULL)
		return (-1);
	if (structs_init(etype, NULL, delem) == -1)
		goto fail1;

	/* Create bit array. Each bit indicates an element that is present. */
	if ((bits = MALLOC(TYPED_MEM_TEMP, bitslen)) == NULL)
		goto fail2;
	memset(bits, 0, bitslen);
	tlen = 4 + bitslen;			/* length word + bits array */

	/* Create array of individual encodings, one per element */
	if ((ecodes = MALLOC(TYPED_MEM_TEMP,
	    ary->length * sizeof(*ecodes))) == NULL)
		goto fail3;
	for (i = 0; i < ary->length; i++) {
		const void *const elem = (char *)ary->elems + (i * etype->size);
		struct structs_data *const ecode = &ecodes[i];

		/* Check for default value, leave out if same as */
		if ((*etype->equal)(etype, elem, delem) == 1) {
			memset(ecode, 0, sizeof(*ecode));
			continue;
		}
		bits[i / 8] |= (1 << (i % 8));

		/* Encode element */
		if ((*etype->encode)(etype, TYPED_MEM_TEMP, ecode, elem) == -1)
			goto fail4;
		tlen += ecode->length;
	}

	/* Allocate final encoded region */
	if ((code->data = MALLOC(mtype, tlen)) == NULL)
		goto fail4;

	/* Copy array length */
	elength = htonl(ary->length);
	memcpy(code->data, &elength, 4);
	code->length = 4;

	/* Copy bits array */
	memcpy(code->data + code->length, bits, bitslen);
	code->length += bitslen;

	/* Copy encoded elements */
	for (i = 0; i < ary->length; i++) {
		struct structs_data *const ecode = &ecodes[i];

		memcpy(code->data + code->length, ecode->data, ecode->length);
		code->length += ecode->length;
	}

	/* OK */
	r = 0;

	/* Clean up and exit */
fail4:	while (i-- > 0)
		FREE(TYPED_MEM_TEMP, ecodes[i].data);
	FREE(TYPED_MEM_TEMP, ecodes);
fail3:	FREE(TYPED_MEM_TEMP, bits);
fail2:	structs_free(etype, NULL, delem);
fail1:	FREE(TYPED_MEM_TEMP, delem);
	return (r);
}

int
structs_array_decode(const struct structs_type *type, const u_char *code,
	size_t cmax, void *data, char *ebuf, size_t emax)
{
	const struct structs_type *const etype = type->args[0].v;
	const char *const mtype = type->args[1].v;
	struct structs_array *const ary = data;
	const u_char *bits;
	u_int32_t elength;
	u_int bitslen;
	int clen;
	u_int i;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Get number of elements */
	if (cmax < 4)
		goto truncated;
	memcpy(&elength, code, 4);
	ary->length = ntohl(elength);
	code += 4;
	cmax -= 4;
	clen = 4;

	/* Get bits array */
	bitslen = NUM_BYTES(ary->length);
	if (cmax < bitslen) {
truncated:	strlcpy(ebuf, "encoded array is truncated", emax);
		errno = EINVAL;
		return (-1);
	}
	bits = code;
	code += bitslen;
	cmax -= bitslen;
	clen += bitslen;

	/* Allocate array elements */
	if ((ary->elems = MALLOC(mtype, ary->length * etype->size)) == NULL)
		return (-1);

	/* Decode elements */
	for (i = 0; i < ary->length; i++) {
		void *const edata = (char *)ary->elems + (i * etype->size);
		int eclen;

		/* If element not present, assign it the default value */
		if ((bits[i / 8] & (1 << (i % 8))) == 0) {
			if (structs_init(etype, NULL, edata) == -1)
				goto fail;
			continue;
		}

		/* Decode element */
		if ((eclen = (*etype->decode)(etype,
		    code, cmax, edata, ebuf, emax)) == -1)
			goto fail;

		/* Go to next encoded element */
		code += eclen;
		cmax -= eclen;
		clen += eclen;
		continue;

		/* Un-do work done so far */
fail:		while (i-- > 0) {
			structs_free(etype, NULL,
			    (char *)ary->elems + (i * etype->size));
		}
		FREE(mtype, ary->elems);
		return (-1);
	}

	/* Done */
	return (clen);
}

void
structs_array_free(const struct structs_type *type, void *data)
{
	const struct structs_type *const etype = type->args[0].v;
	const char *mtype = type->args[1].v;
	struct structs_array *const ary = data;
	u_int i;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY)
		return;

	/* Free individual elements */
	for (i = 0; i < ary->length; i++)
		(*etype->uninit)(etype, (char *)ary->elems + (i * etype->size));

	/* Free array itself */
	FREE(mtype, ary->elems);
	memset(ary, 0, sizeof(*ary));
}

int
structs_array_length(const struct structs_type *type,
	const char *name, const void *data)
{
	const struct structs_array *ary = data;

	/* Find array */
	if ((type = structs_find(type, name, (void **)&ary, 0)) == NULL)
		return (-1);

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Return length */
	return (ary->length);
}

int
structs_array_reset(const struct structs_type *type,
	const char *name, void *data)
{
	/* Find array */
	if ((type = structs_find(type, name, &data, 1)) == NULL)
		return (-1);

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Free it, which resets it as well */
	structs_array_free(type, data);
	return (0);
}

int
structs_array_insert(const struct structs_type *type,
	const char *name, u_int index, void *data)
{
	struct structs_array *ary = data;
	const struct structs_type *etype;
	const char *mtype;
	void *mem;

	/* Find array */
	if ((type = structs_find(type, name, (void **)&ary, 0)) == NULL)
		return (-1);
	etype = type->args[0].v;
	mtype = type->args[1].v;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Check index */
	if (index > ary->length) {
		errno = EDOM;
		return (-1);
	}

	/* Reallocate array, leaving room for new element and a shift */
	if ((mem = REALLOC(mtype,
	    ary->elems, (ary->length + 2) * etype->size)) == NULL)
		return (-1);
	ary->elems = mem;

	/* Initialize new element; we'll move it into place later */
	if ((*etype->init)(etype,
	    (char *)ary->elems + (ary->length + 1) * etype->size) == -1)
		return (-1);

	/* Shift array over by one and move new element into place */
	memmove((char *)ary->elems + (index + 1) * etype->size,
	    (char *)ary->elems + index * etype->size,
	    (ary->length - index) * etype->size);
	memcpy((char *)ary->elems + index * etype->size,
	    (char *)ary->elems + (ary->length + 1) * etype->size, etype->size);
	ary->length++;
	return (0);
}

int
structs_array_delete(const struct structs_type *type,
	const char *name, u_int index, void *data)
{
	const struct structs_type *etype;
	struct structs_array *ary = data;

	/* Find array */
	if ((type = structs_find(type, name, (void **)&ary, 0)) == NULL)
		return (-1);
	etype = type->args[0].v;

	/* Make sure it's really an array type */
	if (type->tclass != STRUCTS_TYPE_ARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Check index */
	if (index >= ary->length) {
		errno = EDOM;
		return (-1);
	}

	/* Free element */
	(*etype->uninit)(etype, (char *)ary->elems + (index * etype->size));

	/* Shift array */
	memmove((char *)ary->elems + index * etype->size,
	    (char *)ary->elems + (index + 1) * etype->size,
	    (--ary->length - index) * etype->size);
	return (0);
}

/*
 * Given a name, prepare all arrays in the name path for being
 * set with a new element.
 *
 * This means that any array index equal to the length of the array
 * causes a new element to be appended to the array rather than
 * an element not found error, and that any array index equal to
 * zero means to reset the entire array to be empty.
 *
 * This allows an entire array to be set by setting each element
 * in order, as long as this function is called with the name of
 * the element before each set operation.
 */
int
structs_array_prep(const struct structs_type *type,
	const char *name, void *data)
{
	char *nbuf;
	char *s;

	/* Skip if name cannot be an array element name */
	if (name == NULL || *name == '\0')
		return (0);

	/* Copy name into writable buffer */
	if ((nbuf = MALLOC(TYPED_MEM_TEMP, strlen(name) + 2)) == NULL)
		return (-1);
	nbuf[0] = '*';					/* for debugging */
	strcpy(nbuf + 1, name);

	/* Check all array element names in name path */
	for (s = nbuf; s != NULL; s = strchr(s + 1, STRUCTS_SEPARATOR)) {
		const struct structs_type *atype;
		struct structs_array *ary = data;
		u_long index;
		char *eptr;
		char *t;
		char ch;

		/* Go to next descendant node using prefix of name */
		ch = *s;
		*s = '\0';
		if ((atype = structs_find(type,
		    nbuf + (s != nbuf), (void **)&ary, 1)) == NULL) {
			FREE(TYPED_MEM_TEMP, nbuf);
			return (-1);
		}
		*s = ch;

		/* If not array type, continue */
		if (atype->tclass != STRUCTS_TYPE_ARRAY)
			continue;

		/* Get array index */
		if ((t = strchr(s + 1, STRUCTS_SEPARATOR)) != NULL)
			*t = '\0';
		index = strtoul(s + 1, &eptr, 10);
		if (eptr == s + 1 || *eptr != '\0') {
			errno = ENOENT;
			FREE(TYPED_MEM_TEMP, nbuf);
			return (-1);
		}

		/* If setting an element that already exists, accept */
		if (index < ary->length)
			goto next;

		/* Must be setting the next new item in the array */
		if (index != ary->length) {
			errno = ENOENT;
			FREE(TYPED_MEM_TEMP, nbuf);
			return (-1);
		}

		/* Add new item; it will be in an initialized state */
		if (structs_array_insert(atype, NULL, ary->length, ary) == -1) {
			FREE(TYPED_MEM_TEMP, nbuf);
			return (-1);
		}

next:
		/* Repair name buffer for next time */
		if (t != NULL)
			*t = STRUCTS_SEPARATOR;
	}

	/* Done */
	FREE(TYPED_MEM_TEMP, nbuf);
	return (0);
}

/*********************************************************************
			FIXEDARRAY TYPES
*********************************************************************/

int
structs_fixedarray_init(const struct structs_type *type, void *data)
{
	const struct structs_type *const etype = type->args[0].v;
	const u_int length = type->args[2].i;
	u_int i;

	for (i = 0; i < length; i++) {
		if ((*etype->init)(etype,
		    (char *)data + (i * etype->size)) == -1) {
			    while (i-- > 0) {
				    (*etype->uninit)(etype,
					(char *)data + (i * etype->size));
			    }
			    return (-1);
		}
	}
	return (0);
}

int
structs_fixedarray_copy(const struct structs_type *type,
	const void *from, void *to)
{
	const struct structs_type *const etype = type->args[0].v;
	const u_int length = type->args[2].i;
	u_int i;

	/* Make sure it's really a fixedarray type */
	if (type->tclass != STRUCTS_TYPE_FIXEDARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Copy elements into it */
	for (i = 0; i < length; i++) {
		const void *const from_elem = (char *)from + (i * etype->size);
		void *const to_elem = (char *)to + (i * etype->size);

		if ((*etype->copy)(etype, from_elem, to_elem) == -1)
			break;
	}

	/* If there was a failure, undo the half completed job */
	if (i < length) {
		while (i-- > 0)
			(*etype->uninit)(etype, (char *)to + (i * etype->size));
		return (-1);
	}

	/* Done */
	return (0);
}

int
structs_fixedarray_equal(const struct structs_type *type,
	const void *v1, const void *v2)
{
	const struct structs_type *const etype = type->args[0].v;
	const u_int length = type->args[2].i;
	u_int i;

	/* Make sure it's really a fixedarray type */
	if (type->tclass != STRUCTS_TYPE_FIXEDARRAY)
		return (0);

	/* Compare individual elements */
	for (i = 0;
	    i < length && (*etype->equal)(etype,
		(char *)v1 + (i * etype->size), (char *)v2 + (i * etype->size));
	    i++);
	return (i == length);
}

int
structs_fixedarray_encode(const struct structs_type *type, const char *mtype,
	struct structs_data *code, const void *data)
{
	const struct structs_type *const etype = type->args[0].v;
	const u_int length = type->args[2].i;
	const u_int bitslen = NUM_BYTES(length);
	struct structs_data *ecodes;
	u_char *bits;
	void *delem;
	u_int tlen;
	int r = -1;
	u_int i;

	/* Make sure it's really a fixedarray type */
	if (type->tclass != STRUCTS_TYPE_FIXEDARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Get the default value for an element */
	if ((delem = MALLOC(TYPED_MEM_TEMP, etype->size)) == NULL)
		return (-1);
	if (structs_init(etype, NULL, delem) == -1)
		goto fail1;

	/* Create bit array. Each bit indicates an element that is present. */
	if ((bits = MALLOC(TYPED_MEM_TEMP, bitslen)) == NULL)
		goto fail2;
	memset(bits, 0, bitslen);
	tlen = bitslen;

	/* Create array of individual encodings, one per element */
	if ((ecodes = MALLOC(TYPED_MEM_TEMP, length * sizeof(*ecodes))) == NULL)
		goto fail3;
	for (i = 0; i < length; i++) {
		const void *const elem = (char *)data + (i * etype->size);
		struct structs_data *const ecode = &ecodes[i];

		/* Check for default value, leave out if same as */
		if ((*etype->equal)(etype, elem, delem) == 1) {
			memset(ecode, 0, sizeof(*ecode));
			continue;
		}
		bits[i / 8] |= (1 << (i % 8));

		/* Encode element */
		if ((*etype->encode)(etype, TYPED_MEM_TEMP, ecode, elem) == -1)
			goto fail4;
		tlen += ecode->length;
	}

	/* Allocate final encoded region */
	if ((code->data = MALLOC(mtype, tlen)) == NULL)
		goto fail4;

	/* Copy bits array */
	memcpy(code->data, bits, bitslen);
	code->length = bitslen;

	/* Copy encoded elements */
	for (i = 0; i < length; i++) {
		struct structs_data *const ecode = &ecodes[i];

		memcpy(code->data + code->length, ecode->data, ecode->length);
		code->length += ecode->length;
	}

	/* OK */
	r = 0;

	/* Clean up and exit */
fail4:	while (i-- > 0)
		FREE(TYPED_MEM_TEMP, ecodes[i].data);
	FREE(TYPED_MEM_TEMP, ecodes);
fail3:	FREE(TYPED_MEM_TEMP, bits);
fail2:	structs_free(etype, NULL, delem);
fail1:	FREE(TYPED_MEM_TEMP, delem);
	return (r);
}

int
structs_fixedarray_decode(const struct structs_type *type, const u_char *code,
	size_t cmax, void *data, char *ebuf, size_t emax)
{
	const struct structs_type *const etype = type->args[0].v;
	const u_int length = type->args[2].i;
	const u_int bitslen = NUM_BYTES(length);
	const u_char *bits;
	int clen = 0;
	u_int i;

	/* Make sure it's really a fixedarray type */
	if (type->tclass != STRUCTS_TYPE_FIXEDARRAY) {
		errno = EINVAL;
		return (-1);
	}

	/* Get bits array */
	if (cmax < bitslen) {
		strlcpy(ebuf, "encoded array is truncated", emax);
		errno = EINVAL;
		return (-1);
	}
	bits = code;
	code += bitslen;
	cmax -= bitslen;
	clen += bitslen;

	/* Decode elements */
	for (i = 0; i < length; i++) {
		void *const edata = (char *)data + (i * etype->size);
		int eclen;

		/* If element not present, assign it the default value */
		if ((bits[i / 8] & (1 << (i % 8))) == 0) {
			if (structs_init(etype, NULL, edata) == -1)
				goto fail;
			continue;
		}

		/* Decode element */
		if ((eclen = (*etype->decode)(etype,
		    code, cmax, edata, ebuf, emax)) == -1)
			goto fail;

		/* Go to next encoded element */
		code += eclen;
		cmax -= eclen;
		clen += eclen;
		continue;

		/* Un-do work done so far */
fail:		while (i-- > 0) {
			structs_free(etype, NULL,
			    (char *)data + (i * etype->size));
		}
		return (-1);
	}

	/* Done */
	return (clen);
}

void
structs_fixedarray_free(const struct structs_type *type, void *data)
{
	const struct structs_type *const etype = type->args[0].v;
	const u_int length = type->args[2].i;
	u_int i;

	/* Make sure it's really a fixedarray type */
	if (type->tclass != STRUCTS_TYPE_FIXEDARRAY)
		return;

	/* Free elements */
	for (i = 0; i < length; i++)
		(*etype->uninit)(etype, (char *)data + (i * etype->size));
}

