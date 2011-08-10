
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

#ifndef _PDEL_UTIL_PEVENT_H_
#define _PDEL_UTIL_PEVENT_H_

/*
 * Event-based scheduling
 */

struct pevent;
struct pevent_ctx;
struct mesg_port;

#define PEVENT_MAX_EVENTS		128

/*
 * Event handler function type
 */
typedef void	pevent_handler_t(void *arg);

/*
 * Event types
 */
enum pevent_type {
	PEVENT_READ = 1,
	PEVENT_WRITE,
	PEVENT_TIME,
	PEVENT_MESG_PORT,
	PEVENT_USER
};

/*
 * Event info structure filled in by pevent_get_info().
 */
struct pevent_info {
	enum pevent_type	type;	/* event type */
	union {
	    int			fd;	/* file descriptor (READ, WRITE) */
	    int			millis;	/* delay in milliseconds (TIME) */
	    struct mesg_port	*port;	/* mesg_port(3) (MESG_PORT) */
	}			u;
};

/*
 * Event flags
 */
#define PEVENT_RECURRING	0x0001
#define PEVENT_OWN_THREAD	0x0002

__BEGIN_DECLS

/*
 * Create a new event context.
 */
extern struct	pevent_ctx *pevent_ctx_create(const char *mtype,
			const pthread_attr_t *attr);

/*
 * Destroy an event context.
 *
 * All events are unregistered. This is safe to be called
 * from within an event handler.
 */
extern void	pevent_ctx_destroy(struct pevent_ctx **ctxp);

/*
 * Return the number of registered events.
 */
extern u_int	pevent_ctx_count(struct pevent_ctx *ctx);

/*
 * Create a new event.
 */
extern int	pevent_register(struct pevent_ctx *ctx, struct pevent **peventp,
			int flags, pthread_mutex_t *mutex,
			pevent_handler_t *handler, void *arg,
			enum pevent_type type, ...);

/*
 * Trigger a user event.
 */
extern void	pevent_trigger(struct pevent *pevent);

/*
 * Get the type and parameters for an event.
 */
extern int	pevent_get_info(struct pevent *pevent,
			struct pevent_info *info);

/*
 * Destroy an event.
 */
extern void	pevent_unregister(struct pevent **eventp);

__END_DECLS

#endif	/* _PDEL_UTIL_PEVENT_H_ */
