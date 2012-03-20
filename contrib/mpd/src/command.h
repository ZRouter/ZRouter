
/*
 * command.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "vars.h"

/*
 * DEFINITIONS
 */

  #define CMD_SUBMENU	((int (*)(Context ctx, int ac, char *av[], void *arg)) 1)
  
  #define CMD_ERR_USAGE	-1
  #define CMD_ERR_UNDEF	-2
  #define CMD_ERR_AMBIG	-3
  #define CMD_ERR_RECUR	-4
  #define CMD_ERR_UNFIN	-5
  #define CMD_ERR_NOCTX	-6
  #define CMD_ERR_OTHER	-7

  /* Configuration options */
  enum {
#ifdef USE_WRAP
    GLOBAL_CONF_TCPWRAPPER,	/* enable tcp-wrapper */
#endif
    GLOBAL_CONF_ONESHOT		/* enable OneShot mode */
  };

  struct globalconf {
    struct optinfo	options;
  };

  struct cmdtab;
  typedef const struct cmdtab	*CmdTab;
  struct cmdtab
  {
    const char	*name;
    const char	*desc;
    int		(*func)(Context ctx, int ac, char *av[], void *arg);
    int		(*admit)(Context ctx, CmdTab cmd);
    int		priv;
    void	*arg;
  };

  extern const struct cmdtab gCommands[];

/*
 * FUNCTIONS
 */

  extern int	DoConsole(void);
  extern int	DoCommand(Context ctx, int ac, char *av[], const char *file, int line);
  extern int	DoCommandTab(Context ctx, CmdTab cmdlist, int ac, char *av[]);
  extern int	HelpCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	FindCommand(Context ctx, CmdTab cmds, char* str, CmdTab *cp);
  extern int	AdmitBund(Context ctx, CmdTab cmd);
  extern int	AdmitLink(Context ctx, CmdTab cmd);
  extern int	AdmitRep(Context ctx, CmdTab cmd);
  extern int	AdmitPhys(Context ctx, CmdTab cmd);
  extern int	AdmitDev(Context ctx, CmdTab cmd);

#endif

