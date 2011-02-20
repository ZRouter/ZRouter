
/*
 * ccp_pred1.h
 *
 * Rewritten by Alexander Motin <mav@FreeBSD.org>
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _PRED_H_
#define _PRED_H_

#include "defs.h"
#include "mbuf.h"
#include "comp.h"

#ifdef USE_NG_PRED1
#include <netgraph/ng_pred1.h>
#endif

/*
 * DEFINITIONS
 */

  #define PRED1_TABLE_SIZE	0x10000

#ifndef USE_NG_PRED1
  struct pred1_stats {
	uint64_t	FramesPlain;
	uint64_t	FramesComp;
	uint64_t	FramesUncomp;
	uint64_t	InOctets;
	uint64_t	OutOctets;
	uint64_t	Errors;
  };
  typedef struct pred1_stats	*Pred1Stats;
#endif

  struct pred1info
  {
#ifndef USE_NG_PRED1
    u_short	iHash;
    u_short	oHash;
    u_char	*InputGuessTable;
    u_char	*OutputGuessTable;
    struct pred1_stats	recv_stats;
    struct pred1_stats	xmit_stats;
#endif
  };
  typedef struct pred1info	*Pred1Info;

/*
 * VARIABLES
 */

  extern const struct comptype	gCompPred1Info;

#endif

