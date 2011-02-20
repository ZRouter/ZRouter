/*
 * See ``COPYRIGHT.mpd''
 *
 * $Id: event.c,v 1.19 2008/03/11 14:26:55 amotin Exp $
 *
 */

  #include "ppp.h"
  #include "event.h"

/*
 * DEFINITIONS
 */

  struct pevent_ctx	*gPeventCtx = NULL;

/*
 * INTERNAL FUNCTIONS
 */

  static void		EventHandler(void *arg);

/*
 * EventInit()
 *
 */

int
EventInit(void)
{

  gPeventCtx = pevent_ctx_create(MB_EVENT, NULL);
  if (!gPeventCtx) {
    Log(LG_ERR, ("%s: error pevent_ctx_create: %d", __FUNCTION__, errno));
    return(-1);
  }

  return(0);
}

/*
 * EventStop()
 *
 * Stop servicing events
 */

void
EventStop(void)
{
  pevent_ctx_destroy(&gPeventCtx);
}

/*
 * EventDump()
 */

void
EventDump(Context ctx, const char *msg)
{
  u_int	n;

  n = pevent_ctx_count(gPeventCtx);
  Printf("%d Events registered\n", n);
}

/*
 * EventRegister()
 */

int
EventRegister2(EventRef *refp, int type, int val, int flags,
	void (*action)(int type, void *cookie), void *cookie, const char *dbg,
	const char *file, int line)
{
    Log(LG_EVENTS, ("EVENT: Registering event %s at %s:%d", dbg, file, line));
    if (!gPeventCtx)
	EventInit();

    refp->arg = cookie;
    refp->handler = action;
    refp->type = type;
    refp->pe = NULL;
    refp->dbg = dbg;

    if (pevent_register(gPeventCtx, &refp->pe, flags, &gGiantMutex, EventHandler,
	    refp, type, val) == -1) {
        Log(LG_ERR, ("%s: error pevent_register: %s", __FUNCTION__, strerror(errno)));
        return(-1);
    }
  
    Log(LG_EVENTS, ("EVENT: Registering event %s done at %s:%d", dbg, file, line));
    return(0);
}

/*
 * EventUnRegister()
 */

int
EventUnRegister2(EventRef *refp, const char *file, int line)
{
    Log(LG_EVENTS, ("EVENT: Unregistering event %s at %s:%d", refp->dbg, file, line));
    pevent_unregister(&refp->pe);
    Log(LG_EVENTS, ("EVENT: Unregistering event %s done at %s:%d", refp->dbg, file, line));
    return(0);
}

/*
 * EventIsRegistered()
 */

int
EventIsRegistered(EventRef *ref)
{
    if (ref->pe == NULL)
	return FALSE;

    return TRUE;
}

/*
 * EventTimerRemain()
 *
 * Returns the number of milliseconds remaining on a timer.
 * Returns -1 if the timer is not registered or is not a timer event.
 */

int
EventTimerRemain(EventRef *refp)
{
    struct pevent_info	info;

    if (pevent_get_info(refp->pe, &info) == -1)
	return(-1);

    return(info.u.millis);
}

static void
EventHandler(void *arg)
{
    EventRef	*refp = (EventRef *) arg;
    const char	*dbg = refp->dbg;

    Log(LG_EVENTS, ("EVENT: Processing event %s", dbg));
    (refp->handler)(refp->type, refp->arg);
    Log(LG_EVENTS, ("EVENT: Processing event %s done", dbg));
}
