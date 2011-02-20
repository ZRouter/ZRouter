
/*
 * modem.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _MODEM_H_
#define	_MODEM_H_

#include "command.h"
#include "phys.h"

/*
 * VARIABLES
 */

  extern const struct cmdtab	ModemSetCmds[];
  extern const struct phystype	gModemPhysType;

/*
 * FUNCTIONS
 */

  extern char		*ModemGetVar(Link l, const char *name);

#endif

