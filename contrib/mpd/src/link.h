
/*
 * link.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _LINK_H_
#define _LINK_H_

#include "defs.h"
#include "proto.h"
#include "lcp.h"
#include "ip.h"
#include "mp.h"
#include "vars.h"
#include "auth.h"
#include "fsm.h"
#include "mbuf.h"
#include "phys.h"
#include "vars.h"
#include <netgraph/ng_ppp.h>
#include <regex.h>

/*
 * DEFINITIONS
 */

  /* Bounds */
  /* Default bundle-layer FSM retry timeout */
  #define LINK_DEFAULT_RETRY	2

  /* Default latency and bandwidth */
  #define LINK_DEFAULT_BANDWIDTH	64000		/* 64k */
  #define LINK_DEFAULT_LATENCY		2000		/* 2ms */

  enum {
    LINK_ACTION_FORWARD,
    LINK_ACTION_BUNDLE,
    LINK_ACTION_DROP
  };

  struct linkaction {
    int 		action;
    char		regex[128];
    regex_t		regexp;
    char		arg[LINK_MAX_NAME];	/* Link/Bundle template name */
    SLIST_ENTRY(linkaction) next;
  };

  /* Configuration options */
  enum {
    LINK_CONF_PAP,
    LINK_CONF_CHAPMD5,
    LINK_CONF_CHAPMSv1,
    LINK_CONF_CHAPMSv2,
    LINK_CONF_EAP,
    LINK_CONF_INCOMING,
    LINK_CONF_ACFCOMP,
    LINK_CONF_PROTOCOMP,
    LINK_CONF_MSDOMAIN,
    LINK_CONF_MAGICNUM,
    LINK_CONF_PASSIVE,
    LINK_CONF_CHECK_MAGIC,
    LINK_CONF_RINGBACK,
    LINK_CONF_NO_ORIG_AUTH,
    LINK_CONF_CALLBACK,
    LINK_CONF_MULTILINK,	/* multi-link */
    LINK_CONF_SHORTSEQ,		/* multi-link short sequence numbers */
    LINK_CONF_TIMEREMAIN,	/* Send LCP Time-Remain if known */
    LINK_CONF_PEER_AS_CALLING,
    LINK_CONF_REPORT_MAC
  };

  /* Configuration for a link */
  struct linkconf {
    u_short		mtu;		/* Initial MTU value */
    u_short		mru;		/* Initial MRU value */
    uint16_t		mrru;		/* Initial MRRU value */
    uint32_t		accmap;		/* Initial ACCMAP value */
    short		retry_timeout;	/* FSM timeout for retries */
    short		max_redial;	/* Max failed connect attempts */
    short		redial_delay;	/* Failed connect retry time */
    char		*ident;		/* LCP ident string */
    struct optinfo	options;	/* Configured options */
    int			max_children;	/* Maximal number of children */
  };

  struct linkbm {
    struct ng_ppp_link_stat
		idleStats;		/* Link management stats */
  };
  typedef struct linkbm	*LinkBm;

  /* Values for link origination (must fit in 2 bits) */
  #define LINK_ORIGINATE_UNKNOWN	0
  #define LINK_ORIGINATE_LOCAL		1
  #define LINK_ORIGINATE_REMOTE		2

  #define LINK_ORIGINATION(o)	((o) == LINK_ORIGINATE_LOCAL ? "local" :    \
  				 (o) == LINK_ORIGINATE_REMOTE ? "remote" :  \
				 "unknown")

  /* Total state of a link */
  struct linkst {
    char		name[LINK_MAX_NAME];	/* Human readable name */
    int			id;			/* Index of this link in gLinks */
    u_char		tmpl;			/* This is template, not an instance */
    u_char		stay;			/* Must not disappear */
    u_char		state;			/* Device current state */
    u_char		joined_bund;		/* Link successfully joined bundle */
    u_char		originate;		/* Who originated the connection */
    u_char		die;			/* LCP agreed to die */
    u_char		dead;			/* Dead flag (shutted down) */
    Bund		bund;			/* My bundle */
    Rep			rep;			/* Rep connected to the device */
    int			bundleIndex;		/* Link number in bundle */
    int			parent;			/* Index of the parent in gLinks */
    int			children;		/* Number of children */
    int			refs;			/* Number of references */
    char		hook[NG_HOOKSIZ];	/* session hook name */
    ng_ID_t		nodeID;			/* ID of the tee node */
    MsgHandler		msgs;			/* Link events */
    SLIST_HEAD(, linkaction) actions;

    /* State info */
    struct linkconf	conf;		/* Link configuration */
    struct lcpstate	lcp;		/* LCP state info */
    struct linkbm	bm;		/* Link bandwidth mgmt info */
    struct ng_ppp_link_stat64	stats;	/* Link statistics */
#ifndef NG_PPP_STATS64
    struct ng_ppp_link_stat oldStats;	/* Previous stats for 64bit emulation */
#endif

    /* Link properties */
    short		num_redial;	/* Counter for retry attempts */
    u_char		upReasonValid;
    u_char		downReasonValid;
    char		*upReason;	/* Reason for link going up */
    char		*downReason;	/* Reason for link going down */
    int			bandwidth;	/* Bandwidth in bits per second */
    int			latency;	/* Latency in microseconds */
    time_t		last_up;	/* Time this link last got up */
    char		msession_id[AUTH_MAX_SESSIONID]; /* a uniq msession-id */
    char		session_id[AUTH_MAX_SESSIONID];	/* a uniq session-id */

    PhysType		type;			/* Device type descriptor */
    void		*info;			/* Type specific info */
    MsgHandler		pmsgs;			/* Message channel */
    struct pppTimer	openTimer;		/* Open retry timer */
  };

  
/*
 * VARIABLES
 */

  extern const struct cmdtab	LinkSetCmds[];

  extern int		gLinksCsock;		/* Socket node control socket */
  extern int		gLinksDsock;		/* Socket node data socket */

/*
 * FUNCTIONS
 */
  extern int	LinksInit(void);
  extern void	LinksShutdown(void);

  extern void	LinkUp(Link l);
  extern void	LinkDown(Link l);
  extern void	LinkOpen(Link l);
  extern void	LinkClose(Link l);
  extern int	LinkOpenCmd(Context ctx);
  extern int	LinkCloseCmd(Context ctx);

  extern int	LinkCreate(Context ctx, int ac, char *av[], void *arg);
  extern int	LinkDestroy(Context ctx, int ac, char *av[], void *arg);
  extern Link	LinkInst(Link lt, char *name, int tmpl, int stay);
  extern void	LinkShutdownCheck(Link l, short state);
  extern void	LinkShutdown(Link l);
  extern int	LinkNgInit(Link l);
  extern int	LinkNgJoin(Link l);
  extern int	LinkNgToRep(Link l);
  extern int	LinkNgLeave(Link l);
  extern void	LinkNgShutdown(Link l);
  extern int	LinkNuke(Link link);
  extern int	LinkStat(Context ctx, int ac, char *av[], void *arg);
  extern void	LinkUpdateStats(Link l);
  extern void	LinkResetStats(Link l);
  extern Link	LinkFind(const char *name);
  extern int	LinkCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	SessionCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	AuthnameCommand(Context ctx, int ac, char *av[], void *arg);
  extern void	RecordLinkUpDownReason(Bund b, Link l, int up, const char *fmt,
			  const char *arg, ...);

  extern const char	*LinkMatchAction(Link l, int stage, char *login);

#endif

