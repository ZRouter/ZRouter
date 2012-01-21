
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "structs/structs.h"
#include "structs/type/int.h"
#include "structs/type/array.h"
#include "structs/type/string.h"
#include "structs/type/struct.h"
#include "structs/type/union.h"
#include "util/typed_mem.h"

/* Special handling for array length as a read-only field */
static const struct structs_type structs_type_array_length = {
	sizeof(u_int),
	"uint",
	STRUCTS_TYPE_PRIMITIVE,
	structs_region_init,
	structs_region_copy,
	structs_region_equal,
	structs_int_ascify,
	structs_notsupp_binify,
	structs_region_encode_netorder,
	structs_notsupp_decode,
	structs_nothing_free,
	{ { (void *)2 }, { (void *)0 } }    /* args for structs_int_ascify */
};

/* Special handling for union field name as a read-only field */
static const struct structs_type structs_type_union_field_name = {
	sizeof(char *),
	"string",
	STRUCTS_TYPE_PRIMITIVE,
	structs_notsupp_init,
	structs_notsupp_copy,
	structs_string_equal,
	structs_string_ascify,
	structs_notsupp_binify,
	structs_notsupp_encode,
	structs_notsupp_decode,
	structs_nothing_free,
	{ { (void *)"union field_name" },    /* args for structs_type_string */
	{ (void *)0 } }
};

/*
 * Initialize an item.
 */
int
structs_init(const struct structs_type *type, const char *name, void *data)
{
	/* Find item */
	if ((type = structs_find(type, name, &data, 0)) == NULL)
		return (-1);

	/* Initialize it */
	return ((*type->init)(type, data));
}

/*
 * Reset an item.
 */
int
structs_reset(const struct structs_type *type, const char *name, void *data)
{
	void *temp;

	/* Find item */
	if ((type = structs_find(type, name, &data, 0)) == NULL)
		return (-1);

	/* Make a temporary new copy */
	if ((temp = MALLOC(TYPED_MEM_TEMP, type->size)) == NULL)
		return (-1);
	memset(temp, 0, type->size);
	if ((*type->init)(type, temp) == -1) {
		FREE(TYPED_MEM_TEMP, temp);
		return (-1);
	}

	/* Replace existing item, freeing it first */
	(*type->uninit)(type, data);
	memcpy(data, temp, type->size);
	FREE(TYPED_MEM_TEMP, temp);
	return (0);
}

/*
 * Free an item, returning it to its initialized state.
 *
 * If "name" is NULL or empty string, the entire structure is free'd.
 */
int
structs_free(const struct structs_type *type, const char *name, void *data)
{
	const int errno_save = errno;

	/* Find item */
	if ((type = structs_find(type, name, &data, 0)) == NULL)
		return (-1);

	/* Free it */
	(*type->uninit)(type, data);
	errno = errno_save;
	return (0);
}

/*
 * Get a copy of an item.
 *
 * If "name" is NULL or empty string, the entire structure is copied.
 *
 * It is assumed that "to" points to a region big enough to hold
 * the copy of the item. "to" will not be free'd first, so it should
 * not already be initialized.
 *
 * Note: "name" is only applied to "from". The structs types of
 * "from.<name>" and "to" must be the same.
 */
int
structs_get(const struct structs_type *type,
	const char *name, const void *from, void *to)
{
	/* Find item */
	if ((type = structs_find(type, name, (void **)&from, 0)) == NULL)
		return (-1);

	/* Copy item */
	return ((*type->copy)(type, from, to));
}

/*
 * Set an item in a structure.
 *
 * If "name" is NULL or empty string, the entire structure is copied.
 *
 * It is assumed that "to" is already initialized.
 *
 * Note: "name" is only applied to "to". The structs types of
 * "from" and "to.<name>" must be the same.
 */
int
structs_set(const struct structs_type *type,
	const void *from, const char *name, void *to)
{
	void *copy;

	/* Find item */
	if ((type = structs_find(type, name, &to, 0)) == NULL)
		return (-1);

	/* Make a new copy of 'from' */
	if ((copy = MALLOC(TYPED_MEM_TEMP, type->size)) == NULL)
		return (-1);
	if ((*type->copy)(type, from, copy) == -1) {
		FREE(TYPED_MEM_TEMP, copy);
		return (-1);
	}

	/* Free overwritten item in 'to' */
	(*type->uninit)(type, to);

	/* Move new item in its place */
	memcpy(to, copy, type->size);

	/* Done */
	FREE(TYPED_MEM_TEMP, copy);
	return (0);
}

/*
 * Get the ASCII form of an item.
 */
char *
structs_get_string(const struct structs_type *type,
	const char *name, const void *data, const char *mtype)
{
	/* Find item */
	if ((type = structs_find(type, name, (void **)&data, 0)) == NULL)
		return (NULL);

	/* Ascify it */
	return ((*type->ascify)(type, mtype, data));
}

/*
 * Set an item's value from a string.
 *
 * The referred to item must be of a type that supports ASCII encoding,
 * and is assumed to be already initialized.
 */
int
structs_set_string(const struct structs_type *type, const char *name,
	const char *ascii, void *data, char *ebuf, size_t emax)
{
	void *temp;
	char dummy[1];

	/* Sanity check */
	if (ascii == NULL)
		ascii = "";
	if (ebuf == NULL) {
		ebuf = dummy;
		emax = sizeof(dummy);
	}

	/* Initialize error buffer */
	if (emax > 0)
		*ebuf = '\0';

	/* Find item */
	if ((type = structs_find(type, name, &data, 1)) == NULL) {
		strlcpy(ebuf, strerror(errno), emax);
		return (-1);
	}

	/* Binify item into temporary storage */
	if ((temp = MALLOC(TYPED_MEM_TEMP, type->size)) == NULL)
		return (-1);
	memset(temp, 0, type->size);
	if ((*type->binify)(type, ascii, temp, ebuf, emax) == -1) {
		FREE(TYPED_MEM_TEMP, temp);
		if (emax > 0 && *ebuf == '\0')
			strlcpy(ebuf, strerror(errno), emax);
		return (-1);
	}

	/* Replace existing item, freeing it first */
	(*type->uninit)(type, data);
	memcpy(data, temp, type->size);
	FREE(TYPED_MEM_TEMP, temp);
	return (0);
}

/*
 * Get the binary encoded form of an item, allocated with type 'mtype'.
 */
int
structs_get_binary(const struct structs_type *type, const char *name,
	const void *data, const char *mtype, struct structs_data *code)
{
	/* Find item */
	if ((type = structs_find(type, name, (void **)&data, 0)) == NULL) {
		memset(code, 0, sizeof(*code));
		return (-1);
	}

	/* Encode it */
	if ((*type->encode)(type, mtype, code, data) == -1) {
		memset(code, 0, sizeof(*code));
		return (-1);
	}

	/* Done */
	return (0);
}

/*
 * Set an item's value from its binary encoded value.
 */
int
structs_set_binary(const struct structs_type *type, const char *name,
	const struct structs_data *code, void *data, char *ebuf, size_t emax)
{
	void *temp;
	int clen;

	/* Initialize error buffer */
	if (emax > 0)
		*ebuf = '\0';

	/* Find item */
	if ((type = structs_find(type, name, (void **)&data, 0)) == NULL) {
		strlcpy(ebuf, strerror(errno), emax);
		return (-1);
	}

	/* Decode item into temporary storage */
	if ((temp = MALLOC(TYPED_MEM_TEMP, type->size)) == NULL)
		return (-1);
	memset(temp, 0, type->size);
	if ((clen = (*type->decode)(type, code->data,
	    code->length, temp, ebuf, emax)) == -1) {
		FREE(TYPED_MEM_TEMP, temp);
		if (emax > 0 && *ebuf == '\0')
			strlcpy(ebuf, strerror(errno), emax);
		return (-1);
	}
	assert(clen <= code->length);

	/* Replace existing item, freeing it first */
	(*type->uninit)(type, data);
	memcpy(data, temp, type->size);
	FREE(TYPED_MEM_TEMP, temp);

	/* Done */
	return (clen);
}

/*
 * Test for equality.
 */
int
structs_equal(const struct structs_type *type,
	const char *name, const void *data1, const void *data2)
{
	/* Find items */
	if (structs_find(type, name, (void **)&data1, 0) == NULL)
		return (-1);
	if ((type = structs_find(type, name, (void **)&data2, 0)) == NULL)
		return (-1);

	/* Compare them */
	return ((*type->equal)(type, data1, data2));
}

/*
 * Find an item in a structure.
 */
const struct structs_type *
structs_find(const struct structs_type *type, const char *name,
	void **datap, int set_union)
{
	void *data = *datap;
	const char *next;

	/* Empty string means stop recursing */
	if (name == NULL || *name == '\0')
		return (type);

	/* Primitive types don't have sub-elements */
	if (type->tclass == STRUCTS_TYPE_PRIMITIVE) {
		errno = ENOENT;
		return (NULL);
	}

	/* Dereference through pointer(s) */
	while (type->tclass == STRUCTS_TYPE_POINTER) {
		type = type->args[0].v;
		data = *((void **)data);
	}

	/* Get next name component */
	if ((next = strchr(name, STRUCTS_SEPARATOR)) != NULL)
		next++;

	/* Find element of aggregate structure update type and data */
	switch (type->tclass) {
	case STRUCTS_TYPE_ARRAY:
	    {
		const struct structs_type *const etype = type->args[0].v;
		const struct structs_array *const ary = data;
		u_long index;
		char *eptr;

		/* Special handling for "length" */
		if (strcmp(name, "length") == 0) {
			type = &structs_type_array_length;
			data = (void *)&ary->length;
			break;
		}

		/* Decode an index */
		index = strtoul(name, &eptr, 10);
		if (!isdigit(*name)
		    || eptr == name
		    || (*eptr != '\0' && *eptr != STRUCTS_SEPARATOR)) {
			errno = ENOENT;
			return (NULL);
		}
		if (index >= ary->length) {
			errno = EDOM;
			return (NULL);
		}
		type = etype;
		data = (char *)ary->elems + (index * etype->size);
		break;
	    }
	case STRUCTS_TYPE_FIXEDARRAY:
	    {
		const struct structs_type *const etype = type->args[0].v;
		const u_int length = type->args[2].i;
		u_long index;
		char *eptr;

		/* Special handling for "length" */
		if (strcmp(name, "length") == 0) {
			type = &structs_type_array_length;
			data = (void *)&type->args[2].i;
			break;
		}

		/* Decode an index */
		index = strtoul(name, &eptr, 10);
		if (!isdigit(*name)
		    || eptr == name
		    || (*eptr != '\0' && *eptr != STRUCTS_SEPARATOR)) {
			errno = ENOENT;
			return (NULL);
		}
		if (index >= length) {
			errno = EDOM;
			return (NULL);
		}
		type = etype;
		data = (char *)data + (index * etype->size);
		break;
	    }
	case STRUCTS_TYPE_STRUCTURE:
	    {
		const struct structs_field *field;

		/* Find the field */
		for (field = type->args[0].v; field->name != NULL; field++) {
			const size_t fnlen = strlen(field->name);

			/* Handle field names with separator in them */
			if (strncmp(name, field->name, fnlen) == 0
			    && (name[fnlen] == '\0'
			      || name[fnlen] == STRUCTS_SEPARATOR)) {
				next = (name[fnlen] != '\0') ?
				    name + fnlen + 1 : NULL;
				break;
			}
		}
		if (field->name == NULL) {
			errno = ENOENT;
			return (NULL);
		}
		type = field->type;
		data = (char *)data + field->offset;
		break;
	    }
	case STRUCTS_TYPE_UNION:
	    {
		const struct structs_ufield *const fields = type->args[0].v;
		const char *const mtype = type->args[1].s;
		struct structs_union *const un = data;
		const size_t oflen = strlen(un->field_name);
		const struct structs_ufield *ofield;
		const struct structs_ufield *field;
		void *new_un;
		void *data2;

		/* Special handling for "field_name" */
		if (strcmp(name, "field_name") == 0) {
			type = &structs_type_union_field_name;
			data = (void *)&un->field_name;
			break;
		}

		/* Find the old field */
		for (ofield = fields; ofield->name != NULL; ofield++) {
			if (strcmp(ofield->name, un->field_name) == 0)
				break;
		}
		if (ofield->name == NULL) {
			assert(0);
			errno = EINVAL;
			return (NULL);
		}

		/*
		 * Check if the union is already set to the desired field,
		 * handling field names with the separator char in them.
		 */
		if (strncmp(name, un->field_name, oflen) == 0
		    && (name[oflen] == '\0'
		      || name[oflen] == STRUCTS_SEPARATOR)) {
			next = (name[oflen] != '\0') ? name + oflen + 1 : NULL;
			field = ofield;
			goto union_done;
		}

		/* Is modifying the union to get the right name acceptable? */
		if (!set_union) {
			errno = ENOENT;
			return (NULL);
		}

		/* Find the new field */
		for (field = fields; field->name != NULL; field++) {
			const size_t fnlen = strlen(field->name);

			/* Handle field names with separator in them */
			if (strncmp(name, field->name, fnlen) == 0
			    && (name[fnlen] == '\0'
			      || name[fnlen] == STRUCTS_SEPARATOR)) {
				next = (name[fnlen] != '\0') ?
				    name + fnlen + 1 : NULL;
				break;
			}
		}
		if (field->name == NULL) {
			errno = ENOENT;
			return (NULL);
		}

		/* Create a new union with the new field type */
		if ((new_un = MALLOC(mtype, field->type->size)) == NULL)
			return (NULL);
		if ((*field->type->init)(field->type, new_un) == -1) {
			FREE(mtype, new_un);
			return (NULL);
		}

		/* See if name would be found with new union instead of old */
		data2 = new_un;
		if (next != NULL
		    && structs_find(field->type, next, &data2, 1) == NULL) {
			(*field->type->uninit)(field->type, new_un);
			FREE(mtype, new_un);
			return (NULL);
		}

		/* Replace existing union with new one having desired type */
		(*ofield->type->uninit)(ofield->type, un->un);
		FREE(mtype, un->un);
		un->un = new_un;
		*((const char **)&un->field_name) = field->name;

	union_done:
		/* Continue recursing */
		type = field->type;
		data = un->un;
		break;
	    }
	default:
		assert(0);
		return (NULL);
	}

	/* Recurse on sub-element */
	if ((type = structs_find(type, next, &data, set_union)) == NULL)
		return (NULL);

	/* Done */
	*datap = data;
	return (type);
}

/*
 * Traverse a structure.
 */

struct structs_trav {
	char		**list;
	u_int		len;
	u_int		alloc;
	const char	*mtype;
};

static int	structs_trav(struct structs_trav *t, const char *name,
			const struct structs_type *type, const void *data);

int
structs_traverse(const struct structs_type *type,
	const void *data, char ***listp, const char *mtype)
{
	struct structs_trav t;

	/* Initialize traversal structure */
	memset(&t, 0, sizeof(t));
	t.mtype = mtype;

	/* Recurse */
	if (structs_trav(&t, "", type, data) == -1) {
		while (t.len > 0)
			FREE(t.mtype, t.list[--t.len]);
		FREE(t.mtype, t.list);
		return (-1);
	}

	/* Return the result */
	*listp = t.list;
	return (t.len);
}

static int
structs_trav(struct structs_trav *t, const char *name,
	const struct structs_type *type, const void *data)
{
	const char *const dot = (*name == '\0' ? "" : ".");
	char *ename;
	int i;

	/* Dereference through pointer(s) */
	while (type->tclass == STRUCTS_TYPE_POINTER) {
		type = type->args[0].v;
		data = *((void **)data);
	}

	switch (type->tclass) {
	case STRUCTS_TYPE_PRIMITIVE:
	    {
		/* Grow list as necessary */
		if (t->len == t->alloc) {
			u_int new_alloc;
			char **new_list;

			new_alloc = (t->alloc + 32) * 2;
			if ((new_list = REALLOC(t->mtype,
			    t->list, new_alloc * sizeof(*t->list))) == NULL)
				return (-1);
			t->list = new_list;
			t->alloc = new_alloc;
		}

		/* Add new name to list */
		if ((t->list[t->len] = STRDUP(t->mtype, name)) == NULL)
			return (-1);
		t->len++;

		/* Done */
		return (0);
	    }

	case STRUCTS_TYPE_ARRAY:
	    {
		const struct structs_type *const etype = type->args[0].v;
		const struct structs_array *const ary = data;

		/* Iterate over array elements */
		for (i = 0; i < ary->length; i++) {
			const void *const edata
			    = (char *)ary->elems + (i * etype->size);

			if (ASPRINTF(t->mtype,
			    &ename, "%s%s%d", name, dot, i) == -1)
				return (-1);
			if (structs_trav(t, ename, etype, edata) == -1) {
				FREE(t->mtype, ename);
				return (-1);
			}
			FREE(t->mtype, ename);
		}

		/* Done */
		return (0);
	    }

	case STRUCTS_TYPE_FIXEDARRAY:
	    {
		const struct structs_type *const etype = type->args[0].v;
		const u_int length = type->args[2].i;

		/* Iterate over array elements */
		for (i = 0; i < length; i++) {
			const void *const edata
			    = (char *)data + (i * etype->size);

			if (ASPRINTF(t->mtype,
			    &ename, "%s%s%d", name, dot, i) == -1)
				return (-1);
			if (structs_trav(t, ename, etype, edata) == -1) {
				FREE(t->mtype, ename);
				return (-1);
			}
			FREE(t->mtype, ename);
		}

		/* Done */
		return (0);
	    }

	case STRUCTS_TYPE_STRUCTURE:
	    {
		const struct structs_field *field;

		/* Iterate over structure fields */
		for (field = type->args[0].v; field->name != NULL; field++) {
			const void *const edata
			    = (char *)data + field->offset;

			if (ASPRINTF(t->mtype,
			    &ename, "%s%s%s", name, dot, field->name) == -1)
				return (-1);
			if (structs_trav(t, ename, field->type, edata) == -1) {
				FREE(t->mtype, ename);
				return (-1);
			}
			FREE(t->mtype, ename);
		}

		/* Done */
		return (0);
	    }

	case STRUCTS_TYPE_UNION:
	    {
		const struct structs_ufield *const fields = type->args[0].v;
		const struct structs_union *const un = data;
		const struct structs_ufield *field;

		/* Find field */
		for (field = fields; field->name != NULL
		    && strcmp(un->field_name, field->name) != 0; field++);
		if (field->name == NULL) {
			assert(0);
			errno = EINVAL;
			return (-1);
		}

		/* Do selected union field */
		if (ASPRINTF(t->mtype,
		    &ename, "%s%s%s", name, dot, field->name) == -1)
			return (-1);
		if (structs_trav(t, ename, field->type, un->un) == -1) {
			FREE(t->mtype, ename);
			return (-1);
		}
		FREE(t->mtype, ename);

		/* Done */
		return (0);
	    }

	default:
		assert(0);
		errno = ECONNABORTED;
		return (-1);
	}
}

