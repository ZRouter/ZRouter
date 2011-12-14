
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

#ifndef _PDEL_STRUCTS_STRUCTS_H_
#define _PDEL_STRUCTS_STRUCTS_H_

/*********************************************************************

			    STRUCTS TYPES
			    -------------

    A "structs type" is a descriptor for a C data structure that
    allows access to the data structure in an automated fashion.
    For example, once a data structure is described by a associated
    structs type, the following operations can be performed
    automatically on any instance of the data structure:

	o Initialization to default value
	o Comparison of two instances for equality
	o "Deep" copying (aka. "cloning")
	o Access to arbitrary sub-fields by name (aka. "introspection")
	o Conversion to/from ASCII (primitive types)
	o Import/export from/to XML, with precise input validation,
	  automating conversion between internal and external formats
	o Conversion into XML-RPC request or reply
	o Import/export from/to a machine independent binary encoding
	o Inserting, deleting, and extending elements in arrays

    In addition, any required dynamic memory allocation and free'ing
    is handled automatically.

    Supported data structures include the normal primitive C types,
    plus arrays, structures, unions, and pointers to supported types,
    as well as some other useful types such as opaque binary data,
    IP address, regular expression, etc. User-defined types may be
    added as well.

    Put simply, a structs type describes how to interpret a region
    of memory so that the operations listed above may be peformed
    in an automated fashion.

    Arrays must be defined using the "structs_array" structure.
    Unions must be defined using the "structs_union" structure.

    Nested items in a data structure can be identified by names,
    where each name component is either the name of a structure or
    union field, or an array index. Name components are separated
    by dots. For example, "foo.list.12" (see below).

    EXAMPLE
    -------

    Consider the C data structure defined by "struct janfu":

	struct fubar {
		struct structs_array	list;	// array of strings
	};

	struct janfu {
		u_int16_t		value;	// 16 bit hex value
		struct fubar		*foo;	// fubar structure
	};

    A memory region containing a 'struct janfu' would be described
    in C code like so:

	// Descriptor for field 'list' in 'struct fubar': array of strings
	static const struct structs_type list_array_type =
	    STRUCTS_ARRAY_TYPE(&structs_type_string, "list_elem", "list_elem");

	// Descriptor for 'struct fubar'
	static const struct structs_field fubar_fields[] = {
	    STRUCTS_STRUCT_FIELD(fubar, list, &list_array_type),
	    STRUCTS_STRUCT_FIELD_END
	};
	static const struct structs_type fubar_type =
	    STRUCTS_STRUCT_TYPE(fubar, &fubar_fields);

	// Descriptor for a variable of type 'struct fubar *'
	static const struct structs_type foo_ptr_type =
	    STRUCTS_POINTER_TYPE(&fubar_type, "struct fubar");

	// Descriptor for 'struct janfu'
	static const struct structs_field janfu_fields[] = {
	    STRUCTS_STRUCT_FIELD(janfu, value, &structs_type_uint16),
	    STRUCTS_STRUCT_FIELD(janfu, foo, &foo_ptr_type),
	    STRUCTS_STRUCT_FIELD_END
	};
	static const struct structs_type janfu_type =
	    STRUCTS_STRUCT_TYPE(janfu, &janfu_fields);

    The type "janfu_type" describes a 'struct janfu' data structure.

    Then "foo.list.12" refers to the 12th item in the "list" array
    (assuming it actually exists), counting from zero. It's value
    could be accessed like this:

	void
	print_list_12(const struct janfu *j)
	{
	    char *s;

	    s = structs_get_string(&janfu_type, "foo.list.12", j, NULL);
	    if (s == NULL) {
		    fprintf(stderr, "structs_get_string: %s: %s",
			"foo.list.12", strerror(errno));
		    exit(1);
	    }
	    printf("foo.list.12 = %s\n", s);
	    free(s);
	}

    An instance of a "struct janfu", if output as XML, might look
    like this:

	<?xml version="1.0" standalone="yes"?>
	<janfu>
	    <value>0x1234</value>
	    <list>
		<list_elem>first list element</list_elem>
		<list_elem>second list element</list_elem>
		<list_elem>third list element</list_elem>
	    </list>
	</janfu>

    A function call that would produce the above output is:

	void
	dump_janfu(struct janfu *j)
	{
	    structs_xml_output(&janfu_type, "janfu", NULL, j, stdout, NULL, 0);
	}

*********************************************************************/

/* Name component separator */
#define STRUCTS_SEPARATOR	'.'

struct structs_type;

/* For representing arbitrary binary data */
struct structs_data {
	u_int	length;			/* number of bytes */
	u_char	*data;			/* bytes */
};

/*********************************************************************
			STRUCTS TYPE METHODS
*********************************************************************/

/*
 * The structs type "init" method.
 *
 * The region of memory pointed to by "data" must be at least
 * type->size bytes and should be uninitialized (i.e., it should
 * not contain a valid instance of the type).
 *
 * This method should initialize the memory with a valid instance
 * of the type, setting this new instance to be equal to the default
 * value for the type, and allocating any resources required for the
 * instance in the process.
 *
 * Returns 0 if successful, or -1 (and sets errno) if there was an error.
 */
typedef int	structs_init_t(const struct structs_type *type, void *data);

/*
 * The structs type "copy" method.
 *
 * The region of memory pointed to by "from" contains a valid instance
 * of the type described by "type".
 *
 * The region of memory pointed to by "to" is uninitialized.
 *
 * This method should initialize "to" with a "deep" copy of the
 * data structure pointed to by "from". The copy must be equal
 * (according to the "equal" method) to the original.
 *
 * Returns 0 if successful, or -1 (and sets errno) if there was an error.
 */
typedef int	structs_copy_t(const struct structs_type *type,
			const void *from, void *to);

/*
 * The structs type "equal" method.
 *
 * The regions of memory pointed to by "data1" and "data2" contain valid
 * instances of the type described by "type".
 *
 * Returns 1 if "data1" and "data2" are equal (by a "deep" comparison), or
 * 0 if not, or -1 (and sets errno) if there was an error.
 */
typedef int	structs_equal_t(const struct structs_type *type,
			const void *data1, const void *data2);

/*
 * The structs type "ascify" method.
 *
 * The region of memory pointed to by "data" contains a valid instance
 * of the type described by "type".
 *
 * The method should convert this instance into a NUL-terminated
 * ASCII string stored in a dynamically allocated buffer that is
 * allocated with the memory type described by "mtype".
 *
 * The ASCII string must be unique, in that it can be used as input
 * to the "binify" method to create an instance equal (according to the
 * "equal" method) to the original.
 *
 * Returns the ASCII string buffer if successful, or NULL
 * (and sets errno) if error.
 */
typedef char	*structs_ascify_t(const struct structs_type *type,
			const char *mtype, const void *data);

/*
 * The structs type "binify" method.
 *
 * The region of memory pointed to by "data" is uninitialized.
 *
 * The variable "ascii" points to a NUL-terminated string. This method
 * should use the contents of the string to create a new instace of
 * the type in the memory pointed to by "data".
 *
 * Returns 0 if successful, or -1 if there was an error. In the latter
 * case, either errno should be set, or a more informative error string
 * may be printed (including NUL) into "ebuf", which has total size "emax"
 * (see snprintf(3)).
 *
 * This method must be robust in the face of bogus data in "ascii";
 * it must return an error if "ascii" is invalid.
 */
typedef int	structs_binify_t(const struct structs_type *type,
			const char *ascii, void *data, char *ebuf, size_t emax);

/*
 * The structs type "encode" method.
 *
 * The region of memory pointed to by "data" contains a valid instance
 * of the type described by "type".
 *
 * This method should encode the data structure into a host order
 * independent, self-delimiting sequence of bytes. The sequence must be
 * stored in a dynamically allocated buffer allocated with the memory type
 * described by "mtype", and "code" must be filled in with the buffer's
 * pointer and length.
 *
 * The binary sequence must be unique, in that it can be used as input
 * to the "decode" method to create an instance equal (according to the
 * "equal" method) to the original.
 *
 * Returns 0 if successful, or -1 (and sets errno) if there was an error.
 */
typedef	int	structs_encode_t(const struct structs_type *type,
			const char *mtype, struct structs_data *code,
			const void *data);

/*
 * The structs type "decode" method.
 *
 * The region of memory pointed to by "data" is uninitialized.
 *
 * The "code" parameter points to a sequence of "cmax" binary data bytes.
 * This method should decode the binary data and use it to reconstruct
 * an instance of "type" in the memory region pointed to by "data".
 *
 * Returns the number of bytes consumed in the decoding process if
 * successful, or -1 (and sets errno) if there was an error. In the
 * latter case, either errno should be set, or a more informative error
 * string may be printed (including NUL) into "ebuf", which has total
 * size "emax" (see snprintf(3)).
 */
typedef	int	structs_decode_t(const struct structs_type *type,
			const u_char *code, size_t cmax, void *data,
			char *ebuf, size_t emax);

/*
 * The structs type "uninit" method.
 *
 * The region of memory pointed to by "data" contains a valid instance
 * of the type described by "type".
 *
 * This method should free any resources associated with the instance
 * pointed to by "data".  Upon return, the memory pointed to by "data"
 * may be no longer used or referenced.
 */
typedef void	structs_uninit_t(const struct structs_type *type, void *data);

/*********************************************************************
			STRUCTS TYPE DEFINITION
*********************************************************************/

/*
 * This structure defines a structs type.
 */
struct structs_type {
	size_t			size;		/* size of an instance */
	const char		*name;		/* human informative name */
	int			tclass;		/* see classes listed below */
	structs_init_t		*init;		/* type "init" method */
	structs_copy_t		*copy;		/* type "copy" method */
	structs_equal_t		*equal;		/* type "equal" method */
	structs_ascify_t	*ascify;	/* type "ascify" method */
	structs_binify_t	*binify;	/* type "binify" method */
	structs_encode_t	*encode;	/* type "encode" method */
	structs_decode_t	*decode;	/* type "decode" method */
	structs_uninit_t	*uninit;	/* type "uninit" method */
	union {					/* type specific arguments */
		const void	*v;
		const char	*s;
		int		i;
	}			args[3];
};

/* Classes of types */
#define STRUCTS_TYPE_PRIMITIVE	0
#define STRUCTS_TYPE_POINTER	1
#define STRUCTS_TYPE_ARRAY	2
#define STRUCTS_TYPE_FIXEDARRAY	3
#define STRUCTS_TYPE_STRUCTURE	4
#define STRUCTS_TYPE_UNION	5

/*********************************************************************
			GENERIC TYPE METHODS
*********************************************************************/

__BEGIN_DECLS

/* These operate on simple block regions of memory */
extern structs_init_t		structs_region_init;
extern structs_copy_t		structs_region_copy;
extern structs_equal_t		structs_region_equal;

extern structs_encode_t		structs_region_encode;
extern structs_decode_t		structs_region_decode;
extern structs_encode_t		structs_region_encode_netorder;
extern structs_decode_t		structs_region_decode_netorder;

/* These always return an error with errno set to EOPNOTSUPP */
extern structs_init_t		structs_notsupp_init;
extern structs_copy_t		structs_notsupp_copy;
extern structs_equal_t		structs_notsupp_equal;
extern structs_ascify_t		structs_notsupp_ascify;
extern structs_binify_t		structs_notsupp_binify;
extern structs_encode_t		structs_notsupp_encode;
extern structs_decode_t		structs_notsupp_decode;

/* This copies any type that is ascifyable by converting to ASCII and back */
extern structs_copy_t		structs_ascii_copy;

/* This does nothing */
extern structs_uninit_t		structs_nothing_free;

__END_DECLS

/*********************************************************************
			BASIC STRUCTS FUNCTIONS
*********************************************************************/

__BEGIN_DECLS

/*
 * Initialize an item in a data structure to its default value.
 * This assumes the item is not already initialized.
 *
 * If "name" is NULL or empty string, the entire structure is initialied.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_init(const struct structs_type *type,
			const char *name, void *data);

/*
 * Reset an item in a data structure to its default value.
 * This assumes the item is already initialized.
 *
 * If "name" is NULL or empty string, the entire structure is reset.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_reset(const struct structs_type *type,
			const char *name, void *data);

/*
 * Free an item in a data structure, returning it to its uninitialized state.
 * This function can be thought of as the opposite of structs_init().
 *
 * If "name" is NULL or empty string, the entire structure is free'd.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_free(const struct structs_type *type,
			const char *name, void *data);

/*
 * Compare two data structures of the same type for equality.
 *
 * If "name" is NULL or empty string, the entire structures are compared.
 * Otherwise, the "name" sub-elements of both structures are compared.
 *
 * Returns zero or one (for not equal or equal, respectively),
 * or -1 (and sets errno) if there was an error.
 */
extern int	structs_equal(const struct structs_type *type,
			const char *name, const void *data1, const void *data2);

/*
 * Find an item in a data structure structure.
 *
 * If "name" is NULL or empty string, "type" is returned and "datap"
 * is unmodified.
 *
 * If "set_union" is true, then any unions in the path will be reset
 * so that their fields agree with "name" in the case that "name" contains
 * a different field name than is currently set for that union. However,
 * these changes will only occur if "name" is actually found.
 *
 * Returns the type of the item and points *datap at it, or NULL
 * (and sets errno) if there was an error.
 */
extern const	struct structs_type *structs_find(
			const struct structs_type *type, const char *name,
			void **datap, int set_union);

/*
 * Get a copy of an item from a data structure.
 *
 * If "name" is NULL or empty string, the entire structure is copied.
 *
 * It is assumed that "to" points to a region big enough to hold
 * the copy of the item. "to" will not be free'd first, so it should
 * not already be initialized.
 *
 * Note: "name" is only applied to "from". The structs types of
 * "from.<name>" and "to" must be the same.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_get(const struct structs_type *type,
			const char *name, const void *from, void *to);

/*
 * Set an item in a data structure.
 *
 * If "name" is NULL or empty string, the entire structure is copied.
 *
 * It is assumed that "to" is already initialized.
 *
 * Note: "name" is only applied to "to". The structs types of
 * "from" and "to.<name>" must be the same.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_set(const struct structs_type *type,
			const void *from, const char *name, void *to);

/*
 * Get the ASCII form of an item, in a string allocated with the
 * memory type described by "mtype".
 *
 * The caller is reponsible for freeing the returned string.
 *
 * Returns the ASCII string if successful, otherwise NULL and sets errno.
 */
extern char	*structs_get_string(const struct structs_type *type,
			const char *name, const void *data, const char *mtype);

/*
 * Set the value of an item in a data structure from an ASCII string.
 *
 * The referred to item must be of a type that supports ASCII encoding,
 * and is assumed to be already initialized.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_set_string(const struct structs_type *type,
			const char *name, const char *ascii,
			void *data, char *ebuf, size_t emax);

/*
 * Get the binary encoded form of an item, put into a buffer
 * allocated with memory type "mtype" and described by "code".
 *
 * The caller is reponsible for freeing the allocated bytes.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_get_binary(const struct structs_type *type,
			const char *name, const void *data,
			const char *mtype, struct structs_data *code);

/*
 * Set an item's value from its binary encoded value.
 *
 * The referred to item is assumed to be already initialized.
 *
 * Returns 0 if successful, otherwise -1 and sets errno.
 */
extern int	structs_set_binary(const struct structs_type *type,
			const char *name, const struct structs_data *code,
			void *data, char *ebuf, size_t emax);

/*
 * Traverse a structure, returning the names of all primitive fields
 * (i.e., the "leaves" of the data structure).
 *
 * The number of items is returned, or -1 and errno if there was an error.
 * The caller must free the list by freeing each string in the array,
 * and then the array itself; all memory is allocated using memory type
 * "mtype".
 *
 * If "type" is a primitive type, a list of length one is returned
 * containing an empty string.
 */
extern int	structs_traverse(const struct structs_type *type,
			const void *data, char ***listp, const char *mtype);

__END_DECLS

#endif	/* _PDEL_STRUCTS_STRUCTS_H_ */

