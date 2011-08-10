
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <pthread.h>

#include "structs/structs.h"
#include "structs/type/array.h"
#include "util/typed_mem.h"
#include "util/mesg_port.h"
#include "util/pevent.h"
#ifdef SYSLOG_FACILITY
#define alogf(sev, fmt, arg...) syslog(sev, "%s: " fmt, __FUNCTION__ , ## arg)
#else
#define alogf(sev, fmt, arg...) do{}while(0)
#endif

#include "internal.h"
#include "debug/debug.h"

#define PEVENT_MAGIC		0x31d7699b
#define PEVENT_CTX_MAGIC	0x7842f901

/* Private flags */
#define PEVENT_OCCURRED		0x8000		/* event has occurred */
#define PEVENT_CANCELED		0x4000		/* event canceled or done */
#define PEVENT_ENQUEUED		0x2000		/* in the ctx->events queue */
#define PEVENT_GOT_MUTEX	0x1000		/* user mutex acquired */

#define PEVENT_USER_FLAGS	(PEVENT_RECURRING | PEVENT_OWN_THREAD)

#define READABLE_EVENTS		(POLLIN | POLLRDNORM | POLLERR \
				    | POLLHUP | POLLNVAL)
#define WRITABLE_EVENTS		(POLLOUT | POLLWRNORM | POLLWRBAND \
				    | POLLERR | POLLHUP | POLLNVAL)

/* Event context */
struct pevent_ctx {
	u_int32_t		magic;		/* magic number */
	pthread_mutex_t		mutex;		/* mutex for context */
#if PDEL_DEBUG
	int			mutex_count;	/* mutex count */
#endif
	pthread_attr_t		attr;		/* event thread attributes */
	pthread_t		thread;		/* event thread */
	TAILQ_HEAD(, pevent)	events;		/* pending event list */
	u_int			nevents;	/* length of 'events' list */
	u_int			nrwevents;	/* number read/write events */
	struct pollfd		*fds;		/* poll(2) fds array */
	u_int			fds_alloc;	/* allocated size of 'fds' */
	const char		*mtype;		/* typed_mem(3) memory type */
	char			mtype_buf[TYPED_MEM_TYPELEN];
	int			pipe[2];	/* event thread notify pipe */
	u_char			notified;	/* data in the pipe */
	u_char			has_attr;	/* 'attr' is valid */
	u_int			refs;		/* references to this context */
};

/* Event object */
struct pevent {
	u_int32_t		magic;		/* magic number */
	struct pevent_ctx	*ctx;		/* pointer to event context */
	struct pevent		**peventp;	/* user handle to this event */
	pevent_handler_t	*handler;	/* event handler function */
	void			*arg;		/* event handler function arg */
	int			flags;		/* event flags */
	int			poll_idx;	/* index in poll(2) fds array */
	pthread_mutex_t		*mutex;		/* user mutex, if any */
#if PDEL_DEBUG
	int			mutex_count;	/* mutex count */
#endif
	enum pevent_type	type;		/* type of this event */
	struct timeval		when;		/* expiration for time events */
	u_int			refs;		/* references to this event */
	union {
		int		fd;		/* file descriptor */
		int		millis;		/* time delay */
		struct mesg_port *port;		/* mesg_port */
	}			u;
	TAILQ_ENTRY(pevent)	next;		/* next in ctx->events */
};

/* Macros */
#define PEVENT_ENQUEUE(ctx, ev)						\
	do {								\
		assert(((ev)->flags & PEVENT_ENQUEUED) == 0);		\
		TAILQ_INSERT_TAIL(&(ctx)->events, (ev), next);		\
		(ev)->flags |= PEVENT_ENQUEUED;				\
		(ctx)->nevents++;					\
		if ((ev)->type == PEVENT_READ				\
		    || (ev)->type == PEVENT_WRITE)			\
			(ctx)->nrwevents++;				\
		DBG(PEVENT, "ev %p refs %d -> %d (enqueued)",		\
		    (ev), (ev)->refs, (ev)->refs + 1);			\
		(ev)->refs++;						\
	} while (0)

#define PEVENT_DEQUEUE(ctx, ev)						\
	do {								\
		assert(((ev)->flags & PEVENT_ENQUEUED) != 0);		\
		TAILQ_REMOVE(&(ctx)->events, (ev), next);		\
		(ctx)->nevents--;					\
		if ((ev)->type == PEVENT_READ				\
		    || (ev)->type == PEVENT_WRITE)			\
			(ctx)->nrwevents--;				\
		(ev)->flags &= ~PEVENT_ENQUEUED;			\
		_pevent_unref(ev);					\
	} while (0)

#define PEVENT_SET_OCCURRED(ctx, ev)					\
	do {								\
		(ev)->flags |= PEVENT_OCCURRED;				\
		if ((ev) != TAILQ_FIRST(&ctx->events)) {		\
			TAILQ_REMOVE(&(ctx)->events, (ev), next);	\
			TAILQ_INSERT_HEAD(&(ctx)->events, (ev), next);	\
		}							\
	} while (0)

/* Internal functions */
static void	pevent_ctx_service(struct pevent *ev);
static void	*pevent_ctx_main(void *arg);
static void	pevent_ctx_main_cleanup(void *arg);
static void	*pevent_ctx_execute(void *arg);
static void	pevent_ctx_execute_cleanup(void *arg);
static void	pevent_ctx_notify(struct pevent_ctx *ctx);
static void	pevent_ctx_unref(struct pevent_ctx *ctx);
static void	pevent_cancel(struct pevent *ev);

/* Internal variables */
static char	pevent_byte;

/*
 * Create a new event context.
 */
struct pevent_ctx *
pevent_ctx_create(const char *mtype, const pthread_attr_t *attr)
{
	pthread_mutexattr_t mutexattr;
	struct pevent_ctx *ctx;
	int got_mutexattr = 0;
	int got_mutex = 0;

	/* Create context object */
	if ((ctx = MALLOC(mtype, sizeof(*ctx))) == NULL)
		return (NULL);
	memset(ctx, 0, sizeof(*ctx));
	if (mtype != NULL) {
		strlcpy(ctx->mtype_buf, mtype, sizeof(ctx->mtype_buf));
		ctx->mtype = ctx->mtype_buf;
	}
	TAILQ_INIT(&ctx->events);

	/* Copy thread attributes */
	if (attr != NULL) {
		struct sched_param param;
		int value;

		if ((errno = pthread_attr_init(&ctx->attr)) != 0)
			goto fail;
		ctx->has_attr = 1;
		pthread_attr_getinheritsched(attr, &value);
		pthread_attr_setinheritsched(&ctx->attr, value);
		pthread_attr_getschedparam(attr, &param);
		pthread_attr_setschedparam(&ctx->attr, &param);
		pthread_attr_getschedpolicy(attr, &value);
		pthread_attr_setschedpolicy(&ctx->attr, value);
		pthread_attr_getscope(attr, &value);
		pthread_attr_setscope(&ctx->attr, value);
	}

	/* Initialize mutex */
	if ((errno = pthread_mutexattr_init(&mutexattr) != 0))
		goto fail;
	got_mutexattr = 1;
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	if ((errno = pthread_mutex_init(&ctx->mutex, &mutexattr)) != 0)
		goto fail;
	got_mutex = 1;

	/* Initialize notify pipe */
	if (pipe(ctx->pipe) == -1)
		goto fail;

	/* Finish up */
	pthread_mutexattr_destroy(&mutexattr);
	ctx->magic = PEVENT_CTX_MAGIC;
	ctx->refs = 1;
	DBG(PEVENT, "created ctx %p", ctx);
	return (ctx);

fail:
	/* Clean up after failure */
	if (got_mutex)
		pthread_mutex_destroy(&ctx->mutex);
	if (got_mutexattr)
		pthread_mutexattr_destroy(&mutexattr);
	if (ctx->has_attr)
		pthread_attr_destroy(&ctx->attr);
	FREE(mtype, ctx);
	return (NULL);
}

/*
 * Destroy an event context.
 *
 * All events are unregistered.
 */
void
pevent_ctx_destroy(struct pevent_ctx **ctxp)
{
	struct pevent_ctx *const ctx = *ctxp;

	/* Allow NULL context */
	if (ctx == NULL)
		return;
	*ctxp = NULL;
	DBG(PEVENT, "destroying ctx %p", ctx);

	/* Lock context */
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);

	/* Sanity check */
	assert(ctx->magic == PEVENT_CTX_MAGIC);

	/* Unregister all events */
	while (!TAILQ_EMPTY(&ctx->events))
		pevent_unregister(TAILQ_FIRST(&ctx->events)->peventp);

	/* Decrement context reference count and unlock context */
	pevent_ctx_unref(ctx);
}

/*
 * Return the number of registered events.
 */
u_int
pevent_ctx_count(struct pevent_ctx *ctx)
{
	u_int nevents;

	assert(ctx->magic == PEVENT_CTX_MAGIC);
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);
	nevents = ctx->nevents;
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
	return (nevents);
}

/*
 * Create a new schedule item.
 */
int
pevent_register(struct pevent_ctx *ctx, struct pevent **peventp, int flags,
	pthread_mutex_t *mutex, pevent_handler_t *handler, void *arg,
	enum pevent_type type, ...)
{
	struct pevent *ev;
	va_list args;

	/* Sanity check */
	if (*peventp != NULL) {
		errno = EBUSY;
		return (-1);
	}

	/* Check flags */
	if ((flags & ~PEVENT_USER_FLAGS) != 0) {
		errno = EINVAL;
		return (-1);
	}

	/* Create new event */
	if ((ev = MALLOC(ctx->mtype, sizeof(*ev))) == NULL)
		return (-1);
	memset(ev, 0, sizeof(*ev));
	ev->magic = PEVENT_MAGIC;
	ev->ctx = ctx;
	ev->handler = handler;
	ev->arg = arg;
	ev->flags = flags;
	ev->poll_idx = -1;
	ev->mutex = mutex;
	ev->type = type;
	ev->refs = 1;				/* the caller's reference */
	DBG(PEVENT, "new ev %p in ctx %p", ev, ctx);

	/* Add type-specific info */
	switch (type) {
	case PEVENT_READ:
	case PEVENT_WRITE:
		va_start(args, type);
		ev->u.fd = va_arg(args, int);
		va_end(args);
		break;
	case PEVENT_TIME:
		va_start(args, type);
		ev->u.millis = va_arg(args, int);
		va_end(args);
		if (ev->u.millis < 0)
			ev->u.millis = 0;
		gettimeofday(&ev->when, NULL);
		ev->when.tv_sec += ev->u.millis / 1000;
		ev->when.tv_usec += (ev->u.millis % 1000) * 1000;
		if (ev->when.tv_usec > 1000000) {
			ev->when.tv_sec++;
			ev->when.tv_usec -= 1000000;
		}
		break;
	case PEVENT_MESG_PORT:
		va_start(args, type);
		ev->u.port = va_arg(args, struct mesg_port *);
		va_end(args);
		if (ev->u.port == NULL)
			goto invalid;
		break;
	case PEVENT_USER:
		break;
	default:
	invalid:
		errno = EINVAL;
		_pevent_unref(ev);
		return (-1);
	}

	/* Lock context */
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);

	/* Link to related object (if appropriate) */
	switch (ev->type) {
	case PEVENT_MESG_PORT:
		if (_mesg_port_set_event(ev->u.port, ev) == -1) {
			MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
			_pevent_unref(ev);
			return (-1);
		}
		DBG(PEVENT, "ev %p refs %d -> %d (mesg port)",
		    ev, ev->refs, ev->refs + 1);
		ev->refs++;			/* the mesg_port's reference */
		break;
	default:
		break;
	}

	/* Start or notify event thread */
	if (ctx->thread == 0) {
		if ((errno = pthread_create(&ctx->thread,
		    ctx->has_attr ? &ctx->attr : NULL,
		    pevent_ctx_main, ctx)) != 0) {
			ctx->thread = 0;
			alogf(LOG_ERR, "%s: %m", "pthread_create");
			ev->flags |= PEVENT_CANCELED;
			MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
			_pevent_unref(ev);
			return (-1);
		}
		pthread_detach(ctx->thread);
		DBG(PEVENT, "created tid %p for ctx %p, refs -> %d",
		    ctx->thread, ctx, ctx->refs + 1);
		ctx->refs++;			/* add reference for thread */
	} else
		pevent_ctx_notify(ctx);

	/* Caller gets the one reference */
	ev->peventp = peventp;
	*peventp = ev;

	/* Add event to the pending event list */
	PEVENT_ENQUEUE(ctx, ev);

	/* Unlock context */
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);

	return (0);
}

/*
 * Unregister an event.
 */
void
pevent_unregister(struct pevent **peventp)
{
	struct pevent *const ev = *peventp;
	struct pevent_ctx *ctx;

	/* Allow NULL event */
	if (ev == NULL)
		return;
	ctx = ev->ctx;
	DBG(PEVENT, "unregister ev %p in ctx %p", ev, ctx);

	/* Sanity checks */
	assert(ev->magic == PEVENT_MAGIC);
	assert(ctx->magic == PEVENT_CTX_MAGIC);
	assert(ev->peventp == peventp);

	/* Lock context */
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);
	DBG(PEVENT, "unregister ev %p in ctx %p: lock", ev, ctx);

	/* Dequeue event if queued */
	if ((ev->flags & PEVENT_ENQUEUED) != 0) {
		PEVENT_DEQUEUE(ctx, ev);
		pevent_ctx_notify(ctx);			/* wakeup thread */
	}

	/* Cancel the event */
	pevent_cancel(ev);

	/* Unlock context */
	DBG(PEVENT, "unregister ev %p in ctx %p: unlock", ev, ctx);
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
}

/*
 * Trigger an event.
 */
void
pevent_trigger(struct pevent *ev)
{
	struct pevent_ctx *ctx;

	/* Sanity check */
	if (ev == NULL)
		return;

	/* Get context */
	ctx = ev->ctx;
	DBG(PEVENT, "trigger ev %p in ctx %p", ev, ctx);
	assert(ev->magic == PEVENT_MAGIC);

	/* Lock context */
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);

	/* If event already canceled, bail */
	if ((ev->flags & PEVENT_CANCELED) != 0)
		goto done;

	/* Mark event as having occurred */
	PEVENT_SET_OCCURRED(ctx, ev);

	/* Wake up thread if event is still in the queue */
	if ((ev->flags & PEVENT_ENQUEUED) != 0)
		pevent_ctx_notify(ctx);

done:
	/* Unlock context */
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
}

/*
 * Get event info.
 */
int
pevent_get_info(struct pevent *ev, struct pevent_info *info)
{
	if (ev == NULL) {
		errno = ENXIO;
		return (-1);
	}
	assert(ev->magic == PEVENT_MAGIC);
	memset(info, 0, sizeof(*info));
	info->type = ev->type;
	switch (ev->type) {
	case PEVENT_READ:
	case PEVENT_WRITE:
		info->u.fd = ev->u.fd;
		break;
	case PEVENT_TIME:
		info->u.millis = ev->u.millis;
		break;
	case PEVENT_MESG_PORT:
		info->u.port = ev->u.port;
		break;
	case PEVENT_USER:
		break;
	default:
		errno = EINVAL;
		return (-1);
	}
	return (0);
}

/*
 * Event thread entry point.
 */
static void *
pevent_ctx_main(void *arg)
{
	struct pevent_ctx *const ctx = arg;
	struct timeval now;
	struct pollfd *fd;
	struct pevent *ev;
	struct pevent *next_ev;
	int poll_idx;
	int timeout;
	int r;

	/* Push cleanup hook */
	pthread_cleanup_push(pevent_ctx_main_cleanup, ctx);
	assert(ctx->magic == PEVENT_CTX_MAGIC);

	/* Lock context */
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);

	/* Safety belts */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	/* Get current time */
	gettimeofday(&now, NULL);
	DBG(PEVENT, "ctx %p thread starting", ctx);

loop:
	/* Are there any events left? */
	if (TAILQ_EMPTY(&ctx->events)) {
		DBG(PEVENT, "ctx %p thread exiting", ctx);
		goto done;
	}

	/* Make sure ctx->fds array is long enough */
	if (ctx->fds_alloc < 1 + ctx->nrwevents) {
		const u_int new_alloc = roundup(1 + ctx->nrwevents, 16);
		void *mem;

		if ((mem = REALLOC(ctx->mtype, ctx->fds,
		    new_alloc * sizeof(*ctx->fds))) == NULL)
			alogf(LOG_ERR, "%s: %m", "realloc");
		else {
			ctx->fds = mem;
			ctx->fds_alloc = new_alloc;
		}
	}

	/* If we were intentionally woken up, read the wakeup byte */
	if (ctx->notified) {
		DBG(PEVENT, "ctx %p thread was notified", ctx);
		(void)read(ctx->pipe[0], &pevent_byte, 1);
		ctx->notified = 0;
	}

	/* Add event for the notify pipe */
	poll_idx = 0;
	if (ctx->fds_alloc > 0) {
		fd = &ctx->fds[poll_idx++];
		memset(fd, 0, sizeof(*fd));
		fd->fd = ctx->pipe[0];
		fd->events = POLLRDNORM;
	}

	/* Fill in rest of poll() array */
	timeout = INFTIM;
	TAILQ_FOREACH(ev, &ctx->events, next) {
		switch (ev->type) {
		case PEVENT_READ:
		case PEVENT_WRITE:
		    {
			if (poll_idx >= ctx->fds_alloc) {
				ev->poll_idx = -1;
				break;
			}
			ev->poll_idx = poll_idx++;
			fd = &ctx->fds[ev->poll_idx];
			memset(fd, 0, sizeof(*fd));
			fd->fd = ev->u.fd;
			fd->events = (ev->type == PEVENT_READ) ?
			    POLLRDNORM : POLLWRNORM;
			break;
		    }
		case PEVENT_TIME:
		    {
			struct timeval remain;
			int millis;

			/* Compute milliseconds until event */
			if (timercmp(&ev->when, &now, <=))
				millis = 0;
			else {
				timersub(&ev->when, &now, &remain);
				millis = remain.tv_sec * 1000;
				millis += remain.tv_usec / 1000;
			}

			/* Remember the minimum delay */
			if (timeout == INFTIM || millis < timeout)
				timeout = millis;
			break;
		    }
		default:
			break;
		}
	}

	/* Mark non-poll() events that have occurred */
	TAILQ_FOREACH(ev, &ctx->events, next) {
		assert(ev->magic == PEVENT_MAGIC);
		switch (ev->type) {
		case PEVENT_MESG_PORT:
			if (mesg_port_qlen(ev->u.port) > 0)
				PEVENT_SET_OCCURRED(ctx, ev);
			break;
		default:
			break;
		}
		if ((ev->flags & PEVENT_OCCURRED) != 0)
			timeout = 0;			/* don't delay */
	}

#if PDEL_DEBUG
	/* Debugging */
	DBG(PEVENT, "ctx %p thread event list:", ctx);
	TAILQ_FOREACH(ev, &ctx->events, next)
		DBG(PEVENT, "\tev %p", ev);
#endif

	/* Wait for something to happen */
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
	DBG(PEVENT, "ctx %p thread sleeping", ctx);
	r = poll(ctx->fds, poll_idx, timeout);
	DBG(PEVENT, "ctx %p thread woke up", ctx);
	assert(ctx->magic == PEVENT_CTX_MAGIC);
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);

	/* Check for errors */
	if (r == -1 && errno != EINTR) {
		alogf(LOG_CRIT, "%s: %m", "poll");
		assert(0);
	}

	/* Update current time */
	gettimeofday(&now, NULL);

	/* Mark poll() events that have occurred */
	for (ev = TAILQ_FIRST((&ctx->events)); ev != NULL; ev = next_ev) {
		next_ev = TAILQ_NEXT(ev, next);
		assert(ev->magic == PEVENT_MAGIC);
		switch (ev->type) {
		case PEVENT_READ:
		case PEVENT_WRITE:
			if (ev->poll_idx == -1)
				break;
			fd = &ctx->fds[ev->poll_idx];
			if ((fd->revents & ((ev->type == PEVENT_READ) ?
			    READABLE_EVENTS : WRITABLE_EVENTS)) != 0)
				PEVENT_SET_OCCURRED(ctx, ev);
			break;
		case PEVENT_TIME:
			if (timercmp(&ev->when, &now, <=))
				PEVENT_SET_OCCURRED(ctx, ev);
			break;
		default:
			break;
		}
	}

	/* Service all events that are marked as having occurred */
	while (1) {

		/* Find next event that needs service */
		ev = TAILQ_FIRST(&ctx->events);
		if (ev == NULL || (ev->flags & PEVENT_OCCURRED) == 0)
			break;
		DBG(PEVENT, "ctx %p thread servicing ev %p", ctx, ev);

		/* Detach event and service it while keeping a reference */
		DBG(PEVENT, "ev %p refs %d -> %d (for servicing)",
		    ev, ev->refs, ev->refs + 1);
		ev->refs++;
		PEVENT_DEQUEUE(ctx, ev);
		pevent_ctx_service(ev);
	}

	/* Spin again */
	DBG(PEVENT, "ctx %p thread spin again", ctx);
	goto loop;

done:;
	/* Done */
	pthread_cleanup_pop(1);
	return (NULL);
}

/*
 * Cleanup routine for event thread.
 */
static void
pevent_ctx_main_cleanup(void *arg)
{
	struct pevent_ctx *const ctx = arg;

	DBG(PEVENT, "ctx %p thread cleanup", ctx);
	ctx->thread = 0;
	pevent_ctx_unref(ctx);		/* release thread's reference */
}

/*
 * Service an event.
 *
 * This is called with an extra reference on the event which is
 * removed by pevent_ctx_execute().
 */
static void
pevent_ctx_service(struct pevent *ev)
{
	struct pevent_ctx *const ctx = ev->ctx;
	pthread_t tid;

	/* Does handler want to run in its own thread? */
	if ((ev->flags & PEVENT_OWN_THREAD) != 0) {
		if ((errno = pthread_create(&tid,
		    NULL, pevent_ctx_execute, ev)) != 0) {
			alogf(LOG_ERR,
			    "can't spawn thread for event %p: %m", ev);
			pevent_cancel(ev);	/* removes the reference */
			return;
		}
		pthread_detach(tid);
		DBG(PEVENT, "dispatched thread %p for ev %p", tid, ev);
		return;
	}

	/* Execute handler with context unlocked */
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
	pevent_ctx_execute(ev);
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);
}

/*
 * Execute an event handler.
 *
 * Grab the user mutex, if any, check if event was canceled,
 * execute handler, release mutex.
 *
 * This is called with an extra reference on the event, which is
 * removed by this function.
 *
 * This should be NOT be called with a lock on the event context.
 */
static void *
pevent_ctx_execute(void *arg)
{
	struct pevent *const ev = arg;
	struct pevent_ctx *const ctx = ev->ctx;
	int r;

	/* Sanity check */
	assert(ev->magic == PEVENT_MAGIC);

	/* Register cleanup hook */
	DBG(PEVENT, "ev %p being executed", ev);
	pthread_cleanup_push(pevent_ctx_execute_cleanup, ev);

try_again:
	/* Acquire context mutex */
	DBG(PEVENT, "locking ctx %p for ev %p", ctx, ev);
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);

	/* Check for cancellation to close the cancel/timeout race */
	if ((ev->flags & PEVENT_CANCELED) != 0) {
		DBG(PEVENT, "ev %p was canceled", ev);
		MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
		goto done;
	}

	/*
	 * Acquire user mutex but avoid two potential problems:
	 *
	 * - Race window: other thread could call timer_cancel()
	 *   and destroy the user mutex just before we try to
	 *   acquire it; we must hold context lock to prevent this.
	 *
	 * - Lock reversal deadlock: other threads acquire the
	 *   user lock before the context lock, we do the reverse.
	 *
	 * Once we've acquire both locks, we drop the context lock.
	 */
	if (ev->mutex != NULL) {
		MUTEX_TRYLOCK(ev->mutex, ev->mutex_count);
		if (errno != 0) {
			if (errno != EBUSY) {
				alogf(LOG_ERR,
				    "can't lock mutex for event %p: %m", ev);
				pevent_cancel(ev);
				MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
				goto done;
			}
			MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
			sched_yield();
			goto try_again;
		}
		ev->flags |= PEVENT_GOT_MUTEX;
	}

	/* Remove user's event reference (we still have one though) */
	pevent_cancel(ev);

	/* Release context mutex */
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);

	/* Reschedule a new event if recurring */
	if ((ev->flags & PEVENT_RECURRING) != 0) {
		switch (ev->type) {
		case PEVENT_READ:
		case PEVENT_WRITE:
			r = pevent_register(ev->ctx, ev->peventp,
			    (ev->flags & PEVENT_USER_FLAGS), ev->mutex,
			    ev->handler, ev->arg, ev->type, ev->u.fd);
			break;
		case PEVENT_TIME:
			r = pevent_register(ev->ctx, ev->peventp,
			    (ev->flags & PEVENT_USER_FLAGS), ev->mutex,
			    ev->handler, ev->arg, ev->type, ev->u.millis);
			break;
		case PEVENT_MESG_PORT:
			r = pevent_register(ev->ctx, ev->peventp,
			    (ev->flags & PEVENT_USER_FLAGS), ev->mutex,
			    ev->handler, ev->arg, ev->type, ev->u.port);
			break;
		case PEVENT_USER:
			r = pevent_register(ev->ctx, ev->peventp,
			    (ev->flags & PEVENT_USER_FLAGS), ev->mutex,
			    ev->handler, ev->arg, ev->type);
			break;
		default:
			r = -1;
			assert(0);
		}
		if (r == -1) {
			alogf(LOG_ERR,
			    "can't re-register recurring event %p: %m", ev);
		}
	}

	/* Execute the handler */
	(*ev->handler)(ev->arg);

done:;
	/* Release user mutex and event reference */
	pthread_cleanup_pop(1);
	return (NULL);
}

/*
 * Clean up for pevent_ctx_execute()
 */
static void
pevent_ctx_execute_cleanup(void *arg)
{
	struct pevent *const ev = arg;

	DBG(PEVENT, "ev %p cleanup", ev);
	assert(ev->magic == PEVENT_MAGIC);
	if ((ev->flags & PEVENT_GOT_MUTEX) != 0)
		MUTEX_UNLOCK(ev->mutex, ev->mutex_count);
	_pevent_unref(ev);
}

/*
 * Wakeup event thread because the event list has changed.
 *
 * This assumes the mutex is locked.
 */
static void
pevent_ctx_notify(struct pevent_ctx *ctx)
{
	DBG(PEVENT, "ctx %p being notified", ctx);
	if (!ctx->notified) {
		(void)write(ctx->pipe[1], &pevent_byte, 1);
		ctx->notified = 1;
	}
}

/*
 * Cancel an event (make it so that it never gets triggered) and
 * remove the user reference to it.
 *
 * This assumes the lock is held.
 */
static void
pevent_cancel(struct pevent *ev)
{
	DBG(PEVENT, "canceling ev %p", ev);
	assert((ev->flags & PEVENT_CANCELED) == 0);
	ev->flags |= PEVENT_CANCELED;
	*ev->peventp = NULL;
	_pevent_unref(ev);
}

/*
 * Returns true if the event has been canceled.
 */
int
_pevent_canceled(struct pevent *ev)
{
	assert(ev->magic == PEVENT_MAGIC);
	return ((ev->flags & PEVENT_CANCELED) != 0);
}

/*
 * Drop a reference on an event.
 */
void
_pevent_unref(struct pevent *ev)
{
	struct pevent_ctx *const ctx = ev->ctx;

	DBG(PEVENT, "ev %p refs %d -> %d", ev, ev->refs, ev->refs - 1);
	assert(ev->refs > 0);
	assert(ev->magic == PEVENT_MAGIC);
	MUTEX_LOCK(&ctx->mutex, ctx->mutex_count);
	if (--ev->refs > 0) {
		MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
		return;
	}
	assert((ev->flags & PEVENT_ENQUEUED) == 0);
	ev->magic = ~0;				/* invalidate magic number */
	DBG(PEVENT, "freeing ev %p", ev);
	FREE(ctx->mtype, ev);
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
}

/*
 * Release context lock and drop a context reference.
 *
 * This assumes the mutex is locked. Upon return it will be unlocked.
 */
static void
pevent_ctx_unref(struct pevent_ctx *ctx)
{
	DBG(PEVENT, "ctx %p refs %d -> %d", ctx, ctx->refs, ctx->refs - 1);
	assert(ctx->refs > 0);
	assert(ctx->magic == PEVENT_CTX_MAGIC);
	if (--ctx->refs > 0) {
		MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
		return;
	}
	assert(TAILQ_EMPTY(&ctx->events));
	assert(ctx->nevents == 0);
	assert(ctx->thread == 0);
	(void)close(ctx->pipe[0]);
	(void)close(ctx->pipe[1]);
	MUTEX_UNLOCK(&ctx->mutex, ctx->mutex_count);
	pthread_mutex_destroy(&ctx->mutex);
	if (ctx->has_attr)
		pthread_attr_destroy(&ctx->attr);
	ctx->magic = ~0;			/* invalidate magic number */
	DBG(PEVENT, "freeing ctx %p", ctx);
	FREE(ctx->mtype, ctx->fds);
	FREE(ctx->mtype, ctx);
}

