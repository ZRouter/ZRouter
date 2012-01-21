
/*
 * ccp_mppc.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _CCP_MPPC_H_
#define _CCP_MPPC_H_

#include "defs.h"

#ifdef CCP_MPPC

#include "mbuf.h"
#include "comp.h"

#include <netgraph/ng_message.h>
#include <netgraph/ng_mppc.h>

/*
 * DEFINITIONS
 */

  struct mppcinfo {
    struct optinfo	options;	/* configured protocols */
    uint32_t	peer_reject;		/* types rejected by peer */
    uint32_t	recv_bits;		/* recv config bits */
    uint32_t	xmit_bits;		/* xmit config bits */
    u_char	xmit_key0[MPPE_KEY_LEN];/* xmit start key */
    u_char	recv_key0[MPPE_KEY_LEN];/* recv start key */
  };
  typedef struct mppcinfo	*MppcInfo;

  #define MPPC_PEER_REJECTED(p,x)	((p)->peer_reject & (1<<(x)))
  #define MPPC_PEER_REJ(p,x)	do{(p)->peer_reject |= (1<<(x));}while(0)

/*
 * VARIABLES
 */

  extern const struct comptype	gCompMppcInfo;
  extern const struct cmdtab    MppcSetCmds[];
  extern int	MPPCPresent;
  extern int	MPPEPresent;

  extern int	MppcStat(Context ctx, int ac, char *av[], void *arg);
  extern int 	MppcTestCap(void);

#endif

#endif

