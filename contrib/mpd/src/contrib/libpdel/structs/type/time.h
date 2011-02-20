
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

#ifndef _PDEL_STRUCTS_TYPE_TIME_H_
#define _PDEL_STRUCTS_TYPE_TIME_H_

/*********************************************************************
			    TIME TYPE
*********************************************************************/

/*
 * The data is a variable of type "time_t" which holds a time value
 * containing the number of seconds since 1/1/1970 GMT (as always).
 */

__BEGIN_DECLS

/*
 * GMT time: ASCII representation like "Sat Mar  2 23:29:28 GMT 2001".
 */
extern const struct structs_type	structs_type_time_gmt;

/*
 * Local time: ASCII representation like "Sat Mar  2 15:29:28 PDT 2001".
 *
 * Note: using local time does not really work, because local time
 * zone abbrieviations are not uniquely decodable. Therefore local
 * time representations (such as with structs_type_time_local) should
 * be avoided except where it is known that both encoding and decoding
 * time zones are the same.
 */
extern const struct structs_type	structs_type_time_local;

/*
 * GMT time represented in ISO-8601 format, e.g., "20010302T23:29:28".
 */
extern const struct structs_type	structs_type_time_iso8601;

/*
 * GMT time represented as seconds since the epoch, e.g., "983575768".
 */
extern const struct structs_type	structs_type_time_abs;

/*
 * GMT time represented relative to now, e.g., "3600" or "-123".
 *
 * Useful for expiration times, etc.
 */
extern const struct structs_type	structs_type_time_rel;

__END_DECLS

#endif	/* _PDEL_STRUCTS_TYPE_TIME_H_ */

