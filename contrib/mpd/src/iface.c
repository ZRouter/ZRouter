
/*
 * iface.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 *
 * TCP MSSFIX code copyright (c) 2000 Ruslan Ermilov
 * TCP MSSFIX contributed by Sergey Korolew <dsATbittu.org.ru>
 *
 */

#include "ppp.h"
#include "iface.h"
#include "ipcp.h"
#include "auth.h"
#include "ngfunc.h"
#include "netgraph.h"
#include "util.h"

#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_var.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet6/nd6.h>
#include <netgraph/ng_message.h>
#include <netgraph/ng_iface.h>
#ifdef USE_NG_BPF
#include <netgraph/ng_bpf.h>
#endif
#include <netgraph/ng_tee.h>
#include <netgraph/ng_ksocket.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#ifdef USE_NG_TCPMSS
#include <netgraph/ng_tcpmss.h>
#endif
#ifdef USE_NG_IPACCT
#include <netgraph/ng_ipacct.h>
#undef r_ip_p	/* XXX:DIRTY CONFLICT! */
#endif
#ifdef USE_NG_NETFLOW
#include <netgraph/netflow/ng_netflow.h>
#endif
#ifdef USE_NG_CAR
#include <netgraph/ng_car.h>
#endif

#ifdef USE_NG_BPF
#include <pcap.h>
#endif

/*
 * DEFINITIONS
 */

/* Set menu options */

  enum {
    SET_IDLE,
    SET_SESSION,
    SET_ADDRS,
    SET_ROUTE,
    SET_MTU,
    SET_UP_SCRIPT,
    SET_DOWN_SCRIPT,
    SET_ENABLE,
    SET_DISABLE
  };

/*
 * INTERNAL FUNCTIONS
 */

  static int	IfaceNgIpInit(Bund b, int ready);
  static void	IfaceNgIpShutdown(Bund b);
  static int	IfaceNgIpv6Init(Bund b, int ready);
  static void	IfaceNgIpv6Shutdown(Bund b);

#ifdef USE_NG_NETFLOW
  static int	IfaceInitNetflow(Bund b, char *path, char *hook, char in, char out);
  static int	IfaceSetupNetflow(Bund b, char in, char out);
  static void	IfaceShutdownNetflow(Bund b, char in, char out);
#endif

#ifdef USE_NG_IPACCT
  static int	IfaceInitIpacct(Bund b, char *path, char *hook);
  static void	IfaceShutdownIpacct(Bund b);
#endif

#ifdef USE_NG_NAT
  static int	IfaceInitNAT(Bund b, char *path, char *hook);
  static int	IfaceSetupNAT(Bund b);
  static void	IfaceShutdownNAT(Bund b);
#endif

  static int	IfaceInitTee(Bund b, char *path, char *hook, int v6);
  static void	IfaceShutdownTee(Bund b, int v6);

#if defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF))
  static int    IfaceInitMSS(Bund b, char *path, char *hook);
  static void	IfaceSetupMSS(Bund b, uint16_t maxMSS);
  static void	IfaceShutdownMSS(Bund b);
#endif

#ifdef USE_NG_BPF
  static int    IfaceInitLimits(Bund b, char *path, char *hook);
  static void	IfaceSetupLimits(Bund b);
  static void	IfaceShutdownLimits(Bund b);
#endif

  static int	IfaceSetCommand(Context ctx, int ac, char *av[], void *arg);
  static void	IfaceSessionTimeout(void *arg);
  static void	IfaceIdleTimeout(void *arg);

  static void	IfaceCacheSend(Bund b);
  static void	IfaceCachePkt(Bund b, int proto, Mbuf pkt);
  static int	IfaceIsDemand(int proto, Mbuf pkt);

#ifdef USE_IPFW
  static int	IfaceAllocACL (struct acl_pool ***ap, int start, char * ifname, int number);
  static int	IfaceFindACL (struct acl_pool *ap, char * ifname, int number);
  static char *	IFaceParseACL (char * src, char * ifname);
#endif
  
/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab IfaceSetCmds[] = {
    { "addrs {self} {peer}",		"Set interface addresses",
	IfaceSetCommand, NULL, 2, (void *) SET_ADDRS },
    { "route {dest}[/{width}]",		"Add IP route",
	IfaceSetCommand, NULL, 2, (void *) SET_ROUTE },
    { "mtu {size}",			"Set max allowed interface MTU",
	IfaceSetCommand, NULL, 2, (void *) SET_MTU },
    { "up-script [{progname}]",		"Interface up script",
	IfaceSetCommand, NULL, 2, (void *) SET_UP_SCRIPT },
    { "down-script [{progname}]",	"Interface down script",
	IfaceSetCommand, NULL, 2, (void *) SET_DOWN_SCRIPT },
#ifdef USE_NG_BPF
    { "idle {seconds}",			"Idle timeout",
	IfaceSetCommand, NULL, 2, (void *) SET_IDLE },
#endif
    { "session {seconds}",		"Session timeout",
	IfaceSetCommand, NULL, 2, (void *) SET_SESSION },
    { "enable [opt ...]",		"Enable option",
	IfaceSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",		"Disable option",
	IfaceSetCommand, NULL, 2, (void *) SET_DISABLE },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static const struct confinfo	gConfList[] = {
    { 0,	IFACE_CONF_ONDEMAND,		"on-demand"	},
    { 0,	IFACE_CONF_PROXY,		"proxy-arp"	},
    { 0,	IFACE_CONF_TCPMSSFIX,           "tcpmssfix"	},
    { 0,	IFACE_CONF_TEE,			"tee"		},
    { 0,	IFACE_CONF_NAT,			"nat"		},
    { 0,	IFACE_CONF_NETFLOW_IN,		"netflow-in"	},
    { 0,	IFACE_CONF_NETFLOW_OUT,		"netflow-out"	},
#ifdef NG_NETFLOW_CONF_ONCE
    { 0,	IFACE_CONF_NETFLOW_ONCE,	"netflow-once"	},
#endif
    { 0,	IFACE_CONF_IPACCT,		"ipacct"	},
    { 0,	0,				NULL		},
  };

#ifdef USE_IPFW
  struct acl_pool * rule_pool = NULL; /* Pointer to the first element in the list of rules */
  struct acl_pool * pipe_pool = NULL; /* Pointer to the first element in the list of pipes */
  struct acl_pool * queue_pool = NULL; /* Pointer to the first element in the list of queues */
  struct acl_pool * table_pool = NULL; /* Pointer to the first element in the list of tables */
  int rule_pool_start = 10000; /* Initial number of ipfw rules pool */
  int pipe_pool_start = 10000; /* Initial number of ipfw dummynet pipe pool */
  int queue_pool_start = 10000; /* Initial number of ipfw dummynet queue pool */
  int table_pool_start = 32; /* Initial number of ipfw tables pool */
#endif

#ifdef USE_NG_BPF
  /* A BPF filter that matches TCP SYN packets */
  static const struct bpf_insn gTCPSYNProg[] = {
/*00*/	BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 9),		/* A <- IP protocol */
/*01*/	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_TCP, 0, 6), /* !TCP => 8 */
/*02*/	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 6),	/* A <- fragmentation offset */
/*03*/	BPF_JUMP(BPF_JMP+BPF_JSET+BPF_K, 0x1fff, 4, 0),	/* fragment => 8 */
/*04*/	BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 0),		/* X <- header len */
/*05*/	BPF_STMT(BPF_LD+BPF_B+BPF_IND, 13),		/* A <- TCP flags */
/*06*/	BPF_JUMP(BPF_JMP+BPF_JSET+BPF_K, TH_SYN, 0, 1),	/* !TH_SYN => 8 */
/*07*/	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),		/* accept packet */
/*08*/	BPF_STMT(BPF_RET+BPF_K, 0),			/* reject packet */
  };

  #define TCPSYN_PROG_LEN	(sizeof(gTCPSYNProg) / sizeof(*gTCPSYNProg))

  /* A BPF filter that matches nothing */
  static const struct bpf_insn gNoMatchProg[] = {
	BPF_STMT(BPF_RET+BPF_K, 0)
  };

  #define NOMATCH_PROG_LEN	(sizeof(gNoMatchProg) / sizeof(*gNoMatchProg))

  /* A BPF filter that matches everything */
  static const struct bpf_insn gMatchProg[] = {
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1)
  };

  #define MATCH_PROG_LEN	(sizeof(gMatchProg) / sizeof(*gMatchProg))
#endif /* USE_NG_BPF */

#define IN6MASK128	{{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
			    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}}
static const struct in6_addr in6mask128 = IN6MASK128;


/*
 * IfaceInit()
 */

void
IfaceInit(Bund b)
{
  IfaceState	const iface = &b->iface;

  /* Default configuration */
  iface->mtu = NG_IFACE_MTU_DEFAULT;
  iface->max_mtu = NG_IFACE_MTU_DEFAULT;
  Disable(&iface->options, IFACE_CONF_ONDEMAND);
  Disable(&iface->options, IFACE_CONF_PROXY);
  Disable(&iface->options, IFACE_CONF_TCPMSSFIX);
#ifdef	USE_NG_NAT
  NatInit(b);
#endif
#ifdef USE_NG_BPF
  SLIST_INIT(&iface->ss[0]);
  SLIST_INIT(&iface->ss[1]);
#endif
}

/*
 * IfaceInst()
 */

void
IfaceInst(Bund b, Bund bt)
{
    IfaceState	const iface = &b->iface;

    memcpy(iface, &bt->iface, sizeof(*iface));
}

/*
 * IfaceOpen()
 *
 * Open the interface layer
 */

void
IfaceOpen(Bund b)
{
    IfaceState	const iface = &b->iface;

    Log(LG_IFACE, ("[%s] IFACE: Open event", b->name));

    /* Open is useless without on-demand. */
    if (!Enabled(&iface->options, IFACE_CONF_ONDEMAND)) {
	Log(LG_ERR, ("[%s] 'open iface' is useless without on-demand enabled", b->name));
	return;
    }

    /* If interface is already open do nothing */
    if (iface->open)
	return;
    iface->open = TRUE;

    /* If on-demand, bring up system interface immediately and start
     listening for outgoing packets. The next outgoing packet will
     cause us to open the lower layer(s) */
    BundNcpsJoin(b, NCP_NONE);
}

/*
 * IfaceClose()
 *
 * Close the interface layer
 */

void
IfaceClose(Bund b)
{
    IfaceState	const iface = &b->iface;

    Log(LG_IFACE, ("[%s] IFACE: Close event", b->name));

    /* If interface is already closed do nothing */
    if (!iface->open)
	return;
    iface->open = FALSE;

    /* If there was on-demand, tell that it is not needed anymore */
    BundNcpsLeave(b, NCP_NONE);
}

/*
 * IfaceOpenCmd()
 *
 * Open the interface layer
 */

int
IfaceOpenCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to open template");
    IfaceOpen(ctx->bund);
    return (0);
}

/*
 * IfaceCloseCmd()
 *
 * Close the interface layer
 */

int
IfaceCloseCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to close template");
    IfaceClose(ctx->bund);
    return (0);
}

/*
 * IfaceUp()
 *
 * Our underlying PPP bundle is ready for traffic.
 * We may signal that the interface is in DoD with the IFF_LINK0 flag.
 */

void
IfaceUp(Bund b, int ready)
{
  IfaceState	const iface = &b->iface;
  int		session_timeout = 0, idle_timeout = 0;
#ifdef USE_IPFW
  struct acl	*acls, *acl;
  char			*buf;
  struct acl_pool 	**poollast;
  int 			poollaststart;
  int		prev_number;
  int		prev_real_number;
#endif

  Log(LG_IFACE, ("[%s] IFACE: Up event", b->name));
  iface->last_up = time(NULL);

  if (ready) {

    /* Start Session timer */
    if (b->params.session_timeout > 0) {
	session_timeout = b->params.session_timeout;
    } else if (iface->session_timeout > 0) {
	session_timeout = iface->session_timeout;
    }

    if (session_timeout > 0) {
	Log(LG_IFACE2, ("[%s] IFACE: session-timeout: %d seconds", 
    	    b->name, session_timeout));
	if (session_timeout > INT_MAX / 1100) {
	    session_timeout = INT_MAX / 1100;
	    Log(LG_ERR, ("[%s] IFACE: session-timeout limited to %d seconds", 
    		b->name, session_timeout));
	}
	TimerInit(&iface->sessionTimer, "IfaceSession",
    	    session_timeout * SECONDS, IfaceSessionTimeout, b);
	TimerStart(&iface->sessionTimer);
    }

    /* Start idle timer */
    if (b->params.idle_timeout > 0) {
	idle_timeout = b->params.idle_timeout;
    } else if (iface->idle_timeout > 0) {
	idle_timeout = iface->idle_timeout;
    }
    
    if (idle_timeout > 0) {
	Log(LG_IFACE2, ("[%s] IFACE: idle-timeout: %d seconds", 
    	    b->name, idle_timeout));
	if (idle_timeout > INT_MAX / 1100 * IFACE_IDLE_SPLIT) {
	    idle_timeout = INT_MAX / 1100 * IFACE_IDLE_SPLIT;
	    Log(LG_ERR, ("[%s] IFACE: idle-timeout limited to %d seconds", 
    		b->name, idle_timeout));
	}
	TimerInit(&iface->idleTimer, "IfaceIdle",
    	    idle_timeout * SECONDS / IFACE_IDLE_SPLIT, IfaceIdleTimeout, b);
	TimerStart(&iface->idleTimer);
	iface->traffic[1] = TRUE;
	iface->traffic[0] = FALSE;

	/* Reset statistics */
	memset(&iface->idleStats, 0, sizeof(iface->idleStats));
    }

#ifdef USE_IPFW
  /* Allocate ACLs */
  acls = b->params.acl_pipe;
  poollast = &pipe_pool;
  poollaststart = pipe_pool_start;
  while (acls != NULL) {
    acls->real_number = IfaceAllocACL(&poollast, poollaststart, iface->ifname, acls->number);
    poollaststart = acls->real_number;
    acls = acls->next;
  };
  acls = b->params.acl_queue;
  poollast = &queue_pool;
  poollaststart = queue_pool_start;
  while (acls != NULL) {
    acls->real_number = IfaceAllocACL(&poollast, poollaststart, iface->ifname, acls->number);
    poollaststart = acls->real_number;
    acls = acls->next;
  };
  prev_number = -1;
  prev_real_number = -1;
  acls = b->params.acl_table;
  poollast = &table_pool;
  poollaststart = table_pool_start;
  while (acls != NULL) {
    if (acls->real_number == 0) {
	if (acls->number == prev_number) { /* ACL list is presorted so we need not allocate if equal */
	    acls->real_number = prev_real_number;
	} else {
	    acls->real_number = IfaceAllocACL(&poollast, poollaststart, iface->ifname, acls->number);
	    poollaststart = acls->real_number;
	    prev_number = acls->number;
	    prev_real_number = acls->real_number;
	}
    }
    acls = acls->next;
  };
  acls = b->params.acl_rule;
  poollast = &rule_pool;
  poollaststart = rule_pool_start;
  while (acls != NULL) {
    acls->real_number = IfaceAllocACL(&poollast, poollaststart, iface->ifname, acls->number);
    poollaststart = acls->real_number;
    acls = acls->next;
  };

  /* Set ACLs */
  acls = b->params.acl_pipe;
  while (acls != NULL) {
    ExecCmd(LG_IFACE2, b->name, "%s pipe %d config %s", PATH_IPFW, acls->real_number, acls->rule);
    acls = acls->next;
  }
  acls = b->params.acl_queue;
  while (acls != NULL) {
    buf = IFaceParseACL(acls->rule,iface->ifname);
    ExecCmd(LG_IFACE2, b->name, "%s queue %d config %s", PATH_IPFW, acls->real_number, buf);
    Freee(buf);
    acls = acls->next;
  }
  acls = b->params.acl_table;
  while (acls != NULL) {
    acl = Mdup(MB_IFACE, acls, sizeof(struct acl) + strlen(acls->rule));
    acl->next = iface->tables;
    iface->tables = acl;
    ExecCmd(LG_IFACE2, b->name, "%s table %d add %s", PATH_IPFW, acls->real_number, acls->rule);
    acls = acls->next;
  };
  acls = b->params.acl_rule;
  while (acls != NULL) {
    buf = IFaceParseACL(acls->rule, iface->ifname);
    ExecCmd(LG_IFACE2, b->name, "%s add %d %s via %s", PATH_IPFW, acls->real_number, buf, iface->ifname);
    Freee(buf);
    acls = acls->next;
  };
#endif /* USE_IPFW */

  };

  /* Bring up system interface */
  IfaceChangeFlags(b, 0, IFF_UP | (ready?0:IFF_LINK0));

  /* Send any cached packets */
  IfaceCacheSend(b);

}

/*
 * IfaceDown()
 *
 * Our packet transport mechanism is no longer ready for traffic.
 */

void
IfaceDown(Bund b)
{
  IfaceState	const iface = &b->iface;
#ifdef USE_IPFW
  struct acl_pool	**rp, *rp1;
  char		cb[32768];
  struct acl    *acl, *aclnext;
#endif

  Log(LG_IFACE, ("[%s] IFACE: Down event", b->name));

  /* Bring down system interface */
  IfaceChangeFlags(b, IFF_UP | IFF_LINK0, 0);

  TimerStop(&iface->idleTimer);
  TimerStop(&iface->sessionTimer);

#ifdef USE_IPFW
  /* Remove rule ACLs */
  rp = &rule_pool;
  cb[0]=0;
  while (*rp != NULL) {
    if (strncmp((*rp)->ifname, iface->ifname, IFNAMSIZ) == 0) {
      sprintf(cb+strlen(cb), " %d", (*rp)->real_number);
      rp1 = *rp;
      *rp = (*rp)->next;
      Freee(rp1);
    } else {
      rp = &((*rp)->next);
    };
  };
  if (cb[0]!=0)
    ExecCmdNosh(LG_IFACE2, b->name, "%s delete%s",
      PATH_IPFW, cb);

  /* Remove table ACLs */
  rp = &table_pool;
  while (*rp != NULL) {
    if (strncmp((*rp)->ifname, iface->ifname, IFNAMSIZ) == 0) {
      rp1 = *rp;
      *rp = (*rp)->next;
      Freee(rp1);
    } else {
      rp = &((*rp)->next);
    };
  };
  acl = iface->tables;
  while (acl != NULL) {
    ExecCmd(LG_IFACE2, b->name, "%s table %d delete %s",
	PATH_IPFW, acl->real_number, acl->rule);
    aclnext = acl->next;
    Freee(acl);
    acl = aclnext;
  };
  iface->tables = NULL;

  /* Remove queue ACLs */
  rp = &queue_pool;
  cb[0]=0;
  while (*rp != NULL) {
    if (strncmp((*rp)->ifname, iface->ifname, IFNAMSIZ) == 0) {
      sprintf(cb+strlen(cb), " %d", (*rp)->real_number);
      rp1 = *rp;
      *rp = (*rp)->next;
      Freee(rp1);
    } else {
      rp = &((*rp)->next);
    };
  };
  if (cb[0]!=0)
    ExecCmdNosh(LG_IFACE2, b->name, "%s queue delete%s",
      PATH_IPFW, cb);

  /* Remove pipe ACLs */
  rp = &pipe_pool;
  cb[0]=0;
  while (*rp != NULL) {
    if (strncmp((*rp)->ifname, iface->ifname, IFNAMSIZ) == 0) {
      sprintf(cb+strlen(cb), " %d", (*rp)->real_number);
      rp1 = *rp;
      *rp = (*rp)->next;
      Freee(rp1);
    } else {
      rp = &((*rp)->next);
    };
  };
  if (cb[0]!=0)
    ExecCmdNosh(LG_IFACE2, b->name, "%s pipe delete%s",
      PATH_IPFW, cb);
#endif /* USE_IPFW */

}

/*
 * IfaceListenInput()
 *
 * A packet was received on our demand snooping hook. Stimulate a connection.
 */

void
IfaceListenInput(Bund b, int proto, Mbuf pkt)
{
    IfaceState	const iface = &b->iface;
    int		const isDemand = IfaceIsDemand(proto, pkt);

    /* Does this count as demand traffic? */
    if (iface->open && isDemand) {
	iface->traffic[0] = TRUE;
        Log(LG_IFACE, ("[%s] IFACE: Outgoing %s packet demands connection", b->name,
	    (proto==PROTO_IP)?"IP":"IPv6"));
	RecordLinkUpDownReason(b, NULL, 1, STR_DEMAND, NULL);
        BundOpen(b);
        IfaceCachePkt(b, proto, pkt);
    } else {
	mbfree(pkt);
    }
}

#ifdef USE_IPFW
/*
 * IfaceAllocACL ()
 *
 * Allocates unique real number for new ACL and adds it to the list of used ones.
 */

static int
IfaceAllocACL(struct acl_pool ***ap, int start, char *ifname, int number)
{
    int	i;
    struct acl_pool **rp,*rp1;

    rp1 = Malloc(MB_IFACE, sizeof(struct acl_pool));
    strlcpy(rp1->ifname, ifname, sizeof(rp1->ifname));
    rp1->acl_number = number;

    rp = *ap;
    i = start;
    while (*rp != NULL && (*rp)->real_number <= i) {
        i = (*rp)->real_number+1;
        rp = &((*rp)->next);
    };
    if (*rp == NULL) {
        rp1->next = NULL;
    } else {
        rp1->next = *rp;
    };
    rp1->real_number = i;
    *rp = rp1;
    *ap = rp;
    return(i);
}

/*
 * IfaceFindACL ()
 *
 * Finds ACL in the list and gets its real number.
 */

static int
IfaceFindACL (struct acl_pool *ap, char * ifname, int number)
{
    int	i;
    struct acl_pool *rp;

    rp=ap;
    i=-1;
    while (rp != NULL) {
	if ((rp->acl_number == number) && (strncmp(rp->ifname,ifname,IFNAMSIZ) == 0)) {
    	    i = rp->real_number;
	    break;
	};
        rp = rp->next;
    };
    return(i);
}

/*
 * IFaceParseACL ()
 *
 * Parces ACL and replaces %r, %p and %q macroses 
 * by the real numbers of rules, queues and pipes.
 */

static char *
IFaceParseACL (char * src, char * ifname)
{
    char *buf,*buf1;
    char *begin,*param,*end;
    char t;
    int num,real_number;
    struct acl_pool *ap;
    
    buf = Malloc(MB_IFACE, ACL_LEN);
    buf1 = Malloc(MB_IFACE, ACL_LEN);

    strlcpy(buf, src, ACL_LEN);
    do {
        end = buf;
	begin = strsep(&end, "%");
	param = strsep(&end, " ");
	if (param != NULL) {
	    if (sscanf(param,"%c%d", &t, &num) == 2) {
		switch (t) {
		    case 'r':
			ap = rule_pool;
			break;
		    case 'p':
			ap = pipe_pool;
			break;
		    case 'q':
			ap = queue_pool;
			break;
		    case 't':
			ap = table_pool;
			break;
		    default:
			ap = NULL;
		};
		real_number = IfaceFindACL(ap,ifname,num);
		if (end != NULL) {
		    snprintf(buf1, ACL_LEN, "%s%d %s", begin, real_number, end);
		} else {
		    snprintf(buf1, ACL_LEN, "%s%d", begin, real_number);
		};
		strlcpy(buf, buf1, ACL_LEN);
	    };
	};
    } while (end != NULL);
    Freee(buf1);
    return(buf);
}
#endif /* USE_IPFW */

/*
 * IfaceIpIfaceUp()
 *
 * Bring up the IP interface. The "ready" flag means that
 * IPCP is also up and we can deliver packets immediately.
 */

int
IfaceIpIfaceUp(Bund b, int ready)
{
    IfaceState		const iface = &b->iface;
    struct sockaddr_dl	hwa;
    char		hisaddr[20];
    IfaceRoute		r;
    u_char		*ether;

    if (ready && !iface->conf.self_addr_force) {
	in_addrtou_range(&b->ipcp.want_addr, 32, &iface->self_addr);
    } else {
	u_rangecopy(&iface->conf.self_addr, &iface->self_addr);
    }
    if (ready && !iface->conf.peer_addr_force) {
	in_addrtou_addr(&b->ipcp.peer_addr, &iface->peer_addr);
    } else {
	u_addrcopy(&iface->conf.peer_addr, &iface->peer_addr);
    }

    if (IfaceNgIpInit(b, ready)) {
	Log(LG_ERR, ("[%s] IFACE: IfaceNgIpInit() error, closing IPCP", b->name));
	FsmFailure(&b->ipcp.fsm, FAIL_NEGOT_FAILURE);
	return (-1);
    };

    /* Set addresses */
    if (!u_rangeempty(&iface->self_addr) &&
	    IfaceChangeAddr(b, 1, &iface->self_addr, &iface->peer_addr)) {
	Log(LG_ERR, ("[%s] IFACE: IfaceChangeAddr() error, closing IPCP", b->name));
	FsmFailure(&b->ipcp.fsm, FAIL_NEGOT_FAILURE);
	return (-1);
    };

    /* Proxy ARP for peer if desired and peer's address is known */
    u_addrclear(&iface->proxy_addr);
    if (Enabled(&iface->options, IFACE_CONF_PROXY)) {
	if (u_addrempty(&iface->peer_addr)) {
    	    Log(LG_IFACE,
		("[%s] IFACE: Can't proxy arp for %s",
		b->name, u_addrtoa(&iface->peer_addr,hisaddr,sizeof(hisaddr))));
	} else if (GetEther(&iface->peer_addr, &hwa) < 0) {
    	    Log(LG_IFACE,
		("[%s] IFACE: No interface to proxy arp on for %s",
		b->name, u_addrtoa(&iface->peer_addr,hisaddr,sizeof(hisaddr))));
	} else {
    	    ether = (u_char *) LLADDR(&hwa);
    	    if (ExecCmdNosh(LG_IFACE2, b->name, 
		"%s -S %s %x:%x:%x:%x:%x:%x pub",
		PATH_ARP, u_addrtoa(&iface->peer_addr,hisaddr,sizeof(hisaddr)),
		ether[0], ether[1], ether[2],
		ether[3], ether[4], ether[5]) == 0)
	    iface->proxy_addr = iface->peer_addr;
	}
    }
  
    /* Add static routes */
    SLIST_FOREACH(r, &iface->routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET) {
	    r->ok = (IfaceSetRoute(b, RTM_ADD, &r->dest, &iface->peer_addr) == 0);
	}
    }
    /* Add dynamic routes */
    SLIST_FOREACH(r, &b->params.routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET) {
	    r->ok = (IfaceSetRoute(b, RTM_ADD, &r->dest, &iface->peer_addr) == 0);
	}
    }

#ifdef USE_NG_NAT
    /* Set NAT IP */
    if (iface->nat_up)
	IfaceSetupNAT(b);
#endif

    /* Call "up" script */
    if (*iface->up_script) {
	char	selfbuf[40],peerbuf[40];
	char	ns1buf[21], ns2buf[21];
	int	res;

	if(b->ipcp.want_dns[0].s_addr != 0)
    	    snprintf(ns1buf, sizeof(ns1buf), "dns1 %s", inet_ntoa(b->ipcp.want_dns[0]));
	else
    	    ns1buf[0] = '\0';
	if(b->ipcp.want_dns[1].s_addr != 0)
    	    snprintf(ns2buf, sizeof(ns2buf), "dns2 %s", inet_ntoa(b->ipcp.want_dns[1]));
	else
    	    ns2buf[0] = '\0';

	res = ExecCmd(LG_IFACE2, b->name, "%s %s inet %s %s '%s' '%s' '%s' '%s'",
	    iface->up_script, iface->ifname, u_rangetoa(&iface->self_addr,selfbuf, sizeof(selfbuf)),
    	    u_addrtoa(&iface->peer_addr, peerbuf, sizeof(peerbuf)), 
    	    *b->params.authname ? b->params.authname : "-", 
    	    ns1buf, ns2buf, *b->params.peeraddr ? b->params.peeraddr : "-");
	if (res != 0) {
	    FsmFailure(&b->ipcp.fsm, FAIL_NEGOT_FAILURE);
	    return (-1);
	}
    }
    return (0);
}

/*
 * IfaceIpIfaceDown()
 *
 * Bring down the IP interface. This implies we're no longer ready.
 */

void
IfaceIpIfaceDown(Bund b)
{
    IfaceState	const iface = &b->iface;
    IfaceRoute	r;
    char	buf[48];

    /* Call "down" script */
    if (*iface->down_script) {
	char	selfbuf[40],peerbuf[40];

	ExecCmd(LG_IFACE2, b->name, "%s %s inet %s %s '%s' '%s'",
    	    iface->down_script, iface->ifname, u_rangetoa(&iface->self_addr,selfbuf, sizeof(selfbuf)),
    	    u_addrtoa(&iface->peer_addr, peerbuf, sizeof(peerbuf)), 
    	    *b->params.authname ? b->params.authname : "-",
    	    *b->params.peeraddr ? b->params.peeraddr : "-");
    }

    /* Delete dynamic routes */
    SLIST_FOREACH(r, &b->params.routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET) {
	    if (!r->ok)
		continue;
	    IfaceSetRoute(b, RTM_DELETE, &r->dest, &iface->peer_addr);
	    r->ok = 0;
	}
    }
    /* Delete static routes */
    SLIST_FOREACH(r, &iface->routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET) {
	    if (!r->ok)
		continue;
	    IfaceSetRoute(b, RTM_DELETE, &r->dest, &iface->peer_addr);
	    r->ok = 0;
	}
    }

    /* Delete any proxy arp entry */
    if (!u_addrempty(&iface->proxy_addr))
	ExecCmdNosh(LG_IFACE2, b->name, "%s -d %s", PATH_ARP, u_addrtoa(&iface->proxy_addr, buf, sizeof(buf)));
    u_addrclear(&iface->proxy_addr);

    /* Remove address from interface */
    if (!u_rangeempty(&iface->self_addr))
	IfaceChangeAddr(b, 0, &iface->self_addr, &iface->peer_addr);
    
    IfaceNgIpShutdown(b);
}

/*
 * IfaceIpv6IfaceUp()
 *
 * Bring up the IPv6 interface. The "ready" flag means that
 * IPv6CP is also up and we can deliver packets immediately.
 */

int
IfaceIpv6IfaceUp(Bund b, int ready)
{
    IfaceState		const iface = &b->iface;
    IfaceRoute		r;

    if (ready && !iface->conf.self_ipv6_addr_force) {
        iface->self_ipv6_addr.family = AF_INET6;
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[0] = 0x80fe;  /* Network byte order */
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[1] = 0x0000;
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[2] = 0x0000;
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[3] = 0x0000;
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[4] = ((u_short*)b->ipv6cp.myintid)[0];
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[5] = ((u_short*)b->ipv6cp.myintid)[1];
        iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[6] = ((u_short*)b->ipv6cp.myintid)[2];
	iface->self_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[7] = ((u_short*)b->ipv6cp.myintid)[3];
    } else {
	u_addrcopy(&iface->conf.self_ipv6_addr, &iface->self_ipv6_addr);
    }
    if (ready && !iface->conf.peer_ipv6_addr_force) {
        iface->peer_ipv6_addr.family = AF_INET6;
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[0] = 0x80fe;  /* Network byte order */
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[1] = 0x0000;
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[2] = 0x0000;
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[3] = 0x0000;
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[4] = ((u_short*)b->ipv6cp.hisintid)[0];
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[5] = ((u_short*)b->ipv6cp.hisintid)[1];
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[6] = ((u_short*)b->ipv6cp.hisintid)[2];
        iface->peer_ipv6_addr.u.ip6.__u6_addr.__u6_addr16[7] = ((u_short*)b->ipv6cp.hisintid)[3];
    } else {
	u_addrcopy(&iface->conf.peer_ipv6_addr, &iface->peer_ipv6_addr);
    }

    if (IfaceNgIpv6Init(b, ready)) {
        Log(LG_ERR, ("[%s] IFACE: IfaceNgIpv6Init() failed, closing IPv6CP", b->name));
        FsmFailure(&b->ipv6cp.fsm, FAIL_NEGOT_FAILURE);
        return (-1);
    };
  
    /* Set addresses */
    if (!u_addrempty(&iface->self_ipv6_addr)) {
	struct u_range	rng;
	rng.addr = iface->self_ipv6_addr;
	rng.width = 64;
	if (IfaceChangeAddr(b, 1, &rng, &iface->peer_ipv6_addr)) {
	    Log(LG_ERR, ("[%s] IFACE: IfaceChangeAddr() failed, closing IPv6CP", b->name));
	    FsmFailure(&b->ipv6cp.fsm, FAIL_NEGOT_FAILURE);
    	    return (-1);
	}
    };
  
    /* Add static routes */
    SLIST_FOREACH(r, &iface->routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET6) {
	    r->ok = (IfaceSetRoute(b, RTM_ADD, &r->dest, &iface->peer_ipv6_addr) == 0);
	}
    }
    /* Add dynamic routes */
    SLIST_FOREACH(r, &b->params.routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET6) {
	    r->ok = (IfaceSetRoute(b, RTM_ADD, &r->dest, &iface->peer_ipv6_addr) == 0);
	}
    }

    /* Call "up" script */
    if (*iface->up_script) {
	char	selfbuf[48],peerbuf[48];
	int	res;

	res = ExecCmd(LG_IFACE2, b->name, "%s %s inet6 %s%%%s %s%%%s '%s' '%s'",
    	    iface->up_script, iface->ifname, 
    	    u_addrtoa(&iface->self_ipv6_addr, selfbuf, sizeof(selfbuf)), iface->ifname,
    	    u_addrtoa(&iface->peer_ipv6_addr, peerbuf, sizeof(peerbuf)), iface->ifname, 
    	    *b->params.authname ? b->params.authname : "-",
    	    *b->params.peeraddr ? b->params.peeraddr : "-");
	if (res != 0) {
	    FsmFailure(&b->ipv6cp.fsm, FAIL_NEGOT_FAILURE);
	    return (-1);
	}
    }
    return (0);

}

/*
 * IfaceIpv6IfaceDown()
 *
 * Bring down the IPv6 interface. This implies we're no longer ready.
 */

void
IfaceIpv6IfaceDown(Bund b)
{
    IfaceState		const iface = &b->iface;
    IfaceRoute		r;
    struct u_range	rng;

    /* Call "down" script */
    if (*iface->down_script) {
	char	selfbuf[48],peerbuf[48];

	ExecCmd(LG_IFACE2, b->name, "%s %s inet6 %s%%%s %s%%%s '%s' '%s'",
    	    iface->down_script, iface->ifname, 
    	    u_addrtoa(&iface->self_ipv6_addr, selfbuf, sizeof(selfbuf)), iface->ifname,
    	    u_addrtoa(&iface->peer_ipv6_addr, peerbuf, sizeof(peerbuf)), iface->ifname, 
    	    *b->params.authname ? b->params.authname : "-",
    	    *b->params.peeraddr ? b->params.peeraddr : "-");
    }

    /* Delete dynamic routes */
    SLIST_FOREACH(r, &b->params.routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET6) {
	    if (!r->ok)
		continue;
	    IfaceSetRoute(b, RTM_DELETE, &r->dest, &iface->peer_ipv6_addr);
	    r->ok = 0;
	}
    }
    /* Delete static routes */
    SLIST_FOREACH(r, &iface->routes, next) {
	if (u_rangefamily(&r->dest)==AF_INET6) {
	    if (!r->ok)
		continue;
	    IfaceSetRoute(b, RTM_DELETE, &r->dest, &iface->peer_ipv6_addr);
	    r->ok = 0;
	}
    }

    if (!u_addrempty(&iface->self_ipv6_addr)) {
	/* Remove address from interface */
	rng.addr = iface->self_ipv6_addr;
	rng.width = 64;
	IfaceChangeAddr(b, 0, &rng, &iface->peer_ipv6_addr);
    }

    IfaceNgIpv6Shutdown(b);
}

/*
 * IfaceIdleTimeout()
 */

static void
IfaceIdleTimeout(void *arg)
{
    Bund b = (Bund)arg;

  IfaceState			const iface = &b->iface;
  int				k;

  /* Get updated bpf node traffic statistics */
  BundUpdateStats(b);

  /* Mark current traffic period if there was traffic */
  if (iface->idleStats.recvFrames + iface->idleStats.xmitFrames < 
	b->stats.recvFrames + b->stats.xmitFrames) {
    iface->traffic[0] = TRUE;
  } else {		/* no demand traffic for a whole idle timeout period? */
    for (k = 0; k < IFACE_IDLE_SPLIT && !iface->traffic[k]; k++);
    if (k == IFACE_IDLE_SPLIT) {
      Log(LG_BUND, ("[%s] IFACE: Idle timeout", b->name));
      RecordLinkUpDownReason(b, NULL, 0, STR_IDLE_TIMEOUT, NULL);
      BundClose(b);
      return;
    }
  }

  iface->idleStats = b->stats;

  /* Shift traffic history */
  memmove(iface->traffic + 1,
    iface->traffic, (IFACE_IDLE_SPLIT - 1) * sizeof(*iface->traffic));
  iface->traffic[0] = FALSE;

  /* Restart timer */
  TimerStart(&iface->idleTimer);
}

/*
 * IfaceSessionTimeout()
 */

static void
IfaceSessionTimeout(void *arg)
{
    Bund b = (Bund)arg;

  Log(LG_BUND, ("[%s] IFACE: Session timeout", b->name));

  RecordLinkUpDownReason(b, NULL, 0, STR_SESSION_TIMEOUT, NULL);

  BundClose(b);

}

/*
 * IfaceCachePkt()
 *
 * A packet caused dial-on-demand; save it for later if possible.
 * Consumes the mbuf in any case.
 */

static void
IfaceCachePkt(Bund b, int proto, Mbuf pkt)
{
    IfaceState	const iface = &b->iface;

    /* Only cache network layer data */
    if (!PROT_NETWORK_DATA(proto)) {
	mbfree(pkt);
	return;
    }

    /* Release previously cached packet, if any, and save this one */
    if (iface->dodCache.pkt)
	mbfree(iface->dodCache.pkt);

    iface->dodCache.pkt = pkt;
    iface->dodCache.proto = proto;
    iface->dodCache.ts = time(NULL);
}

/*
 * IfaceCacheSend()
 *
 * Send cached packet
 */

static void
IfaceCacheSend(Bund b)
{
    IfaceState	const iface = &b->iface;

    if (iface->dodCache.pkt) {
	if (iface->dodCache.ts + MAX_DOD_CACHE_DELAY < time(NULL))
    	    mbfree(iface->dodCache.pkt);
	else {
    	    if (NgFuncWritePppFrame(b, NG_PPP_BUNDLE_LINKNUM,
		    iface->dodCache.proto, iface->dodCache.pkt) < 0) {
		Log(LG_ERR, ("[%s] can't write cached pkt: %s",
	    	    b->name, strerror(errno)));
    	    }
	}
	iface->dodCache.pkt = NULL;
    }
}

/*
 * IfaceIsDemand()
 *
 * Determine if this outgoing packet qualifies for dial-on-demand
 */

static int
IfaceIsDemand(int proto, Mbuf pkt)
{
  switch (proto) {
    case PROTO_IP:
      {
        struct ip	*ip;

        if (MBLEN(pkt) < sizeof(struct ip))
	    return (0);

	ip = (struct ip *)MBDATA(pkt);
	switch (ip->ip_p) {
	  case IPPROTO_IGMP:		/* No multicast stuff */
	    return(0);
#if (!defined(__FreeBSD__) || __FreeBSD_version >= 600025)
	  case IPPROTO_ICMP:
	    {
	      struct icmphdr	*icmp;
	      
    	      if (MBLEN(pkt) < (ip->ip_hl * 4 + sizeof(struct icmphdr)))
		return (0);

	      icmp = (struct icmphdr *) ((u_int32_t *) ip + ip->ip_hl);

	      switch (icmp->icmp_type)	/* No ICMP replies */
	      {
		case ICMP_ECHOREPLY:
		case ICMP_UNREACH:
		case ICMP_REDIRECT:
		  return(0);
		default:
		  break;
	      }
	    }
	    break;
#endif
	  case IPPROTO_UDP:
	    {
	      struct udphdr	*udp;

    	      if (MBLEN(pkt) < (ip->ip_hl * 4 + sizeof(struct udphdr)))
		return (0);

	      udp = (struct udphdr *) ((u_int32_t *) ip + ip->ip_hl);

#define NTP_PORT	123
	      if (ntohs(udp->uh_dport) == NTP_PORT)	/* No NTP packets */
		return(0);
	    }
	    break;
	  case IPPROTO_TCP:
	    {
	      struct tcphdr	*tcp;

    	      if (MBLEN(pkt) < (ip->ip_hl * 4 + sizeof(struct tcphdr)))
		return (0);

	      tcp = (struct tcphdr *) ((u_int32_t *) ip + ip->ip_hl);

	      if (tcp->th_flags & TH_RST)	/* No TCP reset packets */
		return(0);
	    }
	    break;
	  default:
	    break;
	}
	break;
      }
    default:
      break;
  }
  return(1);
}

/*
 * IfaceSetCommand()
 */

static int
IfaceSetCommand(Context ctx, int ac, char *av[], void *arg)
{
  IfaceState	const iface = &ctx->bund->iface;

  if (ac == 0)
    return(-1);
  switch ((intptr_t)arg) {
    case SET_IDLE:
      iface->idle_timeout = atoi(*av);
      break;
    case SET_SESSION:
      iface->session_timeout = atoi(*av);
      break;
    case SET_ADDRS:
      {
	struct u_range	self_addr;
	struct u_addr	peer_addr;
	int	self_addr_force = 0, peer_addr_force = 0;
	char	*arg;

	/* Parse */
	if (ac != 2)
	  return(-1);
	arg = av[0];
	if (arg[0] == '!') {
	    self_addr_force = 1;
	    arg++;
	}
	if (!ParseRange(arg, &self_addr, ALLOW_IPV4|ALLOW_IPV6))
	  Error("Bad IP address \"%s\"", av[0]);
	arg = av[1];
	if (arg[0] == '!') {
	    peer_addr_force = 1;
	    arg++;
	}
	if (!ParseAddr(arg, &peer_addr, ALLOW_IPV4|ALLOW_IPV6))
	  Error("Bad IP address \"%s\"", av[1]);
	if (self_addr.addr.family != peer_addr.family)
	  Error("Addresses must be from the same protocol family");

	/* OK */
	if (peer_addr.family == AF_INET) {
	    iface->conf.self_addr = self_addr;
	    iface->conf.peer_addr = peer_addr;
	    iface->conf.self_addr_force = self_addr_force;
	    iface->conf.peer_addr_force = peer_addr_force;
	} else {
	    iface->conf.self_ipv6_addr = self_addr.addr;
	    iface->conf.peer_ipv6_addr = peer_addr;
	    iface->conf.self_ipv6_addr_force = self_addr_force;
	    iface->conf.peer_ipv6_addr_force = peer_addr_force;
	}
      }
      break;

    case SET_ROUTE:
      {
	struct u_range		range;
	IfaceRoute		r;

	/* Check */
	if (ac != 1)
	  return(-1);

	/* Get dest address */
	if (!strcasecmp(av[0], "default")) {
	  u_rangeclear(&range);
	  range.addr.family=AF_INET;
	}
	else if (!ParseRange(av[0], &range, ALLOW_IPV4|ALLOW_IPV6))
	  Error("Bad route dest address \"%s\"", av[0]);
	r = Malloc(MB_IFACE, sizeof(struct ifaceroute));
	r->dest = range;
	r->ok = 0;
	SLIST_INSERT_HEAD(&iface->routes, r, next);
      }
      break;

    case SET_MTU:
      {
	int	max_mtu;

	max_mtu = atoi(av[0]);
	if (max_mtu < IFACE_MIN_MTU || max_mtu > IFACE_MAX_MTU)
	  Error("Invalid interface mtu %d", max_mtu);
	iface->max_mtu = max_mtu;
      }
      break;

    case SET_UP_SCRIPT:
      switch (ac) {
	case 0:
	  *iface->up_script = 0;
	  break;
	case 1:
	  strlcpy(iface->up_script, av[0], sizeof(iface->up_script));
	  break;
	default:
	  return(-1);
      }
      break;

    case SET_DOWN_SCRIPT:
      switch (ac) {
	case 0:
	  *iface->down_script = 0;
	  break;
	case 1:
	  strlcpy(iface->down_script, av[0], sizeof(iface->down_script));
	  break;
	default:
	  return(-1);
      }
      break;

    case SET_ENABLE:
      EnableCommand(ac, av, &iface->options, gConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &iface->options, gConfList);
      break;

    default:
      assert(0);
  }
  return(0);
}

/*
 * IfaceStat()
 */

int
IfaceStat(Context ctx, int ac, char *av[], void *arg)
{
    Bund	const b = ctx->bund;
    IfaceState	const iface = &b->iface;
    IfaceRoute	r;
#ifdef USE_NG_BPF
    int		k;
#endif
    char	buf[48];
#if defined(USE_NG_BPF) || defined(USE_IPFW)
    struct acl	*a;
#endif

    Printf("Interface configuration:\r\n");
    Printf("\tName            : %s\r\n", iface->ifname);
    Printf("\tMaximum MTU     : %d bytes\r\n", iface->max_mtu);
    Printf("\tIdle timeout    : %d seconds\r\n", iface->idle_timeout);
    Printf("\tSession timeout : %d seconds\r\n", iface->session_timeout);
    if (!u_rangeempty(&iface->conf.self_addr)) {
	Printf("\tIP Addresses    : %s%s -> ", iface->conf.self_addr_force?"!":"",
	    u_rangetoa(&iface->conf.self_addr,buf,sizeof(buf)));
	Printf("%s%s\r\n",  iface->conf.peer_addr_force?"!":"",
	    u_addrtoa(&iface->conf.peer_addr,buf,sizeof(buf)));
    }
    if (!u_addrempty(&iface->conf.self_ipv6_addr)) {
	Printf("\tIPv6 Addresses  : %s%s%%%s -> ",  iface->conf.self_ipv6_addr_force?"!":"",
	    u_addrtoa(&iface->conf.self_ipv6_addr,buf,sizeof(buf)), iface->ifname);
	Printf("%s%s%%%s\r\n",  iface->conf.peer_ipv6_addr_force?"!":"",
	    u_addrtoa(&iface->conf.peer_ipv6_addr,buf,sizeof(buf)), iface->ifname);
    }
    Printf("\tEvent scripts\r\n");
    Printf("\t  up-script     : \"%s\"\r\n",
	*iface->up_script ? iface->up_script : "<none>");
    Printf("\t  down-script   : \"%s\"\r\n",
	*iface->down_script ? iface->down_script : "<none>");
    Printf("Interface options:\r\n");
    OptStat(ctx, &iface->options, gConfList);
    if (!SLIST_EMPTY(&iface->routes)) {
	Printf("Static routes via peer:\r\n");
	SLIST_FOREACH(r, &iface->routes, next) {
	    Printf("\t%s\r\n", u_rangetoa(&r->dest,buf,sizeof(buf)));
	}
    }
    Printf("Interface status:\r\n");
    Printf("\tAdmin status    : %s\r\n", iface->open ? "OPEN" : "CLOSED");
    Printf("\tStatus          : %s\r\n", iface->up ? (iface->dod?"DoD":"UP") : "DOWN");
    if (iface->up) {
	Printf("\tSession time    : %ld seconds\r\n", (long int)(time(NULL) - iface->last_up));
	if (b->params.idle_timeout || iface->idle_timeout)
	    Printf("\tIdle timeout    : %d seconds\r\n", b->params.idle_timeout?b->params.idle_timeout:iface->idle_timeout);
	if (b->params.session_timeout || iface->session_timeout)
	    Printf("\tSession timeout : %d seconds\r\n", b->params.session_timeout?b->params.session_timeout:iface->session_timeout);
	Printf("\tMTU             : %d bytes\r\n", iface->mtu);
    }
    if (iface->ip_up && !u_rangeempty(&iface->self_addr)) {
	Printf("\tIP Addresses    : %s -> ", u_rangetoa(&iface->self_addr,buf,sizeof(buf)));
	Printf("%s\r\n", u_addrtoa(&iface->peer_addr,buf,sizeof(buf)));
    }
    if (iface->ipv6_up && !u_addrempty(&iface->self_ipv6_addr)) {
	Printf("\tIPv6 Addresses  : %s%%%s -> ", 
	    u_addrtoa(&iface->self_ipv6_addr,buf,sizeof(buf)), iface->ifname);
	Printf("%s%%%s\r\n", u_addrtoa(&iface->peer_ipv6_addr,buf,sizeof(buf)), iface->ifname);
    }
    if (iface->up) {
        Printf("Dynamic routes via peer:\r\n");
	SLIST_FOREACH(r, &ctx->bund->params.routes, next) {
	    Printf("\t%s\r\n", u_rangetoa(&r->dest,buf,sizeof(buf)));
	}
#ifdef USE_IPFW
	Printf("IPFW pipes:\r\n");
	a = ctx->bund->params.acl_pipe;
	while (a) {
	    Printf("\t%d (%d)\t: '%s'\r\n", a->number, a->real_number, a->rule);
	    a = a->next;
	}
	Printf("IPFW queues:\r\n");
	a = ctx->bund->params.acl_queue;
	while (a) {
	    Printf("\t%d (%d)\t: '%s'\r\n", a->number, a->real_number, a->rule);
	    a = a->next;
	}
	Printf("IPFW tables:\r\n");
	a = ctx->bund->params.acl_table;
	while (a) {
	    if (a->number != 0)
		Printf("\t%d (%d)\t: '%s'\r\n", a->number, a->real_number, a->rule);
	    else
		Printf("\t(%d)\t: '%s'\r\n", a->real_number, a->rule);
	    a = a->next;
	}
	Printf("IPFW rules:\r\n");
	a = ctx->bund->params.acl_rule;
	while (a) {
	    Printf("\t%d (%d)\t: '%s'\r\n", a->number, a->real_number, a->rule);
	    a = a->next;
	}
#endif /* USE_IPFW */
#ifdef USE_NG_BPF
	Printf("Traffic filters:\r\n");
	for (k = 0; k < ACL_FILTERS; k++) {
	    a = ctx->bund->params.acl_filters[k];
	    if (a == NULL)
		a = acl_filters[k];
	    while (a) {
		Printf("\t%d#%d\t: '%s'\r\n", (k + 1), a->number, a->rule);
		a = a->next;
	    }
	}
	Printf("Traffic limits:\r\n");
	for (k = 0; k < 2; k++) {
	    a = ctx->bund->params.acl_limits[k];
	    while (a) {
		Printf("\t%s#%d%s%s\t: '%s'\r\n", (k?"out":"in"), a->number,
		    ((a->name[0])?"#":""), a->name, a->rule);
		a = a->next;
	    }
	}
#endif /* USE_NG_BPF */
    }
    return(0);
}

/*
 * IfaceSetMTU()
 *
 * Set MTU and bandwidth on bundle's interface
 */

void
IfaceSetMTU(Bund b, int mtu)
{
    IfaceState		const iface = &b->iface;
    struct ifreq	ifr;
    int			s;

    /* Get socket */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	Perror("[%s] IFACE: Can't get socket to set MTU", b->name);
	return;
    }

    if ((b->params.mtu > 0) && (mtu > b->params.mtu)) {
	mtu = b->params.mtu;
	Log(LG_IFACE2, ("[%s] IFACE: forcing MTU of auth backend: %d bytes",
	    b->name, mtu));
    }

    /* Limit MTU to configured maximum */
    if (mtu > iface->max_mtu)
        mtu = iface->max_mtu;

    /* Set MTU on interface */
    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, b->iface.ifname, sizeof(ifr.ifr_name));
    ifr.ifr_mtu = mtu;
    Log(LG_IFACE2, ("[%s] IFACE: setting %s MTU to %d bytes",
	b->name, b->iface.ifname, mtu));
    if (ioctl(s, SIOCSIFMTU, (char *)&ifr) < 0)
	Perror("[%s] IFACE: ioctl(%s, %s)", b->name, b->iface.ifname, "SIOCSIFMTU");
    close(s);

    /* Save MTU */
    iface->mtu = mtu;

#if defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF))
    /* Update tcpmssfix config */
    if (iface->mss_up)
        IfaceSetupMSS(b, MAXMSS(mtu));
#endif

}

void
IfaceChangeFlags(Bund b, int clear, int set)
{
    struct ifreq ifrq;
    int s, new_flags;

    Log(LG_IFACE2, ("[%s] IFACE: Change interface flags: -%d +%d",
	b->name, clear, set)); 

    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	Perror("[%s] IFACE: Can't get socket to change interface flags", b->name);
	return;
    }

    memset(&ifrq, '\0', sizeof(ifrq));
    strlcpy(ifrq.ifr_name, b->iface.ifname, sizeof(ifrq.ifr_name));
    ifrq.ifr_name[sizeof(ifrq.ifr_name) - 1] = '\0';
    if (ioctl(s, SIOCGIFFLAGS, &ifrq) < 0) {
	Perror("[%s] IFACE: ioctl(%s, %s)", b->name, b->iface.ifname, "SIOCGIFFLAGS");
	close(s);
	return;
    }
    new_flags = (ifrq.ifr_flags & 0xffff) | (ifrq.ifr_flagshigh << 16);

    new_flags &= ~clear;
    new_flags |= set;

    ifrq.ifr_flags = new_flags & 0xffff;
    ifrq.ifr_flagshigh = new_flags >> 16;

    if (ioctl(s, SIOCSIFFLAGS, &ifrq) < 0) {
	Perror("[%s] IFACE: ioctl(%s, %s)", b->name, b->iface.ifname, "SIOCSIFFLAGS");
	close(s);
	return;
    }
    close(s);
}

#if defined(__KAME__) && !defined(NOINET6)
static void
add_scope(struct sockaddr *sa, int ifindex)
{
  struct sockaddr_in6 *sa6;

  if (sa->sa_family != AF_INET6)
    return;
  sa6 = (struct sockaddr_in6 *)sa;
  if (!IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) &&
      !IN6_IS_ADDR_MC_LINKLOCAL(&sa6->sin6_addr))
    return;
  if (*(u_int16_t *)&sa6->sin6_addr.s6_addr[2] != 0)
    return;
  *(u_int16_t *)&sa6->sin6_addr.s6_addr[2] = htons(ifindex);
}
#endif

int
IfaceChangeAddr(Bund b, int add, struct u_range *self, struct u_addr *peer)
{
    struct ifaliasreq ifra;
    struct in6_aliasreq ifra6;
    struct sockaddr_in *me4, *msk4, *peer4;
    struct sockaddr_storage ssself, sspeer, ssmsk;
    int res = 0;
    int s;
    char buf[48], buf1[48];

    Log(LG_IFACE2, ("[%s] IFACE: %s address %s->%s %s %s",
	b->name, add?"Add":"Remove", u_rangetoa(self, buf, sizeof(buf)), 
	((peer != NULL)?u_addrtoa(peer, buf1, sizeof(buf1)):""),
	add?"to":"from", b->iface.ifname));

    u_rangetosockaddrs(self, &ssself, &ssmsk);
    if (peer)
	u_addrtosockaddr(peer, 0, &sspeer);

    if ((s = socket(self->addr.family, SOCK_DGRAM, 0)) < 0) {
	Perror("[%s] IFACE: Can't get socket to change interface address", b->name);
	return (s);
    }

    switch (self->addr.family) {
      case AF_INET:
	memset(&ifra, '\0', sizeof(ifra));
	strlcpy(ifra.ifra_name, b->iface.ifname, sizeof(ifra.ifra_name));

	me4 = (struct sockaddr_in *)&ifra.ifra_addr;
	memcpy(me4, &ssself, sizeof(*me4));

	msk4 = (struct sockaddr_in *)&ifra.ifra_mask;
	memcpy(msk4, &ssmsk, sizeof(*msk4));

	peer4 = (struct sockaddr_in *)&ifra.ifra_broadaddr;
	if (peer == NULL || peer->family == AF_UNSPEC) {
    	    peer4->sin_family = AF_INET;
    	    peer4->sin_len = sizeof(*peer4);
    	    peer4->sin_addr.s_addr = INADDR_NONE;
	} else
    	    memcpy(peer4, &sspeer, sizeof(*peer4));

	res = ioctl(s, add?SIOCAIFADDR:SIOCDIFADDR, &ifra);
	if (res == -1) {
	    Perror("[%s] IFACE: %s IPv4 address %s %s failed", 
		b->name, add?"Adding":"Removing", add?"to":"from", b->iface.ifname);
	}
	break;

      case AF_INET6:
	memset(&ifra6, '\0', sizeof(ifra6));
	strlcpy(ifra6.ifra_name, b->iface.ifname, sizeof(ifra6.ifra_name));

	memcpy(&ifra6.ifra_addr, &ssself, sizeof(ifra6.ifra_addr));
	memcpy(&ifra6.ifra_prefixmask, &ssmsk, sizeof(ifra6.ifra_prefixmask));
	if (peer == NULL || peer->family == AF_UNSPEC)
    	    ifra6.ifra_dstaddr.sin6_family = AF_UNSPEC;
	else if (memcmp(&((struct sockaddr_in6 *)&ssmsk)->sin6_addr, &in6mask128,
		    sizeof(in6mask128)) == 0)
    	    memcpy(&ifra6.ifra_dstaddr, &sspeer, sizeof(ifra6.ifra_dstaddr));
	ifra6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
	ifra6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

	res = ioctl(s, add?SIOCAIFADDR_IN6:SIOCDIFADDR_IN6, &ifra6);
	if (res == -1) {
	    Perror("[%s] IFACE: %s IPv6 address %s %s failed", 
		b->name, add?"Adding":"Removing", add?"to":"from", b->iface.ifname);
	}
	break;

      default:
        res = -1;
	break;
    }
    close(s);
    return (res);
}

struct rtmsg {
  struct rt_msghdr m_rtm;
  char m_space[256];
};

static size_t
memcpy_roundup(char *cp, const void *data, size_t len)
{
  size_t padlen;

#define ROUNDUP(x) ((x) ? (1 + (((x) - 1) | (sizeof(long) - 1))) : sizeof(long))
  padlen = ROUNDUP(len);
  memcpy(cp, data, len);
  if (padlen > len)
    memset(cp + len, '\0', padlen - len);

  return padlen;
}

int
IfaceSetRoute(Bund b, int cmd, struct u_range *dst,
       struct u_addr *gw)
{
    struct rtmsg rtmes;
    int s, nb, wb;
    char *cp;
    const char *cmdstr = (cmd == RTM_ADD ? "Add" : "Delete");
    struct sockaddr_storage sadst, samask, sagw;
    char buf[48], buf1[48];

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0) {
	Perror("[%s] IFACE: Can't get route socket", b->name);
	return (-1);
    }
    memset(&rtmes, '\0', sizeof(rtmes));
    rtmes.m_rtm.rtm_version = RTM_VERSION;
    rtmes.m_rtm.rtm_type = cmd;
    rtmes.m_rtm.rtm_addrs = RTA_DST;
    rtmes.m_rtm.rtm_seq = ++gRouteSeq;
    rtmes.m_rtm.rtm_pid = gPid;
    rtmes.m_rtm.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;

    u_rangetosockaddrs(dst, &sadst, &samask);
#if defined(__KAME__) && !defined(NOINET6)
    add_scope((struct sockaddr *)&sadst, b->iface.ifindex);
#endif

    cp = rtmes.m_space;
    cp += memcpy_roundup(cp, &sadst, sadst.ss_len);
    if (gw != NULL) {
	u_addrtosockaddr(gw, 0, &sagw);
#if defined(__KAME__) && !defined(NOINET6)
	add_scope((struct sockaddr *)&sagw, b->iface.ifindex);
#endif
    	cp += memcpy_roundup(cp, &sagw, sagw.ss_len);
    	rtmes.m_rtm.rtm_addrs |= RTA_GATEWAY;
    } else if (cmd == RTM_ADD) {
    	Log(LG_ERR, ("[%s] IfaceSetRoute: gw is not set\n", b->name));
    	close(s);
    	return (-1);
    }

    if (u_rangehost(dst)) {
	rtmes.m_rtm.rtm_flags |= RTF_HOST;
    } else {
	cp += memcpy_roundup(cp, &samask, samask.ss_len);
	rtmes.m_rtm.rtm_addrs |= RTA_NETMASK;
    }

    nb = cp - (char *)&rtmes;
    rtmes.m_rtm.rtm_msglen = nb;
    wb = write(s, &rtmes, nb);
    if (wb < 0) {
    	Log(LG_ERR, ("[%s] IFACE: %s route %s %s failed: %s",
	    b->name, cmdstr, u_rangetoa(dst, buf, sizeof(buf)), 
	    ((gw != NULL)?u_addrtoa(gw, buf1, sizeof(buf1)):""),
	    (rtmes.m_rtm.rtm_errno != 0)?strerror(rtmes.m_rtm.rtm_errno):strerror(errno)));
	close(s);
	return (-1);
    }
    close(s);
    Log(LG_IFACE2, ("[%s] IFACE: %s route %s %s",
	    b->name, cmdstr, u_rangetoa(dst, buf, sizeof(buf)), 
	    ((gw != NULL)?u_addrtoa(gw, buf1, sizeof(buf1)):"")));
    return (0);
}

#ifndef USE_NG_TCPMSS
void
IfaceCorrectMSS(Mbuf pkt, uint16_t maxmss)
{
  struct ip	*iphdr;
  struct tcphdr	*tc;
  int		pktlen, hlen, olen, optlen, accumulate;
  uint16_t	*mss;
  u_char	*opt;

  if (pkt == NULL)
    return;

  iphdr = (struct ip *)MBDATAU(pkt);
  hlen = iphdr->ip_hl << 2;
  pktlen = MBLEN(pkt) - hlen;
  tc = (struct tcphdr *)(MBDATAU(pkt) + hlen);
  hlen = tc->th_off << 2;

  /* Invalid header length or header without options. */
  if (hlen <= sizeof(struct tcphdr) || hlen > pktlen)
    return;

  /* MSS option only allowed within SYN packets. */  
  if (!(tc->th_flags & TH_SYN))
    return;

  for (olen = hlen - sizeof(struct tcphdr), opt = (u_char *)(tc + 1);
	olen > 0; olen -= optlen, opt += optlen) {
    if (*opt == TCPOPT_EOL)
      break;
    else if (*opt == TCPOPT_NOP)
      optlen = 1;
    else {
      optlen = *(opt + 1);
      if (optlen <= 0 || optlen > olen)
	break;
      if (*opt == TCPOPT_MAXSEG) {
	if (optlen != TCPOLEN_MAXSEG)
	  continue;
	mss = (u_int16_t *)(opt + 2);
	if (ntohs(*mss) > maxmss) {
	  accumulate = *mss;
	  *mss = htons(maxmss);
	  accumulate -= *mss;
	  ADJUST_CHECKSUM(accumulate, tc->th_sum);
	}
      }
    }
  }
}
#endif

static int
IfaceNgIpInit(Bund b, int ready)
{
    struct ngm_connect	cn;
    char		path[NG_PATHSIZ];
    char		hook[NG_HOOKSIZ];

    if (!ready) {
	/* Dial-on-Demand mode */
	/* Use demand hook of the socket node */
	snprintf(path, sizeof(path), ".:");
	snprintf(hook, sizeof(hook), "4%d", b->id);

    } else {

	snprintf(path, sizeof(path), "[%x]:", b->nodeID);
	strcpy(hook, NG_PPP_HOOK_INET);

#ifdef USE_NG_NAT
	/* Add a nat node if configured */
	if (Enabled(&b->iface.options, IFACE_CONF_NAT)) {
	    if (IfaceInitNAT(b, path, hook))
		goto fail;
	    b->iface.nat_up = 1;
	}
#endif

	/* Add a tee node if configured */
	if (Enabled(&b->iface.options, IFACE_CONF_TEE)) {
	    if (IfaceInitTee(b, path, hook, 0))
		goto fail;
	    b->iface.tee_up = 1;
	}
  
#ifdef USE_NG_IPACCT
	/* Connect a ipacct node if configured */
	if (Enabled(&b->iface.options, IFACE_CONF_IPACCT)) {
	    if (IfaceInitIpacct(b, path, hook))
		goto fail;
	    b->iface.ipacct_up = 1;
	}
#endif	/* USE_NG_IPACCT */

#ifdef USE_NG_NETFLOW
#ifdef NG_NETFLOW_CONF_INGRESS
	/* Connect a netflow node if configured */
	if (Enabled(&b->iface.options, IFACE_CONF_NETFLOW_IN) ||
	    Enabled(&b->iface.options, IFACE_CONF_NETFLOW_OUT)) {
	    if (IfaceInitNetflow(b, path, hook, 
		Enabled(&b->iface.options, IFACE_CONF_NETFLOW_IN)?1:0,
		Enabled(&b->iface.options, IFACE_CONF_NETFLOW_OUT)?1:0))
		goto fail;
	    if (Enabled(&b->iface.options, IFACE_CONF_NETFLOW_IN))
		b->iface.nfin_up = 1;
	    if (Enabled(&b->iface.options, IFACE_CONF_NETFLOW_OUT))
		b->iface.nfout_up = 1;
	}
#else	/* NG_NETFLOW_CONF_INGRESS */
	/* Connect a netflow node if configured */
	if (Enabled(&b->iface.options, IFACE_CONF_NETFLOW_IN)) {
	    if (IfaceInitNetflow(b, path, hook, 1, 0))
		goto fail;
	    b->iface.nfin_up = 1;
	}

	if (Enabled(&b->iface.options, IFACE_CONF_NETFLOW_OUT)) {
	    if (IfaceInitNetflow(b, path, hook, 0, 1))
		goto fail;
	    b->iface.nfout_up = 1;
	}
#endif	/* NG_NETFLOW_CONF_INGRESS */
#endif	/* USE_NG_NETFLOW */

    }

#if defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF))
    if (Enabled(&b->iface.options, IFACE_CONF_TCPMSSFIX)) {
	if (IfaceInitMSS(b, path, hook))
    	    goto fail;
	b->iface.mss_up = 1;
    }
#endif

#ifdef USE_NG_BPF
    if (IfaceInitLimits(b, path, hook))
	goto fail;
#endif

    /* Connect graph to the iface node. */
    strcpy(cn.ourhook, hook);
    snprintf(cn.path, sizeof(cn.path), "%s:", b->iface.ifname);
    strcpy(cn.peerhook, NG_IFACE_HOOK_INET);
    if (NgSendMsg(gLinksCsock, path,
    	    NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    	    b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
	goto fail;
    }

    if (ready) {
#ifdef USE_NG_NETFLOW
#ifdef NG_NETFLOW_CONF_INGRESS
	if (b->iface.nfin_up || b->iface.nfout_up)
	    IfaceSetupNetflow(b, b->iface.nfin_up, b->iface.nfout_up);
#else /* NG_NETFLOW_CONF_INGRESS */
	if (b->iface.nfin_up)
	    IfaceSetupNetflow(b, 1, 0);

	if (b->iface.nfout_up)
	    IfaceSetupNetflow(b, 0, 1);
#endif /* NG_NETFLOW_CONF_INGRESS */
#endif /* USE_NG_NETFLOW */
    }

#if defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF))
    if (b->iface.mss_up)
        IfaceSetupMSS(b, MAXMSS(b->iface.mtu));
#endif
    
#ifdef USE_NG_BPF
    IfaceSetupLimits(b);
#endif

    /* OK */
    return(0);

fail:
    return(-1);
}

/*
 * IfaceNgIpShutdown()
 */

static void
IfaceNgIpShutdown(Bund b)
{
    char		path[NG_PATHSIZ];

#ifdef USE_NG_BPF
    IfaceShutdownLimits(b); /* Limits must shutdown first to save final stats. */
#endif
#ifdef USE_NG_NAT
    if (b->iface.nat_up)
	IfaceShutdownNAT(b);
    b->iface.nat_up = 0;
#endif
    if (b->iface.tee_up)
	IfaceShutdownTee(b, 0);
    b->iface.tee_up = 0;
#ifdef USE_NG_NETFLOW
#ifdef NG_NETFLOW_CONF_INGRESS
    if (b->iface.nfin_up || b->iface.nfout_up)
	IfaceShutdownNetflow(b, b->iface.nfin_up, b->iface.nfout_up);
    b->iface.nfin_up = 0;
    b->iface.nfout_up = 0;
#else /* NG_NETFLOW_CONF_INGRESS */
    if (b->iface.nfin_up)
	IfaceShutdownNetflow(b, 1, 0);
    b->iface.nfin_up = 0;
    if (b->iface.nfout_up)
	IfaceShutdownNetflow(b, 0, 1);
    b->iface.nfout_up = 0;
#endif /* NG_NETFLOW_CONF_INGRESS */
#endif
#ifdef USE_NG_IPACCT
    if (b->iface.ipacct_up)
	IfaceShutdownIpacct(b);
    b->iface.ipacct_up = 0;
#endif
#if defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF))
    if (b->iface.mss_up)
	IfaceShutdownMSS(b);
#endif
    b->iface.mss_up = 0;

    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    NgFuncDisconnect(gLinksCsock, b->name, path, NG_PPP_HOOK_INET);

    snprintf(path, sizeof(path), "%s:", b->iface.ifname);
    NgFuncDisconnect(gLinksCsock, b->name, path, NG_IFACE_HOOK_INET);
}

static int
IfaceNgIpv6Init(Bund b, int ready)
{
    struct ngm_connect	cn;
    char		path[NG_PATHSIZ];
    char		hook[NG_HOOKSIZ];

    if (!ready) {
	/* Dial-on-Demand mode */
	/* Use demand hook of the socket node */
	snprintf(path, sizeof(path), ".:");
	snprintf(hook, sizeof(hook), "6%d", b->id);
    } else {
	snprintf(path, sizeof(path), "[%x]:", b->nodeID);
	strcpy(hook, NG_PPP_HOOK_IPV6);

	/* Add a tee node if configured */
	if (Enabled(&b->iface.options, IFACE_CONF_TEE)) {
	    if (IfaceInitTee(b, path, hook, 1))
		goto fail;
	    b->iface.tee6_up = 1;
	}
  
    }

    /* Connect graph to the iface node. */
    strcpy(cn.ourhook, hook);
    snprintf(cn.path, sizeof(cn.path), "%s:", b->iface.ifname);
    strcpy(cn.peerhook, NG_IFACE_HOOK_INET6);
    if (NgSendMsg(gLinksCsock, path,
    	    NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    	    b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
	goto fail;
    }

    /* OK */
    return(0);

fail:
    return(-1);
}

/*
 * IfaceNgIpv6Shutdown()
 */

static void
IfaceNgIpv6Shutdown(Bund b)
{
    char		path[NG_PATHSIZ];

    if (b->iface.tee6_up)
	IfaceShutdownTee(b, 1);
    b->iface.tee6_up = 0;

    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    NgFuncDisconnect(gLinksCsock, b->name, path, NG_PPP_HOOK_IPV6);

    snprintf(path, sizeof(path), "%s:", b->iface.ifname);
    NgFuncDisconnect(gLinksCsock, b->name, path, NG_IFACE_HOOK_INET6);
}

#ifdef USE_NG_NAT
static int
IfaceInitNAT(Bund b, char *path, char *hook)
{
    NatState      const nat = &b->iface.nat;
    struct ngm_mkpeer	mp;
    struct ngm_name	nm;
    struct in_addr	ip;
#ifdef NG_NAT_LOG
    struct ng_nat_mode	mode;
#endif  
    Log(LG_IFACE2, ("[%s] IFACE: Connecting NAT", b->name));
  
    strcpy(mp.type, NG_NAT_NODE_TYPE);
    strcpy(mp.ourhook, hook);
    strcpy(mp.peerhook, NG_NAT_HOOK_IN);
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
      Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
	b->name, NG_NAT_NODE_TYPE, path, mp.ourhook, strerror(errno)));
      return(-1);
    }
    strlcat(path, ".", NG_PATHSIZ);
    strlcat(path, hook, NG_PATHSIZ);
    snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-nat", gPid, b->name);
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
      Log(LG_ERR, ("[%s] can't name %s node: %s",
	b->name, NG_NAT_NODE_TYPE, strerror(errno)));
      return(-1);
    }
    strcpy(hook, NG_NAT_HOOK_OUT);

    /* Set NAT IP */
    if (u_addrempty(&nat->alias_addr)) {
	ip.s_addr = 1; // Set something just to make it ready
    } else {
	ip = nat->alias_addr.u.ip4;
    }
    if (NgSendMsg(gLinksCsock, path,
	    NGM_NAT_COOKIE, NGM_NAT_SET_IPADDR, &ip, sizeof(ip)) < 0) {
	Log(LG_ERR, ("[%s] can't set NAT ip: %s",
    	    b->name, strerror(errno)));
    }

#ifdef NG_NAT_LOG
    /* Set NAT mode */
    mode.flags = 0;
    if (Enabled(&nat->options, NAT_CONF_LOG))
	mode.flags |= NG_NAT_LOG;
    if (!Enabled(&nat->options, NAT_CONF_INCOMING))
	mode.flags |= NG_NAT_DENY_INCOMING;
    if (Enabled(&nat->options, NAT_CONF_SAME_PORTS))
	mode.flags |= NG_NAT_SAME_PORTS;
    if (Enabled(&nat->options, NAT_CONF_UNREG_ONLY))
	mode.flags |= NG_NAT_UNREGISTERED_ONLY;
    
    mode.mask = NG_NAT_LOG | NG_NAT_DENY_INCOMING | 
	NG_NAT_SAME_PORTS | NG_NAT_UNREGISTERED_ONLY;
    if (NgSendMsg(gLinksCsock, path,
	    NGM_NAT_COOKIE, NGM_NAT_SET_MODE, &mode, sizeof(mode)) < 0) {
	Log(LG_ERR, ("[%s] can't set NAT mode: %s",
    	    b->name, strerror(errno)));
    }

    /* Set NAT target IP */
    if (!u_addrempty(&nat->target_addr)) {
	ip = nat->target_addr.u.ip4;
	if (NgSendMsg(gLinksCsock, path,
		NGM_NAT_COOKIE, NGM_NAT_SET_IPADDR, &ip, sizeof(ip)) < 0) {
	    Log(LG_ERR, ("[%s] can't set NAT target IP: %s",
    		b->name, strerror(errno)));
	}
    }
#endif

    return(0);
}

static int
IfaceSetupNAT(Bund b)
{
    NatState	const nat = &b->iface.nat;
    char	path[NG_PATHSIZ];
    int k;

    if (u_addrempty(&nat->alias_addr)) {
	snprintf(path, sizeof(path), "mpd%d-%s-nat:", gPid, b->name);
	if (NgSendMsg(gLinksCsock, path,
    		NGM_NAT_COOKIE, NGM_NAT_SET_IPADDR,
		&b->iface.self_addr.addr.u.ip4,
		sizeof(b->iface.self_addr.addr.u.ip4)) < 0) {
	    Log(LG_ERR, ("[%s] can't set NAT ip: %s",
    		b->name, strerror(errno)));
	    return (-1);
	}
    }
#ifdef NG_NAT_DESC_LENGTH
    /* redirect-port */
    for(k = 0; k < NM_PORT; k++) {
      if(nat->nrpt_id[k]) {
	if (NgSendMsg(gLinksCsock, path,
		NGM_NAT_COOKIE, NGM_NAT_REDIRECT_PORT, &nat->nrpt[k],
		sizeof(struct ng_nat_redirect_port)) < 0) {
	    Log(LG_ERR, ("[%s] can't set NAT redirect-port: %s",
		b->name, strerror(errno)));
	}
      }
    }
    /* redirect-addr */
    for(k = 0; k < NM_ADDR; k++) {
      if(nat->nrad_id[k]) {
	if (NgSendMsg(gLinksCsock, path,
		NGM_NAT_COOKIE, NGM_NAT_REDIRECT_ADDR, &nat->nrad[k],
		sizeof(struct ng_nat_redirect_addr)) < 0) {
	    Log(LG_ERR, ("[%s] can't set NAT redirect-addr: %s",
		b->name, strerror(errno)));
	}
      }
    }
    /* redirect-proto */
    for(k = 0; k < NM_PROTO; k++) {
      if(nat->nrpr_id[k]) {
	if (NgSendMsg(gLinksCsock, path,
		NGM_NAT_COOKIE, NGM_NAT_REDIRECT_PROTO, &nat->nrpr[k],
		sizeof(struct ng_nat_redirect_proto)) < 0) {
	    Log(LG_ERR, ("[%s] can't set NAT redirect-proto: %s",
		b->name, strerror(errno)));
	}
      }
    }
#endif
    return (0);
}

static void
IfaceShutdownNAT(Bund b)
{
    char	path[NG_PATHSIZ];

    snprintf(path, sizeof(path), "mpd%d-%s-nat:", gPid, b->name);
    NgFuncShutdownNode(gLinksCsock, b->name, path);
}
#endif /* USE_NG_NAT */

static int
IfaceInitTee(Bund b, char *path, char *hook, int v6)
{
    struct ngm_mkpeer	mp;
    struct ngm_name	nm;

    Log(LG_IFACE2, ("[%s] IFACE: Connecting tee%s", b->name, v6?"6":""));
  
    strcpy(mp.type, NG_TEE_NODE_TYPE);
    strcpy(mp.ourhook, hook);
    strcpy(mp.peerhook, NG_TEE_HOOK_RIGHT);
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
      Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
	b->name, NG_TEE_NODE_TYPE, path, mp.ourhook, strerror(errno)));
      return(-1);
    }
    strlcat(path, ".", NG_PATHSIZ);
    strlcat(path, hook, NG_PATHSIZ);
    snprintf(nm.name, sizeof(nm.name), "%s-tee%s", b->iface.ifname, v6?"6":"");
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
      Log(LG_ERR, ("[%s] can't name %s node: %s",
	b->name, NG_TEE_NODE_TYPE, strerror(errno)));
      return(-1);
    }
    strcpy(hook, NG_TEE_HOOK_LEFT);

    return(0);
}

static void
IfaceShutdownTee(Bund b, int v6)
{
    char	path[NG_PATHSIZ];

    snprintf(path, sizeof(path), "%s-tee%s:", b->iface.ifname, v6?"6":"");
    NgFuncShutdownNode(gLinksCsock, b->name, path);
}

#ifdef USE_NG_IPACCT
static int
IfaceInitIpacct(Bund b, char *path, char *hook)
{
    struct ngm_mkpeer	mp;
    struct ngm_name	nm;
    struct ngm_connect  cn;
    char		path1[NG_PATHSIZ];
    struct {
	struct ng_ipacct_mesg m;
	int		data;
    } ipam;

    Log(LG_IFACE2, ("[%s] IFACE: Connecting ipacct", b->name));
  
    strcpy(mp.type, NG_TEE_NODE_TYPE);
    strcpy(mp.ourhook, hook);
    strcpy(mp.peerhook, NG_TEE_HOOK_RIGHT);
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
      Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
	b->name, NG_TEE_NODE_TYPE, path, mp.ourhook, strerror(errno)));
      return(-1);
    }
    strlcat(path, ".", NG_PATHSIZ);
    strlcat(path, hook, NG_PATHSIZ);
    snprintf(nm.name, sizeof(nm.name), "%s_acct_tee", b->iface.ifname);
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
      Log(LG_ERR, ("[%s] can't name %s node: %s",
	b->name, NG_TEE_NODE_TYPE, strerror(errno)));
      return(-1);
    }
    strcpy(hook, NG_TEE_HOOK_LEFT);

    strcpy(mp.type, NG_IPACCT_NODE_TYPE);
    strcpy(mp.ourhook, NG_TEE_HOOK_RIGHT2LEFT);
    snprintf(mp.peerhook, sizeof(mp.peerhook), "%s_in", b->iface.ifname);
    if (NgSendMsg(gLinksCsock, path,
	NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
      Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
	b->name, NG_IPACCT_NODE_TYPE, path, mp.ourhook, strerror(errno)));
      return(-1);
    }
    snprintf(path1, sizeof(path1), "%s.%s", path, NG_TEE_HOOK_RIGHT2LEFT);
    snprintf(nm.name, sizeof(nm.name), "%s_ip_acct", b->iface.ifname);
    if (NgSendMsg(gLinksCsock, path1,
	NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
      Log(LG_ERR, ("[%s] can't name %s node: %s",
	b->name, NG_IPACCT_NODE_TYPE, strerror(errno)));
      return(-1);
    }
    strcpy(cn.ourhook, NG_TEE_HOOK_LEFT2RIGHT);
    strcpy(cn.path, NG_TEE_HOOK_RIGHT2LEFT);
    snprintf(cn.peerhook, sizeof(cn.peerhook), "%s_out", b->iface.ifname);
    if (NgSendMsg(gLinksCsock, path, NGM_GENERIC_COOKIE, NGM_CONNECT, &cn,
	sizeof(cn)) < 0) {
      Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s", 
        b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
      return (-1);
    }
    
    snprintf(ipam.m.hname, sizeof(ipam.m.hname), "%s_in", b->iface.ifname);
    ipam.data = DLT_RAW;
    if (NgSendMsg(gLinksCsock, path1, NGM_IPACCT_COOKIE, NGM_IPACCT_SETDLT, 
	&ipam, sizeof(ipam)) < 0) {
      Log(LG_ERR, ("[%s] can't set DLT \"%s\"->\"%s\": %s", 
        b->name, path, ipam.m.hname, strerror(errno)));
      return (-1);
    }
    ipam.data = 10000;
    if (NgSendMsg(gLinksCsock, path1, NGM_IPACCT_COOKIE, NGM_IPACCT_STHRS, 
	&ipam, sizeof(ipam)) < 0) {
      Log(LG_ERR, ("[%s] can't set DLT \"%s\"->\"%s\": %s", 
        b->name, path, ipam.m.hname, strerror(errno)));
      return (-1);
    }
    
    snprintf(ipam.m.hname, sizeof(ipam.m.hname), "%s_out", b->iface.ifname);
    ipam.data = DLT_RAW;
    if (NgSendMsg(gLinksCsock, path1, NGM_IPACCT_COOKIE, NGM_IPACCT_SETDLT, 
	&ipam, sizeof(ipam)) < 0) {
      Log(LG_ERR, ("[%s] can't set DLT \"%s\"->\"%s\": %s", 
        b->name, path, ipam.m.hname, strerror(errno)));
      return (-1);
    }
    ipam.data = 10000;
    if (NgSendMsg(gLinksCsock, path1, NGM_IPACCT_COOKIE, NGM_IPACCT_STHRS, 
	&ipam, sizeof(ipam)) < 0) {
      Log(LG_ERR, ("[%s] can't set DLT \"%s\"->\"%s\": %s", 
        b->name, path, ipam.m.hname, strerror(errno)));
      return (-1);
    }

    return(0);
}

static void
IfaceShutdownIpacct(Bund b)
{
    char	path[NG_PATHSIZ];

    snprintf(path, sizeof(path), "%s_acct_tee:", b->iface.ifname);
    NgFuncShutdownNode(gLinksCsock, b->name, path);
}
#endif

#ifdef USE_NG_NETFLOW
static int
IfaceInitNetflow(Bund b, char *path, char *hook, char in, char out)
{
    struct ngm_connect	cn;
    int nif;

#ifdef NG_NETFLOW_CONF_INGRESS
    nif = gNetflowIface + b->id;
#else
    nif = gNetflowIface + b->id*2 + out;
#endif

    Log(LG_IFACE2, ("[%s] IFACE: Connecting netflow (%s)", b->name, out?"out":"in"));
  
    /* Create global ng_netflow(4) node if not yet. */
    if (gNetflowNodeID == 0) {
	if (NgFuncInitGlobalNetflow())
	    return(-1);
    }

    /* Connect ng_netflow(4) node to the ng_bpf(4)/ng_tee(4) node. */
    strcpy(cn.ourhook, hook);
    snprintf(cn.path, sizeof(cn.path), "[%x]:", gNetflowNodeID);
#ifndef NG_NETFLOW_CONF_INGRESS
    if (out) {
	snprintf(cn.peerhook, sizeof(cn.peerhook), "%s%d", NG_NETFLOW_HOOK_OUT,
	    nif);
    } else {
#endif
	snprintf(cn.peerhook, sizeof(cn.peerhook), "%s%d", NG_NETFLOW_HOOK_DATA,
	    nif);
#ifndef NG_NETFLOW_CONF_INGRESS
    }
#endif
    if (NgSendMsg(gLinksCsock, path, NGM_GENERIC_COOKIE, NGM_CONNECT, &cn,
	sizeof(cn)) < 0) {
      Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s", 
        b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
      return (-1);
    }
    strlcat(path, ".", NG_PATHSIZ);
    strlcat(path, hook, NG_PATHSIZ);
#ifndef NG_NETFLOW_CONF_INGRESS
    if (out) {
	snprintf(hook, NG_HOOKSIZ, "%s%d", NG_NETFLOW_HOOK_DATA, nif);
    } else {
#endif
	snprintf(hook, NG_HOOKSIZ, "%s%d", NG_NETFLOW_HOOK_OUT, nif);
#ifndef NG_NETFLOW_CONF_INGRESS
    }
#endif
    return (0);
}

static int
IfaceSetupNetflow(Bund b, char in, char out)
{
    char path[NG_PATHSIZ];
    struct ng_netflow_setdlt	 nf_setdlt;
    struct ng_netflow_setifindex nf_setidx;
#ifdef NG_NETFLOW_CONF_INGRESS
    struct ng_netflow_setconfig  nf_setconf;
#endif
    int nif;

#ifdef NG_NETFLOW_CONF_INGRESS
    nif = gNetflowIface + b->id;
#else
    nif = gNetflowIface + b->id*2 + out;
#endif
    
    /* Configure data link type and interface index. */
    snprintf(path, sizeof(path), "[%x]:", gNetflowNodeID);
    nf_setdlt.iface = nif;
    nf_setdlt.dlt = DLT_RAW;
    if (NgSendMsg(gLinksCsock, path, NGM_NETFLOW_COOKIE, NGM_NETFLOW_SETDLT,
	&nf_setdlt, sizeof(nf_setdlt)) < 0) {
      Log(LG_ERR, ("[%s] can't configure data link type on %s: %s", b->name,
	path, strerror(errno)));
      goto fail;
    }
#ifdef NG_NETFLOW_CONF_INGRESS
    nf_setconf.iface = nif;
    nf_setconf.conf = 
	(in?NG_NETFLOW_CONF_INGRESS:0) |
	(out?NG_NETFLOW_CONF_EGRESS:0) |
	(Enabled(&b->iface.options, IFACE_CONF_NETFLOW_ONCE)?NG_NETFLOW_CONF_ONCE:0);
    if (NgSendMsg(gLinksCsock, path, NGM_NETFLOW_COOKIE, NGM_NETFLOW_SETCONFIG,
	&nf_setconf, sizeof(nf_setconf)) < 0) {
      Log(LG_ERR, ("[%s] can't set config on %s: %s", b->name,
	path, strerror(errno)));
      goto fail;
    }
#endif
#ifndef NG_NETFLOW_CONF_INGRESS
    if (!out) {
#endif
	nf_setidx.iface = nif;
	nf_setidx.index = if_nametoindex(b->iface.ifname);
	if (NgSendMsg(gLinksCsock, path, NGM_NETFLOW_COOKIE, NGM_NETFLOW_SETIFINDEX,
	    &nf_setidx, sizeof(nf_setidx)) < 0) {
    	  Log(LG_ERR, ("[%s] can't configure interface index on %s: %s", b->name,
	    path, strerror(errno)));
    	  goto fail;
	}
#ifndef NG_NETFLOW_CONF_INGRESS
    }
#endif

    return 0;
fail:
    return -1;
}

static void
IfaceShutdownNetflow(Bund b, char in, char out)
{
    char	path[NG_PATHSIZ];
    char	hook[NG_HOOKSIZ];
    int nif;

#ifdef NG_NETFLOW_CONF_INGRESS
    nif = gNetflowIface + b->id;
#else
    nif = gNetflowIface + b->id*2 + out;
#endif

    snprintf(path, NG_PATHSIZ, "[%x]:", gNetflowNodeID);
    snprintf(hook, NG_HOOKSIZ, "%s%d", NG_NETFLOW_HOOK_DATA, nif);
    NgFuncDisconnect(gLinksCsock, b->name, path, hook);
    snprintf(hook, NG_HOOKSIZ, "%s%d", NG_NETFLOW_HOOK_OUT, nif);
    NgFuncDisconnect(gLinksCsock, b->name, path, hook);
}
#endif

#if defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF))
static int
IfaceInitMSS(Bund b, char *path, char *hook)
{
	struct ngm_mkpeer	mp;
	struct ngm_name		nm;
#ifndef USE_NG_TCPMSS
	struct ngm_connect	cn;
#endif

	Log(LG_IFACE2, ("[%s] IFACE: Connecting tcpmssfix", b->name));
  
#ifdef USE_NG_TCPMSS
	/* Create ng_tcpmss(4) node. */
	strcpy(mp.type, NG_TCPMSS_NODE_TYPE);
	strlcpy(mp.ourhook, hook, sizeof(mp.ourhook));
	strcpy(mp.peerhook, "in");
	if (NgSendMsg(gLinksCsock, path,
    		NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
    	    Log(LG_ERR, ("can't create %s node at \"%s\"->\"%s\": %s", 
    		NG_TCPMSS_NODE_TYPE, path, mp.ourhook, strerror(errno)));
	    goto fail;
	}

	strlcat(path, ".", NG_PATHSIZ);
	strlcat(path, hook, NG_PATHSIZ);
	snprintf(hook, NG_HOOKSIZ, "out");

	/* Set the new node's name. */
	snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-mss", gPid, b->name);
	if (NgSendMsg(gLinksCsock, path,
    		NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
    	    Log(LG_ERR, ("can't name %s node: %s", NG_TCPMSS_NODE_TYPE,
    		strerror(errno)));
	    goto fail;
	}

#else
    /* Create a bpf node for SYN detection. */
    strcpy(mp.type, NG_BPF_NODE_TYPE);
    strlcpy(mp.ourhook, hook, sizeof(mp.ourhook));
    strcpy(mp.peerhook, "ppp");
    if (NgSendMsg(gLinksCsock, path,
	    NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
    	Log(LG_ERR, ("can't create %s node at \"%s\"->\"%s\": %s", 
    	    NG_BPF_NODE_TYPE, path, mp.ourhook, strerror(errno)));
	goto fail;
    }

    strlcat(path, ".", NG_PATHSIZ);
    strlcat(path, hook, NG_PATHSIZ);
    strcpy(hook, "iface");

    /* Set the new node's name. */
    snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-mss", gPid, b->name);
    if (NgSendMsg(gLinksCsock, path,
	    NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
    	Log(LG_ERR, ("can't name tcpmssfix %s node: %s", NG_BPF_NODE_TYPE,
    	    strerror(errno)));
	goto fail;
    }

    /* Connect to the bundle socket node. */
    strlcpy(cn.path, path, sizeof(cn.path));
    snprintf(cn.ourhook, sizeof(cn.ourhook), "i%d", b->id);
    strcpy(cn.peerhook, MPD_HOOK_TCPMSS_IN);
    if (NgSendMsg(gLinksCsock, ".:", NGM_GENERIC_COOKIE, NGM_CONNECT, &cn,
    	    sizeof(cn)) < 0) {
    	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s", 
    	    b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
    	goto fail;
    }

    strlcpy(cn.path, path, sizeof(cn.path));
    snprintf(cn.ourhook, sizeof(cn.ourhook), "o%d", b->id);
    strcpy(cn.peerhook, MPD_HOOK_TCPMSS_OUT);
    if (NgSendMsg(gLinksCsock, ".:", NGM_GENERIC_COOKIE, NGM_CONNECT, &cn,
    	    sizeof(cn)) < 0) {
    	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s", 
    	    b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
    	goto fail;
    }
#endif /* USE_NG_TCPMSS */

    return (0);
fail:
    return (-1);
}

/*
 * BundConfigMSS()
 *
 * Configure the tcpmss node to reduce MSS to given value.
 */

static void
IfaceSetupMSS(Bund b, uint16_t maxMSS)
{
#ifdef USE_NG_TCPMSS
  struct	ng_tcpmss_config tcpmsscfg;
  char		path[NG_PATHSIZ];

  snprintf(path, sizeof(path), "mpd%d-%s-mss:", gPid, b->name);

  /* Send configure message. */
  memset(&tcpmsscfg, 0, sizeof(tcpmsscfg));
  tcpmsscfg.maxMSS = maxMSS;

  snprintf(tcpmsscfg.inHook, sizeof(tcpmsscfg.inHook), "in");
  snprintf(tcpmsscfg.outHook, sizeof(tcpmsscfg.outHook), "out");
  if (NgSendMsg(gLinksCsock, path, NGM_TCPMSS_COOKIE, NGM_TCPMSS_CONFIG,
      &tcpmsscfg, sizeof(tcpmsscfg)) < 0) {
    Log(LG_ERR, ("[%s] can't configure %s node program: %s", b->name,
      NG_TCPMSS_NODE_TYPE, strerror(errno)));
  }
  snprintf(tcpmsscfg.inHook, sizeof(tcpmsscfg.inHook), "out");
  snprintf(tcpmsscfg.outHook, sizeof(tcpmsscfg.outHook), "in");
  if (NgSendMsg(gLinksCsock, path, NGM_TCPMSS_COOKIE, NGM_TCPMSS_CONFIG,
      &tcpmsscfg, sizeof(tcpmsscfg)) < 0) {
    Log(LG_ERR, ("[%s] can't configure %s node program: %s", b->name,
      NG_TCPMSS_NODE_TYPE, strerror(errno)));
  }
#else
    union {
	u_char			buf[NG_BPF_HOOKPROG_SIZE(TCPSYN_PROG_LEN)];
	struct ng_bpf_hookprog	hprog;
    }				u;
    struct ng_bpf_hookprog	*const hp = &u.hprog;
    char			hook[NG_HOOKSIZ];

    /* Setup programs for ng_bpf hooks */
    snprintf(hook, sizeof(hook), "i%d", b->id);

    memset(&u, 0, sizeof(u));
    strcpy(hp->thisHook, "ppp");
    hp->bpf_prog_len = TCPSYN_PROG_LEN;
    memcpy(&hp->bpf_prog, &gTCPSYNProg,
        TCPSYN_PROG_LEN * sizeof(*gTCPSYNProg));
    strcpy(hp->ifMatch, MPD_HOOK_TCPMSS_IN);
    strcpy(hp->ifNotMatch, "iface");

    if (NgSendMsg(gLinksCsock, hook, NGM_BPF_COOKIE,
	    NGM_BPF_SET_PROGRAM, hp, NG_BPF_HOOKPROG_SIZE(hp->bpf_prog_len)) < 0) {
	Log(LG_ERR, ("[%s] can't set %s node program: %s",
    	    b->name, NG_BPF_NODE_TYPE, strerror(errno)));
    }

    memset(&u, 0, sizeof(u));
    strcpy(hp->thisHook, MPD_HOOK_TCPMSS_IN);
    hp->bpf_prog_len = NOMATCH_PROG_LEN;
    memcpy(&hp->bpf_prog, &gNoMatchProg,
        NOMATCH_PROG_LEN * sizeof(*gNoMatchProg));
    strcpy(hp->ifMatch, "ppp");
    strcpy(hp->ifNotMatch, "ppp");

    if (NgSendMsg(gLinksCsock, hook, NGM_BPF_COOKIE,
	    NGM_BPF_SET_PROGRAM, hp, NG_BPF_HOOKPROG_SIZE(hp->bpf_prog_len)) < 0) {
	Log(LG_ERR, ("[%s] can't set %s node program: %s",
    	    b->name, NG_BPF_NODE_TYPE, strerror(errno)));
    }

    snprintf(hook, sizeof(hook), "o%d", b->id);
    memset(&u, 0, sizeof(u));
    strcpy(hp->thisHook, "iface");
    hp->bpf_prog_len = TCPSYN_PROG_LEN;
    memcpy(&hp->bpf_prog, &gTCPSYNProg,
        TCPSYN_PROG_LEN * sizeof(*gTCPSYNProg));
    strcpy(hp->ifMatch, MPD_HOOK_TCPMSS_OUT);
    strcpy(hp->ifNotMatch, "ppp");

    if (NgSendMsg(gLinksCsock, hook, NGM_BPF_COOKIE,
	    NGM_BPF_SET_PROGRAM, hp, NG_BPF_HOOKPROG_SIZE(hp->bpf_prog_len)) < 0) {
	Log(LG_ERR, ("[%s] can't set %s node program: %s",
    	    b->name, NG_BPF_NODE_TYPE, strerror(errno)));
    }

    memset(&u, 0, sizeof(u));
    strcpy(hp->thisHook, MPD_HOOK_TCPMSS_OUT);
    hp->bpf_prog_len = NOMATCH_PROG_LEN;
    memcpy(&hp->bpf_prog, &gNoMatchProg,
        NOMATCH_PROG_LEN * sizeof(*gNoMatchProg));
    strcpy(hp->ifMatch, "iface");
    strcpy(hp->ifNotMatch, "iface");

    if (NgSendMsg(gLinksCsock, hook, NGM_BPF_COOKIE,
	    NGM_BPF_SET_PROGRAM, hp, NG_BPF_HOOKPROG_SIZE(hp->bpf_prog_len)) < 0) {
	Log(LG_ERR, ("[%s] can't set %s node program: %s",
    	    b->name, NG_BPF_NODE_TYPE, strerror(errno)));
    }

#endif /* USE_NG_TCPMSS */
}

static void
IfaceShutdownMSS(Bund b)
{
	char	path[NG_PATHSIZ];

#ifdef USE_NG_TCPMSS
	snprintf(path, sizeof(path), "mpd%d-%s-mss:", gPid, b->name);
	NgFuncShutdownNode(gLinksCsock, b->name, path);
#else
	snprintf(path, sizeof(path), "i%d", b->id);
	NgFuncShutdownNode(gLinksCsock, b->name, path);
#endif
}
#endif /* defined(USE_NG_TCPMSS) || (!defined(USE_NG_TCPMSS) && defined(USE_NG_BPF)) */

#ifdef USE_NG_BPF
static int
IfaceInitLimits(Bund b, char *path, char *hook)
{
    struct ngm_mkpeer	mp;
    struct ngm_name	nm;

    if (b->params.acl_limits[0] || b->params.acl_limits[1]) {

	Log(LG_IFACE2, ("[%s] IFACE: Connecting limits", b->name));
  
	/* Create a bpf node for traffic filtering. */
	strcpy(mp.type, NG_BPF_NODE_TYPE);
	strlcpy(mp.ourhook, hook, sizeof(mp.ourhook));
	strcpy(mp.peerhook, "ppp");
	if (NgSendMsg(gLinksCsock, path,
		NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
    	    Log(LG_ERR, ("can't create %s node at \"%s\"->\"%s\": %s", 
    		NG_BPF_NODE_TYPE, path, mp.ourhook, strerror(errno)));
	    goto fail;
	}

	strlcat(path, ".", NG_PATHSIZ);
	strlcat(path, hook, NG_PATHSIZ);
	strcpy(hook, "iface");

	b->iface.limitID = NgGetNodeID(gLinksCsock, path);
	if (b->iface.limitID == 0) {
    	    Log(LG_ERR, ("can't get limits %s node ID: %s", NG_BPF_NODE_TYPE,
    		strerror(errno)));
	}

	/* Set the new node's name. */
	snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-lim", gPid, b->name);
	if (NgSendMsg(gLinksCsock, path,
		NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
    	    Log(LG_ERR, ("can't name limits %s node: %s", NG_BPF_NODE_TYPE,
    		strerror(errno)));
	    goto fail;
	}

    }

    return (0);
fail:
    return (-1);
}

/*
 * BundConfigLimits()
 *
 * Configure the bpf & car nodes.
 */

static void
IfaceSetupLimits(Bund b)
{
#define	ACL_MAX_PROGLEN	65536
    union {
	u_char			buf[NG_BPF_HOOKPROG_SIZE(ACL_MAX_PROGLEN)];
	struct ng_bpf_hookprog	hprog;
    }				*hpu;
    struct ng_bpf_hookprog	*hp;
    struct ngm_connect  cn;
    int			i;
    
    hpu = Malloc(MB_IFACE, sizeof(*hpu));
    hp = &hpu->hprog;

    if (b->params.acl_limits[0] || b->params.acl_limits[1]) {
	char		path[NG_PATHSIZ];
	int		num, dir;

	snprintf(path, sizeof(path), "mpd%d-%s-lim:", gPid, b->name);
	
	for (dir = 0; dir < 2; dir++) {
	    char	inhook[2][NG_HOOKSIZ];
	    char	inhookn[2][NG_HOOKSIZ];
	    char	outhook[NG_HOOKSIZ];
	    struct acl	*l;

	    if (dir == 0) {
		strcpy(inhook[0], "ppp");
		strcpy(outhook, "iface");
	    } else {
		strcpy(inhook[0], "iface");
		strcpy(outhook, "ppp");
	    }
	    strcpy(inhook[1], "");
	    num = 0;
	    for (l = b->params.acl_limits[dir]; l; l = l->next) {
	        char		str[ACL_LEN];
#define	ACL_MAX_PARAMS	7	/* one more then max number of arguments */
	        int		ac;
	        char		*av[ACL_MAX_PARAMS];
		int		p;
		char		stathook[NG_HOOKSIZ];
		struct svcs	*ss = NULL;
		struct svcssrc	*sss = NULL;

		Log(LG_IFACE2, ("[%s] IFACE: limit %s#%d%s%s: '%s'",
        	    b->name, (dir?"out":"in"), l->number,
		    ((l->name[0])?"#":""), l->name, l->rule));
		strlcpy(str, l->rule, sizeof(str));
    		ac = ParseLine(str, av, ACL_MAX_PARAMS, 0);
	        if (ac < 1 || ac >= ACL_MAX_PARAMS) {
		    Log(LG_ERR, ("[%s] IFACE: incorrect limit: '%s'",
    			b->name, l->rule));
		    continue;
		}
		
		stathook[0] = 0;
	    	memset(hpu, 0, sizeof(hpu));
		/* Prepare filter */
		if (strcasecmp(av[0], "all") == 0) {
		    hp->bpf_prog_len = MATCH_PROG_LEN;
		    memcpy(&hp->bpf_prog, &gMatchProg,
    		        MATCH_PROG_LEN * sizeof(*gMatchProg));
		} else if (strncasecmp(av[0], "flt", 3) == 0) {
		    struct acl  *f;
		    int		flt;
		    
		    flt = atoi(av[0] + 3);
		    if (flt <= 0 || flt > ACL_FILTERS) {
			Log(LG_ERR, ("[%s] IFACE: incorrect filter number: '%s'",
    			    b->name, av[0]));
		    } else if ((f = b->params.acl_filters[flt - 1]) == NULL &&
			(f = acl_filters[flt - 1]) == NULL) {
			Log(LG_ERR, ("[%s] IFACE: Undefined filter: '%s'",
    			    b->name, av[0]));
		    } else {
			struct bpf_program pr;
		    	char		*buf;
		    	int		bufbraces;

#define ACL_BUF_SIZE	256*1024
			buf = Malloc(MB_IFACE, ACL_BUF_SIZE);
			buf[0] = 0;
			bufbraces = 0;
			while (f) {
			    char	*b1, *b2, *sbuf;
			    sbuf = Mstrdup(MB_IFACE, f->rule);
			    b2 = sbuf;
			    b1 = strsep(&b2, " ");
			    if (b2 != NULL) {
			        if (strcasecmp(b1, "match") == 0) {
			    	    strlcat(buf, "( ", ACL_BUF_SIZE);
				    strlcat(buf, b2, ACL_BUF_SIZE);
				    strlcat(buf, " ) ", ACL_BUF_SIZE);
				    if (f->next) {
				        strlcat(buf, "|| ( ", ACL_BUF_SIZE);
				        bufbraces++;
				    }
				} else if (strcasecmp(b1, "nomatch") == 0) {
				    strlcat(buf, "( not ( ", ACL_BUF_SIZE);
				    strlcat(buf, b2, ACL_BUF_SIZE);
				    strlcat(buf, " ) ) ", ACL_BUF_SIZE);
				    if (f->next) {
				        strlcat(buf, "&& ( ", ACL_BUF_SIZE);
				        bufbraces++;
				    }
				} else {
			    	    Log(LG_ERR, ("[%s] IFACE: filter action '%s' is unknown",
        		    		b->name, b1));
				}
			    };
			    Freee(sbuf);
			    f = f->next;
			}
			for (i = 0; i < bufbraces; i++)
			    strlcat(buf, ") ", ACL_BUF_SIZE);
			Log(LG_IFACE2, ("[%s] IFACE: flt%d: '%s'",
        		    b->name, flt, buf));
			
			if (pcap_compile_nopcap((u_int)-1, DLT_RAW, &pr, buf, 1, 0xffffff00)) {
			    Log(LG_ERR, ("[%s] IFACE: filter '%s' compilation error",
    			        b->name, av[0]));
			    /* Incorrect matches nothing. */
			    hp->bpf_prog_len = NOMATCH_PROG_LEN;
			    memcpy(&hp->bpf_prog, &gNoMatchProg,
    		    		NOMATCH_PROG_LEN * sizeof(*gNoMatchProg));
			} else if (pr.bf_len > ACL_MAX_PROGLEN) {
			    Log(LG_ERR, ("[%s] IFACE: filter '%s' is too long",
        		        b->name, av[0]));
			    pcap_freecode(&pr);
			    /* Incorrect matches nothing. */
			    hp->bpf_prog_len = NOMATCH_PROG_LEN;
			    memcpy(&hp->bpf_prog, &gNoMatchProg,
    		    		NOMATCH_PROG_LEN * sizeof(*gNoMatchProg));
			} else {
			    hp->bpf_prog_len = pr.bf_len;
			    memcpy(&hp->bpf_prog, pr.bf_insns,
    			        pr.bf_len * sizeof(struct bpf_insn));
			    pcap_freecode(&pr);
			}
			Freee(buf);
		    }
		} else {
		    Log(LG_ERR, ("[%s] IFACE: incorrect filter: '%s'",
    		        b->name, av[0]));
		    /* Incorrect matches nothing. */
		    hp->bpf_prog_len = NOMATCH_PROG_LEN;
		    memcpy(&hp->bpf_prog, &gNoMatchProg,
    		        NOMATCH_PROG_LEN * sizeof(*gNoMatchProg));
		}
		
		/* Prepare action */
		p = 1;
		if (ac == 1) {
		    if (!l->next) {
			strcpy(hp->ifMatch, outhook);
			strcpy(inhookn[0], "");
		    } else {
			sprintf(hp->ifMatch, "%d-%d-m", dir, num);
			sprintf(inhookn[0], "%d-%d-mi", dir, num);

			/* Connect nomatch hook to bpf itself. */
			strcpy(cn.ourhook, hp->ifMatch);
			strcpy(cn.path, path);
			strcpy(cn.peerhook, inhookn[0]);
			if (NgSendMsg(gLinksCsock, path,
		        	NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
		    	    Log(LG_ERR, ("[%s] IFACE: can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
			        b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
			}
			strcpy(stathook, inhookn[0]);
		    }
		} else if (strcasecmp(av[p], "pass") == 0) {
		    strcpy(hp->ifMatch, outhook);
		    strcpy(inhookn[0], "");
		} else if (strcasecmp(av[p], "deny") == 0) {
		    strcpy(hp->ifMatch, "deny");
		    strcpy(inhookn[0], "");
#ifdef USE_NG_CAR
		} else if ((strcasecmp(av[p], "shape") == 0) ||
			   (strcasecmp(av[p], "rate-limit") == 0)) {
		    struct ngm_mkpeer 	mp;
		    struct ng_car_bulkconf car;
		    char		tmppath[NG_PATHSIZ];

		    sprintf(hp->ifMatch, "%d-%d-m", dir, num);

		    /* Create a car node for traffic shaping. */
		    strcpy(mp.type, NG_CAR_NODE_TYPE);
		    snprintf(mp.ourhook, sizeof(mp.ourhook), "%d-%d-m", dir, num);
		    strcpy(mp.peerhook, ((dir == 0)?NG_CAR_HOOK_LOWER:NG_CAR_HOOK_UPPER));
		    if (NgSendMsg(gLinksCsock, path,
		    	    NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
		    	Log(LG_ERR, ("[%s] IFACE: can't create %s node at \"%s\"->\"%s\": %s", 
		    	    b->name, NG_CAR_NODE_TYPE, path, mp.ourhook, strerror(errno)));
		    }

		    snprintf(tmppath, sizeof(tmppath), "%s%d-%d-m", path, dir, num);

		    /* Connect car to bpf. */
		    snprintf(cn.ourhook, sizeof(cn.ourhook), "%d-%d-mi", dir, num);
		    strlcpy(cn.path, tmppath, sizeof(cn.path));
		    strcpy(cn.peerhook, ((dir == 0)?NG_CAR_HOOK_UPPER:NG_CAR_HOOK_LOWER));
		    if (NgSendMsg(gLinksCsock, path,
		            NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
		        Log(LG_ERR, ("[%s] IFACE: can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
			    b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
		    }
			
		    bzero(&car, sizeof(car));
			
		    if (strcasecmp(av[p], "shape") == 0) {
		        car.upstream.mode = NG_CAR_SHAPE;
		    } else {
		        car.upstream.mode = NG_CAR_RED;
		    }
		    p++;

		    if ((ac > p) && (av[p][0] >= '0') && (av[p][0] <= '9')) {
		        car.upstream.cir = atol(av[p]);
		        p++;
		        if ((ac > p) && (av[p][0] >= '0') && (av[p][0] <= '9')) {
		    	    car.upstream.cbs = atol(av[p]);
			    p++;
			    if ((ac > p) && (av[p][0] >= '0') && (av[p][0] <= '9')) {
			        car.upstream.ebs = atol(av[p]);
			        p++;
			    } else {
			        car.upstream.ebs = car.upstream.cbs * 2;
			    }
			} else {
			    car.upstream.cbs = car.upstream.cir / 8;
			    car.upstream.ebs = car.upstream.cbs * 2;
			}
		    } else {
		        car.upstream.cir = 8000;
		        car.upstream.cbs = car.upstream.cir / 8;
		        car.upstream.ebs = car.upstream.cbs * 2;
		    }
		    car.upstream.green_action = NG_CAR_ACTION_FORWARD;
		    car.upstream.yellow_action = NG_CAR_ACTION_FORWARD;
		    car.upstream.red_action = NG_CAR_ACTION_DROP;
			
		    car.downstream = car.upstream;
						
		    if (NgSendMsg(gLinksCsock, tmppath,
		            NGM_CAR_COOKIE, NGM_CAR_SET_CONF, &car, sizeof(car)) < 0) {
		        Log(LG_ERR, ("[%s] IFACE: can't set %s configuration: %s",
			    b->name, NG_CAR_NODE_TYPE, strerror(errno)));
		    }
			
		    if (ac > p) {
			if (strcasecmp(av[p], "pass") == 0) {
			    union {
		    		u_char	buf[NG_BPF_HOOKPROG_SIZE(MATCH_PROG_LEN)];
		    		struct ng_bpf_hookprog	hprog;
			    } hpu1;
			    struct ng_bpf_hookprog	*const hp1 = &hpu1.hprog;

			    memset(&hpu1, 0, sizeof(hpu1));
			    strcpy(hp1->ifMatch, outhook);
			    strcpy(hp1->ifNotMatch, outhook);
			    hp1->bpf_prog_len = MATCH_PROG_LEN;
			    memcpy(&hp1->bpf_prog, &gMatchProg,
    			        MATCH_PROG_LEN * sizeof(*gMatchProg));
		    	    sprintf(hp1->thisHook, "%d-%d-mi", dir, num);
			    if (NgSendMsg(gLinksCsock, path, NGM_BPF_COOKIE, NGM_BPF_SET_PROGRAM,
			    	    hp1, NG_BPF_HOOKPROG_SIZE(hp1->bpf_prog_len)) < 0) {
				Log(LG_ERR, ("[%s] IFACE: can't set %s node program: %s",
	    			    b->name, NG_BPF_NODE_TYPE, strerror(errno)));
			    }
			    			    
			    strcpy(stathook, hp1->thisHook);
			    strcpy(inhookn[0], "");
			} else {
			    Log(LG_ERR, ("[%s] IFACE: unknown action: '%s'",
    			        b->name, av[p]));
			    strcpy(inhookn[0], "");
			}
		    } else {
			sprintf(inhookn[0], "%d-%d-mi", dir, num);
			strcpy(stathook, inhookn[0]);
		    }
#endif /* USE_NG_CAR */
	        } else {
		    Log(LG_ERR, ("[%s] IFACE: unknown action: '%s'",
    		        b->name, av[1]));
		    strcpy(inhookn[0], "");
		}
		
		/* Prepare nomatch */
		if (l->next && strcasecmp(av[0], "all")) {
		    /* If there is next limit and there is possible nomatch,
		     * then pass nomatch there. */
		    sprintf(hp->ifNotMatch, "%d-%d-n", dir, num);
		    sprintf(inhookn[1], "%d-%d-ni", dir, num);

		    /* Connect nomatch hook to bpf itself. */
		    strcpy(cn.ourhook, hp->ifNotMatch);
		    strcpy(cn.path, path);
		    strcpy(cn.peerhook, inhookn[1]);
		    if (NgSendMsg(gLinksCsock, path,
		            NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
		        Log(LG_ERR, ("[%s] IFACE: can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
			    b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
		    }
		} else {
		    /* There is no next limit, pass nomatch. */
		    strcpy(hp->ifNotMatch, outhook);
		    strcpy(inhookn[1], "");
		}
		
		/* Remember how to collect stats for this limit */
		if (l->name[0]) {
		    SLIST_FOREACH(ss, &b->iface.ss[dir], next) {
			if (strcmp(ss->name, l->name) == 0)
			    break;
		    }
		    if (ss == NULL) {
			ss = Malloc(MB_IFACE, sizeof(*ss));
			strlcpy(ss->name, l->name, sizeof(ss->name));
			SLIST_INIT(&ss->src);
			SLIST_INSERT_HEAD(&b->iface.ss[dir], ss, next);
		    }
		    if (stathook[0]) {
			sss = Malloc(MB_IFACE, sizeof(*sss));
			strlcpy(sss->hook, stathook, sizeof(sss->hook));
			sss->type = SSSS_IN;
			SLIST_INSERT_HEAD(&ss->src, sss, next);
		    }
		}
		
		for (i = 0; i < 2; i++) {
		    if (inhook[i][0] != 0) {
			if (l->name[0] && !stathook[0]) {
			    sss = Malloc(MB_IFACE, sizeof(*sss));
			    strlcpy(sss->hook, inhook[i], sizeof(sss->hook));
			    sss->type = SSSS_MATCH;
			    SLIST_INSERT_HEAD(&ss->src, sss, next);
			}
		
		        strcpy(hp->thisHook, inhook[i]);
		        if (NgSendMsg(gLinksCsock, path, NGM_BPF_COOKIE, NGM_BPF_SET_PROGRAM,
		    		hp, NG_BPF_HOOKPROG_SIZE(hp->bpf_prog_len)) < 0) {
			    Log(LG_ERR, ("[%s] IFACE: can't set %s node program: %s",
	    		        b->name, NG_BPF_NODE_TYPE, strerror(errno)));
			}
		    }
		    strcpy(inhook[i], inhookn[i]);
		}

		num++;
	    }
	
	    /* Connect left hooks to output */
	    for (i = 0; i < 2; i++) {
		if (inhook[i][0] != 0) {
		    memset(hpu, 0, sizeof(*hpu));
		    strcpy(hp->thisHook, inhook[i]);
		    hp->bpf_prog_len = MATCH_PROG_LEN;
		    memcpy(&hp->bpf_prog, &gMatchProg,
    			MATCH_PROG_LEN * sizeof(*gMatchProg));
		    strcpy(hp->ifMatch, outhook);
		    strcpy(hp->ifNotMatch, outhook);
		    if (NgSendMsg(gLinksCsock, path, NGM_BPF_COOKIE, NGM_BPF_SET_PROGRAM, 
			    hp, NG_BPF_HOOKPROG_SIZE(hp->bpf_prog_len)) < 0) {
			Log(LG_ERR, ("[%s] IFACE: can't set %s node %s %s program (2): %s",
	    		    b->name, NG_BPF_NODE_TYPE, path, hp->thisHook, strerror(errno)));
		    }
		}
	    }
	}
    }
    Freee(hpu);
}

static void
IfaceShutdownLimits(Bund b)
{
    char path[NG_PATHSIZ];
    struct svcs *ss;
    struct svcssrc *sss;
    struct svcstat curstats;
    int		i;

    if (b->n_up > 0) {
	bzero(&curstats, sizeof(curstats));
    	IfaceGetStats(b, &curstats);
    	IfaceAddStats(&b->iface.prevstats, &curstats);
	IfaceFreeStats(&curstats);
    }

    if (b->params.acl_limits[0] || b->params.acl_limits[1]) {
	snprintf(path, sizeof(path), "[%x]:", b->iface.limitID);
	NgFuncShutdownNode(gLinksCsock, b->name, path);
    }

    for (i = 0; i < ACL_DIRS; i++) {
	while ((ss = SLIST_FIRST(&b->iface.ss[i])) != NULL) {
	    while ((sss = SLIST_FIRST(&ss->src)) != NULL) {
    		SLIST_REMOVE_HEAD(&ss->src, next);
		Freee(sss);
	    }
	    SLIST_REMOVE_HEAD(&b->iface.ss[i], next);
	    Freee(ss);
	}
    }
}

void
IfaceGetStats(Bund b, struct svcstat *stat)
{
    char path[NG_PATHSIZ];
    struct svcs 	*ss;
    struct svcssrc	*sss;
    int	dir;

    union {
        u_char          buf[sizeof(struct ng_mesg) + sizeof(struct ng_bpf_hookstat)];
	struct ng_mesg  reply;
    }                   u;
    struct ng_bpf_hookstat     *const hs = (struct ng_bpf_hookstat *)(void *)u.reply.data;

    snprintf(path, sizeof(path), "[%x]:", b->iface.limitID);
    for (dir = 0; dir < ACL_DIRS; dir++) {
	SLIST_FOREACH(ss, &b->iface.ss[dir], next) {
	    struct svcstatrec *ssr;
	
	    SLIST_FOREACH(ssr, &stat->stat[dir], next) {
		if (strcmp(ssr->name, ss->name) == 0)
		    break;
	    }
	    if (!ssr) {
		ssr = Malloc(MB_IFACE, sizeof(*ssr));
		strlcpy(ssr->name, ss->name, sizeof(ssr->name));
		SLIST_INSERT_HEAD(&stat->stat[dir], ssr, next);
	    }
    
	    SLIST_FOREACH(sss, &ss->src, next) {
		if (NgSendMsg(gLinksCsock, path,
    		    NGM_BPF_COOKIE, NGM_BPF_GET_STATS, sss->hook, strlen(sss->hook)+1) < 0)
    		    continue;
		if (NgRecvMsg(gLinksCsock, &u.reply, sizeof(u), NULL) < 0)
    		    continue;
		
		switch(sss->type) {
		case SSSS_IN:
		    ssr->Packets += hs->recvFrames;
		    ssr->Octets += hs->recvOctets;
		    break;
		case SSSS_MATCH:
		    ssr->Packets += hs->recvMatchFrames;
		    ssr->Octets += hs->recvMatchOctets;
		    break;
		case SSSS_NOMATCH:
		    ssr->Packets += hs->recvFrames - hs->recvMatchFrames;
		    ssr->Octets += hs->recvOctets - hs->recvMatchOctets;
		    break;
		case SSSS_OUT:
		    ssr->Packets += hs->xmitFrames;
		    ssr->Octets += hs->xmitOctets;
		    break;
		}
	    }
	}
    }
}

void
IfaceAddStats(struct svcstat *stat1, struct svcstat *stat2)
{
    struct svcstatrec   *ssr1, *ssr2;
    int                 dir;

    for (dir = 0; dir < ACL_DIRS; dir++) {
	SLIST_FOREACH(ssr2, &stat2->stat[dir], next) {
	    SLIST_FOREACH(ssr1, &stat1->stat[dir], next)
		if (strcmp(ssr1->name, ssr2->name) == 0) {
		    break;
	    }
	    if (!ssr1) {
		ssr1 = Malloc(MB_IFACE, sizeof(*ssr1));
		strlcpy(ssr1->name, ssr2->name, sizeof(ssr1->name));
		SLIST_INSERT_HEAD(&stat1->stat[dir], ssr1, next);
	    }
	    ssr1->Packets += ssr2->Packets;
	    ssr1->Octets += ssr2->Octets;
	}
    }
}

void
IfaceFreeStats(struct svcstat *stat)
{
    struct svcstatrec   *ssr;
    int                 i;

    for (i = 0; i < ACL_DIRS; i++) {
	while ((ssr = SLIST_FIRST(&stat->stat[i])) != NULL) {
    	    SLIST_REMOVE_HEAD(&stat->stat[i], next);
	    Freee(ssr);
	}
    }
}
#endif /* USE_NG_BPF */
