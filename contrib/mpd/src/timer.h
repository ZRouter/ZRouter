
/*
 * timer.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _TIMER_H_
#define	_TIMER_H_

#include "defs.h"
#include "event.h"

/*
 * DEFINITIONS
 */

  #define TICKSPERSEC	1000		/* Microsecond granularity */
  #define SECONDS	TICKSPERSEC	/* Timers count in usec */

  struct pppTimer;
  typedef struct pppTimer *PppTimer;

  struct pppTimer
  {
    EventRef	event;			/* Event registration */
    u_int	load;			/* Initial load value */
    void	(*func)(void *arg);	/* Called when timer expires */
    void	*arg;			/* Arg passed to timeout function */
    const char	*desc;
    const char	*dbg;
  };

/*
 * FUNCTIONS
 */

#define	TimerInit(timer, desc, load, handler, arg)				\
	    TimerInit2(timer, desc, load, handler, arg, #handler)
  extern void	TimerInit2(PppTimer timer, const char *desc,
		  int load, void (*handler)(void *), void *arg, const char *dbg);
#define	TimerStart(t)	\
	    TimerStart2(t, __FILE__, __LINE__)
  extern void	TimerStart2(PppTimer t, const char *file, int line);
#define	TimerStartRecurring(t)	\
	    TimerStartRecurring2(t, __FILE__, __LINE__)
  extern void	TimerStartRecurring2(PppTimer t, const char *file, int line);
#define	TimerStop(t)	\
	    TimerStop2(t, __FILE__, __LINE__)
  extern void	TimerStop2(PppTimer t, const char *file, int line);
  extern int	TimerRemain(PppTimer t);
  extern int	TimerStarted(PppTimer t);

#endif

