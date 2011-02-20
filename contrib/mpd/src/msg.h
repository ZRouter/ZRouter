
/*
 * msg.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _MSG_H_
#define _MSG_H_

/*
 * DEFINITIONS
 */

/* Messages you can send to a link or a bundle */

  #define MSG_OPEN		1	/* Bring yourself up */
  #define MSG_CLOSE		2	/* Bring yourself down */
  #define MSG_UP		3	/* Lower layer went up */
  #define MSG_DOWN		4	/* Lower layer went down */
  #define MSG_SHUTDOWN		5	/* Object should disappear */

/* Forward decl */

  struct msghandler
  {
    void	(*func)(int type, void *arg);
    const char	*dbg;
  };

  typedef struct msghandler	MsgHandler;

/*
 * FUNCTIONS
 */

#define	MsgRegister(m, func)	\
	    MsgRegister2(m, func, #func "()")
  extern void		MsgRegister2(MsgHandler *m, void (*func)(int typ, void *arg), const char *dbg);
  extern void		MsgUnRegister(MsgHandler *m);
  extern void		MsgSend(MsgHandler *m, int type, void *arg);
  extern const char	*MsgName(int msg);

#endif

