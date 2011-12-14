
/*
 * bund.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _BUND_H_
#define _BUND_H_

#include "defs.h"
#include "ip.h"
#include "mp.h"
#include "ipcp.h"
#include "ipv6cp.h"
#include "chap.h"
#include "ccp.h"
#include "ecp.h"
#include "msg.h"
#include "auth.h"
#include "command.h"
#include <netgraph/ng_message.h>

/*
 * DEFINITIONS
 */

  /* Configuration options */
  enum {
    BUND_CONF_IPCP,		/* IPCP */
    BUND_CONF_IPV6CP,		/* IPV6CP */
    BUND_CONF_COMPRESSION,	/* compression */
    BUND_CONF_ENCRYPTION,	/* encryption */
    BUND_CONF_CRYPT_REQD,	/* encryption is required */
    BUND_CONF_BWMANAGE,		/* dynamic bandwidth */
    BUND_CONF_ROUNDROBIN	/* round-robin MP scheduling */
  };

  /* Default bundle-layer FSM retry timeout */
  #define BUND_DEFAULT_RETRY	2

  enum {
    NCP_NONE = 0,
    NCP_IPCP,
    NCP_IPV6CP,
    NCP_ECP,
    NCP_CCP
  };

/*

  Bundle bandwidth management

  We treat the first link as different from the rest. It connects
  immediately when there is (qualifying) outgoing traffic. The
  idle timeout applies globally, no matter how many links are up.

  Additional links are connected/disconnected according to a simple
  algorithm that uses the following constants:

  S	Sampling interval. Number of seconds over which we average traffic.

  N	Number of sub-intervals we chop the S seconds into (granularity). 

  Hi	Hi water mark: if traffic is more than H% of total available
	bandwidth, averaged over S seconds, time to add the second link.

  Lo	Low water mark: if traffic is less than L% of total available
	bandwidth during all N sub-intervals, time to hang up the second link.

  Mc	Minimum amount of time after connecting a link before
	connecting next.

  Md	Minimum amount of time after disconnecting any link before
	disconnecting next.

  We treat incoming and outgoing traffic separately when comparing
  against Hi and Lo.

*/

  #define BUND_BM_DFL_S		60	/* Length of sampling interval (secs) */
  #define BUND_BM_DFL_Hi	80	/* High water mark % */
  #define BUND_BM_DFL_Lo	20	/* Low water mark % */
  #define BUND_BM_DFL_Mc	30	/* Min connect period (secs) */
  #define BUND_BM_DFL_Md	90	/* Min disconnect period (secs) */

  #define BUND_BM_N	6		/* Number of sampling intervals */

  struct bundbm {
    u_int		traffic[2][BUND_BM_N];	/* Traffic deltas */
    u_int		avail[BUND_BM_N];	/* Available traffic deltas */
    u_char		wasUp[BUND_BM_N];	/* Sub-intervals link was up */
    time_t		last_open;	/* Time we last open any link */
    time_t		last_close;	/* Time we last closed any link */
    struct pppTimer	bmTimer;	/* Bandwidth mgmt timer */
    u_int		total_bw;	/* Total bandwidth available */
  };
  typedef struct bundbm	*BundBm;

  /* Configuration for a bundle */
  struct bundconf {
    short		retry_timeout;		/* Timeout for retries */
    u_short		bm_S;			/* Bandwidth mgmt constants */
    u_short		bm_Hi;
    u_short		bm_Lo;
    u_short		bm_Mc;
    u_short		bm_Md;
    struct optinfo	options;		/* Configured options */
    char		linkst[NG_PPP_MAX_LINKS][LINK_MAX_NAME]; /* Link names for DoD */
  };

  #define BUND_STATS_UPDATE_INTERVAL    65 * SECONDS

  /* Total state of a bundle */
  struct bundle {
    char		name[LINK_MAX_NAME];	/* Name of this bundle */
    int			id;			/* Index of this bundle in gBundles */
    u_char		tmpl;			/* This is template, not an instance */
    u_char		stay;			/* Must not disappear */
    u_char		dead;			/* Dead flag */
    Link		links[NG_PPP_MAX_LINKS];	/* Real links in this bundle */
    u_short		n_links;		/* Number of links in bundle */
    u_short		n_up;			/* Number of links joined the bundle */
    ng_ID_t		nodeID;			/* ID of ppp node */
    char		hook[NG_HOOKSIZ];	/* session hook name */
    MsgHandler		msgs;			/* Bundle events */
    int			refs;			/* Number of references */

    /* PPP node config */
    struct ng_ppp_node_conf	pppConfig;

    /* Data chunks */
    char		msession_id[AUTH_MAX_SESSIONID]; /* a uniq session-id */    
    u_int16_t		peer_mrru;	/* MRRU set by peer, or zero */
    struct discrim	peer_discrim;	/* Peer's discriminator */
    struct bundbm	bm;		/* Bandwidth management state */
    struct bundconf	conf;		/* Configuration for this bundle */
    struct ng_ppp_link_stat64	stats;	/* Statistics for this bundle */
#ifndef NG_PPP_STATS64
    struct ng_ppp_link_stat oldStats;	/* Previous stats for 64bit emulation */
    struct pppTimer     statsUpdateTimer;       /* update Timer */
#endif
    time_t		last_up;	/* Time first link got up */
    struct ifacestate	iface;		/* IP state info */
    struct ipcpstate	ipcp;		/* IPCP state info */
    struct ipv6cpstate	ipv6cp;		/* IPV6CP state info */
    struct ccpstate	ccp;		/* CCP state info */
    struct ecpstate	ecp;		/* ECP state info */
    u_int		ncpstarted;	/* Bitmask of active NCPs wich is sufficient to keep bundle open */

    /* Link management stuff */
    struct pppTimer	bmTimer;		/* Bandwidth mgmt timer */
    struct pppTimer	reOpenTimer;		/* Re-open timer */

    /* Boolean variables */
    u_char		open;		/* In the open state */
    u_char		originate;	/* Who originated the connection */
    
    struct authparams   params;         /* params to pass to from auth backend */
  };
  
/*
 * VARIABLES
 */

  extern struct discrim		self_discrim;	/* My discriminator */
  extern const struct cmdtab	BundSetCmds[];

/*
 * FUNCTIONS
 */

  extern void	BundOpen(Bund b);
  extern void	BundClose(Bund b);
  extern int	BundOpenCmd(Context ctx);
  extern int	BundCloseCmd(Context ctx);
  extern int	BundStat(Context ctx, int ac, char *av[], void *arg);
  extern void	BundUpdateParams(Bund b);
  extern int	BundCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	MSessionCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	IfaceCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	BundCreate(Context ctx, int ac, char *av[], void *arg);
  extern int	BundDestroy(Context ctx, int ac, char *av[], void *arg);
  extern Bund	BundInst(Bund bt, char *name, int tmpl, int stay);
  extern Bund	BundFind(const char *name);
  extern void	BundShutdown(Bund b);
  extern void   BundUpdateStats(Bund b);
  extern void	BundUpdateStatsTimer(void *cookie);
  extern void	BundResetStats(Bund b);

  extern int	BundJoin(Link l);
  extern void	BundLeave(Link l);
  extern void	BundNcpsJoin(Bund b, int proto);
  extern void	BundNcpsLeave(Bund b, int proto);
  extern void	BundNcpsStart(Bund b, int proto);
  extern void	BundNcpsFinish(Bund b, int proto);
  extern void	BundOpenLinks(Bund b);
  extern void	BundCloseLinks(Bund b);
  extern int	BundCreateOpenLink(Bund b, int n);
  extern void	BundOpenLink(Link l);

  extern void	BundNcpsOpen(Bund b);
  extern void	BundNcpsClose(Bund b);

  extern void	BundShowLinks(Context ctx, Bund sb);

#endif

