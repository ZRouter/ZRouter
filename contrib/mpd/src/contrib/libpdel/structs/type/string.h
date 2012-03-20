
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

#ifndef _PDEL_STRUCTS_TYPE_STRING_H_
#define _PDEL_STRUCTS_TYPE_STRING_H_

/*********************************************************************
			STRING TYPES
*********************************************************************/

__BEGIN_DECLS

/*
 * Dynamically allocated string types
 *
 * Type-specific arguments:
 *	[const char *]	Memory allocation type
 *	[int]		Whether to store empty string as NULL or ""
 */
extern structs_init_t		structs_string_init;
extern structs_equal_t		structs_string_equal;
extern structs_ascify_t		structs_string_ascify;
extern structs_binify_t		structs_string_binify;
extern structs_encode_t		structs_string_encode;
extern structs_decode_t		structs_string_decode;
extern structs_uninit_t		structs_string_free;

__END_DECLS

#define STRUCTS_TYPE_STRING_MTYPE	"structs_type_string"

#define STRUCTS_STRING_TYPE(mtype, asnull) {				\
	sizeof(char *),							\
	"string",							\
	STRUCTS_TYPE_PRIMITIVE,						\
	structs_string_init,						\
	structs_ascii_copy,						\
	structs_string_equal,						\
	structs_string_ascify,						\
	structs_string_binify,						\
	structs_string_encode,						\
	structs_string_decode,						\
	structs_string_free,						\
	{ { (void *)(mtype) }, { (void *)(asnull) }, { NULL } }		\
}

__BEGIN_DECLS

/* A string type with allocation type "structs_type_string" and never NULL */
extern const struct structs_type	structs_type_string;

/* A string type with allocation type "structs_type_string" and can be NULL */
extern const struct structs_type	structs_type_string_null;

__END_DECLS

/*********************************************************************
		    BOUNDED LENGTH STRING TYPES
*********************************************************************/

__BEGIN_DECLS

/*
 * Bounded length string types
 *
 * Type-specific arguments:
 *	[int]		Size of string buffer
 */
extern structs_equal_t		structs_bstring_equal;
extern structs_ascify_t		structs_bstring_ascify;
extern structs_binify_t		structs_bstring_binify;

__END_DECLS

#define STRUCTS_FIXEDSTRING_TYPE(bufsize) {				\
	(bufsize),							\
	"fixedstring",							\
	STRUCTS_TYPE_PRIMITIVE,						\
	structs_region_init,						\
	structs_region_copy,						\
	structs_bstring_equal,						\
	structs_bstring_ascify,						\
	structs_bstring_binify,						\
	structs_string_encode,						\
	structs_string_decode,						\
	structs_nothing_free,						\
	{ { NULL }, { NULL }, { NULL } }				\
}

#endif	/* _PDEL_STRUCTS_TYPE_STRING_H_ */

