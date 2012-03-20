
/*
 * ccp.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _CCP_H_
#define	_CCP_H_

#include "defs.h"
#include "fsm.h"
#include "mbuf.h"
#include "comp.h"
#include "console.h"
#include "command.h"
#include "log.h"

#ifdef CCP_PRED1
#include "ccp_pred1.h"
#endif
#ifdef CCP_DEFLATE
#include "ccp_deflate.h"
#endif
#ifdef CCP_MPPC
#include "ccp_mppc.h"
#endif

#include <netgraph/ng_message.h>

/*
 * DEFINITIONS
 */

  /* Compression types */
  #define CCP_TY_OUI		0	/* OUI */
  #define CCP_TY_PRED1		1	/* Predictor type 1 */
  #define CCP_TY_PRED2		2	/* Predictor type 2 */
  #define CCP_TY_PUDDLE		3	/* Puddle Jumper */
  #define CCP_TY_HWPPC		16	/* Hewlett-Packard PPC */
  #define CCP_TY_STAC		17	/* Stac Electronics LZS */
  #define CCP_TY_MPPC		18	/* Microsoft PPC */
  #define CCP_TY_GAND		19	/* Gandalf FZA */
  #define CCP_TY_V42BIS		20	/* V.42bis compression */
  #define CCP_TY_BSD		21	/* BSD LZW Compress */
  #define CCP_TY_LZS_DCP	23	/* LZS-DCP Compression Protocol */
  #define CCP_TY_DEFLATE24	24	/* Gzip "deflate" compression WRONG! */
  #define CCP_TY_DCE		25	/* Data Compression in Data Circuit-Terminating Equipment */
  #define CCP_TY_DEFLATE	26	/* Gzip "deflate" compression */
  #define CCP_TY_V44		27	/* V.44/LZJH Compression Protocol */

  #define CCP_OVERHEAD		2

  /* CCP state */
  struct ccpstate {
    struct fsm		fsm;		/* CCP FSM */
    struct optinfo	options;	/* configured protocols */
    CompType		xmit;		/* xmit protocol */
    CompType		recv;		/* recv protocol */
    uint32_t		self_reject;	/* types rejected by me */
    uint32_t		peer_reject;	/* types rejected by peer */
#ifdef CCP_PRED1
    struct pred1info	pred1;		/* Predictor-1 state */
#endif
#ifdef CCP_DEFLATE
    struct deflateinfo	deflate;	/* Deflate state */
#endif
#ifdef CCP_MPPC
    struct mppcinfo	mppc;		/* MPPC/MPPE state */
#endif
    ng_ID_t		comp_node_id;	/* compressor node id */
    ng_ID_t		decomp_node_id;	/* decompressor node id */
    uint32_t		recv_resets;	/* Number of ResetReq we have got from other side */
    uint32_t		xmit_resets;	/* Number of ResetReq we have sent to other side */
    u_char		crypt_check;	/* We checked for required encryption */
  };
  typedef struct ccpstate	*CcpState;

  #define CCP_PEER_REJECTED(p,x)	((p)->peer_reject & (1<<(x)))
  #define CCP_SELF_REJECTED(p,x)	((p)->self_reject & (1<<(x)))

  #define CCP_PEER_REJ(p,x)	do{(p)->peer_reject |= (1<<(x));}while(0)
  #define CCP_SELF_REJ(p,x)	do{(p)->self_reject |= (1<<(x));}while(0)

/*
 * VARIABLES
 */

  extern const struct cmdtab	CcpSetCmds[];

  extern int		gCcpCsock;		/* Socket node control socket */
  extern int		gCcpDsock;		/* Socket node data socket */

/*
 * FUNCTIONS
 */

  extern int	CcpsInit(void);
  extern void	CcpsShutdown(void);

  extern void	CcpInit(Bund b);
  extern void	CcpInst(Bund b, Bund bt);
  extern void	CcpUp(Bund b);
  extern void	CcpDown(Bund b);
  extern void	CcpOpen(Bund b);
  extern void	CcpClose(Bund b);
  extern int	CcpOpenCmd(Context ctx);
  extern int	CcpCloseCmd(Context ctx);
  extern void	CcpInput(Bund b, Mbuf bp);
  extern Mbuf	CcpDataInput(Bund b, Mbuf bp);
  extern Mbuf	CcpDataOutput(Bund b, Mbuf bp);
  extern int	CcpSubtractBloat(Bund b, int size);
  extern void	CcpSendResetReq(Bund b);
  extern void	CcpRecvMsg(Bund b, struct ng_mesg *msg, int len);
  extern int	CcpStat(Context ctx, int ac, char *av[], void *arg);

#endif

