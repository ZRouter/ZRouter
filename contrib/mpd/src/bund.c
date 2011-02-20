
/*
 * bund.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 *
 * Bundle handling stuff
 */

#include "ppp.h"
#include "bund.h"
#include "ipcp.h"
#include "ccp.h"
#include "mp.h"
#include "iface.h"
#include "link.h"
#include "msg.h"
#include "ngfunc.h"
#include "log.h"
#include "util.h"
#include "input.h"

#include <netgraph.h>
#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_iface.h>
#ifdef USE_NG_VJC
#include <netgraph/ng_vjc.h>
#endif

/*
 * DEFINITIONS
 */

  /* #define DEBUG_BOD */

  #define BUND_REOPEN_DELAY	3	/* wait this long before closing */
  #define BUND_REOPEN_PAUSE	3	/* wait this long before re-opening */

  #define BUND_MIN_TOT_BW	9600

  /* Set menu options */
  enum {
    SET_PERIOD,
    SET_LOW_WATER,
    SET_HIGH_WATER,
    SET_MIN_CONNECT,
    SET_MIN_DISCONNECT,
    SET_LINKS,
    SET_AUTHNAME,
    SET_PASSWORD,
    SET_RETRY,
    SET_ACCEPT,
    SET_DENY,
    SET_ENABLE,
    SET_DISABLE,
    SET_YES,
    SET_NO
  };

/*
 * INTERNAL FUNCTIONS
 */

  static int	BundNgInit(Bund b);
  static void	BundNgShutdown(Bund b, int iface, int ppp);

  static void	BundBmStart(Bund b);
  static void	BundBmStop(Bund b);
  static void	BundBmTimeout(void *arg);

  static void	BundReasses(Bund b);
  static int	BundSetCommand(Context ctx, int ac, char *av[], void *arg);

  static void	BundNcpsUp(Bund b);
  static void	BundNcpsDown(Bund b);

  static void	BundReOpenLinks(void *arg);
  static void	BundCloseLink(Link l);

  static void	BundMsg(int type, void *cookie);

/*
 * GLOBAL VARIABLES
 */

  struct discrim	self_discrim;

  const struct cmdtab BundSetCmds[] = {
    { "period {seconds}",		"BoD sampling period",
	BundSetCommand, NULL, 2, (void *) SET_PERIOD },
    { "lowat {percent}",		"BoD low water mark",
	BundSetCommand, NULL, 2, (void *) SET_LOW_WATER },
    { "hiwat {percent}",		"BoD high water mark",
	BundSetCommand, NULL, 2, (void *) SET_HIGH_WATER },
    { "min-con {seconds}",		"BoD min connected time",
	BundSetCommand, NULL, 2, (void *) SET_MIN_CONNECT },
    { "min-dis {seconds}",		"BoD min disconnected time",
	BundSetCommand, NULL, 2, (void *) SET_MIN_DISCONNECT },
    { "links {link list ...}",		"Links list for BoD/DoD",
	BundSetCommand, NULL, 2, (void *) SET_LINKS },
    { "fsm-timeout {seconds}",		"FSM retry timeout",
	BundSetCommand, NULL, 2, (void *) SET_RETRY },
    { "accept {opt ...}",		"Accept option",
	BundSetCommand, NULL, 2, (void *) SET_ACCEPT },
    { "deny {opt ...}",			"Deny option",
	BundSetCommand, NULL, 2, (void *) SET_DENY },
    { "enable {opt ...}",		"Enable option",
	BundSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable {opt ...}",		"Disable option",
	BundSetCommand, NULL, 2, (void *) SET_DISABLE },
    { "yes {opt ...}",			"Enable and accept option",
	BundSetCommand, NULL, 2, (void *) SET_YES },
    { "no {opt ...}",			"Disable and deny option",
	BundSetCommand, NULL, 2, (void *) SET_NO },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static const struct confinfo	gConfList[] = {
    { 0,	BUND_CONF_IPCP,		"ipcp"		},
    { 0,	BUND_CONF_IPV6CP,	"ipv6cp"	},
    { 0,	BUND_CONF_COMPRESSION,	"compression"	},
    { 0,	BUND_CONF_ENCRYPTION,	"encryption"	},
    { 0,	BUND_CONF_CRYPT_REQD,	"crypt-reqd"	},
    { 0,	BUND_CONF_BWMANAGE,	"bw-manage"	},
    { 0,	BUND_CONF_ROUNDROBIN,	"round-robin"	},
    { 0,	0,			NULL		},
  };

/*
 * BundOpen()
 */

void
BundOpen(Bund b)
{
    REF(b);
    MsgSend(&b->msgs, MSG_OPEN, b);
}

/*
 * BundClose()
 */

void
BundClose(Bund b)
{
    REF(b);
    MsgSend(&b->msgs, MSG_CLOSE, b);
}

/*
 * BundOpenCmd()
 */

int
BundOpenCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to open template");
    BundOpen(ctx->bund);
    return (0);
}

/*
 * BundCloseCmd()
 */

int
BundCloseCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to close template");
    BundClose(ctx->bund);
    return (0);
}

/*
 * BundJoin()
 *
 * This is called when a link enters the NETWORK phase.
 *
 * Verify that link is OK to come up as part of it's bundle.
 * If so, join it to the bundle. Returns FALSE if there's a problem.
 * If this is the first link to join, and it's not supporting
 * multi-link, then prevent any further links from joining.
 *
 * Right now this is fairly simple minded: you have to define
 * the links in a bundle first, then stick to that plan. For
 * a server this might be too restrictive a policy.
 *
 * Returns zero if fails, otherwise the new number of up links.
 */

int
BundJoin(Link l)
{
    Bund	b, bt;
    LcpState	const lcp = &l->lcp;
    int		k;

    if (gShutdownInProgress) {
	Log(LG_BUND, ("Shutdown sequence in progress, BundJoin() denied"));
        return(0);
    }

    if (!l->bund) {
	b = NULL;
	if (lcp->peer_mrru) {
	    for (k = 0; k < gNumBundles; k++) {
		if (gBundles[k] && !gBundles[k]->tmpl && gBundles[k]->peer_mrru &&
		    MpDiscrimEqual(&lcp->peer_discrim, &gBundles[k]->peer_discrim) &&
		    !strcmp(lcp->auth.params.authname, gBundles[k]->params.authname)) {
		        break;
		}
	    }
	    if (k != gNumBundles) {
	        b = gBundles[k];
	    }
	}
	if (!b) {
	    const char	*bundt;
	    if (strncmp(l->lcp.auth.params.action, "bundle ", 7) == 0) {
		bundt = l->lcp.auth.params.action + 7;
	    } else {
		bundt = LinkMatchAction(l, 3, l->lcp.auth.params.authname);
	    }
	    if (bundt) {
		if (strcmp(bundt,"##DROP##") == 0) {
		    /* Action told we must drop this connection */
    		    Log(LG_BUND, ("[%s] Drop link", l->name));
		    return (0);
		}
		if ((bt = BundFind(bundt))) {
		    if (bt->tmpl) {
    			Log(LG_BUND, ("[%s] Creating new bundle using template \"%s\".", l->name, bundt));
    			b = BundInst(bt, NULL, 0, 0);
		    } else {
			b = bt;
		    }
		} else {
    		    Log(LG_BUND, ("[%s] Bundle \"%s\" not found.", l->name, bundt));
		    return (0);
		}
	    } else {
    		Log(LG_BUND, ("[%s] No bundle specified", l->name));
		return (0);
	    }
	    if (!b) {
    		Log(LG_BUND, ("[%s] Bundle creation error", l->name));
		return (0);
	    }
	}
	if (b->n_up > 0 &&
	  (b->peer_mrru == 0 || lcp->peer_mrru == 0 || lcp->want_mrru == 0)) {
    	    Log(LG_BUND, ("[%s] Can't join bundle %s without "
		"multilink negotiated.", l->name, b->name));
	    return (0);
	}
	if (b->n_up > 0 &&
	  (!MpDiscrimEqual(&lcp->peer_discrim, &b->peer_discrim) ||
	  strcmp(lcp->auth.params.authname, b->params.authname))) {
    	    Log(LG_BUND, ("[%s] Can't join bundle %s with different "
		"peer discriminator/authname.", l->name, b->name));
	    return (0);
	}
	k = 0;
	while (k < NG_PPP_MAX_LINKS && b->links[k] != NULL)
	    k++;
	if (k < NG_PPP_MAX_LINKS) {
	    l->bund = b;
	    l->bundleIndex = k;
	    b->links[k] = l;
	    b->n_links++;
	} else {
    	    Log(LG_BUND, ("[%s] No more then %d links per bundle allowed. "
		"Can't join budle.", l->name, NG_PPP_MAX_LINKS));
	    return (0);
	}
    }

    b = l->bund;

    Log(LG_LINK, ("[%s] Link: Join bundle \"%s\"", l->name, b->name));

    b->open = TRUE; /* Open bundle on incoming */

    if (LinkNgJoin(l)) {
	Log(LG_ERR, ("[%s] Bundle netgraph join failed", l->name));
	l->bund = NULL;
	b->links[l->bundleIndex] = NULL;
	if (!b->stay)
	    BundShutdown(b);
	return(0);
    }
    l->joined_bund = 1;
    b->n_up++;

    LinkResetStats(l);

    if (b->n_up == 1) {

	/* Cancel re-open timer; we've come up somehow (eg, LCP renegotiation) */
	TimerStop(&b->reOpenTimer);

	b->last_up = time(NULL);

	/* Copy auth params from the first link */
	authparamsCopy(&l->lcp.auth.params,&b->params);

	/* Initialize multi-link stuff */
	if ((b->peer_mrru = lcp->peer_mrru)) {
    	    b->peer_discrim = lcp->peer_discrim;
	}

	/* Start bandwidth management */
	BundBmStart(b);
    }

    /* Reasses MTU, bandwidth, etc. */
    BundReasses(b);

    /* Configure this link */
    b->pppConfig.links[l->bundleIndex].enableLink = 1;
    b->pppConfig.links[l->bundleIndex].mru = lcp->peer_mru;
    b->pppConfig.links[l->bundleIndex].enableACFComp = lcp->peer_acfcomp;
    b->pppConfig.links[l->bundleIndex].enableProtoComp = lcp->peer_protocomp;
    b->pppConfig.links[l->bundleIndex].bandwidth = (l->bandwidth / 8 + 5) / 10;
    b->pppConfig.links[l->bundleIndex].latency = (l->latency + 500) / 1000;

    /* What to do when the first link comes up */
    if (b->n_up == 1) {

	/* Configure the bundle */
	b->pppConfig.bund.enableMultilink = (lcp->peer_mrru && lcp->want_mrru)?1:0;
	/* ng_ppp does not allow MRRU less then 1500 bytes. */
	b->pppConfig.bund.mrru = (lcp->peer_mrru < 1500) ? 1500 : lcp->peer_mrru;
	b->pppConfig.bund.xmitShortSeq = lcp->peer_shortseq;
	b->pppConfig.bund.recvShortSeq = lcp->want_shortseq;
	b->pppConfig.bund.enableRoundRobin =
    	    Enabled(&b->conf.options, BUND_CONF_ROUNDROBIN);

	/* generate a uniq msession_id */
	snprintf(b->msession_id, AUTH_MAX_SESSIONID, "%d-%s",
    	    (int)(time(NULL) % 10000000), b->name);
      
	b->originate = l->originate;
    }

    /* Update PPP node configuration */
    NgFuncSetConfig(b);

    /* copy msession_id to link */
    strlcpy(l->msession_id, b->msession_id, sizeof(l->msession_id));

    /* What to do when the first link comes up */
    if (b->n_up == 1) {

	BundNcpsOpen(b);
	BundNcpsUp(b);

	BundResetStats(b);

#ifndef NG_PPP_STATS64    
	/* starting bundle statistics timer */
	TimerInit(&b->statsUpdateTimer, "BundUpdateStats", 
	    BUND_STATS_UPDATE_INTERVAL, BundUpdateStatsTimer, b);
	TimerStartRecurring(&b->statsUpdateTimer);
#endif
    }

    AuthAccountStart(l, AUTH_ACCT_START);

    return(b->n_up);
}

/*
 * BundLeave()
 *
 * This is called when a link leaves the NETWORK phase.
 */

void
BundLeave(Link l)
{
    Bund	b = l->bund;

    /* Elvis has left the bundle */
    assert(b->n_up > 0);
  
    Log(LG_LINK, ("[%s] Link: Leave bundle \"%s\"", l->name, b->name));

    AuthAccountStart(l, AUTH_ACCT_STOP);

    /* Disable link */
    b->pppConfig.links[l->bundleIndex].enableLink = 0;
    b->pppConfig.links[l->bundleIndex].mru = LCP_DEFAULT_MRU;
    NgFuncSetConfig(b);

    LinkNgLeave(l);
    l->joined_bund = 0;
    b->n_up--;
    
    /* Divorce link and bundle */
    b->links[l->bundleIndex] = NULL;
    b->n_links--;
    l->bund = NULL;

    BundReasses(b);
    
    /* Forget session_ids */
    l->msession_id[0] = 0;
  
    /* Special stuff when last link goes down... */
    if (b->n_up == 0) {
  
#ifndef NG_PPP_STATS64
	/* stopping bundle statistics timer */
	TimerStop(&b->statsUpdateTimer);
#endif

	/* Reset statistics and auth information */
	BundBmStop(b);

	BundNcpsClose(b);
	BundNcpsDown(b);
	
#ifdef USE_NG_BPF
	IfaceFreeStats(&b->iface.prevstats);
#endif

	authparamsDestroy(&b->params);

	b->msession_id[0] = 0;
 
	/* try to open again later */
	if (b->open && Enabled(&b->conf.options, BUND_CONF_BWMANAGE) &&
	  !Enabled(&b->iface.options, IFACE_CONF_ONDEMAND) && !gShutdownInProgress) {
	    if (b->n_links != 0 || b->conf.linkst[0][0]) {
		/* wait BUND_REOPEN_DELAY to see if it comes back up */
    	        int delay = BUND_REOPEN_DELAY;
    		delay += ((random() ^ gPid ^ time(NULL)) & 1);
    		Log(LG_BUND, ("[%s] Bundle: Last link has gone, reopening in %d seconds", 
    		    b->name, delay));
    	        TimerStop(&b->reOpenTimer);
    		TimerInit(&b->reOpenTimer, "BundReOpen",
		    delay * SECONDS, BundReOpenLinks, b);
    		TimerStart(&b->reOpenTimer);
		return;
	    } else {
    		Log(LG_BUND, ("[%s] Bundle: Last link has gone, no links for bw-manage defined", 
    		    b->name));
	    }
	}
	b->open = FALSE;
	if (!b->stay)
	    BundShutdown(b);
    }
}

/*
 * BundReOpenLinks()
 *
 * The last link went down, and we waited BUND_REOPEN_DELAY seconds for
 * it to come back up. It didn't, so close all the links and re-open them
 * BUND_REOPEN_PAUSE seconds from now.
 *
 * The timer calling this is cancelled whenever any link comes up.
 */

static void
BundReOpenLinks(void *arg)
{
    Bund b = (Bund)arg;
    
    Log(LG_BUND, ("[%s] Bundle: Last link has gone, reopening...", b->name));
    BundOpenLinks(b);
}

/*
 * BundMsg()
 *
 * Deal with incoming message to the bundle
 */

static void
BundMsg(int type, void *arg)
{
    Bund	b = (Bund)arg;

    if (b->dead) {
	UNREF(b);
	return;
    }
    Log(LG_BUND, ("[%s] Bundle: %s event in state %s",
	b->name, MsgName(type), b->open ? "OPENED" : "CLOSED"));
    TimerStop(&b->reOpenTimer);
    switch (type) {
    case MSG_OPEN:
        b->open = TRUE;
	BundOpenLinks(b);
        break;

    case MSG_CLOSE:
        b->open = FALSE;
        BundCloseLinks(b);
        break;

    default:
        assert(FALSE);
    }
    UNREF(b);
}

/*
 * BundOpenLinks()
 *
 * Open one link or all links, depending on whether bandwidth
 * management is in effect or not.
 */

void
BundOpenLinks(Bund b)
{
    int	k;

    TimerStop(&b->reOpenTimer);
    if (Enabled(&b->conf.options, BUND_CONF_BWMANAGE)) {
	if (b->n_links != 0)
	    return;
	for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	    if (b->links[k]) {
    		BundOpenLink(b->links[k]);
		break;
	    } else if (b->conf.linkst[k][0]) {
		BundCreateOpenLink(b, k);
		break;
	    }
	}
    } else {
	for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	    if (b->links[k])
    		BundOpenLink(b->links[k]);
	    else if (b->conf.linkst[k][0])
		BundCreateOpenLink(b, k);
	}
    }
}

/*
 * BundCreateOpenLink()
 */

int
BundCreateOpenLink(Bund b, int n)
{
    if (!b->links[n]) {
	if (b->conf.linkst[n][0]) {
	    Link l;
	    Link lt = LinkFind(b->conf.linkst[n]);
	    if (!lt) {
		Log(LG_BUND, ("[%s] Bund: Link \"%s\" not found", b->name, b->conf.linkst[n]));
		return (-1);
	    }
	    if (PhysIsBusy(lt)) {
		Log(LG_BUND, ("[%s] Bund: Link \"%s\" is busy", b->name, b->conf.linkst[n]));
		return (-1);
	    }
	    if (lt->tmpl) {
		l = LinkInst(lt, NULL, 0, 0);
	    } else
		l = lt;
	    if (!l) {
		Log(LG_BUND, ("[%s] Bund: Link \"%s\" creation error", b->name, b->conf.linkst[n]));
		return (-1);
	    }
	    b->links[n] = l;
	    b->n_links++;
	    l->bund = b;
	    l->bundleIndex = n;
	    l->conf.max_redial = -1;
	} else {
	    Log(LG_BUND, ("[%s] Bund: Link %d name not specified", b->name, n));
	    return (-1);
	}
    }
    BundOpenLink(b->links[n]);
    return (0);
}

/*
 * BundOpenLink()
 */

void
BundOpenLink(Link l)
{
  Log(LG_BUND, ("[%s] opening link \"%s\"...", l->bund->name, l->name));
  LinkOpen(l);
}

/*
 * BundCloseLinks()
 *
 * Close all links
 */

void
BundCloseLinks(Bund b)
{
  int	k;

  TimerStop(&b->reOpenTimer);
  for (k = 0; k < NG_PPP_MAX_LINKS; k++)
    if (b->links[k] && OPEN_STATE(b->links[k]->lcp.fsm.state))
      BundCloseLink(b->links[k]);
}

/*
 * BundCloseLink()
 */

static void
BundCloseLink(Link l)
{
    Log(LG_BUND, ("[%s] Bundle: closing link \"%s\"...", l->bund->name, l->name));
    LinkClose(l);
}

/*
 * BundNcpsOpen()
 */

void
BundNcpsOpen(Bund b)
{
  if (Enabled(&b->conf.options, BUND_CONF_IPCP))
    IpcpOpen(b);
  if (Enabled(&b->conf.options, BUND_CONF_IPV6CP))
    Ipv6cpOpen(b);
  if (Enabled(&b->conf.options, BUND_CONF_COMPRESSION))
    CcpOpen(b);
  if (Enabled(&b->conf.options, BUND_CONF_ENCRYPTION))
    EcpOpen(b);
}

/*
 * BundNcpsUp()
 */

static void
BundNcpsUp(Bund b)
{
  if (Enabled(&b->conf.options, BUND_CONF_IPCP))
    IpcpUp(b);
  if (Enabled(&b->conf.options, BUND_CONF_IPV6CP))
    Ipv6cpUp(b);
  if (Enabled(&b->conf.options, BUND_CONF_COMPRESSION))
    CcpUp(b);
  if (Enabled(&b->conf.options, BUND_CONF_ENCRYPTION))
    EcpUp(b);
}

void
BundNcpsStart(Bund b, int proto)
{
    b->ncpstarted |= ((1<<proto)>>1);
}

void
BundNcpsFinish(Bund b, int proto)
{
    b->ncpstarted &= (~((1<<proto)>>1));
    if (!b->ncpstarted) {
	Log(LG_BUND, ("[%s] Bundle: No NCPs left. Closing links...", b->name));
	RecordLinkUpDownReason(b, NULL, 0, STR_PROTO_ERR, NULL);
	BundCloseLinks(b); /* We have nothing to live for */
    }
}

void
BundNcpsJoin(Bund b, int proto)
{
	IfaceState	iface = &b->iface;

	if (iface->dod) {
		if (iface->ip_up) {
			iface->ip_up = 0;
			IfaceIpIfaceDown(b);
		}
		if (iface->ipv6_up) {
			iface->ipv6_up = 0;
			IfaceIpv6IfaceDown(b);
		}
		iface->dod = 0;
		iface->up = 0;
		IfaceDown(b);
	}
    
	switch(proto) {
	case NCP_IPCP:
		if (!iface->ip_up) {
			iface->ip_up = 1;
			if (IfaceIpIfaceUp(b, 1)) {
			    iface->ip_up = 0;
			    return;
			};
		}
		break;
	case NCP_IPV6CP:
		if (!iface->ipv6_up) {
			iface->ipv6_up = 1;
			if (IfaceIpv6IfaceUp(b, 1)) {
			    iface->ipv6_up = 0;
			    return;
			};
		}
		break;
	case NCP_NONE: /* Manual call by 'open iface' */
		if (Enabled(&b->conf.options, BUND_CONF_IPCP) &&
		    !iface->ip_up) {
			iface->ip_up = 1;
			if (IfaceIpIfaceUp(b, 0)) {
			    iface->ip_up = 0;
			    return;
			};
		}
		if (Enabled(&b->conf.options, BUND_CONF_IPV6CP) &&
		    !iface->ipv6_up) {
			iface->ipv6_up = 1;
			if (IfaceIpv6IfaceUp(b, 0)) {
			    iface->ipv6_up = 0;
			    return;
			};
		}
		break;
	}

	if (!iface->up) {
		iface->up = 1;
		if (proto == NCP_NONE) {
			iface->dod = 1;
			IfaceUp(b, 0);
		} else {
			IfaceUp(b, 1);
		}
	}
}

void
BundNcpsLeave(Bund b, int proto)
{
	IfaceState	iface = &b->iface;
	switch(proto) {
	case NCP_IPCP:
		if (iface->ip_up) {
			iface->ip_up=0;
			IfaceIpIfaceDown(b);
		}
		break;
	case NCP_IPV6CP:
		if (iface->ipv6_up) {
			iface->ipv6_up=0;
			IfaceIpv6IfaceDown(b);
		}
		break;
	case NCP_NONE:
		if (iface->ip_up) {
			iface->ip_up=0;
			IfaceIpIfaceDown(b);
		}
		if (iface->ipv6_up) {
			iface->ipv6_up=0;
			IfaceIpv6IfaceDown(b);
		}
		break;
	}
    
	if ((iface->up) && (!iface->ip_up) && (!iface->ipv6_up)) {
		iface->dod=0;
		iface->up=0;
		IfaceDown(b);
    		if (iface->open) {
			if (Enabled(&b->conf.options, BUND_CONF_IPCP)) {
				iface->ip_up=1;
				if (IfaceIpIfaceUp(b, 0))
				    iface->ip_up = 0;
			}
			if (Enabled(&b->conf.options, BUND_CONF_IPV6CP)) {
				iface->ipv6_up=1;
				if (IfaceIpv6IfaceUp(b, 0))
				    iface->ipv6_up = 0;
			}
			if (iface->ip_up || iface->ipv6_up) {
			    iface->dod=1;
			    iface->up=1;
			    IfaceUp(b, 0);
			}
		}
	}
}

/*
 * BundNcpsDown()
 */

static void
BundNcpsDown(Bund b)
{
  if (Enabled(&b->conf.options, BUND_CONF_IPCP))
    IpcpDown(b);
  if (Enabled(&b->conf.options, BUND_CONF_IPV6CP))
    Ipv6cpDown(b);
  if (Enabled(&b->conf.options, BUND_CONF_COMPRESSION))
    CcpDown(b);
  if (Enabled(&b->conf.options, BUND_CONF_ENCRYPTION))
    EcpDown(b);
}

/*
 * BundNcpsClose()
 */

void
BundNcpsClose(Bund b)
{
  if (Enabled(&b->conf.options, BUND_CONF_IPCP))
    IpcpClose(b);
  if (Enabled(&b->conf.options, BUND_CONF_IPV6CP))
    Ipv6cpClose(b);
  if (Enabled(&b->conf.options, BUND_CONF_COMPRESSION))
    CcpClose(b);
  if (Enabled(&b->conf.options, BUND_CONF_ENCRYPTION))
    EcpClose(b);
}

/*
 * BundReasses()
 *
 * Here we do a reassessment of things after a new link has been
 * added to or removed from the bundle.
 */

static void
BundReasses(Bund b)
{
  BundBm	const bm = &b->bm;

  /* Update system interface parameters */
  BundUpdateParams(b);

  Log(LG_BUND, ("[%s] Bundle: Status update: up %d link%s, total bandwidth %d bps",
    b->name, b->n_up, b->n_up == 1 ? "" : "s", bm->total_bw));

}

/*
 * BundUpdateParams()
 *
 * Recalculate interface MTU and bandwidth.
 */

void
BundUpdateParams(Bund b)
{
  BundBm	const bm = &b->bm;
  int		k, mtu, the_link = 0;

    /* Recalculate how much bandwidth we have */
    bm->total_bw = 0;
    for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	if (b->links[k] && b->links[k]->lcp.phase == PHASE_NETWORK) {
    	    bm->total_bw += b->links[k]->bandwidth;
    	    the_link = k;
	}
    }
    if (bm->total_bw < BUND_MIN_TOT_BW)
	bm->total_bw = BUND_MIN_TOT_BW;

    /* Recalculate MTU corresponding to peer's MRU */
    if (b->n_up == 0) {
        mtu = NG_IFACE_MTU_DEFAULT;	/* Reset to default settings */

    } else if (!b->peer_mrru) {		/* If no multilink, use peer MRU */
	mtu = MIN(b->links[the_link]->lcp.peer_mru,
		  b->links[the_link]->type->mtu);

    } else {	  	/* Multilink, use peer MRRU */
        mtu = MIN(b->peer_mrru, MP_MAX_MRRU);
    }

    /* Subtract to make room for various frame-bloating protocols */
    if (b->n_up > 0) {
	if (Enabled(&b->conf.options, BUND_CONF_COMPRESSION))
    	    mtu = CcpSubtractBloat(b, mtu);
	if (Enabled(&b->conf.options, BUND_CONF_ENCRYPTION))
    	    mtu = EcpSubtractBloat(b, mtu);
    }

    /* Update interface MTU */
    IfaceSetMTU(b, mtu);
}

/*
 * BundCommand()
 *
 * Show list of all bundles or set bundle
 */

int
BundCommand(Context ctx, int ac, char *av[], void *arg)
{
    Bund	sb;
    int		j, k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
    	Printf("Defined bundles:\r\n");
    	for (k = 0; k < gNumBundles; k++) {
	    if ((sb = gBundles[k]) != NULL) {
	        Printf("\t%-15s", sb->name);
	        for (j = 0; j < NG_PPP_MAX_LINKS; j++) {
		    if (sb->links[j])
		        Printf("%s ", sb->links[j]->name);
		}
		Printf("\r\n");
	    }
    	}
	return (0);
    }

    if ((sb = BundFind(av[0])) == NULL) {
        RESETREF(ctx->lnk, NULL);
	RESETREF(ctx->bund, NULL);
	RESETREF(ctx->rep, NULL);
        Error("Bundle \"%s\" not defined.", av[0]);
    }

    /* Change bundle, and link also if needed */
    RESETREF(ctx->bund, sb);
    if (ctx->lnk == NULL || ctx->lnk->bund != ctx->bund) {
        RESETREF(ctx->lnk, ctx->bund->links[0]);
    }
    RESETREF(ctx->rep, NULL);
    return(0);
}

/*
 * MSessionCommand()
 */

int
MSessionCommand(Context ctx, int ac, char *av[], void *arg)
{
    int		k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
    	Printf("Present msessions:\r\n");
	for (k = 0; k < gNumBundles; k++) {
	    if (gBundles[k] && gBundles[k]->msession_id[0])
    		Printf("\t%s\r\n", gBundles[k]->msession_id);
	}
	return (0);
    }

    /* Find bundle */
    for (k = 0;
	k < gNumBundles && (gBundles[k] == NULL || 
	    strcmp(gBundles[k]->msession_id, av[0]));
	k++);
    if (k == gNumBundles) {
	/* Change default link and bundle */
	RESETREF(ctx->lnk, NULL);
	RESETREF(ctx->bund, NULL);
	RESETREF(ctx->rep, NULL);
	Error("msession \"%s\" is not found", av[0]);
    }

    /* Change default link and bundle */
    RESETREF(ctx->bund, gBundles[k]);
    if (ctx->lnk == NULL || ctx->lnk->bund != ctx->bund) {
        RESETREF(ctx->lnk, ctx->bund->links[0]);
    }
    RESETREF(ctx->rep, NULL);

    return(0);
}

/*
 * IfaceCommand()
 */

int
IfaceCommand(Context ctx, int ac, char *av[], void *arg)
{
    int		k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
    	Printf("Present ifaces:\r\n");
	for (k = 0; k < gNumBundles; k++) {
	    if (gBundles[k] && gBundles[k]->iface.ifname[0])
    		Printf("\t%s\t%s\r\n", gBundles[k]->iface.ifname, gBundles[k]->name);
	}
	return (0);
    }

    /* Find bundle */
    for (k = 0;
	k < gNumBundles && (gBundles[k] == NULL || 
	    strcmp(gBundles[k]->iface.ifname, av[0]));
	k++);
    if (k == gNumBundles) {
	/* Change default link and bundle */
	RESETREF(ctx->lnk, NULL);
	RESETREF(ctx->bund, NULL);
	RESETREF(ctx->rep, NULL);
	Error("iface \"%s\" is not found", av[0]);
    }

    /* Change default link and bundle */
    RESETREF(ctx->bund, gBundles[k]);
    if (ctx->lnk == NULL || ctx->lnk->bund != ctx->bund) {
        RESETREF(ctx->lnk, ctx->bund->links[0]);
    }
    RESETREF(ctx->rep, NULL);

    return(0);
}

/*
 * BundCreate()
 */

int
BundCreate(Context ctx, int ac, char *av[], void *arg)
{
    Bund	b, bt = NULL;
    u_char	tmpl = 0;
    u_char	stay = 0;
    int	k;

    RESETREF(ctx->lnk, NULL);
    RESETREF(ctx->bund, NULL);
    RESETREF(ctx->rep, NULL);

    if (ac < 1)
	return(-1);

    if (strcmp(av[0], "template") == 0) {
	tmpl = 1;
	stay = 1;
    } else if (strcmp(av[0], "static") == 0)
	stay = 1;

    if (ac - stay < 1 || ac - stay > 2)
	return(-1);

    if (strlen(av[0 + stay]) >= (LINK_MAX_NAME - tmpl * 5))
	Error("Bundle name \"%s\" is too long", av[0 + stay]);

    /* See if bundle name already taken */
    if ((b = BundFind(av[0 + stay])) != NULL)
	Error("Bundle \"%s\" already exists", av[0 + stay]);

    if (ac - stay == 2) {
	/* See if template name specified */
	if ((bt = BundFind(av[1 + stay])) == NULL)
	    Error("Bundle template \"%s\" not found", av[1 + stay]);
	if (!bt->tmpl)
	    Error("Bundle \"%s\" is not a template", av[1 + stay]);
    }

    if (bt) {
	b = BundInst(bt, av[0 + stay], tmpl, stay);
    } else {
	/* Create a new bundle structure */
	b = Malloc(MB_BUND, sizeof(*b));
	strlcpy(b->name, av[0 + stay], sizeof(b->name));
	b->tmpl = tmpl;
	b->stay = stay;

	/* Add bundle to the list of bundles and make it the current active bundle */
	for (k = 0; k < gNumBundles && gBundles[k] != NULL; k++);
	if (k == gNumBundles)			/* add a new bundle pointer */
	    LengthenArray(&gBundles, sizeof(*gBundles), &gNumBundles, MB_BUND);

	b->id = k;
	gBundles[k] = b;
	REF(b);

	/* Get message channel */
	MsgRegister(&b->msgs, BundMsg);

	/* Initialize bundle configuration */
	b->conf.retry_timeout = BUND_DEFAULT_RETRY;
	b->conf.bm_S = BUND_BM_DFL_S;
	b->conf.bm_Hi = BUND_BM_DFL_Hi;
	b->conf.bm_Lo = BUND_BM_DFL_Lo;
	b->conf.bm_Mc = BUND_BM_DFL_Mc;
	b->conf.bm_Md = BUND_BM_DFL_Md;

	Enable(&b->conf.options, BUND_CONF_IPCP);
	Disable(&b->conf.options, BUND_CONF_IPV6CP);

	Disable(&b->conf.options, BUND_CONF_BWMANAGE);
	Disable(&b->conf.options, BUND_CONF_COMPRESSION);
	Disable(&b->conf.options, BUND_CONF_ENCRYPTION);
        Disable(&b->conf.options, BUND_CONF_CRYPT_REQD);
  
        /* Init iface and NCP's */
	IfaceInit(b);
        IpcpInit(b);
        Ipv6cpInit(b);
        CcpInit(b);
        EcpInit(b);

	if (!tmpl) {
	    /* Setup netgraph stuff */
	    if (BundNgInit(b) < 0) {
		gBundles[b->id] = NULL;
		Freee(b);
		Error("Bundle netgraph initialization failed");
	    }
	}
    }
  
    RESETREF(ctx->bund, b);
  
    /* Done */
    return(0);
}

/*
 * BundDestroy()
 */

int
BundDestroy(Context ctx, int ac, char *av[], void *arg)
{
    Bund 	b;

    if (ac > 1)
	return(-1);

    if (ac == 1) {
	if ((b = BundFind(av[0])) == NULL)
	    Error("Bund \"%s\" not found", av[0]);
    } else {
	if (ctx->bund) {
	    b = ctx->bund;
	} else
	    Error("No bundle selected to destroy");
    }
    
    if (b->tmpl) {
	b->tmpl = 0;
	b->stay = 0;
	BundShutdown(b);
    } else {
	b->stay = 0;
	if (b->n_up) {
	    BundClose(b);
	} else {
	    BundShutdown(b);
	}
    }

    return (0);
}

/*
 * BundInst()
 */

Bund
BundInst(Bund bt, char *name, int tmpl, int stay)
{
    Bund	b;
    int	k;

    /* Create a new bundle structure */
    b = Mdup(MB_BUND, bt, sizeof(*b));
    b->tmpl = tmpl;
    b->stay = stay;
    b->refs = 0;

    /* Add bundle to the list of bundles and make it the current active bundle */
    for (k = 0; k < gNumBundles && gBundles[k] != NULL; k++);
    if (k == gNumBundles)			/* add a new bundle pointer */
	LengthenArray(&gBundles, sizeof(*gBundles), &gNumBundles, MB_BUND);

    b->id = k;
    if (name)
	strlcpy(b->name, name, sizeof(b->name));
    else
	snprintf(b->name, sizeof(b->name), "%s-%d", bt->name, k);
    gBundles[k] = b;
    REF(b);

    /* Inst iface and NCP's */
    IfaceInst(b, bt);
    IpcpInst(b, bt);
    Ipv6cpInst(b, bt);
    CcpInst(b, bt);
    EcpInst(b, bt);

    if (!tmpl) {
	/* Setup netgraph stuff */
	if (BundNgInit(b) < 0) {
	    Log(LG_ERR, ("[%s] Bundle netgraph initialization failed", b->name));
	    gBundles[b->id] = NULL;
	    Freee(b);
	    return(0);
	}
    }

    return (b);
}

/*
 * BundShutdown()
 *
 * Shutdown the netgraph stuff associated with bundle
 */

void
BundShutdown(Bund b)
{
    Link	l;
    int		k;

    Log(LG_BUND, ("[%s] Bundle: Shutdown", b->name));
    for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	if ((l = b->links[k]) != NULL)
	    if (!l->stay)
		LinkShutdown(l);
    }

    if (b->hook[0])
	BundNgShutdown(b, 1, 1);
    gBundles[b->id] = NULL;
    MsgUnRegister(&b->msgs);
    b->dead = 1;
    UNREF(b);
}

/*
 * BundStat()
 *
 * Show state of a bundle
 */

int
BundStat(Context ctx, int ac, char *av[], void *arg)
{
  Bund	sb;
  int	k, bw, tbw, nup;
  char	buf[64];

  /* Find bundle they're talking about */
  switch (ac) {
    case 0:
      sb = ctx->bund;
      break;
    case 1:
      if ((sb = BundFind(av[0])) == NULL)
	Error("Bundle \"%s\" not defined", av[0]);
      break;
    default:
      return(-1);
  }

  /* Show stuff about the bundle */
  for (tbw = bw = nup = k = 0; k < NG_PPP_MAX_LINKS; k++) {
    if (sb->links[k]) {
	if (sb->links[k]->lcp.phase == PHASE_NETWORK) {
    	    nup++;
    	    bw += sb->links[k]->bandwidth;
	}
	tbw += sb->links[k]->bandwidth;
    }
  }

  Printf("Bundle '%s'%s:\r\n", sb->name, sb->tmpl?" (template)":(sb->stay?" (static)":""));
  Printf("\tLinks          : ");
  BundShowLinks(ctx, sb);
  Printf("\tStatus         : %s\r\n", sb->open ? "OPEN" : "CLOSED");
  if (sb->n_up)
    Printf("\tSession time   : %ld seconds\r\n", (long int)(time(NULL) - sb->last_up));
  Printf("\tMultiSession Id: %s\r\n", sb->msession_id);
  Printf("\tTotal bandwidth: %u bits/sec\r\n", tbw);
  Printf("\tAvail bandwidth: %u bits/sec\r\n", bw);
  Printf("\tPeer authname  : \"%s\"\r\n", sb->params.authname);

  /* Show configuration */
  Printf("Configuration:\r\n");
  Printf("\tRetry timeout  : %d seconds\r\n", sb->conf.retry_timeout);
  Printf("\tBW-manage:\r\n");
  Printf("\t  Period       : %d seconds\r\n", sb->conf.bm_S);
  Printf("\t  Low mark     : %d%%\r\n", sb->conf.bm_Lo);
  Printf("\t  High mark    : %d%%\r\n", sb->conf.bm_Hi);
  Printf("\t  Min conn     : %d seconds\r\n", sb->conf.bm_Mc);
  Printf("\t  Min disc     : %d seconds\r\n", sb->conf.bm_Md);
  Printf("\t  Links        : ");
  for (k = 0; k < NG_PPP_MAX_LINKS; k++)
    Printf("%s ", sb->conf.linkst[k]);
  Printf("\r\n");
  Printf("Bundle level options:\r\n");
  OptStat(ctx, &sb->conf.options, gConfList);

    /* Show peer info */
    Printf("Multilink PPP:\r\n");
    Printf("\tStatus         : %s\r\n",
	sb->peer_mrru ? "Active" : "Inactive");
    if (sb->peer_mrru) {
      Printf("\tPeer MRRU      : %d bytes\r\n", sb->peer_mrru);
      Printf("\tPeer auth name : \"%s\"\r\n", sb->params.authname);
      Printf("\tPeer discrimin.: %s\r\n", MpDiscrimText(&sb->peer_discrim, buf, sizeof(buf)));
    }

    if (!sb->tmpl) {
	/* Show stats */
	BundUpdateStats(sb);
	Printf("Traffic stats:\r\n");

	Printf("\tInput octets   : %llu\r\n", (unsigned long long)sb->stats.recvOctets);
	Printf("\tInput frames   : %llu\r\n", (unsigned long long)sb->stats.recvFrames);
	Printf("\tOutput octets  : %llu\r\n", (unsigned long long)sb->stats.xmitOctets);
	Printf("\tOutput frames  : %llu\r\n", (unsigned long long)sb->stats.xmitFrames);
	Printf("\tBad protocols  : %llu\r\n", (unsigned long long)sb->stats.badProtos);
	Printf("\tRunts          : %llu\r\n", (unsigned long long)sb->stats.runts);
	Printf("\tDup fragments  : %llu\r\n", (unsigned long long)sb->stats.dupFragments);
	Printf("\tDrop fragments : %llu\r\n", (unsigned long long)sb->stats.dropFragments);
    }

    return(0);
}

/* 
 * BundUpdateStats()
 */

void
BundUpdateStats(Bund b)
{
#ifndef NG_PPP_STATS64
  struct ng_ppp_link_stat	stats;
#endif
  int	l = NG_PPP_BUNDLE_LINKNUM;

#if (__FreeBSD_version < 602104 || (__FreeBSD_version >= 700000 && __FreeBSD_version < 700029))
  /* Workaround for broken ng_ppp bundle stats */
  if (!b->peer_mrru)
    l = 0;
#endif

#ifndef NG_PPP_STATS64
  if (NgFuncGetStats(b, l, &stats) != -1) {
    b->stats.xmitFrames += abs(stats.xmitFrames - b->oldStats.xmitFrames);
    b->stats.xmitOctets += abs(stats.xmitOctets - b->oldStats.xmitOctets);
    b->stats.recvFrames += abs(stats.recvFrames - b->oldStats.recvFrames);
    b->stats.recvOctets += abs(stats.recvOctets - b->oldStats.recvOctets);
    b->stats.badProtos  += abs(stats.badProtos - b->oldStats.badProtos);
    b->stats.runts	  += abs(stats.runts - b->oldStats.runts);
    b->stats.dupFragments += abs(stats.dupFragments - b->oldStats.dupFragments);
    b->stats.dropFragments += abs(stats.dropFragments - b->oldStats.dropFragments);
  }

  b->oldStats = stats;
#else
    NgFuncGetStats64(b, l, &b->stats);
#endif
}

/* 
 * BundUpdateStatsTimer()
 */

void
BundUpdateStatsTimer(void *cookie)
{
    Bund	b = (Bund)cookie;
    int		k;
  
    BundUpdateStats(b);
    for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	if (b->links[k] && b->links[k]->joined_bund)
	    LinkUpdateStats(b->links[k]);
    }
}

/*
 * BundResetStats()
 */

void
BundResetStats(Bund b)
{
  NgFuncClrStats(b, NG_PPP_BUNDLE_LINKNUM);
  memset(&b->stats, 0, sizeof(b->stats));
#ifndef NG_PPP_STATS64
  memset(&b->oldStats, 0, sizeof(b->oldStats));
#endif
}

/*
 * BundShowLinks()
 */

void
BundShowLinks(Context ctx, Bund sb)
{
    int		j;

    for (j = 0; j < NG_PPP_MAX_LINKS; j++) {
	if (sb->links[j]) {
	    Printf("%s[%s/%s] ", sb->links[j]->name,
		FsmStateName(sb->links[j]->lcp.fsm.state),
		gPhysStateNames[sb->links[j]->state]);
	}
    }
    Printf("\r\n");
}

/*
 * BundFind()
 *
 * Find a bundle structure
 */

Bund
BundFind(const char *name)
{
  int	k;

  for (k = 0;
    k < gNumBundles && (!gBundles[k] || strcmp(gBundles[k]->name, name));
    k++);
  return((k < gNumBundles) ? gBundles[k] : NULL);
}

/*
 * BundBmStart()
 *
 * Start bandwidth management timer
 */

static void
BundBmStart(Bund b)
{
    int	k;

    /* Reset bandwidth management stats */
    memset(&b->bm.traffic, 0, sizeof(b->bm.traffic));
    memset(&b->bm.avail, 0, sizeof(b->bm.avail));
    memset(&b->bm.wasUp, 0, sizeof(b->bm.wasUp));
    for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	if (b->links[k]) {
	    memset(&b->links[k]->bm.idleStats,
    		0, sizeof(b->links[k]->bm.idleStats));
	}
    }

  /* Start bandwidth management timer */
  TimerStop(&b->bm.bmTimer);
  if (Enabled(&b->conf.options, BUND_CONF_BWMANAGE)) {
    TimerInit(&b->bm.bmTimer, "BundBm",
      (b->conf.bm_S * SECONDS) / BUND_BM_N,
      BundBmTimeout, b);
    TimerStart(&b->bm.bmTimer);
  }
}

/*
 * BundBmStop()
 */

static void
BundBmStop(Bund b)
{
  TimerStop(&b->bm.bmTimer);
}

/*
 * BundBmTimeout()
 *
 * Do a bandwidth management update
 */

static void
BundBmTimeout(void *arg)
{
    Bund		b = (Bund)arg;

    const time_t	now = time(NULL);
    u_int		availTotal;
    u_int		inUtilTotal = 0, outUtilTotal = 0;
    u_int		inBitsTotal, outBitsTotal;
    u_int		inUtil[BUND_BM_N];	/* Incoming % utilization */
    u_int		outUtil[BUND_BM_N];	/* Outgoing % utilization */
    int			j, k;

    /* Shift and update stats */
    memmove(&b->bm.wasUp[1], &b->bm.wasUp[0],
	(BUND_BM_N - 1) * sizeof(b->bm.wasUp[0]));
    b->bm.wasUp[0] = b->n_up;
    memmove(&b->bm.avail[1], &b->bm.avail[0],
	(BUND_BM_N - 1) * sizeof(b->bm.avail[0]));
    b->bm.avail[0] = b->bm.total_bw;

    /* Shift stats */
    memmove(&b->bm.traffic[0][1], &b->bm.traffic[0][0],
	(BUND_BM_N - 1) * sizeof(b->bm.traffic[0][0]));
    memmove(&b->bm.traffic[1][1], &b->bm.traffic[1][0],
	(BUND_BM_N - 1) * sizeof(b->bm.traffic[1][0]));
    b->bm.traffic[0][0] = 0;
    b->bm.traffic[1][0] = 0;
    for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
	if (b->links[k] && b->links[k]->joined_bund) {
	    Link	const l = b->links[k];

	    struct ng_ppp_link_stat	oldStats;
	
	    /* Get updated link traffic statistics */
	    oldStats = l->bm.idleStats;
	    NgFuncGetStats(l->bund, l->bundleIndex, &l->bm.idleStats);
	    b->bm.traffic[0][0] += l->bm.idleStats.recvOctets - oldStats.recvOctets;
	    b->bm.traffic[1][0] += l->bm.idleStats.xmitOctets - oldStats.xmitOctets;
	}
    }

    /* Compute utilizations */
    memset(&inUtil, 0, sizeof(inUtil));
    memset(&outUtil, 0, sizeof(outUtil));
    availTotal = inBitsTotal = outBitsTotal = 0;
    for (j = 0; j < BUND_BM_N; j++) {
	u_int	avail, inBits, outBits;

	avail = (b->bm.avail[j] * b->conf.bm_S) / BUND_BM_N;
	inBits = b->bm.traffic[0][j] * 8;
	outBits = b->bm.traffic[1][j] * 8;

	availTotal += avail;
	inBitsTotal += inBits;
	outBitsTotal += outBits;

	/* Compute bandwidth utilizations as percentages */
	if (avail != 0) {
    	    inUtil[j] = ((float) inBits / avail) * 100;
    	    outUtil[j] = ((float) outBits / avail) * 100;
	}
    }

    /* Compute total averaged utilization */
    if (availTotal != 0) {
	inUtilTotal = ((float) inBitsTotal / availTotal) * 100;
	outUtilTotal = ((float) outBitsTotal / availTotal) * 100;
    }

  {
    char	ins[100], outs[100];

    ins[0] = 0;
    for (j = 0; j < BUND_BM_N; j++) {
	snprintf(ins + strlen(ins), sizeof(ins) - strlen(ins),
		" %3u ", b->bm.wasUp[BUND_BM_N - 1 - j]);
    }
    Log(LG_BUND2, ("[%s]                       %s", b->name, ins));

    snprintf(ins, sizeof(ins), " IN util: total %3u%%  ", inUtilTotal);
    snprintf(outs, sizeof(outs), "OUT util: total %3u%%  ", outUtilTotal);
    for (j = 0; j < BUND_BM_N; j++) {
      snprintf(ins + strlen(ins), sizeof(ins) - strlen(ins),
	" %3u%%", inUtil[BUND_BM_N - 1 - j]);
      snprintf(outs + strlen(outs), sizeof(outs) - strlen(outs),
	" %3u%%", outUtil[BUND_BM_N - 1 - j]);
    }
    Log(LG_BUND2, ("[%s] %s", b->name, ins));
    Log(LG_BUND2, ("[%s] %s", b->name, outs));
  }

  /* See if it's time to bring up another link */
  if (now - b->bm.last_open >= b->conf.bm_Mc
      && (inUtilTotal >= b->conf.bm_Hi || outUtilTotal >= b->conf.bm_Hi)) {
    for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
cont:
	if (!b->links[k] && b->conf.linkst[k][0])
		break;
    }
    if (k < NG_PPP_MAX_LINKS) {
	Log(LG_BUND, ("[%s] opening link \"%s\" due to increased demand",
    	    b->name, b->conf.linkst[k]));
	b->bm.last_open = now;
	if (b->links[k]) {
	    RecordLinkUpDownReason(NULL, b->links[k], 1, STR_PORT_NEEDED, NULL);
	    BundOpenLink(b->links[k]);
	} else {
	    if (BundCreateOpenLink(b, k)) {
		if (k < NG_PPP_MAX_LINKS) {
		    k++;
		    goto cont;
		}
	    } else
		RecordLinkUpDownReason(NULL, b->links[k], 1, STR_PORT_NEEDED, NULL);
	}
    }
  }

  /* See if it's time to bring down a link */
  if (now - b->bm.last_close >= b->conf.bm_Md
      && (inUtilTotal < b->conf.bm_Lo && outUtilTotal < b->conf.bm_Lo)
      && b->n_links > 1) {
    k = NG_PPP_MAX_LINKS - 1;
    while (k >= 0 && (!b->links[k] || !OPEN_STATE(b->links[k]->lcp.fsm.state)))
	k--;
    assert(k >= 0);
    Log(LG_BUND, ("[%s] Bundle: closing link %s due to reduced demand",
      b->name, b->links[k]->name));
    b->bm.last_close = now;
    RecordLinkUpDownReason(NULL, b->links[k], 0, STR_PORT_UNNEEDED, NULL);
    BundCloseLink(b->links[k]);
  }

  /* Restart timer */
  TimerStart(&b->bm.bmTimer);
}

/*
 * BundNgInit()
 *
 * Setup the initial PPP netgraph framework. Initializes these fields
 * in the supplied bundle structure:
 *
 *	iface.ifname	- Interface name
 *	csock		- Control socket for socket netgraph node
 *	dsock		- Data socket for socket netgraph node
 *
 * Returns -1 if error.
 */

static int
BundNgInit(Bund b)
{
    struct ngm_mkpeer	mp;
    struct ngm_name	nm;
    int			newIface = 0;
    int			newPpp = 0;

    /* Create new iface node */
    if (NgFuncCreateIface(b,
	b->iface.ifname, sizeof(b->iface.ifname)) < 0) {
      Log(LG_ERR, ("[%s] can't create netgraph interface", b->name));
      goto fail;
    }
    newIface = 1;
    b->iface.ifindex = if_nametoindex(b->iface.ifname);
    Log(LG_BUND|LG_IFACE, ("[%s] Bundle: Interface %s created",
	b->name, b->iface.ifname));
 
    /* Create new PPP node */
    snprintf(b->hook, sizeof(b->hook), "b%d", b->id);

    strcpy(mp.type, NG_PPP_NODE_TYPE);
    strcpy(mp.ourhook, b->hook);
    strcpy(mp.peerhook, NG_PPP_HOOK_BYPASS);
    if (NgSendMsg(gLinksCsock, ".:",
    	    NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
    	    b->name, mp.type, ".:", mp.ourhook, strerror(errno)));
	goto fail;
    }
    newPpp = 1;

    /* Get PPP node ID */
    b->nodeID = NgGetNodeID(gLinksCsock, b->hook);

    /* Give it a name */
    snprintf(nm.name, sizeof(nm.name), "mpd%d-%s", gPid, b->name);
    if (NgSendMsg(gLinksCsock, b->hook,
    	    NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
	Log(LG_ERR, ("[%s] can't name %s node \"%s\": %s",
    	    b->name, NG_PPP_NODE_TYPE, b->hook, strerror(errno)));
	goto fail;
    }

    /* OK */
    return(0);

fail:
    BundNgShutdown(b, newIface, newPpp);
    return(-1);
}

/*
 * NgFuncShutdown()
 */

void
BundNgShutdown(Bund b, int iface, int ppp)
{
    char	path[NG_PATHSIZ];

    if (iface) {
	snprintf(path, sizeof(path), "%s:", b->iface.ifname);
	NgFuncShutdownNode(gLinksCsock, b->name, path);
    }
    if (ppp) {
	snprintf(path, sizeof(path), "[%x]:", b->nodeID);
	NgFuncShutdownNode(gLinksCsock, b->name, path);
    }
    b->hook[0] = 0;
}

/*
 * BundSetCommand()
 */

static int
BundSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    Bund	b = ctx->bund;
    int		i, val;

    if (ac == 0)
	return(-1);
    switch ((intptr_t)arg) {
	case SET_PERIOD:
    	    b->conf.bm_S = atoi(*av);
    	    break;
	case SET_LOW_WATER:
    	    b->conf.bm_Lo = atoi(*av);
    	    break;
	case SET_HIGH_WATER:
    	    b->conf.bm_Hi = atoi(*av);
    	    break;
	case SET_MIN_CONNECT:
    	    b->conf.bm_Mc = atoi(*av);
    	    break;
	case SET_MIN_DISCONNECT:
    	    b->conf.bm_Md = atoi(*av);
    	    break;
	case SET_LINKS:
	    if (ac > NG_PPP_MAX_LINKS)
		return (-1);
    	    for (i = 0; i < ac; i++)
		strlcpy(b->conf.linkst[i], av[i], LINK_MAX_NAME);
    	    for (; i < NG_PPP_MAX_LINKS; i++)
	        b->conf.linkst[i][0] = 0;
    	    break;

	case SET_RETRY:
    	    val = atoi(*av);
    	    if (val < 1 || val > 10)
		Error("[%s] incorrect fsm-timeout value %d", b->name, val);
	    else
		b->conf.retry_timeout = val;
    	    break;

	case SET_ACCEPT:
    	    AcceptCommand(ac, av, &b->conf.options, gConfList);
    	    break;

	case SET_DENY:
    	    DenyCommand(ac, av, &b->conf.options, gConfList);
    	    break;

	case SET_ENABLE:
    	    EnableCommand(ac, av, &b->conf.options, gConfList);
    	    break;

	case SET_DISABLE:
    	    DisableCommand(ac, av, &b->conf.options, gConfList);
    	    break;

	case SET_YES:
    	    YesCommand(ac, av, &b->conf.options, gConfList);
    	    break;

	case SET_NO:
    	    NoCommand(ac, av, &b->conf.options, gConfList);
    	    break;

	default:
    	    assert(0);
    }
    return(0);
}

