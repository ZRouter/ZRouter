
/*
 * ipcp.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _IPCP_H_
#define _IPCP_H_

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include "command.h"
#include "phys.h"
#include "fsm.h"
#include "timer.h"
#include "vars.h"

/*
 * DEFINITONS
 */
 
   /* Configuration options */
  enum {
    IPCP_CONF_VJCOMP,
    IPCP_CONF_REQPRIDNS,
    IPCP_CONF_REQSECDNS,
    IPCP_CONF_REQPRINBNS,
    IPCP_CONF_REQSECNBNS,
    IPCP_CONF_PRETENDIP
  };


  struct ipcpvjcomp {
    u_short	proto;			/* Protocol (only VJCOMP supported) */
    u_char	maxchan;		/* Number of compression slots - 1 */
    u_char	compcid;		/* Whether conn-id is compressible */
  };

  struct ipcpconf {
    struct optinfo	options;	/* Configuraion options */
    struct u_range	self_allow;	/* My allowed IP addresses */
    struct u_range	peer_allow;	/* His allowed IP addresses */
    char		self_ippool[LINK_MAX_NAME];
    char		ippool[LINK_MAX_NAME];
    struct in_addr	peer_dns[2];	/* DNS servers for peer to use */
    struct in_addr	peer_nbns[2];	/* NBNS servers for peer to use */
  };
  typedef struct ipcpconf	*IpcpConf;

  struct ipcpstate {
    struct ipcpconf	conf;		/* Configuration */

    struct in_addr	want_addr;	/* IP address I'm willing to use */
    struct in_addr	peer_addr;	/* IP address he is willing to use */

    struct u_range	self_allow;	/* My allowed IP addresses */
    struct u_range	peer_allow;	/* His allowed IP addresses */

    u_char		self_ippool_used;
    u_char		ippool_used;
#ifdef USE_NG_VJC
    struct ipcpvjcomp	peer_comp;	/* Peer's IP compression config */
    struct ipcpvjcomp	want_comp;	/* My IP compression config */
#endif
    struct in_addr	want_dns[2];	/* DNS servers we got from peer */
    struct in_addr	want_nbns[2];	/* NBNS servers we got from peer */

    uint32_t		peer_reject;	/* Request codes rejected by peer */

    struct fsm		fsm;
  };
  typedef struct ipcpstate	*IpcpState;

/*
 * VARIABLES
 */

  extern const struct cmdtab	IpcpSetCmds[];

/*
 * FUNCTIONS
 */

  extern void	IpcpInit(Bund b);
  extern void	IpcpInst(Bund b, Bund bt);
  extern void	IpcpUp(Bund b);
  extern void	IpcpDown(Bund b);
  extern void	IpcpOpen(Bund b);
  extern void	IpcpClose(Bund b);
  extern int	IpcpOpenCmd(Context ctx);
  extern int	IpcpCloseCmd(Context ctx);
  extern void	IpcpInput(Bund b, Mbuf bp);
  extern void	IpcpDefAddress(void);
  extern int	IpcpStat(Context ctx, int ac, char *av[], void *arg);

#endif


