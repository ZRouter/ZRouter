
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

#ifndef _PDEL_STRUCTS_TYPE_UNION_H_
#define _PDEL_STRUCTS_TYPE_UNION_H_

/*********************************************************************
			UNION TYPES
*********************************************************************/

/*
 * Union type.
 *
 * The data must be a 'struct structs_union', or a structure
 * defined using the DEFINE_STRUCTS_UNION() macro.
 */
struct structs_union {
#ifdef __cplusplus
	const char	*field_name;		/* name of field in use */
#else
	const char	*const field_name;	/* name of field in use */
#endif
	void		*un;			/* pointer to field contents */
};

/*
 * Use this to get the 'un' field declared having the correct type.
 */
#ifdef __cplusplus
#define DEFINE_STRUCTS_UNION(sname, uname)				\
struct sname {								\
	const char	*field_name;	    /* name of field in use */	\
	union uname	*un;		    /* pointer to the field */	\
}
#else
#define DEFINE_STRUCTS_UNION(sname, uname)				\
struct sname {								\
	const char	*const field_name;  /* name of field in use */	\
	union uname	*un;		    /* pointer to the field */	\
}
#endif

/* This structure describes one field in a union */
struct structs_ufield {
	const char			*name;		/* name of field */
	const struct structs_type	*type;		/* type of field */
	char				*dummy;		/* for type checking */
};

/* Use this macro to initialize a 'struct structs_ufield' */
#define STRUCTS_UNION_FIELD(fname, ftype) 				\
	{ #fname, ftype, NULL }

/* This macro terminates a list of 'struct structs_ufield' structures */
#define STRUCTS_UNION_FIELD_END		{ NULL, NULL, NULL }

__BEGIN_DECLS

/*
 * Union type
 *
 * Type-specific arguments:
 *	[const struct structs_ufield *]	List of union fields, terminated
 *					    by entry with name == NULL.
 *	[const char *]			Memory allocation type for "un"
 */
extern structs_init_t		structs_union_init;
extern structs_copy_t		structs_union_copy;
extern structs_equal_t		structs_union_equal;
extern structs_encode_t		structs_union_encode;
extern structs_decode_t		structs_union_decode;
extern structs_uninit_t		structs_union_free;

#define STRUCTS_UNION_TYPE(uname, flist) {				\
	sizeof(struct structs_union),					\
	"union",							\
	STRUCTS_TYPE_UNION,						\
	structs_union_init,						\
	structs_union_copy,						\
	structs_union_equal,						\
	structs_notsupp_ascify,						\
	structs_notsupp_binify,						\
	structs_union_encode,						\
	structs_union_decode,						\
	structs_union_free,						\
	{ { (void *)(flist) }, { (void *)("union " #uname) }, { NULL } }\
}

/* Functions */
extern int	structs_union_set(const struct structs_type *type,
			const char *name, void *data, const char *field_name);

__END_DECLS

#endif	/* _PDEL_STRUCTS_TYPE_UNION_H_ */

