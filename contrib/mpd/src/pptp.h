
/*
 * pptp.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _PPTP_H
#define	_PPTP_H_

#include "command.h"
#include "phys.h"
#include "ip.h"

/*
 * VARIABLES
 */

  extern const struct cmdtab	PptpSetCmds[];
  extern const struct phystype	gPptpPhysType;

  extern int		PptpsStat(Context ctx, int ac, char *av[], void *arg);

#endif

