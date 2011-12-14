
/*
 * timer.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"

/*
 * INTERNAL FUNCTIONS
 */

  static void	TimerExpires(int type, void *cookie);

/*
 * TimerInit()
 */

void
TimerInit2(PppTimer timer, const char *desc,
  int load, void (*handler)(void *), void *arg, const char *dbg)
{
  memset(timer, 0, sizeof(*timer));
  timer->load	= (load >= 0) ? load : 0;
  timer->func	= handler;
  timer->arg	= arg;
  timer->desc	= desc;
  timer->dbg	= dbg;
}

/*
 * TimerStart()
 */

void
TimerStart2(PppTimer timer, const char *file, int line)
{
    /* Stop timer if running */
    assert(timer->func);
    if (EventIsRegistered(&timer->event))
	EventUnRegister(&timer->event);

    Log(LG_EVENTS, ("EVENT: Starting timer \"%s\" %s() for %d ms at %s:%d",
	timer->desc, timer->dbg, timer->load, file, line));
    /* Register timeout event */
    EventRegister(&timer->event, EVENT_TIMEOUT,
	timer->load, 0, TimerExpires, timer);
}

/*
 * TimerStartRecurring()
 */

void
TimerStartRecurring2(PppTimer timer, const char *file, int line)
{
    /* Stop timer if running */
    assert(timer->func);
    Log(LG_EVENTS, ("EVENT: Starting recurring timer \"%s\" %s() for %d ms at %s:%d",
	timer->desc, timer->dbg, timer->load, file, line));
    if (EventIsRegistered(&timer->event))
	EventUnRegister(&timer->event);

    /* Register timeout event */
    EventRegister(&timer->event, EVENT_TIMEOUT,
	timer->load, EVENT_RECURRING, TimerExpires, timer);
}

/*
 * TimerStop()
 */

void
TimerStop2(PppTimer timer, const char *file, int line)
{
    /* Stop timer if running */
    Log(LG_EVENTS, ("EVENT: Stopping timer \"%s\" %s() at %s:%d",
	timer->desc, timer->dbg, file, line));
    if (EventIsRegistered(&timer->event))
	EventUnRegister(&timer->event);
}

/*
 * TimerExpires()
 */

static void
TimerExpires(int type, void *cookie)
{
    PppTimer	const timer = (PppTimer) cookie;
    const char	*desc = timer->desc;
    const char	*dbg = timer->dbg;

    Log(LG_EVENTS, ("EVENT: Processing timer \"%s\" %s()", desc, dbg));
    (*timer->func)(timer->arg);
    Log(LG_EVENTS, ("EVENT: Processing timer \"%s\" %s() done", desc, dbg));
}

/*
 * TimerRemain()
 *
 * Return number of ticks left on a timer, or -1 if not running.
 */

int
TimerRemain(PppTimer t)
{
  return(EventTimerRemain(&t->event));
}

/*
 * TimerStarted()
 *
 * Return Timer status.
 */

int
TimerStarted(PppTimer t)
{
  return (EventIsRegistered(&t->event));
}

