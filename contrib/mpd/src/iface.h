
/*
 * iface.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _IFACE_H_
#define _IFACE_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if_dl.h>
#include <net/bpf.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netgraph/ng_message.h>
#include <netgraph/ng_ppp.h>
#ifdef USE_NG_BPF
#include <netgraph/ng_bpf.h>
#endif
#include "mbuf.h"
#include "timer.h"
#ifdef	USE_NG_NAT
#include "nat.h"
#endif
#include "vars.h"

/*
 * DEFINITIONS
 */

  #define IFACE_MAX_ROUTES	32
  #define IFACE_MAX_SCRIPT	128

  #define IFACE_IDLE_SPLIT	4
  
  #define IFACE_MIN_MTU		296
  #define IFACE_MAX_MTU		65536

  /*
   * We are in a liberal position about MSS
   * (RFC 879, section 7).
   */
  #define MAXMSS(mtu) (mtu - sizeof(struct ip) - sizeof(struct tcphdr))

/* Configuration options */

  enum {
    IFACE_CONF_ONDEMAND,
    IFACE_CONF_PROXY,
    IFACE_CONF_TCPMSSFIX,
    IFACE_CONF_TEE,
    IFACE_CONF_NAT,
    IFACE_CONF_NETFLOW_IN,
    IFACE_CONF_NETFLOW_OUT,
    IFACE_CONF_NETFLOW_ONCE,
    IFACE_CONF_IPACCT
  };

  /* Dial-on-demand packet cache */
  struct dodcache {
    Mbuf		pkt;
    time_t		ts;
    u_short		proto;
  };

  #define MAX_DOD_CACHE_DELAY	30
  
  struct ifaceconf {
    struct u_range	self_addr;		/* Interface's IP address */
    struct u_addr	peer_addr;		/* Peer's IP address */
    struct u_addr	self_ipv6_addr;
    struct u_addr	peer_ipv6_addr;
    u_char		self_addr_force;
    u_char		peer_addr_force;
    u_char		self_ipv6_addr_force;
    u_char		peer_ipv6_addr_force;
    char		ifname[IFNAMSIZ];	/* Name of my interface */
#ifdef SIOCSIFDESCR
    char		*ifdescr;		/* Interface description*/
#endif
#ifdef SIOCAIFGROUP
    char		ifgroup[IFNAMSIZ];	/* Group of my interface */
#endif
  };

  struct ifaceroute {
    struct u_range	dest;			/* Destination of route */
    u_char		ok;			/* Route installed OK */
    SLIST_ENTRY(ifaceroute)	next;
  };
  typedef struct ifaceroute	*IfaceRoute;
  
  struct ifacestate {
    char		ifname[IFNAMSIZ];	/* Name of my interface */
    char		ngname[IFNAMSIZ];	/* Name of my Netgraph node */
    uint		ifindex;		/* System interface index */
#ifdef SIOCSIFDESCR
    char		*ifdescr;		/* Interface description*/
#endif
    struct ifaceconf	conf;
    u_char		traffic[IFACE_IDLE_SPLIT];	/* Mark any traffic */
    u_short		mtu;			/* Interface MTU */
    u_short		max_mtu;		/* Configured maximum MTU */
    struct optinfo	options;		/* Configuration options */
    u_int		idle_timeout;		/* Idle timeout */
    u_int		session_timeout;	/* Session timeout */
    SLIST_HEAD(, ifaceroute) routes;
#ifdef USE_IPFW
    struct acl 		*tables;		/* List of IP added to tables by iface */
#endif
    struct u_range	self_addr;		/* Interface's IP address */
    struct u_addr	peer_addr;		/* Peer's IP address */
    struct u_addr	proxy_addr;		/* Proxied IP address */
    struct u_addr	self_ipv6_addr;
    struct u_addr	peer_ipv6_addr;
    struct pppTimer	idleTimer;		/* Idle timer */
    struct pppTimer	sessionTimer;		/* Session timer */
    char		up_script[IFACE_MAX_SCRIPT];
    char		down_script[IFACE_MAX_SCRIPT];
#ifdef USE_NG_BPF
    ng_ID_t		limitID;		/* ID of limit (bpf) node */
    SLIST_HEAD(, svcs)	ss[ACL_DIRS];		/* Where to get service stats */
    struct svcstat	prevstats;		/* Stats from gone layers */
#endif
    time_t		last_up;		/* Time this iface last got up */
    u_char		open:1;			/* In an open state */
    u_char		dod:1;			/* Interface flagged -link0 */
    u_char		up:1;			/* interface is up */
    u_char		ip_up:1;		/* IP interface is up */
    u_char		ipv6_up:1;		/* IPv6 interface is up */
    u_char		nat_up:1;		/* NAT is up */
    u_char		tee_up:1;		/* TEE is up */
    u_char		tee6_up:1;		/* TEE6 is up */
    u_char		nfin_up:1;		/* NFIN is up */
    u_char		nfout_up:1;		/* NFOUT is up */
    u_char		mss_up:1;		/* MSS is up */
    u_char		ipacct_up:1;		/* IPACCT is up */
    
    struct dodcache	dodCache;		/* Dial-on-demand cache */
    
#ifdef	USE_NG_NAT
    struct natstate	nat;			/* NAT config */
#endif

    struct ng_ppp_link_stat64	idleStats;	/* Statistics for idle timeout */
  };
  typedef struct ifacestate	*IfaceState;

#ifdef USE_IPFW
  struct acl_pool {			/* Pool of used ACL numbers */
    char		ifname[IFNAMSIZ];     /* Name of interface */
    unsigned short	acl_number;		/* ACL number given by RADIUS unique on this interface */
    unsigned short	real_number;		/* Real ACL number unique on this system */
    struct acl_pool	*next;
  };
#endif

/*
 * VARIABLES
 */

  extern const struct cmdtab	IfaceSetCmds[];

#ifdef USE_IPFW
  extern struct acl_pool * rule_pool; /* Pointer to the first element in the list of rules */
  extern struct acl_pool * pipe_pool; /* Pointer to the first element in the list of pipes */
  extern struct acl_pool * queue_pool; /* Pointer to the first element in the list of queues */
  extern struct acl_pool * table_pool; /* Pointer to the first element in the list of tables */
  extern int rule_pool_start; /* Initial number of ipfw rules pool */
  extern int pipe_pool_start; /* Initial number of ipfw dummynet pipe pool */
  extern int queue_pool_start; /* Initial number of ipfw dummynet queue pool */
  extern int table_pool_start; /* Initial number of ipfw tables pool */
#endif

/*
 * FUNCTIONS
 */

  extern void	IfaceInit(Bund b);
  extern void	IfaceInst(Bund b, Bund bt);
  extern void	IfaceDestroy(Bund b);
  extern void	IfaceOpen(Bund b);
  extern void	IfaceClose(Bund b);
  extern int	IfaceOpenCmd(Context ctx);
  extern int	IfaceCloseCmd(Context ctx);
  extern int	IfaceIpIfaceUp(Bund b, int ready);
  extern void	IfaceIpIfaceDown(Bund b);
  extern int	IfaceIpv6IfaceUp(Bund b, int ready);
  extern void	IfaceIpv6IfaceDown(Bund b);
  extern void	IfaceUp(Bund b, int ready);
  extern void	IfaceDown(Bund b);
  extern int	IfaceStat(Context ctx, int ac, char *av[], void *arg);

  extern void	IfaceListenInput(Bund b, int proto, Mbuf pkt);
  #ifndef USE_NG_TCPMSS
  extern void	IfaceCorrectMSS(Mbuf pkt, uint16_t maxmss);
  #endif
  extern void	IfaceSetMTU(Bund b, int mtu);
  extern void	IfaceChangeFlags(Bund b, int clear, int set);
  extern int	IfaceChangeAddr(Bund b, int add, struct u_range *self, struct u_addr *peer);
  extern int	IfaceSetRoute(Bund b, int cmd, struct u_range *dst, struct u_addr *gw);

#ifdef USE_NG_BPF
  extern void	IfaceGetStats(Bund b, struct svcstat *stat);
  extern void	IfaceAddStats(struct svcstat *stat1, struct svcstat *stat2);
  extern void	IfaceFreeStats(struct svcstat *stat);
#endif

#endif

