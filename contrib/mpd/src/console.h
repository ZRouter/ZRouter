
/*
 * console.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _CONSOLE_H_
#define	_CONSOLE_H_

#include "ppp.h"

/*
 * DEFINITIONS
 */

  #define MAX_CONSOLE_ARGS	50
  #define MAX_CONSOLE_LINE	400
  #define MAX_CONSOLE_HIST	10

  #define Printf(fmt, args...)	do { 						\
	  			  if (ctx->cs)	 				\
	  			    ctx->cs->write(ctx->cs, fmt, ## args);	\
  				} while (0)

  #define Error(fmt, args...)	do { 						\
				  snprintf(ctx->errmsg, sizeof(ctx->errmsg),	\
				    fmt, ## args);				\
				  return(CMD_ERR_OTHER);			\
  				} while (0)

  /* Configuration options */
  enum {
    CONSOLE_LOGGING	/* enable logging */
  };

  struct console {
    int			fd;		/* listener */
    struct u_addr 	addr;
    in_port_t		port;
    SLIST_HEAD(, console_session) sessions;	/* active sessions */
    EventRef		event;		/* connect-event */
    pthread_rwlock_t	lock;
  };

  typedef struct console *Console;

  struct console_session;

  typedef struct console_session *ConsoleSession;

  struct context {
	Bund		bund;
	Link		lnk;
	Rep		rep;
	ConsoleSession	cs;
	int		depth;		/* Number recursive 'load' calls */
	char		errmsg[256];	/* Error message of the last command */
	int		priv;
  };

  struct console_user {
	char		username[32];
	char		password[32];
	int		priv;
  };

  typedef struct console_user *ConsoleUser;

  struct console_session {
    Console		console;
    struct optinfo	options;	/* Configured options */
    int			fd;		/* connection fd */
    void		*cookie;	/* device dependent cookie */
    EventRef		readEvent;
    struct context	context;
    struct console_user user;
    struct u_addr	peer_addr;
    in_port_t           peer_port;
    void		(*prompt)(struct console_session *);
    void		(*write)(struct console_session *, const char *fmt, ...);
    void		(*writev)(struct console_session *, const char *fmt, va_list vl);
    void		(*close)(struct console_session *);
    u_char		state;
    u_char		telnet;
    u_char		escaped;
    u_char		exit;
    int			cmd_len;
    char		cmd[MAX_CONSOLE_LINE];
    int			currhist;
    char		history[MAX_CONSOLE_HIST][MAX_CONSOLE_LINE];	/* last command */
    SLIST_ENTRY(console_session)	next;
  };

/*
 * VARIABLES
 */

  extern const struct cmdtab ConsoleSetCmds[];
  extern struct ghash		*gUsers;		/* allowed users */
  extern pthread_rwlock_t	gUsersLock;


/*
 * FUNCTIONS
 */

  extern int	ConsoleInit(Console c);
  extern int	ConsoleOpen(Console c);
  extern int	ConsoleClose(Console c);
  extern int	ConsoleStat(Context ctx, int ac, char *av[], void *arg);
  extern Context	StdConsoleConnect(Console c);
  extern void	ConsoleShutdown(Console c);

  extern int	UserCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	UserStat(Context ctx, int ac, char *av[], void *arg);

#endif

