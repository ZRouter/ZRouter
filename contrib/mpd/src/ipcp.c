
/*
 * ipcp.c
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "ipcp.h"
#include "fsm.h"
#include "ip.h"
#include "iface.h"
#include "msg.h"
#include "ngfunc.h"
#include "ippool.h"
#include "util.h"

#include <netgraph.h>
#include <sys/mbuf.h>
#ifdef USE_NG_VJC
#include <net/slcompress.h>
#include <netgraph/ng_vjc.h>
#endif

/*
 * DEFINITIONS
 */

  #define IPCP_KNOWN_CODES	(   (1 << CODE_CONFIGREQ)	\
				  | (1 << CODE_CONFIGACK)	\
				  | (1 << CODE_CONFIGNAK)	\
				  | (1 << CODE_CONFIGREJ)	\
				  | (1 << CODE_TERMREQ)		\
				  | (1 << CODE_TERMACK)		\
				  | (1 << CODE_CODEREJ)		)

  #define TY_IPADDRS		1
  #define TY_COMPPROTO		2
  #define TY_IPADDR		3
  #define TY_PRIMARYDNS		129
  #define TY_PRIMARYNBNS	130
  #define TY_SECONDARYDNS	131
  #define TY_SECONDARYNBNS	132

  /* Keep sync with above */
  #define o2b(x)		(((x)<128)?(x):(x)-128+3)

  #define IPCP_REJECTED(p,x)	((p)->peer_reject & (1<<o2b(x)))
  #define IPCP_PEER_REJ(p,x)	do{(p)->peer_reject |= (1<<o2b(x));}while(0)

#ifdef USE_NG_VJC
  #define IPCP_VJCOMP_MIN_MAXCHAN	(NG_VJC_MIN_CHANNELS - 1)
  #define IPCP_VJCOMP_MAX_MAXCHAN	(NG_VJC_MAX_CHANNELS - 1)
  #define IPCP_VJCOMP_DEFAULT_MAXCHAN	IPCP_VJCOMP_MAX_MAXCHAN
#endif

  /* Set menu options */
  enum {
    SET_RANGES,
    SET_ENABLE,
    SET_DNS,
    SET_NBNS,
    SET_DISABLE,
    SET_ACCEPT,
    SET_DENY,
    SET_YES,
    SET_NO
  };

/*
 * INTERNAL FUNCTIONS
 */

  static void	IpcpConfigure(Fsm fp);
  static void	IpcpUnConfigure(Fsm fp);

  static u_char	*IpcpBuildConfigReq(Fsm fp, u_char *cp);
  static void	IpcpDecodeConfig(Fsm fp, FsmOption list, int num, int mode);
  static void	IpcpLayerStart(Fsm fp);
  static void	IpcpLayerFinish(Fsm fp);
  static void	IpcpLayerUp(Fsm fp);
  static void	IpcpLayerDown(Fsm fp);
  static void	IpcpFailure(Fsm fp, enum fsmfail reason);

#ifdef USE_NG_VJC
  static int	IpcpNgInitVJ(Bund b);
  static void	IpcpNgShutdownVJ(Bund b);
#endif

  static int	IpcpSetCommand(Context ctx, int ac, char *av[], void *arg);

/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab IpcpSetCmds[] = {
    { "ranges {self}[/{width}]|ippool {pool} {peer}[/{width}]|ippool {pool}",	"Allowed IP address ranges",
	IpcpSetCommand, NULL, 2, (void *) SET_RANGES },
    { "enable [opt ...]",		"Enable option",
	IpcpSetCommand, NULL, 2, (void *) SET_ENABLE},
    { "dns primary [secondary]",	"Set peer DNS servers",
	IpcpSetCommand, NULL, 2, (void *) SET_DNS},
    { "nbns primary [secondary]",	"Set peer NBNS servers",
	IpcpSetCommand, NULL, 2, (void *) SET_NBNS},
    { "disable [opt ...]",		"Disable option",
	IpcpSetCommand, NULL, 2, (void *) SET_DISABLE},
    { "accept [opt ...]",		"Accept option",
	IpcpSetCommand, NULL, 2, (void *) SET_ACCEPT},
    { "deny [opt ...]",			"Deny option",
	IpcpSetCommand, NULL, 2, (void *) SET_DENY},
    { "yes [opt ...]",			"Enable and accept option",
	IpcpSetCommand, NULL, 2, (void *) SET_YES},
    { "no [opt ...]",			"Disable and deny option",
	IpcpSetCommand, NULL, 2, (void *) SET_NO},
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static const struct fsmoptinfo	gIpcpConfOpts[] = {
    { "IPADDRS",	TY_IPADDRS,		8, 8, FALSE },
#ifdef USE_NG_VJC
    { "COMPPROTO",	TY_COMPPROTO,		4, 4, TRUE },
#endif
    { "IPADDR",		TY_IPADDR,		4, 4, TRUE },
    { "PRIDNS",		TY_PRIMARYDNS,		4, 4, TRUE },
    { "PRINBNS",	TY_PRIMARYNBNS,		4, 4, TRUE },
    { "SECDNS",		TY_SECONDARYDNS,	4, 4, TRUE },
    { "SECNBNS",	TY_SECONDARYNBNS,	4, 4, TRUE },
    { NULL }
  };

  static const struct confinfo gConfList[] = {
#ifdef USE_NG_VJC
    { 1,	IPCP_CONF_VJCOMP,	"vjcomp"	},
#endif
    { 0,	IPCP_CONF_REQPRIDNS,	"req-pri-dns"	},
    { 0,	IPCP_CONF_REQSECDNS,	"req-sec-dns"	},
    { 0,	IPCP_CONF_REQPRINBNS,	"req-pri-nbns"	},
    { 0,	IPCP_CONF_REQSECNBNS,	"req-sec-nbns"	},
    { 0,	IPCP_CONF_PRETENDIP,	"pretend-ip"	},
    { 0,	0,			NULL		},
  };

  static const struct fsmtype gIpcpFsmType = {
    "IPCP",
    PROTO_IPCP,
    IPCP_KNOWN_CODES,
    FALSE,
    LG_IPCP, LG_IPCP2,
    NULL,
    IpcpLayerUp,
    IpcpLayerDown,
    IpcpLayerStart,
    IpcpLayerFinish,
    IpcpBuildConfigReq,
    IpcpDecodeConfig,
    IpcpConfigure,
    IpcpUnConfigure,
    NULL,
    NULL,
    NULL,
    NULL,
    IpcpFailure,
    NULL,
    NULL,
    NULL,
  };

/*
 * IpcpStat()
 */

int
IpcpStat(Context ctx, int ac, char *av[], void *arg)
{
#ifdef USE_NG_VJC
  char			path[NG_PATHSIZ];
#endif
  IpcpState		const ipcp = &ctx->bund->ipcp;
  Fsm			fp = &ipcp->fsm;
#ifdef USE_NG_VJC
  union {
      u_char		buf[sizeof(struct ng_mesg) + sizeof(struct slcompress)];
      struct ng_mesg	reply;
  }			u;
  struct slcompress	*const sls = (struct slcompress *)(void *)u.reply.data;
#endif
  char			buf[48];

  Printf("[%s] %s [%s]\r\n", Pref(fp), Fsm(fp), FsmStateName(fp->state));
  Printf("Allowed IP address ranges:\r\n");
    if (ipcp->conf.self_ippool[0]) {
	Printf("\tPeer: ippool %s\r\n",
	  ipcp->conf.self_ippool);
    } else {
	Printf("\tSelf: %s\r\n",
	    u_rangetoa(&ipcp->conf.self_allow,buf,sizeof(buf)));
    }
    if (ipcp->conf.ippool[0]) {
	Printf("\tPeer: ippool %s\r\n",
	  ipcp->conf.ippool);
    } else {
	Printf("\tPeer: %s\r\n",
	  u_rangetoa(&ipcp->conf.peer_allow,buf,sizeof(buf)));
    }
  Printf("IPCP Options:\r\n");
  OptStat(ctx, &ipcp->conf.options, gConfList);
  Printf("Current addressing:\r\n");
  Printf("\tSelf: %s\r\n", inet_ntoa(ipcp->want_addr));
  Printf("\tPeer: %s\r\n", inet_ntoa(ipcp->peer_addr));
#ifdef USE_NG_VJC
  Printf("Compression:\r\n");
  Printf("\tSelf: ");
  if (ipcp->want_comp.proto != 0)
    Printf("%s, %d compression channels, CID %scompressible\r\n",
      ProtoName(ntohs(ipcp->want_comp.proto)),
      ipcp->want_comp.maxchan + 1, ipcp->want_comp.compcid ? "" : "not ");
  else
    Printf("None\r\n");
  Printf("\tPeer: ");
  if (ipcp->peer_comp.proto != 0)
    Printf("%s, %d compression channels, CID %scompressible\n",
      ProtoName(ntohs(ipcp->peer_comp.proto)),
      ipcp->peer_comp.maxchan + 1, ipcp->peer_comp.compcid ? "" : "not ");
  else
    Printf("None\r\n");
#endif /* USE_NG_VJC */
  Printf("Server info we give to peer:\r\n");
  Printf("DNS servers : %15s", inet_ntoa(ipcp->conf.peer_dns[0]));
  Printf("  %15s\r\n", inet_ntoa(ipcp->conf.peer_dns[1]));
  Printf("NBNS servers: %15s", inet_ntoa(ipcp->conf.peer_nbns[0]));
  Printf("  %15s\r\n", inet_ntoa(ipcp->conf.peer_nbns[1]));
  Printf("Server info peer gave to us:\r\n");
  Printf("DNS servers : %15s", inet_ntoa(ipcp->want_dns[0]));
  Printf("  %15s\r\n", inet_ntoa(ipcp->want_dns[1]));
  Printf("NBNS servers: %15s", inet_ntoa(ipcp->want_nbns[0]));
  Printf("  %15s\r\n", inet_ntoa(ipcp->want_nbns[1]));

#ifdef USE_NG_VJC
  /* Get VJC state */
  snprintf(path, sizeof(path), "mpd%d-%s:%s", gPid, ctx->bund->name, NG_PPP_HOOK_VJC_IP);
  if (NgFuncSendQuery(path, NGM_VJC_COOKIE, NGM_VJC_GET_STATE,
      NULL, 0, &u.reply, sizeof(u), NULL) < 0)
    return(0);

  Printf("VJ Compression:\r\n");
  Printf("\tOut comp : %d\r\n", sls->sls_compressed);
  Printf("\tOut total: %d\r\n", sls->sls_packets);
  Printf("\tMissed   : %d\r\n", sls->sls_misses);
  Printf("\tSearched : %d\r\n", sls->sls_searches);
  Printf("\tIn comp  : %d\r\n", sls->sls_compressedin);
  Printf("\tIn uncomp: %d\r\n", sls->sls_uncompressedin);
  Printf("\tIn error : %d\r\n", sls->sls_errorin);
  Printf("\tIn tossed: %d\r\n", sls->sls_tossed);
#endif /* USE_NG_VJC */
  return(0);
}

/*
 * IpcpInit()
 */

void
IpcpInit(Bund b)
{
  IpcpState		const ipcp = &b->ipcp;

  /* Init state machine */
  memset(ipcp, 0, sizeof(*ipcp));
  FsmInit(&ipcp->fsm, &gIpcpFsmType, b);

  /* Come up with a default IP address for my side of the link */
  u_rangeclear(&ipcp->conf.self_allow);
  GetAnyIpAddress(&ipcp->conf.self_allow.addr, NULL);

#ifdef USE_NG_VJC
  /* Default we want VJ comp */
  Enable(&ipcp->conf.options, IPCP_CONF_VJCOMP);
  Accept(&ipcp->conf.options, IPCP_CONF_VJCOMP);
#endif
}

/*
 * IpcpInst()
 */

void
IpcpInst(Bund b, Bund bt)
{
  IpcpState		const ipcp = &b->ipcp;

  /* Init state machine */
  memcpy(ipcp, &bt->ipcp, sizeof(*ipcp));
  FsmInst(&ipcp->fsm, &bt->ipcp.fsm, b);
}

/*
 * IpcpConfigure()
 */

static void
IpcpConfigure(Fsm fp)
{
    Bund 	b = (Bund)fp->arg;
    IpcpState	const ipcp = &b->ipcp;
    char	buf[48];

    /* FSM stuff */
    ipcp->peer_reject = 0;

    /* Get allowed IP addresses from config and/or from current bundle */
    if (ipcp->conf.self_ippool[0]) {
	if (IPPoolGet(ipcp->conf.self_ippool, &ipcp->self_allow.addr)) {
	    Log(LG_IPCP, ("[%s] IPCP: Can't get IP from pool \"%s\" for self",
		b->name, ipcp->conf.self_ippool));
	} else {
	    Log(LG_IPCP, ("[%s] IPCP: Got IP %s from pool \"%s\" for self",
		b->name,
		u_addrtoa(&ipcp->self_allow.addr, buf, sizeof(buf)),
		ipcp->conf.self_ippool));
	    ipcp->self_allow.width = 32;
	    ipcp->self_ippool_used = 1;
	}
    } else
	ipcp->self_allow = ipcp->conf.self_allow;

    if ((b->params.range_valid) && (!u_rangeempty(&b->params.range)))
	ipcp->peer_allow = b->params.range;
    else if (b->params.ippool[0]) {
	/* Get IP from pool if needed */
	if (IPPoolGet(b->params.ippool, &ipcp->peer_allow.addr)) {
	    Log(LG_IPCP, ("[%s] IPCP: Can't get IP from pool \"%s\" for peer",
		b->name, b->params.ippool));
	} else {
	    Log(LG_IPCP, ("[%s] IPCP: Got IP %s from pool \"%s\" for peer",
		b->name,
		u_addrtoa(&ipcp->peer_allow.addr, buf, sizeof(buf)),
		b->params.ippool));
	    ipcp->peer_allow.width = 32;
	    b->params.ippool_used = 1;
	}
    } else if (ipcp->conf.ippool[0]) {
	if (IPPoolGet(ipcp->conf.ippool, &ipcp->peer_allow.addr)) {
	    Log(LG_IPCP, ("[%s] IPCP: Can't get IP from pool \"%s\"",
		b->name, ipcp->conf.ippool));
	} else {
	    Log(LG_IPCP, ("[%s] IPCP: Got IP %s from pool \"%s\" for peer",
		b->name,
		u_addrtoa(&ipcp->peer_allow.addr, buf, sizeof(buf)),
		ipcp->conf.ippool));
	    ipcp->peer_allow.width = 32;
	    ipcp->ippool_used = 1;
	}
    } else
	ipcp->peer_allow = ipcp->conf.peer_allow;
    
    /* Initially request addresses as specified by config */
    u_addrtoin_addr(&ipcp->self_allow.addr, &ipcp->want_addr);
    u_addrtoin_addr(&ipcp->peer_allow.addr, &ipcp->peer_addr);

#ifdef USE_NG_VJC
    /* Van Jacobson compression */
    ipcp->peer_comp.proto = 0;
    ipcp->peer_comp.maxchan = IPCP_VJCOMP_DEFAULT_MAXCHAN;
    ipcp->peer_comp.compcid = 0;

    ipcp->want_comp.proto =
	(b->params.vjc_enable || Enabled(&ipcp->conf.options, IPCP_CONF_VJCOMP)) ?
	    htons(PROTO_VJCOMP) : 0;
    ipcp->want_comp.maxchan = IPCP_VJCOMP_MAX_MAXCHAN;

    /* If any of our links are unable to give receive error indications, we must
     tell the peer not to compress the slot-id in VJCOMP packets (cf. RFC1144).
     To be on the safe side, we always say this. */
    ipcp->want_comp.compcid = 0;
#endif

    /* DNS and NBNS servers */
    memset(&ipcp->want_dns, 0, sizeof(ipcp->want_dns));
    memset(&ipcp->want_nbns, 0, sizeof(ipcp->want_nbns));
}

/*
 * IpcpUnConfigure()
 */

static void
IpcpUnConfigure(Fsm fp)
{
    Bund 	b = (Bund)fp->arg;
    IpcpState	const ipcp = &b->ipcp;
  
    if (ipcp->self_ippool_used) {
	struct u_addr ip;
	in_addrtou_addr(&ipcp->want_addr, &ip);
	IPPoolFree(ipcp->conf.self_ippool, &ip);
	ipcp->self_ippool_used = 0;
    }
    if (b->params.ippool_used) {
	struct u_addr ip;
	in_addrtou_addr(&ipcp->peer_addr, &ip);
	IPPoolFree(b->params.ippool, &ip);
	b->params.ippool_used = 0;
    } else if (ipcp->ippool_used) {
	struct u_addr ip;
	in_addrtou_addr(&ipcp->peer_addr, &ip);
	IPPoolFree(ipcp->conf.ippool, &ip);
	ipcp->ippool_used = 0;
    }
}

/*
 * IpcpBuildConfigReq()
 */

static u_char *
IpcpBuildConfigReq(Fsm fp, u_char *cp)
{
    Bund 	b = (Bund)fp->arg;
    IpcpState	const ipcp = &b->ipcp;

    /* Put in my desired IP address */
    if (!IPCP_REJECTED(ipcp, TY_IPADDR) || ipcp->want_addr.s_addr == 0)
	cp = FsmConfValue(cp, TY_IPADDR, 4, &ipcp->want_addr.s_addr);

#ifdef USE_NG_VJC
    /* Put in my requested compression protocol */
    if (ipcp->want_comp.proto != 0 && !IPCP_REJECTED(ipcp, TY_COMPPROTO))
	cp = FsmConfValue(cp, TY_COMPPROTO, 4, &ipcp->want_comp);
#endif

  /* Request peer's DNS and NBNS servers */
  {
    const int	sopts[2][2] = { { IPCP_CONF_REQPRIDNS, IPCP_CONF_REQSECDNS },
				{ IPCP_CONF_REQPRINBNS, IPCP_CONF_REQSECNBNS }};
    const int	nopts[2][2] = { { TY_PRIMARYDNS, TY_SECONDARYDNS }, 
				{ TY_PRIMARYNBNS, TY_SECONDARYNBNS } };
    struct in_addr	*vals[2] = { ipcp->want_dns, ipcp->want_nbns };
    int			sopt, pri;

    for (sopt = 0; sopt < 2; sopt++) {
      for (pri = 0; pri < 2; pri++) {
	const int	opt = nopts[sopt][pri];

	/* Add option if we desire it and it hasn't been rejected */
	if (Enabled(&ipcp->conf.options, sopts[sopt][pri])
	    && !IPCP_REJECTED(ipcp, opt)) {
	  cp = FsmConfValue(cp, opt, 4, &vals[sopt][pri]);
	}
      }
    }
  }

/* Done */

  return(cp);
}

/*
 * IpcpLayerStart()
 *
 * Tell the lower layer (the bundle) that we need it
 */

static void
IpcpLayerStart(Fsm fp)
{
  BundNcpsStart((Bund)(fp->arg), NCP_IPCP);
}

/*
 * IpcpLayerFinish()
 *
 * Tell the lower layer (the bundle) that we no longer need it
 */

static void
IpcpLayerFinish(Fsm fp)
{
  BundNcpsFinish((Bund)(fp->arg), NCP_IPCP);
}

/*
 * IpcpLayerUp()
 *
 * Called when IPCP has reached the OPEN state
 */

static void
IpcpLayerUp(Fsm fp)
{
    Bund 			b = (Bund)fp->arg;
    IpcpState			const ipcp = &b->ipcp;
    char			ipbuf[20];
#ifdef USE_NG_VJC
    char			path[NG_PATHSIZ];
    struct ngm_vjc_config	vjc;
#endif
    struct u_addr		tmp;

    /* Determine actual address we'll use for ourselves */
    in_addrtou_addr(&ipcp->want_addr, &tmp);
    if (!IpAddrInRange(&ipcp->self_allow, &tmp)) {
	Log(LG_IPCP, ("[%s]   Note: ignoring negotiated %s IP %s,",
    	    b->name, "self", inet_ntoa(ipcp->want_addr)));
	u_addrtoin_addr(&ipcp->self_allow.addr, &ipcp->want_addr);
	Log(LG_IPCP, ("[%s]        using %s instead.",
    	    b->name, inet_ntoa(ipcp->want_addr)));
    }

    /* Determine actual address we'll use for peer */
    in_addrtou_addr(&ipcp->peer_addr, &tmp);
    if (!IpAddrInRange(&ipcp->peer_allow, &tmp)
    	    && !u_addrempty(&ipcp->peer_allow.addr)) {
	Log(LG_IPCP, ("[%s]   Note: ignoring negotiated %s IP %s,",
    	    b->name, "peer", inet_ntoa(ipcp->peer_addr)));
	u_addrtoin_addr(&ipcp->peer_allow.addr, &ipcp->peer_addr);
	Log(LG_IPCP, ("[%s]        using %s instead.",
    	    b->name, inet_ntoa(ipcp->peer_addr)));
    }

    /* Report */
    strlcpy(ipbuf, inet_ntoa(ipcp->peer_addr), sizeof(ipbuf));
    Log(LG_IPCP, ("[%s]   %s -> %s", b->name, inet_ntoa(ipcp->want_addr), ipbuf));

#ifdef USE_NG_VJC
    memset(&vjc, 0, sizeof(vjc));
    if (ntohs(ipcp->peer_comp.proto) == PROTO_VJCOMP || 
	    ntohs(ipcp->want_comp.proto) == PROTO_VJCOMP) {
  
	IpcpNgInitVJ(b);

	/* Configure VJ compression node */
	vjc.enableComp = ntohs(ipcp->peer_comp.proto) == PROTO_VJCOMP;
	vjc.enableDecomp = ntohs(ipcp->want_comp.proto) == PROTO_VJCOMP;
	vjc.maxChannel = ipcp->peer_comp.maxchan;
	vjc.compressCID = ipcp->peer_comp.compcid;
        snprintf(path, sizeof(path), "[%x]:%s", b->nodeID, NG_PPP_HOOK_VJC_IP);
	if (NgSendMsg(gLinksCsock, path,
    		NGM_VJC_COOKIE, NGM_VJC_SET_CONFIG, &vjc, sizeof(vjc)) < 0) {
	    Log(LG_ERR, ("[%s] can't config %s node: %s",
    		b->name, NG_VJC_NODE_TYPE, strerror(errno)));
	}
    }
#endif /* USE_NG_VJC */

    /* Enable IP packets in the PPP node */
    b->pppConfig.bund.enableIP = 1;
#ifdef USE_NG_VJC
    b->pppConfig.bund.enableVJCompression = vjc.enableComp;
    b->pppConfig.bund.enableVJDecompression = vjc.enableDecomp;
#endif
    NgFuncSetConfig(b);

    BundNcpsJoin(b, NCP_IPCP);
}

/*
 * IpcpLayerDown()
 *
 * Called when IPCP leaves the OPEN state
 */

static void
IpcpLayerDown(Fsm fp)
{
    Bund 	b = (Bund)fp->arg;
#ifdef USE_NG_VJC
    IpcpState	const ipcp = &b->ipcp;
#endif

    BundNcpsLeave(b, NCP_IPCP);

    /* Turn off IP packets */
    b->pppConfig.bund.enableIP = 0;
#ifdef USE_NG_VJC
    b->pppConfig.bund.enableVJCompression = 0;
    b->pppConfig.bund.enableVJDecompression = 0;
#endif
    NgFuncSetConfig(b);

#ifdef USE_NG_VJC
    if (ntohs(ipcp->peer_comp.proto) == PROTO_VJCOMP || 
	    ntohs(ipcp->want_comp.proto) == PROTO_VJCOMP) {
	IpcpNgShutdownVJ(b);
    }
#endif /* USE_NG_VJC */
}

/*
 * IpcpUp()
 */

void
IpcpUp(Bund b)
{
    FsmUp(&b->ipcp.fsm);
}

/*
 * IpcpDown()
 */

void
IpcpDown(Bund b)
{
    FsmDown(&b->ipcp.fsm);
}

/*
 * IpcpOpen()
 */

void
IpcpOpen(Bund b)
{
    FsmOpen(&b->ipcp.fsm);
}

/*
 * IpcpClose()
 */

void
IpcpClose(Bund b)
{
    FsmClose(&b->ipcp.fsm);
}

/*
 * IpcpOpenCmd()
 */

int
IpcpOpenCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to open template");
    FsmOpen(&ctx->bund->ipcp.fsm);
    return (0);
}

/*
 * IpcpCloseCmd()
 */

int
IpcpCloseCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to close template");
    FsmClose(&ctx->bund->ipcp.fsm);
    return (0);
}

/*
 * IpcpFailure()
 */

static void
IpcpFailure(Fsm fp, enum fsmfail reason)
{
    Bund 	b = (Bund)fp->arg;
    RecordLinkUpDownReason(b, NULL, 0, STR_PROTO_ERR, STR_IPCP_FAILED, FsmFailureStr(reason));
}

/*
 * IpcpDecodeConfig()
 */

static void
IpcpDecodeConfig(Fsm fp, FsmOption list, int num, int mode)
{
    Bund 	b = (Bund)fp->arg;
  IpcpState		const ipcp = &b->ipcp;
  struct in_addr	*wantip, *peerip;
  int			k;

  /* Decode each config option */
  for (k = 0; k < num; k++) {
    FsmOption	const opt = &list[k];
    FsmOptInfo	const oi = FsmFindOptInfo(gIpcpConfOpts, opt->type);

    if (!oi) {
      Log(LG_IPCP, ("[%s]   UNKNOWN[%d] len=%d", b->name, opt->type, opt->len));
      if (mode == MODE_REQ)
	FsmRej(fp, opt);
      continue;
    }
    if (!oi->supported) {
      Log(LG_IPCP, ("[%s]   %s", b->name, oi->name));
      if (mode == MODE_REQ) {
	Log(LG_IPCP, ("[%s]     Not supported", b->name));
	FsmRej(fp, opt);
      }
      continue;
    }
    if (opt->len < oi->minLen + 2 || opt->len > oi->maxLen + 2) {
      Log(LG_IPCP, ("[%s]   %s", b->name, oi->name));
      if (mode == MODE_REQ) {
	Log(LG_IPCP, ("[%s]     bogus len=%d", b->name, opt->len));
	FsmRej(fp, opt);
      }
      continue;
    }
    switch (opt->type) {
      case TY_IPADDR:
	{
	  struct in_addr	ip;
	  struct u_addr		tmp;

	  memcpy(&ip, opt->data, 4);
	  in_addrtou_addr(&ip, &tmp);
	  Log(LG_IPCP, ("[%s]   %s %s", b->name, oi->name, inet_ntoa(ip)));
	  switch (mode) {
	    case MODE_REQ:
	      if (!IpAddrInRange(&ipcp->peer_allow, &tmp) || !ip.s_addr) {
		if (ipcp->peer_addr.s_addr == 0)
		  Log(LG_IPCP, ("[%s]     no IP address available for peer!", b->name));
		if (Enabled(&ipcp->conf.options, IPCP_CONF_PRETENDIP)) {
		  Log(LG_IPCP, ("[%s]     pretending that %s is OK, will ignore",
		      b->name, inet_ntoa(ip)));
		  ipcp->peer_addr = ip;
		  FsmAck(fp, opt);
		  break;
		}
		memcpy(opt->data, &ipcp->peer_addr, 4);
		Log(LG_IPCP, ("[%s]     NAKing with %s", b->name, inet_ntoa(ipcp->peer_addr)));
		FsmNak(fp, opt);
		break;
	      }
	      Log(LG_IPCP, ("[%s]     %s is OK", b->name, inet_ntoa(ip)));
	      ipcp->peer_addr = ip;
	      FsmAck(fp, opt);
	      break;
	    case MODE_NAK:
	      {
		if (IpAddrInRange(&ipcp->self_allow, &tmp)) {
		  Log(LG_IPCP, ("[%s]     %s is OK", b->name, inet_ntoa(ip)));
		  ipcp->want_addr = ip;
		} else if (Enabled(&ipcp->conf.options, IPCP_CONF_PRETENDIP)) {
		  Log(LG_IPCP, ("[%s]     pretending that %s is OK, will ignore",
		      b->name, inet_ntoa(ip)));
		  ipcp->want_addr = ip;
		} else
		  Log(LG_IPCP, ("[%s]     %s is unacceptable", b->name, inet_ntoa(ip)));
	      }
	      break;
	    case MODE_REJ:
	      IPCP_PEER_REJ(ipcp, opt->type);
	      if (ipcp->want_addr.s_addr == 0)
		Log(LG_IPCP, ("[%s]     Problem: I need an IP address!", b->name));
	      break;
	  }
	}
	break;

#ifdef USE_NG_VJC
      case TY_COMPPROTO:
	{
	  struct ipcpvjcomp	vj;

	  memcpy(&vj, opt->data, sizeof(vj));
	  Log(LG_IPCP, ("[%s]   %s %s, %d comp. channels, %s comp-cid",
	    b->name, oi->name, ProtoName(ntohs(vj.proto)),
	    vj.maxchan + 1, vj.compcid ? "allow" : "no"));
	  switch (mode) {
	    case MODE_REQ:
	      if (!Acceptable(&ipcp->conf.options, IPCP_CONF_VJCOMP) && 
	    	  !b->params.vjc_enable) {
		FsmRej(fp, opt);
		break;
	      }
	      if (ntohs(vj.proto) == PROTO_VJCOMP
		  && vj.maxchan <= IPCP_VJCOMP_MAX_MAXCHAN
		  && vj.maxchan >= IPCP_VJCOMP_MIN_MAXCHAN) {
		ipcp->peer_comp = vj;
		FsmAck(fp, opt);
		break;
	      }
	      vj.proto = htons(PROTO_VJCOMP);
	      vj.maxchan = IPCP_VJCOMP_MAX_MAXCHAN;
	      vj.compcid = 0;
	      memcpy(opt->data, &vj, sizeof(vj));
	      FsmNak(fp, opt);
	      break;
	    case MODE_NAK:
	      if (ntohs(vj.proto) != PROTO_VJCOMP) {
		Log(LG_IPCP, ("[%s]     Can't accept proto 0x%04x",
		  b->name, (u_short) ntohs(vj.proto)));
		break;
	      }
	      if (vj.maxchan != ipcp->want_comp.maxchan) {
		if (vj.maxchan <= IPCP_VJCOMP_MAX_MAXCHAN
		    && vj.maxchan >= IPCP_VJCOMP_MIN_MAXCHAN) {
		  Log(LG_IPCP, ("[%s]     Adjusting # compression channels", b->name));
		  ipcp->want_comp.maxchan = vj.maxchan;
		} else {
		  Log(LG_IPCP, ("[%s]     Can't handle %d maxchan", b->name, vj.maxchan));
		}
	      }
	      if (vj.compcid) {
		Log(LG_IPCP, ("[%s]     Can't accept comp-cid", b->name));
		break;
	      }
	      break;
	    case MODE_REJ:
	      IPCP_PEER_REJ(ipcp, opt->type);
	      ipcp->want_comp.proto = 0;
	      break;
	  }
	}
	break;
#endif /* USE_NG_VJC */

      case TY_PRIMARYDNS:
        if (b->params.peer_dns[0].s_addr != 0)
	    peerip = &b->params.peer_dns[0];
	else
	    peerip = &ipcp->conf.peer_dns[0];
	wantip = &ipcp->want_dns[0];
	goto doDnsNbns;
      case TY_PRIMARYNBNS:
        if (b->params.peer_nbns[0].s_addr != 0)
	    peerip = &b->params.peer_nbns[0];
	else
	    peerip = &ipcp->conf.peer_nbns[0];
	wantip = &ipcp->want_nbns[0];
	goto doDnsNbns;
      case TY_SECONDARYDNS:
        if (b->params.peer_dns[1].s_addr != 0)
	    peerip = &b->params.peer_dns[1];
	else
	    peerip = &ipcp->conf.peer_dns[1];
	wantip = &ipcp->want_dns[1];
	goto doDnsNbns;
      case TY_SECONDARYNBNS:
        if (b->params.peer_nbns[1].s_addr != 0)
	    peerip = &b->params.peer_nbns[1];
	else
	    peerip = &ipcp->conf.peer_nbns[1];
	wantip = &ipcp->want_nbns[1];
doDnsNbns:
	{
	  struct in_addr	hisip;

	  memcpy(&hisip, opt->data, 4);
	  Log(LG_IPCP, ("[%s]   %s %s", b->name, oi->name, inet_ntoa(hisip)));
	  switch (mode) {
	    case MODE_REQ:
	      if (hisip.s_addr == 0) {		/* he's asking for one */
		if (peerip->s_addr == 0) {	/* we don't got one */
		  FsmRej(fp, opt);
		  break;
		}
		Log(LG_IPCP, ("[%s]     NAKing with %s", b->name, inet_ntoa(*peerip)));
		memcpy(opt->data, peerip, sizeof(*peerip));
		FsmNak(fp, opt);		/* we got one for him */
		break;
	      }
	      FsmAck(fp, opt);			/* he knows what he wants */
	      break;
	    case MODE_NAK:	/* we asked for his server, he's telling us */
	      *wantip = hisip;
	      break;
	    case MODE_REJ:	/* we asked for his server, he's ignorant */
	      IPCP_PEER_REJ(ipcp, opt->type);
	      break;
	  }
	}
	break;

      default:
	assert(0);
    }
  }
}

/*
 * IpcpInput()
 *
 * Deal with an incoming IPCP packet
 */

void
IpcpInput(Bund b, Mbuf bp)
{
    FsmInput(&b->ipcp.fsm, bp);
}

#ifdef USE_NG_VJC
static int
IpcpNgInitVJ(Bund b)
{
  struct ngm_mkpeer	mp;
  struct ngm_connect	cn;
  char path[NG_PATHSIZ];
  struct ngm_name	nm;

  /* Add a VJ compression node */
  snprintf(path, sizeof(path), "[%x]:", b->nodeID);
  strcpy(mp.type, NG_VJC_NODE_TYPE);
  strcpy(mp.ourhook, NG_PPP_HOOK_VJC_IP);
  strcpy(mp.peerhook, NG_VJC_HOOK_IP);
  if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
    Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
      b->name, NG_VJC_NODE_TYPE, path, mp.ourhook, strerror(errno)));
    goto fail;
  }

  /* Give it a name */
  strlcat(path, NG_PPP_HOOK_VJC_IP, sizeof(path));
  snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-vjc", gPid, b->name);
  if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
    Log(LG_ERR, ("[%s] can't name %s node: %s",
      b->name, NG_VJC_NODE_TYPE, strerror(errno)));
    goto fail;
  }

  /* Connect the other three hooks between the ppp and vjc nodes */
  snprintf(path, sizeof(path), "[%x]:", b->nodeID);
  strcpy(cn.path, NG_PPP_HOOK_VJC_IP);
  strcpy(cn.ourhook, NG_PPP_HOOK_VJC_COMP);
  strcpy(cn.peerhook, NG_VJC_HOOK_VJCOMP);
  if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
    Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
      b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
    goto fail;
  }
  strcpy(cn.ourhook, NG_PPP_HOOK_VJC_UNCOMP);
  strcpy(cn.peerhook, NG_VJC_HOOK_VJUNCOMP);
  if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
    Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s", 
      b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
    goto fail;
  }
  strcpy(cn.ourhook, NG_PPP_HOOK_VJC_VJIP);
  strcpy(cn.peerhook, NG_VJC_HOOK_VJIP);
  if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
    Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
      b->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
    goto fail;
  }

    return 0;
fail:
    return -1;
}

static void
IpcpNgShutdownVJ(Bund b)
{
    char	path[NG_PATHSIZ];

    snprintf(path, sizeof(path), "[%x]:%s", b->nodeID, NG_PPP_HOOK_VJC_IP);
    NgFuncShutdownNode(gLinksCsock, b->name, path);
}
#endif /* USE_NG_VJC */

/*
 * IpcpSetCommand()
 */

static int
IpcpSetCommand(Context ctx, int ac, char *av[], void *arg)
{
  IpcpState		const ipcp = &ctx->bund->ipcp;
  struct in_addr	*ips;

  if (ac == 0)
    return(-1);
  switch ((intptr_t)arg) {
    case SET_RANGES:
      {
	struct u_range	self_new_allow;
	struct u_range	peer_new_allow;
	int pos = 0, self_new_pool = -1, peer_new_pool = -1;

	/* Parse args */
	if (ac < 2)
	    return (-1);
	if (strcmp(av[pos], "ippool") == 0) {
	    self_new_pool = pos+1;
	    pos+=2;
	} else {
	    if (!ParseRange(av[pos], &self_new_allow, ALLOW_IPV4))
		return(-1);
	    pos++;
	}
	if (pos >= ac)
	    return (-1);
	if (strcmp(av[pos], "ippool") == 0) {
	    if ((pos + 1) >= ac)
		return (-1);
	    peer_new_pool = pos+1;
	    pos+=2;
	} else {
	    if (!ParseRange(av[pos], &peer_new_allow, ALLOW_IPV4))
		return(-1);
	    pos++;
	}
	if (pos != ac)
	    return (-1);

	if (self_new_pool >= 0)
	    strlcpy(ipcp->conf.self_ippool, av[self_new_pool], sizeof(ipcp->conf.self_ippool));
	else
	    ipcp->conf.self_ippool[0] = 0;
	if (peer_new_pool >= 0)
	    strlcpy(ipcp->conf.ippool, av[peer_new_pool], sizeof(ipcp->conf.ippool));
	else
	    ipcp->conf.ippool[0] = 0;
	ipcp->conf.self_allow = self_new_allow;
	ipcp->conf.peer_allow = peer_new_allow;

      }
      break;

    case SET_DNS:
      ips = ipcp->conf.peer_dns;
      goto getPrimSec;
      break;
    case SET_NBNS:
      ips = ipcp->conf.peer_nbns;
getPrimSec:
      if (!inet_aton(av[0], &ips[0]))
	Error("invalid IP address: \'%s\'", av[0]);
      ips[1].s_addr = 0;
      if (ac > 1 && !inet_aton(av[1], &ips[1]))
	Error("invalid IP address: \'%s\'", av[1]);
      break;

    case SET_ACCEPT:
      AcceptCommand(ac, av, &ipcp->conf.options, gConfList);
      break;

    case SET_DENY:
      DenyCommand(ac, av, &ipcp->conf.options, gConfList);
      break;

    case SET_ENABLE:
      EnableCommand(ac, av, &ipcp->conf.options, gConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &ipcp->conf.options, gConfList);
      break;

    case SET_YES:
      YesCommand(ac, av, &ipcp->conf.options, gConfList);
      break;

    case SET_NO:
      NoCommand(ac, av, &ipcp->conf.options, gConfList);
      break;

    default:
      assert(0);
  }
  return(0);
}

