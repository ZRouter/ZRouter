
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "util/paction.h"
#include "util/typed_mem.h"

#include "debug/debug.h"

#define	PACTION_MTYPE		"paction"

/* Action structure */
struct paction {
	pthread_t		tid;		/* action thread */
	struct paction		**actionp;	/* user action reference */
	pthread_mutex_t		mutex;		/* action mutex */
	pthread_mutex_t		*umutex;	/* user mutex */
	paction_handler_t	*handler;	/* action handler */
#if PDEL_DEBUG
	int			mutex_count;
	int			umutex_count;
#endif
	paction_finish_t	*finish;	/* action finisher */
	void			*arg;		/* action argument */
	u_char			may_cancel;	/* ok to cancel action thread */
	u_char			canceled;	/* action was canceled */
};

/* Internal functions */
static void	*paction_main(void *arg);
static void	paction_cleanup(void *arg);

/*
 * Start an action.
 */
int
paction_start(struct paction **actionp, pthread_mutex_t *mutex,
	paction_handler_t *handler, paction_finish_t *finish, void *arg)
{
	struct paction *action;

	/* Check if action already in progress */
	if (*actionp != NULL) {
		errno = EBUSY;
		return (-1);
	}

	/* Create new action */
	if ((action = MALLOC(PACTION_MTYPE, sizeof(*action))) == NULL)
		return (-1);
	memset(action, 0, sizeof(*action));
	action->actionp = actionp;
	action->umutex = mutex;
	action->handler = handler;
	action->finish = finish;
	action->arg = arg;

	/* Create mutex */
	if ((errno = pthread_mutex_init(&action->mutex, NULL)) != 0) {
		FREE(PACTION_MTYPE, action);
		return (-1);
	}

	/* Spawn thread */
	if ((errno = pthread_create(&action->tid,
	    NULL, paction_main, action)) != 0) {
		pthread_mutex_destroy(&action->mutex);
		FREE(PACTION_MTYPE, action);
		return (-1);
	}
	pthread_detach(action->tid);

	/* Done */
	*actionp = action;
	return (0);
}

/*
 * Cancel an action.
 */
void
paction_cancel(struct paction **actionp)
{
	struct paction *action = *actionp;

	/* Allow NULL action */
	if (action == NULL)
		return;

	/* Lock action */
	MUTEX_LOCK(&action->mutex, action->mutex_count);

	/* Mark action as canceled; this should only happen once */
	assert(!action->canceled);
	action->canceled = 1;

	/* Invalidate user's reference */
	assert(action->actionp == actionp);
	*action->actionp = NULL;
	action->actionp = NULL;

	/*
	 * Don't cancel the thread before paction_main() starts, because
	 * then paction_cleanup() would never get invoked. Also don't
	 * pthread_cancel() the thread after the handler has completed,
	 * because we might cancel in the middle of the cleanup.
	 */
	if (action->may_cancel)
		pthread_cancel(action->tid);

	/* Unlock action */
	MUTEX_UNLOCK(&action->mutex, action->mutex_count);
}

/*
 * Action thread main entry point.
 */
static void *
paction_main(void *arg)
{
	struct paction *const action = arg;

	/* Cleanup when thread exits */
	pthread_cleanup_push(paction_cleanup, action);

	/* Begin allowing pthread_cancel()'s */
	assert(!action->may_cancel);
	action->may_cancel = 1;

	/* Handle race between paction_cancel() and paction_main() */
	if (action->canceled)			/* race condition ok */
		goto done;

	/* Invoke handler */
	(*action->handler)(action->arg);

done:;
	/* Stop allowing pthread_cancel()'s */
	MUTEX_LOCK(&action->mutex, action->mutex_count);
	action->may_cancel = 0;
	MUTEX_UNLOCK(&action->mutex, action->mutex_count);

	/* Consume any last-minute pthread_cancel() still pending */
	pthread_testcancel();

	/* Done */
	pthread_cleanup_pop(1);
	return (NULL);
}

/*
 * Action thread cleanup
 */
static void
paction_cleanup(void *arg)
{
	struct paction *const action = arg;

	/*
	 * Acquire the action mutex and then the user mutex. We must
	 * do it in this order to avoid referencing the user mutex
	 * after paction_cancel() has been called (because after that
	 * the user mutex may have been destroyed).
	 *
	 * However, because paction_cancel() also acquires the action
	 * mutex and it may be called with the user mutex already held,
	 * there is a possibility for deadlock due to reverse lock ordering.
	 * We avoid this by looping and yielding.
	 */
	while (1) {

		/* Lock the action */
		MUTEX_LOCK(&action->mutex, action->mutex_count);

		/* Check for cancellation */
		if (action->canceled) {
			action->umutex = NULL;
			goto canceled;
		}

		/* Try to lock the user mutex */
		if (action->umutex == NULL)
			break;
		MUTEX_TRYLOCK(action->umutex, action->umutex_count);
		if (errno == 0)
			break;
		assert(errno == EBUSY);

		/* User mutex is busy, so unlock the action and try again */
		MUTEX_UNLOCK(&action->mutex, action->mutex_count);

		/* Let other threads progress */
		usleep(10 * 1000);			/* 10 milliseconds */
	}

	/* Invalidate user reference */
	assert(action->actionp != NULL);
	*action->actionp = NULL;
	action->actionp = NULL;

canceled:
	/* Unlock action */
	MUTEX_UNLOCK(&action->mutex, action->mutex_count);

	/* Invoke finisher */
	(*action->finish)(action->arg, action->canceled);

	/* Release user mutex */
	if (action->umutex != NULL)
		MUTEX_UNLOCK(action->umutex, action->umutex_count);

	/* Destroy action */
	pthread_mutex_destroy(&action->mutex);
	FREE(PACTION_MTYPE, action);
}

