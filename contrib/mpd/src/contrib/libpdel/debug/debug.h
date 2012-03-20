
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

#ifndef _PDEL_PDEL_DEBUG_H_
#define _PDEL_PDEL_DEBUG_H_

/*
 * This flag globally enables/disables debugging.
 */
#define PDEL_DEBUG			0

#if PDEL_DEBUG
/*
 * Flags that enable various debugging options.
 */
enum {
	PDEL_DEBUG_HTTP,
	PDEL_DEBUG_HTTP_HDRS,
	PDEL_DEBUG_HTTP_CONNECTION_CACHE,
	PDEL_DEBUG_HTTP_SERVLET_COOKIEAUTH,
	PDEL_DEBUG_PEVENT,
	PDEL_DEBUG_MUTEX,
};

/*
 * Runtime variable which controls debugging.
 */
#define PDEL_DEBUG_FLAGS (0 						\
	| (1 << PDEL_DEBUG_HTTP)					\
	| (1 << PDEL_DEBUG_PEVENT)					\
	| (1 << PDEL_DEBUG_MUTEX)					\
	| 0)

#endif	/* PDEL_DEBUG */

/*
 * Macro to test whether a specific debug flag is enabled.
 */
#if PDEL_DEBUG
#define PDEL_DEBUG_ENABLED(f)						\
	((PDEL_DEBUG_FLAGS & (1 << PDEL_DEBUG_ ## f)) != 0)
#else
#define PDEL_DEBUG_ENABLED(f)		(0)
#endif

/*
 * Macro for printing some debugging output.
 */
#if PDEL_DEBUG
#define DBG(c, fmt, args...)						\
	do {								\
		char buf[240];						\
		const char *s;						\
		int r, n;						\
									\
		if (PDEL_DEBUG_ENABLED(c)) {				\
			snprintf(buf, sizeof(buf),			\
			    "%p[%s]: " fmt "\n", pthread_self(),	\
			    __FUNCTION__ , ## args);			\
			for (r = strlen(buf), s = buf;			\
			    r > 0; r -= n, s += n) {			\
				if ((n = write(1, s, r)) == -1)		\
					break;				\
			}						\
		}							\
	} while (0)
#else
#define DBG(c, fmt, args...)	do { } while (0)
#endif	/* !PDEL_DEBUG */

/*
 * Mutex macros with additional debugging.
 */
#if PDEL_DEBUG

#define MUTEX_LOCK(m, c)						\
	do {								\
		int _r;							\
									\
		DBG(MUTEX, "locking %s", #m);				\
		_r = pthread_mutex_lock(m);				\
		assert(_r == 0);					\
		(c)++;							\
		DBG(MUTEX, "%s locked -> %d", #m, (c));			\
	} while (0)

#define MUTEX_UNLOCK(m, c)						\
	do {								\
		int _r;							\
									\
		(c)--;							\
		DBG(MUTEX, "unlocking %s -> %d", #m, (c));		\
		_r = pthread_mutex_unlock(m);				\
		assert(_r == 0);					\
		DBG(MUTEX, "%s unlocked", #m);				\
	} while (0)

#define MUTEX_TRYLOCK(m, c)						\
	do {								\
		int _r;							\
									\
		DBG(MUTEX, "try locking %s", #m);			\
		_r = pthread_mutex_trylock(m);				\
		if (_r == 0) {						\
			(c)++;						\
			DBG(MUTEX, "%s locked -> %d", #m, (c));		\
		} else							\
			DBG(MUTEX, "%s lock failed", #m);		\
		errno = _r;						\
	} while (0)

#else	/* !PDEL_DEBUG */

#define MUTEX_LOCK(m, c)	(void)pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m, c)	(void)pthread_mutex_unlock(m)
#define MUTEX_TRYLOCK(m, c)	(errno = pthread_mutex_trylock(m))

#endif	/* !PDEL_DEBUG */

#endif	/* _PDEL_PDEL_DEBUG_H_ */
