
/*
 * lcp.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _LCP_H_
#define _LCP_H_

#include "fsm.h"
#include "timer.h"
#include "auth.h"

/*
 * DEFINITIONS
 */

  /* MRU defs */
  #define LCP_DEFAULT_MRU	1500	/* Per RFC 1661 */
  #define LCP_MIN_MRU		296

  #define LCP_NUM_AUTH_PROTOS	5

  struct lcpauthproto {
    ushort		proto;
    u_char		alg;
    u_char		conf;
  };
  typedef struct lcpauthproto	*LcpAuthProto;

    enum lcp_phase {
	PHASE_DEAD = 0,
	PHASE_ESTABLISH,
	PHASE_AUTHENTICATE,
	PHASE_NETWORK,
	PHASE_TERMINATE
    };

  /* Link state */
  struct lcpstate {

    /* LCP phase of this link */
    enum lcp_phase	phase;		/* PPP phase */

    /* Authorization info */
    struct auth	auth;			/* Used during authorization phase */

    /* Peers negotiated parameters */
    LcpAuthProto	peer_protos[LCP_NUM_AUTH_PROTOS];	/* list of acceptable auth-protos */
    u_int32_t	peer_accmap;		/* Characters peer needs escaped */
    u_int32_t	peer_magic;		/* Peer's magic number */
    u_int16_t	peer_mru;		/* Peer's max reception packet size */
    u_int16_t	peer_auth;		/* Auth requested by peer, or zero */
    u_int16_t	peer_mrru;		/* MRRU set by peer, or zero */
    u_char	peer_alg;		/* Peer's CHAP algorithm */

    /* My negotiated parameters */
    u_char	want_alg;		/* My CHAP algorithm */
    LcpAuthProto	want_protos[LCP_NUM_AUTH_PROTOS];	/* list of requestable auth-protos */
    u_int32_t	want_accmap;		/* Control chars I want escaped */
    u_int32_t	want_magic;		/* My magic number */
    u_int16_t	want_mru;		/* My MRU */
    u_int16_t	want_auth;		/* Auth I require of peer, or zero */
    u_int16_t	want_mrru;		/* My MRRU, or zero if no MP */

    /* More params */
    u_char	want_protocomp:1;	/* I want protocol compression */
    u_char	want_acfcomp:1;		/* I want a&c field compression */
    u_char	want_shortseq:1;	/* I want short seq numbers */
    u_char	want_callback:1;	/* I want to be called back */

    u_char	peer_protocomp:1;	/* Peer wants protocol field comp */
    u_char	peer_acfcomp:1;		/* Peer wants addr & ctrl field comp */
    u_char	peer_shortseq:1;	/* Peer gets ML short seq numbers */

    /* Misc */
    struct discrim	peer_discrim;	/* Peer's discriminator */
    char	peer_ident[64];		/* Peer's LCP ident string */
    uint32_t	peer_reject;		/* Request codes rejected by peer */
    struct fsm	fsm;			/* Finite state machine */
  };
  typedef struct lcpstate	*LcpState;

  #define TY_VENDOR		0	/* Vendor specific */
  #define TY_MRU		1	/* Maximum-Receive-Unit */
  #define TY_ACCMAP		2	/* Async-Control-Character-Map */
  #define TY_AUTHPROTO		3	/* Authentication-Protocol */
  #define TY_QUALPROTO		4	/* Quality-Protocol */
  #define TY_MAGICNUM		5	/* Magic-Number */
  #define TY_RESERVED		6	/* RESERVED */
  #define TY_PROTOCOMP		7	/* Protocol-Field-Compression */
  #define TY_ACFCOMP		8	/* Address+Control-Field-Compression */
  #define TY_FCSALT		9	/* FCS-Alternatives */
  #define TY_SDP		10	/* Self-Dscribing-Padding */
  #define TY_NUMMODE		11	/* Numbered-Mode */
  #define TY_MULTILINK		12	/* Multi-link procedure (?) */
  #define TY_CALLBACK		13	/* Callback */
  #define TY_CONNECTTIME	14	/* Connect time */
  #define TY_COMPFRAME		15	/* Compound-Frames */
  #define TY_NDS		16	/* Nominal-Data-Encapsulation */
  #define TY_MRRU		17	/* Multi-link MRRU size */
  #define TY_SHORTSEQNUM	18	/* Short seq number header format */
  #define TY_ENDPOINTDISC	19	/* Unique endpoint discrimiator */
  #define TY_PROPRIETARY	20	/* Proprietary */
  #define TY_DCEIDENTIFIER	21	/* DCE-Identifier */
  #define TY_MULTILINKPLUS	22	/* Multi-Link-Plus-Procedure */
  #define TY_BACP		23	/* Link Discriminator for BACP RFC2125 */
  #define TY_LCPAUTHOPT		24	/* LCP-Authentication-Option */
  #define TY_COBS		25	/* Consistent Overhead Byte Stuffing (COBS) */
  #define TY_PREFIXELISION	26	/* Prefix elision */
  #define TY_MULTILINKHEADERFMT	27	/* Multilink header format */
  #define TY_INTERNAT		28	/* Internationalization */
  #define TY_SDATALINKSONET	29	/* Simple Data Link on SONET/SDH */

/*
 * FUNCTIONS
 */

  extern void	LcpInit(Link l);
  extern void	LcpInst(Link l, Link lt);
  extern void	LcpShutdown(Link l);
  extern void	LcpInput(Link l, Mbuf bp);
  extern void	LcpUp(Link l);
  extern void	LcpOpen(Link l);
  extern void	LcpClose(Link l);
  extern void	LcpDown(Link l);
  extern int	LcpStat(Context ctx, int ac, char *av[], void *arg);
  extern void	LcpAuthResult(Link l, int success);

#endif

