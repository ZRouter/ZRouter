
/*
 * msg.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "msg.h"

/*
 * DEFINITIONS
 */

/* Which pipe file descriptor is which */

  #define PIPE_READ		0
  #define PIPE_WRITE		1

  struct mpmsg
  {
    int		type;
    void	(*func)(int type, void *arg);
    void	*arg;
    const char	*dbg;
  };
  typedef struct mpmsg	*Msg;

  #define	MSG_QUEUE_LEN	8192
  #define	MSG_QUEUE_MASK	0x1FFF

  struct mpmsg	msgqueue[MSG_QUEUE_LEN];
  int		msgqueueh = 0;
  int		msgqueuet = 0;
  #define	QUEUELEN()	((msgqueueh >= msgqueuet)?	\
	(msgqueueh - msgqueuet):(msgqueueh + MSG_QUEUE_LEN - msgqueuet))

  int           msgpipe[2];
  int		msgpipesent = 0;
  EventRef	msgevent;

/*
 * INTERNAL FUNCTIONS
 */

  static void	MsgEvent(int type, void *cookie);

/*
 * MsgRegister()
 */

void
MsgRegister2(MsgHandler *m, void (*func)(int type, void *arg), const char *dbg)
{
    if ((msgpipe[0]==0) || (msgpipe[1]==0)) {
	if (pipe(msgpipe) < 0) {
	    Perror("%s: Can't create message pipe", 
		__FUNCTION__);
	    DoExit(EX_ERRDEAD);
	}
	fcntl(msgpipe[PIPE_READ], F_SETFD, 1);
	fcntl(msgpipe[PIPE_WRITE], F_SETFD, 1);

	if (fcntl(msgpipe[PIPE_READ], F_SETFL, O_NONBLOCK) < 0)
    	    Perror("%s: fcntl", __FUNCTION__);
	if (fcntl(msgpipe[PIPE_WRITE], F_SETFL, O_NONBLOCK) < 0)
    	    Perror("%s: fcntl", __FUNCTION__);

	if (EventRegister(&msgevent, EVENT_READ,
		msgpipe[PIPE_READ], EVENT_RECURRING, MsgEvent, NULL) < 0) {
	    Perror("%s: Can't register event", __FUNCTION__);
	    DoExit(EX_ERRDEAD);
        }
    }

    m->func = func;
    m->dbg = dbg;
}

/*
 * MsgUnRegister()
 */

void
MsgUnRegister(MsgHandler *m)
{
    m->func = NULL;
    m->dbg = NULL;
}

/*
 * MsgEvent()
 */

static void
MsgEvent(int type, void *cookie)
{
    char	buf[16];
    /* flush signaling pipe */
    msgpipesent = 0;
    while (read(msgpipe[PIPE_READ], buf, sizeof(buf)) == sizeof(buf));

    while (msgqueuet != msgqueueh) {
	Log(LG_EVENTS, ("EVENT: Message %d to %s received",
	    msgqueue[msgqueuet].type, msgqueue[msgqueuet].dbg));
	(*(msgqueue[msgqueuet].func))(msgqueue[msgqueuet].type, msgqueue[msgqueuet].arg);
	Log(LG_EVENTS, ("EVENT: Message %d to %s processed",
	    msgqueue[msgqueuet].type, msgqueue[msgqueuet].dbg));

	msgqueuet = (msgqueuet + 1) & MSG_QUEUE_MASK;
	SETOVERLOAD(QUEUELEN());
    }
}

/*
 * MsgSend()
 */

void
MsgSend(MsgHandler *m, int type, void *arg)
{
    assert(m);
    assert(m->func);

    msgqueue[msgqueueh].type = type;
    msgqueue[msgqueueh].func = m->func;
    msgqueue[msgqueueh].arg = arg;
    msgqueue[msgqueueh].dbg = m->dbg;

    msgqueueh = (msgqueueh + 1) & MSG_QUEUE_MASK;
    if (msgqueuet == msgqueueh) {
        Log(LG_ERR, ("%s: Fatal message queue overflow!", __FUNCTION__));
        DoExit(EX_ERRDEAD);
    }

    SETOVERLOAD(QUEUELEN());

    if (!msgpipesent) {
	char	buf[1] = { 0x2a };
	if (write(msgpipe[PIPE_WRITE], buf, 1) > 0)
	    msgpipesent = 1;
    }
    Log(LG_EVENTS, ("EVENT: Message %d to %s sent", type, m->dbg));
}

/*
 * MsgName()
 */

const char *
MsgName(int msg)
{
  switch (msg)
  {
    case MSG_OPEN:
      return("OPEN");
    case MSG_CLOSE:
      return("CLOSE");
    case MSG_UP:
      return("UP");
    case MSG_DOWN:
      return("DOWN");
    case MSG_SHUTDOWN:
      return("SHUTDOWN");
    default:
      return("???");
  }
}

