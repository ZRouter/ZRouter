
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

#ifndef _PDEL_STRUCTS_TYPE_ARRAY_H_
#define _PDEL_STRUCTS_TYPE_ARRAY_H_

/*********************************************************************
		    VARIABLE LENGTH ARRAYS
*********************************************************************/

/*
 * The data must be a 'struct structs_array', or a structure
 * defined using the DEFINE_STRUCTS_ARRAY() macro.
 */
struct structs_array {
	u_int	length;		/* number of elements in array */
	void	*elems;		/* array elements */
};

/*
 * Use this to get 'elems' declared as the right type
 */
#define DEFINE_STRUCTS_ARRAY(name, etype)				\
struct name {								\
	u_int	length;		/* number of elements in array */	\
	etype	*elems;		/* array elements */			\
}

/*
 * Macro arguments:
 *	[const struct structs_type *]	Element type
 *	[const char *]			Memory allocation type for "elems"
 *	[const char *]			XML tag for individual elements
 */
#define STRUCTS_ARRAY_TYPE(etype, mtype, etag) {			\
	sizeof(struct structs_array),					\
	"array",							\
	STRUCTS_TYPE_ARRAY,						\
	structs_region_init,						\
	structs_array_copy,						\
	structs_array_equal,						\
	structs_notsupp_ascify,						\
	structs_notsupp_binify,						\
	structs_array_encode,						\
	structs_array_decode,						\
	structs_array_free,						\
	{ { (void *)(etype) }, { (void *)(mtype) }, { (void *)(etag) } }\
}

__BEGIN_DECLS

extern structs_copy_t		structs_array_copy;
extern structs_equal_t		structs_array_equal;
extern structs_encode_t		structs_array_encode;
extern structs_decode_t		structs_array_decode;
extern structs_uninit_t		structs_array_free;

/*
 * Additional functions for handling arrays
 */
extern int	structs_array_length(const struct structs_type *type,
			const char *name, const void *data);
extern int	structs_array_reset(const struct structs_type *type,
			const char *name, void *data);
extern int	structs_array_insert(const struct structs_type *type,
			const char *name, u_int indx, void *data);
extern int	structs_array_delete(const struct structs_type *type,
			const char *name, u_int indx, void *data);
extern int	structs_array_prep(const struct structs_type *type,
			const char *name, void *data);

__END_DECLS

/*********************************************************************
			FIXED LENGTH ARRAYS
*********************************************************************/

/*
 * Fixed length array type.
 */

/*
 * Macro arguments:
 *	[const struct structs_type *]	Element type
 *	[u_int]				Size of each element
 *	[u_int]				Length of array
 *	[const char *]			XML tag for individual elements
 */
#define STRUCTS_FIXEDARRAY_TYPE(etype, esize, alen, etag) {		\
	(esize) * (alen),						\
	"fixedarray",							\
	STRUCTS_TYPE_FIXEDARRAY,					\
	structs_fixedarray_init,					\
	structs_fixedarray_copy,					\
	structs_fixedarray_equal,					\
	structs_notsupp_ascify,						\
	structs_notsupp_binify,						\
	structs_fixedarray_encode,					\
	structs_fixedarray_decode,					\
	structs_fixedarray_free,					\
	{ { (void *)(etype) }, { (void *)(etag) }, { (void *)(alen) } }	\
}

__BEGIN_DECLS

extern structs_init_t		structs_fixedarray_init;
extern structs_copy_t		structs_fixedarray_copy;
extern structs_equal_t		structs_fixedarray_equal;
extern structs_encode_t		structs_fixedarray_encode;
extern structs_decode_t		structs_fixedarray_decode;
extern structs_uninit_t		structs_fixedarray_free;

__END_DECLS

#endif	/* _PDEL_STRUCTS_TYPE_ARRAY_H_ */

