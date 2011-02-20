
/*
 * l2tp.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _L2TP_H
#define	_L2TP_H_

#include "command.h"
#include "phys.h"
#include "ip.h"

/*
 * VARIABLES
 */

  extern const struct cmdtab	L2tpSetCmds[];
  extern const struct phystype	gL2tpPhysType;

  extern int	L2tpsStat(Context ctx, int ac, char *av[], void *arg);

#endif

