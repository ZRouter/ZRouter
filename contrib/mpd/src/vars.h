
/*
 * vars.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _VARS_H_
#define	_VARS_H_

#include <sys/types.h>

/*
 * DEFINITIONS
 */

/* Describes one option */

  struct confinfo
  {
    u_char	peered;		/* Is accept/deny applicable? */
    u_char	option;		/* Option index (0 <= value < 16) */
    const char	*name;		/* Textual name; NULL ends list */
  };
  typedef const struct confinfo	*ConfInfo;

/* Generic option configuration structure */

  struct optinfo
  {
    u_int32_t	enable;		/* Options I want */
    u_int32_t	accept;		/* Options I'll allow */
  };
  typedef struct optinfo	*Options;

  #define Enable(c,x)		((c)->enable |= (1<<(x)))
  #define Disable(c,x)		((c)->enable &= ~(1<<(x)))
  #define Accept(c,x)		((c)->accept |= (1<<(x)))
  #define Deny(c,x)		((c)->accept &= ~(1<<(x)))

  #define Enabled(c,x)		(((c)->enable & (1<<(x))) != 0)
  #define Acceptable(c,x)	(((c)->accept & (1<<(x))) != 0)

/*
 * FUNCTIONS
 */

  extern void	AcceptCommand(int ac, char *av[], Options opt, ConfInfo conf);
  extern void	DenyCommand(int ac, char *av[], Options opt, ConfInfo conf);
  extern void	EnableCommand(int ac, char *av[], Options opt, ConfInfo conf);
  extern void	DisableCommand(int ac, char *av[], Options opt, ConfInfo conf);
  extern void	YesCommand(int ac, char *av[], Options opt, ConfInfo conf);
  extern void	NoCommand(int ac, char *av[], Options opt, ConfInfo conf);
  extern void	OptStat(Context ctx, Options c, ConfInfo conf);

#endif

