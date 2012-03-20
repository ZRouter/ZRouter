
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

#ifndef _PDEL_IO_STRING_FP_H_
#define _PDEL_IO_STRING_FP_H_

__BEGIN_DECLS

/*
 * Create an input stream from a buffer. The buffer is copied if copy != 0.
 *
 * Returns NULL and sets errno if there was an error.
 */
extern FILE	*string_buf_input(const void *buf, size_t len, int copy);

/*
 * Create a new string output buffer. The string will be allocated
 * using the supplied memory type (the string is *not* copied).
 *
 * Returns NULL and sets errno if there was an error.
 */
extern FILE	*string_buf_output(const char *mtype);

/*
 * Get the current contents of the output buffer of a stream created
 * with string_buf_output().
 *
 * If 'reset' is true, then the output buffer is reset and the returned
 * value must be manually free'd (using the same memory type supplied
 * to string_buf_output); otherwise, it should not be freed or modified.
 *
 * In any case, the string is always NUL-terminated.
 *
 * Returns NULL and sets errno if there was an error; the stream will
 * still need to be closed.
 */
extern char	*string_buf_content(FILE *fp, int reset);

/*
 * Get the number of bytes currently in the output buffer
 * of a stream created with string_buf_output().
 */
extern int	string_buf_length(FILE *fp);

__END_DECLS

#endif	/* _PDEL_IO_STRING_FP_H_ */

