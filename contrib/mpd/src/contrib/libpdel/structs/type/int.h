
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

#ifndef _PDEL_STRUCTS_TYPE_INT_H_
#define _PDEL_STRUCTS_TYPE_INT_H_

/*********************************************************************
			INTEGRAL TYPES
*********************************************************************/

__BEGIN_DECLS

extern const struct structs_type	structs_type_char;
extern const struct structs_type	structs_type_uchar;
extern const struct structs_type	structs_type_hchar;
extern const struct structs_type	structs_type_short;
extern const struct structs_type	structs_type_ushort;
extern const struct structs_type	structs_type_hshort;
extern const struct structs_type	structs_type_int;
extern const struct structs_type	structs_type_uint;
extern const struct structs_type	structs_type_hint;
extern const struct structs_type	structs_type_long;
extern const struct structs_type	structs_type_ulong;
extern const struct structs_type	structs_type_hlong;
extern const struct structs_type	structs_type_int8;
extern const struct structs_type	structs_type_uint8;
extern const struct structs_type	structs_type_hint8;
extern const struct structs_type	structs_type_int16;
extern const struct structs_type	structs_type_uint16;
extern const struct structs_type	structs_type_hint16;
extern const struct structs_type	structs_type_int32;
extern const struct structs_type	structs_type_uint32;
extern const struct structs_type	structs_type_hint32;
extern const struct structs_type	structs_type_int64;
extern const struct structs_type	structs_type_uint64;
extern const struct structs_type	structs_type_hint64;

extern structs_ascify_t		structs_int_ascify;
extern structs_binify_t		structs_int_binify;

__END_DECLS

#endif	/* _PDEL_STRUCTS_TYPE_INT_H_ */

