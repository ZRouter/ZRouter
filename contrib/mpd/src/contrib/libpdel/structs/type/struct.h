
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

#ifndef _PDEL_STRUCTS_TYPE_STRUCT_H_
#define _PDEL_STRUCTS_TYPE_STRUCT_H_

/*********************************************************************
			STRUCTURE TYPES
*********************************************************************/

/* This structure describes one field in a structure */
struct structs_field {
	const char			*name;
	const struct structs_type	*type;
	u_int16_t			size;
	u_int16_t			offset;
};

/* Use this to describe a field and name it with the field's C name */
#define STRUCTS_STRUCT_FIELD(sname, fname, ftype) 			\
	{ #fname, ftype, sizeof(((struct sname *)0)->fname), 		\
	    offsetof(struct sname, fname) }

/* Use this to describe a field and give it a different name */
#define STRUCTS_STRUCT_FIELD2(sname, fname, dname, ftype) 		\
	{ dname, ftype, sizeof(((struct sname *)0)->fname), 		\
	    offsetof(struct sname, fname) }

/* This macro terminates a list of 'struct structs_field' structures */
#define STRUCTS_STRUCT_FIELD_END	{ NULL, NULL, 0, 0 }

/*
 *
 * Macro arguments:
 *	name				Structure name (as in 'struct name')
 *	[const struct structs_field *]	List of structure fields, terminated
 *					    by an entry with name == NULL.
 */
#define STRUCTS_STRUCT_TYPE(sname, flist) {				\
	sizeof(struct sname),						\
	"structure",							\
	STRUCTS_TYPE_STRUCTURE,						\
	structs_struct_init,						\
	structs_struct_copy,						\
	structs_struct_equal,						\
	structs_notsupp_ascify,						\
	structs_notsupp_binify,						\
	structs_struct_encode,						\
	structs_struct_decode,						\
	structs_struct_free,						\
	{ { (void *)(flist) }, { NULL }, { NULL } }			\
}

__BEGIN_DECLS

extern structs_init_t		structs_struct_init;
extern structs_copy_t		structs_struct_copy;
extern structs_equal_t		structs_struct_equal;
extern structs_encode_t		structs_struct_encode;
extern structs_decode_t		structs_struct_decode;
extern structs_uninit_t		structs_struct_free;

__END_DECLS

#endif	/* _PDEL_STRUCTS_TYPE_STRUCT_H_ */

