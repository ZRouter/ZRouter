/* $Id: schedule.h,v 1.7 2008/02/06 05:49:39 mk Exp $ */
/*	$KAME: schedule.h,v 1.12 2001/03/06 20:41:02 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "var.h"

/* scheduling table */
/* the head is the nearest event. */
struct sched {
	time_t xtime;		/* event time which is as time(3). */
				/*
				 * if defined FIXY2038PROBLEM, this time
				 * is from the time when called sched_init().
				 */
	void (*func) (void *);	/* call this function when timeout. */
	void *param;		/* pointer to parameter */

	int dead;		/* dead or alive */
	long id;		/* for debug */
	time_t created;		/* for debug */
	time_t tick;		/* for debug */

	TAILQ_ENTRY(sched) chain;
};

/* cancel schedule */
#define SCHED_KILL(s)                                                          \
do {                                                                           \
	sched_kill(s);                                                         \
	s = NULL;                                                              \
} while(0)

/* must be called after it's called from scheduler. */
#define SCHED_INIT(s)	(s) = NULL

struct scheddump {
	time_t xtime;
	long id;
	time_t created;
	time_t tick;
};

struct timeval *scheduler (void);
struct sched *sched_new (time_t, void (*func) (void *), void *);
void sched_kill (struct sched *);
int sched_dump (caddr_t *, int *);
void sched_init (void);
void sched_scrub_param (void *);
