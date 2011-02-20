
/*
 * link.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "link.h"
#include "msg.h"
#include "lcp.h"
#include "phys.h"
#include "command.h"
#include "input.h"
#include "ngfunc.h"
#include "util.h"

#include <netgraph.h>
#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_tee.h>

/*
 * DEFINITIONS
 */

  /* Set menu options */
  enum {
    SET_BUNDLE,
    SET_FORWARD,
    SET_DROP,
    SET_CLEAR,
    SET_BANDWIDTH,
    SET_LATENCY,
    SET_ACCMAP,
    SET_MRRU,
    SET_MRU,
    SET_MTU,
    SET_FSM_RETRY,
    SET_MAX_RETRY,
    SET_RETRY_DELAY,
    SET_MAX_CHILDREN,
    SET_KEEPALIVE,
    SET_IDENT,
    SET_ACCEPT,
    SET_DENY,
    SET_ENABLE,
    SET_DISABLE,
    SET_YES,
    SET_NO
  };

  #define RBUF_SIZE		100

/*
 * INTERNAL FUNCTIONS
 */

  static int	LinkSetCommand(Context ctx, int ac, char *av[], void *arg);
  static void	LinkMsg(int type, void *cookie);
  static void	LinkNgDataEvent(int type, void *cookie);
  static void	LinkReopenTimeout(void *arg);

/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab LinkSetActionCmds[] = {
    { "bundle {bundle} [{regex}]",	"Terminate incomings locally",
	LinkSetCommand, NULL, 2, (void *) SET_BUNDLE },
    { "forward {link} [{regex}]",	"Forward incomings",
	LinkSetCommand, NULL, 2, (void *) SET_FORWARD },
    { "drop [{regex}]",			"drop incomings",
	LinkSetCommand, NULL, 2, (void *) SET_DROP },
    { "clear",				"Clear actions",
	LinkSetCommand, NULL, 2, (void *) SET_CLEAR },
    { NULL },
  };

  const struct cmdtab LinkSetCmds[] = {
    { "action ...",			"Set action on incoming",
	CMD_SUBMENU,	NULL, 2, (void *) LinkSetActionCmds },
    { "bandwidth {bps}",		"Link bandwidth",
	LinkSetCommand, NULL, 2, (void *) SET_BANDWIDTH },
    { "latency {microsecs}",		"Link latency",
	LinkSetCommand, NULL, 2, (void *) SET_LATENCY },
    { "accmap {hex-value}",		"Accmap value",
	LinkSetCommand, NULL, 2, (void *) SET_ACCMAP },
    { "mrru {value}",			"Link MRRU value",
	LinkSetCommand, NULL, 2, (void *) SET_MRRU },
    { "mru {value}",			"Link MRU value",
	LinkSetCommand, NULL, 2, (void *) SET_MRU },
    { "mtu {value}",			"Link MTU value",
	LinkSetCommand, NULL, 2, (void *) SET_MTU },
    { "fsm-timeout {seconds}",		"FSM retry timeout",
	LinkSetCommand, NULL, 2, (void *) SET_FSM_RETRY },
    { "max-redial {num}",		"Max connect attempts",
	LinkSetCommand, NULL, 2, (void *) SET_MAX_RETRY },
    { "redial-delay {num}",		"Delay between connect attempts",
	LinkSetCommand, NULL, 2, (void *) SET_RETRY_DELAY },
    { "max-children {num}",		"Max number of children",
	LinkSetCommand, NULL, 2, (void *) SET_MAX_CHILDREN },
    { "keep-alive {secs} {max}",	"LCP echo keep-alives",
	LinkSetCommand, NULL, 2, (void *) SET_KEEPALIVE },
    { "ident {string}",			"LCP ident string",
	LinkSetCommand, NULL, 2, (void *) SET_IDENT },
    { "accept {opt ...}",		"Accept option",
	LinkSetCommand, NULL, 2, (void *) SET_ACCEPT },
    { "deny {opt ...}",			"Deny option",
	LinkSetCommand, NULL, 2, (void *) SET_DENY },
    { "enable {opt ...}",		"Enable option",
	LinkSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable {opt ...}",		"Disable option",
	LinkSetCommand, NULL, 2, (void *) SET_DISABLE },
    { "yes {opt ...}",			"Enable and accept option",
	LinkSetCommand, NULL, 2, (void *) SET_YES },
    { "no {opt ...}",			"Disable and deny option",
	LinkSetCommand, NULL, 2, (void *) SET_NO },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static struct confinfo	gConfList[] = {
    { 0,	LINK_CONF_INCOMING,	"incoming"	},
    { 1,	LINK_CONF_PAP,		"pap"		},
    { 1,	LINK_CONF_CHAPMD5,	"chap-md5"	},
    { 1,	LINK_CONF_CHAPMSv1,	"chap-msv1"	},
    { 1,	LINK_CONF_CHAPMSv2,	"chap-msv2"	},
    { 1,	LINK_CONF_EAP,		"eap"		},
    { 1,	LINK_CONF_ACFCOMP,	"acfcomp"	},
    { 1,	LINK_CONF_PROTOCOMP,	"protocomp"	},
    { 0,	LINK_CONF_MSDOMAIN,	"keep-ms-domain"},
    { 0,	LINK_CONF_MAGICNUM,	"magicnum"	},
    { 0,	LINK_CONF_PASSIVE,	"passive"	},
    { 0,	LINK_CONF_CHECK_MAGIC,	"check-magic"	},
    { 0,	LINK_CONF_NO_ORIG_AUTH,	"no-orig-auth"	},
    { 0,	LINK_CONF_CALLBACK,	"callback"	},
    { 0,	LINK_CONF_MULTILINK,	"multilink"	},
    { 1,	LINK_CONF_SHORTSEQ,	"shortseq"	},
    { 0,	LINK_CONF_TIMEREMAIN,	"time-remain"	},
    { 0,	LINK_CONF_PEER_AS_CALLING,	"peer-as-calling"	},
    { 0,	LINK_CONF_REPORT_MAC,	"report-mac"	},
    { 0,	0,			NULL		},
  };

    int		gLinksCsock = -1;		/* Socket node control socket */
    int		gLinksDsock = -1;		/* Socket node data socket */
    EventRef	gLinksDataEvent;

int
LinksInit(void)
{
    char	name[NG_NODESIZ];

    /* Create a netgraph socket node */
    snprintf(name, sizeof(name), "mpd%d-lso", gPid);
    if (NgMkSockNode(name, &gLinksCsock, &gLinksDsock) < 0) {
	Log(LG_ERR, ("LinksInit(): can't create %s node: %s",
    	    NG_SOCKET_NODE_TYPE, strerror(errno)));
	return(-1);
    }
    (void) fcntl(gLinksCsock, F_SETFD, 1);
    (void) fcntl(gLinksDsock, F_SETFD, 1);

    /* Listen for happenings on our node */
    EventRegister(&gLinksDataEvent, EVENT_READ,
	gLinksDsock, EVENT_RECURRING, LinkNgDataEvent, NULL);
	
    return (0);
}

void
LinksShutdown(void)
{
    close(gLinksCsock);
    gLinksCsock = -1;
    EventUnRegister(&gLinksDataEvent);
    close(gLinksDsock);
    gLinksDsock = -1;
}

/*
 * LinkOpenCmd()
 */

int
LinkOpenCmd(Context ctx)
{
    if (ctx->lnk->tmpl)
	Error("impossible to open template");
    RecordLinkUpDownReason(NULL, ctx->lnk, 1, STR_MANUALLY, NULL);
    LinkOpen(ctx->lnk);
    return (0);
}

/*
 * LinkCloseCmd()
 */

int
LinkCloseCmd(Context ctx)
{
    if (ctx->lnk->tmpl)
	Error("impossible to close template");
    RecordLinkUpDownReason(NULL, ctx->lnk, 0, STR_MANUALLY, NULL);
    LinkClose(ctx->lnk);
    return (0);
}

/*
 * LinkOpen()
 */

void
LinkOpen(Link l)
{
    REF(l);
    MsgSend(&l->msgs, MSG_OPEN, l);
}

/*
 * LinkClose()
 */

void
LinkClose(Link l)
{
    REF(l);
    MsgSend(&l->msgs, MSG_CLOSE, l);
}

/*
 * LinkUp()
 */

void
LinkUp(Link l)
{
    Log(LG_LINK, ("[%s] Link: UP event", l->name));

    l->originate = PhysGetOriginate(l);
    Log(LG_PHYS2, ("[%s] Link: origination is %s",
	l->name, LINK_ORIGINATION(l->originate)));
    LcpUp(l);
}

/*
 * LinkDown()
 */

void
LinkDown(Link l)
{
    Log(LG_LINK, ("[%s] Link: DOWN event", l->name));

    if (OPEN_STATE(l->lcp.fsm.state)) {
	if (((l->conf.max_redial != 0) && (l->num_redial >= l->conf.max_redial)) ||
	    gShutdownInProgress) {
	    if (l->conf.max_redial >= 0) {
		Log(LG_LINK, ("[%s] Link: giving up after %d reconnection attempts",
		  l->name, l->num_redial));
	    }
	    if (!l->stay)
		l->die = 1;
	    LcpClose(l);
            LcpDown(l);
	} else {
	    int delay = l->conf.redial_delay + ((random() ^ l->id ^ gPid) & 3);

	    TimerStop(&l->openTimer);
	    TimerInit(&l->openTimer, "PhysOpen",
	        delay * SECONDS, LinkReopenTimeout, l);
	    TimerStart(&l->openTimer);

    	    LcpDown(l);

	    l->num_redial++;
	    Log(LG_LINK, ("[%s] Link: reconnection attempt %d in %d seconds",
	      l->name, l->num_redial, delay));
	}
    } else {
	if (!l->stay)
	    l->die = 1;
        LcpDown(l);
    }
}

/*
 * LinkReopenTimeout()
 */

static void
LinkReopenTimeout(void *arg)
{
    Link	const l = (Link)arg;

    if (gShutdownInProgress) {
	LcpClose(l);
	return;
    }

    Log(LG_LINK, ("[%s] Link: reconnection attempt %d",
	l->name, l->num_redial));
    RecordLinkUpDownReason(NULL, l, 1, STR_REDIAL, NULL);
    PhysOpen(l);
}

/*
 * LinkMsg()
 *
 * Deal with incoming message to this link
 */

static void
LinkMsg(int type, void *arg)
{
    Link	l = (Link)arg;

    if (l->dead) {
	UNREF(l);
	return;
    }
    Log(LG_LINK, ("[%s] Link: %s event", l->name, MsgName(type)));
    switch (type) {
	case MSG_OPEN:
    	    l->num_redial = 0;
    	    LcpOpen(l);
    	    break;
	case MSG_CLOSE:
	    TimerStop(&l->openTimer);
    	    LcpClose(l);
    	    break;
	case MSG_SHUTDOWN:
    	    LinkShutdown(l);
    	    break;
	default:
    	    assert(FALSE);
    }
    UNREF(l);
}

/*
 * LinkCreate()
 */

int
LinkCreate(Context ctx, int ac, char *av[], void *arg)
{
    Link 	l, lt = NULL;
    PhysType    pt = NULL;
    u_char 	tmpl = 0;
    u_char 	stay = 0;
    int 	k;

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

    if (ac != stay + 2)
	return(-1);

    if (strlen(av[0 + stay]) >= (LINK_MAX_NAME - tmpl * 5))
	Error("Link name \"%s\" is too long", av[0 + stay]);

    /* See if link name already taken */
    if ((l = LinkFind(av[0 + stay])) != NULL)
	Error("Link \"%s\" already exists", av[0 + stay]);

    for (k = 0; (pt = gPhysTypes[k]); k++) {
        if (!strcmp(pt->name, av[0 + stay]))
	    Error("Name \"%s\" is reserved by device type", av[0 + stay]);
    }

    /* Locate type */
    for (k = 0; (pt = gPhysTypes[k]); k++) {
        if (!strcmp(pt->name, av[1 + stay]))
	    break;
    }
    if (pt != NULL) {
        if (!pt->tmpl && tmpl)
    	    Error("Link type \"%s\" does not support templating", av[1 + stay]);

    } else {
        /* See if template name specified */
	if ((lt = LinkFind(av[1 + stay])) == NULL)
	    Error("Link template \"%s\" not found", av[1 + tmpl]);
	if (!lt->tmpl)
	    Error("Link \"%s\" is not a template", av[1 + stay]);
    }

    /* Create and initialize new link */
    if (lt) {
	l = LinkInst(lt, av[0 + stay], tmpl, stay);
    } else {
	l = Malloc(MB_LINK, sizeof(*l));
	strlcpy(l->name, av[0 + stay], sizeof(l->name));
	l->type = pt;
	l->tmpl = tmpl;
	l->stay = stay;
	l->parent = -1;
	SLIST_INIT(&l->actions);

	/* Initialize link configuration with defaults */
	l->conf.mru = LCP_DEFAULT_MRU;
        l->conf.mtu = LCP_DEFAULT_MRU;
	l->conf.mrru = MP_DEFAULT_MRRU;
        l->conf.accmap = 0x000a0000;
        l->conf.max_redial = -1;
        l->conf.redial_delay = 1;
        l->conf.retry_timeout = LINK_DEFAULT_RETRY;
	l->conf.max_children = 10000;
        l->bandwidth = LINK_DEFAULT_BANDWIDTH;
        l->latency = LINK_DEFAULT_LATENCY;
        l->upReason = NULL;
        l->upReasonValid = 0;
        l->downReason = NULL;
        l->downReasonValid = 0;

        Disable(&l->conf.options, LINK_CONF_CHAPMD5);
        Accept(&l->conf.options, LINK_CONF_CHAPMD5);

        Disable(&l->conf.options, LINK_CONF_CHAPMSv1);
        Deny(&l->conf.options, LINK_CONF_CHAPMSv1);

        Disable(&l->conf.options, LINK_CONF_CHAPMSv2);
        Accept(&l->conf.options, LINK_CONF_CHAPMSv2);

        Disable(&l->conf.options, LINK_CONF_PAP);
	Accept(&l->conf.options, LINK_CONF_PAP);

        Disable(&l->conf.options, LINK_CONF_EAP);
        Accept(&l->conf.options, LINK_CONF_EAP);

        Disable(&l->conf.options, LINK_CONF_MSDOMAIN);

        Enable(&l->conf.options, LINK_CONF_ACFCOMP);
        Accept(&l->conf.options, LINK_CONF_ACFCOMP);

        Enable(&l->conf.options, LINK_CONF_PROTOCOMP);
        Accept(&l->conf.options, LINK_CONF_PROTOCOMP);

        Enable(&l->conf.options, LINK_CONF_MAGICNUM);
        Disable(&l->conf.options, LINK_CONF_PASSIVE);
        Enable(&l->conf.options, LINK_CONF_CHECK_MAGIC);

	Disable(&l->conf.options, LINK_CONF_MULTILINK);
	Enable(&l->conf.options, LINK_CONF_SHORTSEQ);
	Accept(&l->conf.options, LINK_CONF_SHORTSEQ);

        PhysInit(l);
        LcpInit(l);
	
	MsgRegister(&l->msgs, LinkMsg);

	/* Find a free link pointer */
        for (k = 0; k < gNumLinks && gLinks[k] != NULL; k++);
        if (k == gNumLinks)			/* add a new link pointer */
    	    LengthenArray(&gLinks, sizeof(*gLinks), &gNumLinks, MB_LINK);
	    
	l->id = k;
	gLinks[k] = l;
	REF(l);
    }

    RESETREF(ctx->lnk, l);

    return (0);
}

/*
 * LinkDestroy()
 */

int
LinkDestroy(Context ctx, int ac, char *av[], void *arg)
{
    Link 	l;

    if (ac > 1)
	return(-1);

    if (ac == 1) {
	if ((l = LinkFind(av[0])) == NULL)
	    Error("Link \"%s\" not found", av[0]);
    } else {
	if (ctx->lnk) {
	    l = ctx->lnk;
	} else
	    Error("No link selected to destroy");
    }
    
    if (l->tmpl) {
	l->tmpl = 0;
	l->stay = 0;
	LinkShutdown(l);
    } else {
	l->stay = 0;
	if (l->rep) {
	    PhysClose(l);
	} else if (OPEN_STATE(l->lcp.fsm.state)) {
	    LcpClose(l);
	} else {
	    l->die = 1; /* Hack! We should do it as we changed l->stay */
	    LinkShutdownCheck(l, l->lcp.fsm.state);
	}
    }

    return (0);
}

/*
 * LinkInst()
 */

Link
LinkInst(Link lt, char *name, int tmpl, int stay)
{
    Link 	l;
    int		k;
    struct linkaction	*a, *ap, *at;

    /* Create and initialize new link */
    l = Mdup(MB_LINK, lt, sizeof(*l));
    
    ap = NULL;
    SLIST_INIT(&l->actions);
    SLIST_FOREACH(at, &lt->actions, next) {
	a = Mdup(MB_AUTH, at, sizeof(*a));
	regcomp(&a->regexp, a->regex, REG_EXTENDED);
	if (!ap)
	    SLIST_INSERT_HEAD(&l->actions, a, next);
	else
	    SLIST_INSERT_AFTER(ap, a, next);
	ap = a;
    }
    l->tmpl = tmpl;
    l->stay = stay;
    /* Count link as one more child of parent. */
    gChildren++;
    lt->children++;
    l->parent = lt->id;
    l->children = 0;
    l->refs = 0;

    /* Find a free link pointer */
    for (k = 0; k < gNumLinks && gLinks[k] != NULL; k++);
    if (k == gNumLinks)			/* add a new link pointer */
	LengthenArray(&gLinks, sizeof(*gLinks), &gNumLinks, MB_LINK);

    l->id = k;

    if (name)
	strlcpy(l->name, name, sizeof(l->name));
    else
	snprintf(l->name, sizeof(l->name), "%s-%d", lt->name, k);
    gLinks[k] = l;
    REF(l);

    PhysInst(l, lt);
    LcpInst(l, lt);

    return (l);
}

void
LinkShutdownCheck(Link l, short state)
{
    if (state == ST_INITIAL && l->lcp.auth.acct_thread == NULL &&
	    l->die && !l->stay && l->state == PHYS_STATE_DOWN) {
	REF(l);
	MsgSend(&l->msgs, MSG_SHUTDOWN, l);
    }
}

/*
 * LinkShutdown()
 *
 */

void
LinkShutdown(Link l)
{
    struct linkaction	*a;

    Log(LG_LINK, ("[%s] Link: Shutdown", l->name));

    /* Late divorce for DoD case */
    if (l->bund) {
	l->bund->links[l->bundleIndex] = NULL;
	l->bund->n_links--;
	l->bund = NULL;
    }
    gLinks[l->id] = NULL;
    /* Our parent lost one children */
    if (l->parent >= 0) {
	gChildren--;
	gLinks[l->parent]->children--;
    }
    /* Our children are orphans */
    if (l->children) {
	int k;
	for (k = 0; k < gNumLinks; k++) {
	    if (gLinks[k] && gLinks[k]->parent == l->id)
		gLinks[k]->parent = -1;
	}
    }
    MsgUnRegister(&l->msgs);
    if (l->hook[0])
	LinkNgShutdown(l);
    PhysShutdown(l);
    LcpShutdown(l);
    l->dead = 1;
    while ((a = SLIST_FIRST(&l->actions)) != NULL) {
	SLIST_REMOVE_HEAD(&l->actions, next);
	if (a->regex[0])
	    regfree(&a->regexp);
	Freee(a);
    }
    if (l->upReason)
	Freee(l->upReason);
    if (l->downReason)
	Freee(l->downReason);
    MsgUnRegister(&l->msgs);
    UNREF(l);
    CheckOneShot();
}

/*
 * LinkNgInit()
 *
 * Setup the initial link framework.
 *
 * Returns -1 if error.
 */

int
LinkNgInit(Link l)
{
    struct ngm_mkpeer	mp;
    struct ngm_name	nm;

    /* Create TEE node */
    strcpy(mp.type, NG_TEE_NODE_TYPE);
    snprintf(mp.ourhook, sizeof(mp.ourhook), "l%d", l->id);
    strcpy(mp.peerhook, NG_TEE_HOOK_LEFT2RIGHT);
    if (NgSendMsg(gLinksCsock, ".:",
      NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s",
    	    l->name, mp.type, ".:", mp.ourhook, strerror(errno)));
	goto fail;
    }
    strlcpy(l->hook, mp.ourhook, sizeof(l->hook));

    /* Give it a name */
    snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-lt", gPid, l->name);
    if (NgSendMsg(gLinksCsock, l->hook,
      NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
	Log(LG_ERR, ("[%s] can't name %s node \"%s\": %s",
    	    l->name, NG_TEE_NODE_TYPE, l->hook, strerror(errno)));
	goto fail;
    }

    /* Get TEE node ID */
    if ((l->nodeID = NgGetNodeID(gLinksCsock, l->hook)) == 0) {
	Log(LG_ERR, ("[%s] Cannot get %s node id: %s",
	    l->name, NG_TEE_NODE_TYPE, strerror(errno)));
	goto fail;
    };

    /* OK */
    return(0);

fail:
    LinkNgShutdown(l);
    return(-1);
}

/*
 * LinkNgJoin()
 */

int
LinkNgJoin(Link l)
{
    char		path[NG_PATHSIZ];
    struct ngm_connect	cn;

    snprintf(path, sizeof(path), "[%lx]:", (u_long)l->nodeID);

    snprintf(cn.path, sizeof(cn.path), "[%lx]:", (u_long)l->bund->nodeID);
    strcpy(cn.ourhook, NG_TEE_HOOK_RIGHT);
    snprintf(cn.peerhook, sizeof(cn.peerhook), "%s%d", 
	NG_PPP_HOOK_LINK_PREFIX, l->bundleIndex);
    if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    	    l->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
	return(-1);
    }
    
    NgFuncDisconnect(gLinksCsock, l->name, path, NG_TEE_HOOK_LEFT2RIGHT);
    return (0);
}

/*
 * LinkNgLeave()
 */

int
LinkNgLeave(Link l)
{
    char		path[NG_PATHSIZ];
    struct ngm_connect	cn;

    snprintf(cn.path, sizeof(cn.path), "[%lx]:", (u_long)l->nodeID);
    strcpy(cn.ourhook, l->hook);
    strcpy(cn.peerhook, NG_TEE_HOOK_LEFT2RIGHT);
    if (NgSendMsg(gLinksCsock, ".:",
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    	    l->name, ".:", cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
	return(-1);
    }

    snprintf(path, sizeof(path), "[%lx]:", (u_long)l->nodeID);
    NgFuncDisconnect(gLinksCsock, l->name, path, NG_TEE_HOOK_RIGHT);
    return (0);
}

/*
 * LinkNgToRep()
 */

int
LinkNgToRep(Link l)
{
    char		path[NG_PATHSIZ];
    struct ngm_connect	cn;

    /* Connect link to repeater */
    snprintf(path, sizeof(path), "[%lx]:", (u_long)l->nodeID);
    strcpy(cn.ourhook, NG_TEE_HOOK_RIGHT);
    if (!PhysGetUpperHook(l, cn.path, cn.peerhook)) {
        Log(LG_PHYS, ("[%s] Link: can't get repeater hook", l->name));
        return (-1);
    }
    if (NgSendMsg(gLinksCsock, path,
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
	Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    	    l->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
	return(-1);
    }

    /* Shutdown link tee node */
    NgFuncShutdownNode(gLinksCsock, l->name, path);
    l->hook[0] = 0;
    return (0);
}

/*
 * LinkNgShutdown()
 */

void
LinkNgShutdown(Link l)
{
    if (l->hook[0])
	NgFuncShutdownNode(gLinksCsock, l->name, l->hook);
    l->hook[0] = 0;
}

/*
 * LinkNgDataEvent()
 */

static void
LinkNgDataEvent(int type, void *cookie)
{
    Link		l;
    Bund		b;
    u_char		*buf;
    u_int16_t		proto;
    int			ptr;
    Mbuf		bp;
    struct sockaddr_ng	naddr;
    socklen_t		nsize;
    char		*name, *rest;
    int			id, num = 0;

    /* Read all available packets */
    while (1) {
	if (num > 20)
	    return;
	bp = mballoc(4096);
	buf = MBDATA(bp);
	/* Read data */
	nsize = sizeof(naddr);
	if ((bp->cnt = recvfrom(gLinksDsock, buf, MBSPACE(bp), MSG_DONTWAIT, (struct sockaddr *)&naddr, &nsize)) < 0) {
	    mbfree(bp);
	    if (errno == EAGAIN)
    		return;
	    Log(LG_LINK, ("Link socket read error: %s", strerror(errno)));
	    return;
	}
	num++;

	name = naddr.sg_data;
	switch (name[0]) {
	case 'l':
	    name++;
	    id = strtol(name, &rest, 10);
	    if (rest[0] != 0 || !gLinks[id]) {
    		Log(LG_ERR, ("Link: packet from unexisting link \"%s\"",
    		    name));
		mbfree(bp);
		continue;
	    }
	    if (gLinks[id]->dead) {
    		Log(LG_LINK, ("Link: Packet from dead link \"%s\"", name));
		mbfree(bp);
		continue;
	    }
	    l = gLinks[id];

	    /* Extract protocol */
	    ptr = 0;
	    if ((buf[0] == 0xff) && (buf[1] == 0x03))
		ptr = 2;
	    proto = buf[ptr++];
	    if ((proto & 0x01) == 0)
		proto = (proto << 8) + buf[ptr++];

	    if (MBLEN(bp) <= ptr) {
		LogDumpBp(LG_FRAME|LG_ERR, bp,
    		    "[%s] rec'd truncated %d bytes frame from link",
    		    l->name, MBLEN(bp));
		mbfree(bp);
		continue;
	    }

	    /* Debugging */
	    LogDumpBp(LG_FRAME, bp,
    		"[%s] rec'd %d bytes frame from link proto=0x%04x",
    		l->name, MBLEN(bp), proto);
      
	    bp = mbadj(bp, ptr);

	    /* Input frame */
	    InputFrame(l->bund, l, proto, bp);
	    break;
	case 'b':
	case 'i':
	case 'o':
	case '4':
	case '6':
	    name++;
	    id = strtol(name, &rest, 10);
	    if (rest[0] != 0 || !gBundles[id]) {
    		Log(LG_ERR, ("Link: Packet from unexisting bundle \"%s\"",
    		    name));
		mbfree(bp);
		continue;
	    }
	    if (gBundles[id]->dead) {
    		Log(LG_LINK, ("Link: Packet from dead bundle \"%s\"", name));
		mbfree(bp);
		continue;
	    }
	    b = gBundles[id];

	    /* A PPP frame from the bypass hook? */
	    if (naddr.sg_data[0] == 'b') {
    		Link		l;
		u_int16_t	linkNum, proto;

		if (MBLEN(bp) <= 4) {
		    LogDumpBp(LG_FRAME|LG_ERR, bp,
    			"[%s] rec'd truncated %d bytes frame",
    			b->name, MBLEN(bp));
		    continue;
		}

		/* Extract link number and protocol */
		bp = mbread(bp, &linkNum, 2);
		linkNum = ntohs(linkNum);
	        bp = mbread(bp, &proto, 2);
		proto = ntohs(proto);

		/* Debugging */
		LogDumpBp(LG_FRAME, bp,
    		    "[%s] rec'd %d bytes bypass frame link=%d proto=0x%04x",
    		    b->name, MBLEN(bp), (int16_t)linkNum, proto);

		/* Set link */
		assert(linkNum == NG_PPP_BUNDLE_LINKNUM || linkNum < NG_PPP_MAX_LINKS);

		if (linkNum != NG_PPP_BUNDLE_LINKNUM)
		    l = b->links[linkNum];
		else
		    l = NULL;

		InputFrame(b, l, proto, bp);
		continue;
	    }

	    /* Debugging */
	    LogDumpBp(LG_FRAME, bp,
		"[%s] rec'd %d bytes frame on %s hook", b->name, MBLEN(bp), naddr.sg_data);

#ifndef USE_NG_TCPMSS
	    /* A snooped, outgoing TCP SYN frame */
	    if (naddr.sg_data[0] == 'o') {
		IfaceCorrectMSS(bp, MAXMSS(b->iface.mtu));
		naddr.sg_data[0] = 'i';
		NgFuncWriteFrame(gLinksDsock, naddr.sg_data, b->name, bp);
		continue;
	    }

	    /* A snooped, incoming TCP SYN frame */
	    if (naddr.sg_data[0] == 'i') {
		IfaceCorrectMSS(bp, MAXMSS(b->iface.mtu));
		naddr.sg_data[0] = 'o';
		NgFuncWriteFrame(gLinksDsock, naddr.sg_data, b->name, bp);
		continue;
	    }
#endif

	    /* A snooped, outgoing IP frame */
	    if (naddr.sg_data[0] == '4') {
		IfaceListenInput(b, PROTO_IP, bp);
		continue;
	    }

	    /* A snooped, outgoing IPv6 frame */
	    if (naddr.sg_data[0] == '6') {
		IfaceListenInput(b, PROTO_IPV6, bp);
		continue;
	    }

	    break;
	default:
    	    Log(LG_ERR, ("Link: Packet from unknown hook \"%s\"",
    	        name));
	    mbfree(bp);
	}
    }
}

/*
 * LinkFind()
 *
 * Find a link structure
 */

Link
LinkFind(const char *name)
{
    int		k;

    k = gNumLinks;
    if ((sscanf(name, "[%x]", &k) != 1) || (k < 0) || (k >= gNumLinks)) {
        /* Find link */
	for (k = 0;
	    k < gNumLinks && (gLinks[k] == NULL ||
		strcmp(gLinks[k]->name, name));
	    k++);
    };
    if (k == gNumLinks) {
	return (NULL);
    }

    return (gLinks[k]);
}

/*
 * LinkCommand()
 */

int
LinkCommand(Context ctx, int ac, char *av[], void *arg)
{
    Link	l;
    int		k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
        Printf("Defined links:\r\n");
        for (k = 0; k < gNumLinks; k++) {
	    if ((l = gLinks[k]) != NULL) {
		if (l && l->bund)
		    Printf("\t%-15s%s\r\n", 
			l->name, l->bund->name);
		else if (l->rep)
		    Printf("\t%-15s%s\r\n",
			 l->name, l->rep->name);
		else
		    Printf("\t%s\r\n", 
			l->name);
	    }
	}
	return (0);
    }

    if ((l = LinkFind(av[0])) == NULL) {
        RESETREF(ctx->lnk, NULL);
        RESETREF(ctx->bund, NULL);
        RESETREF(ctx->rep, NULL);
	Error("Link \"%s\" is not defined", av[0]);
    }

    /* Change default link and bundle */
    RESETREF(ctx->lnk, l);
    RESETREF(ctx->bund, l->bund);
    RESETREF(ctx->rep, NULL);

    return(0);
}

/*
 * SessionCommand()
 */

int
SessionCommand(Context ctx, int ac, char *av[], void *arg)
{
    int		k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
    	Printf("Present sessions:\r\n");
	for (k = 0; k < gNumLinks; k++) {
	    if (gLinks[k] && gLinks[k]->session_id[0])
    		Printf("\t%s\r\n", gLinks[k]->session_id);
	}
	return (0);
    }

    /* Find link */
    for (k = 0;
	k < gNumLinks && (gLinks[k] == NULL || 
	    strcmp(gLinks[k]->session_id, av[0]));
	k++);
    if (k == gNumLinks) {
	/* Change default link and bundle */
	RESETREF(ctx->lnk, NULL);
	RESETREF(ctx->bund, NULL);
	RESETREF(ctx->rep, NULL);
	Error("Session \"%s\" is not found", av[0]);
    }

    /* Change default link and bundle */
    RESETREF(ctx->lnk, gLinks[k]);
    RESETREF(ctx->bund, ctx->lnk->bund);
    RESETREF(ctx->rep, NULL);

    return(0);
}

/*
 * AuthnameCommand()
 */

int
AuthnameCommand(Context ctx, int ac, char *av[], void *arg)
{
    int		k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
    	Printf("Present users:\r\n");
	for (k = 0; k < gNumLinks; k++) {
	    if (gLinks[k] && gLinks[k]->lcp.auth.params.authname[0])
    		Printf("\t%s\r\n", gLinks[k]->lcp.auth.params.authname);
	}
	return (0);
    }

    /* Find link */
    for (k = 0;
	k < gNumLinks && (gLinks[k] == NULL || 
	    strcmp(gLinks[k]->lcp.auth.params.authname, av[0]));
	k++);
    if (k == gNumLinks) {
	/* Change default link and bundle */
	RESETREF(ctx->lnk, NULL);
	RESETREF(ctx->bund, NULL);
	RESETREF(ctx->rep, NULL);
	Error("User \"%s\" is not found", av[0]);
    }

    /* Change default link and bundle */
    RESETREF(ctx->lnk, gLinks[k]);
    RESETREF(ctx->bund, ctx->lnk->bund);
    RESETREF(ctx->rep, NULL);

    return(0);
}

/*
 * RecordLinkUpDownReason()
 *
 * This is called whenever a reason for the link going up or
 * down has just become known. Record this reason so that when
 * the link actually goes up or down, we can record it.
 *
 * If this gets called more than once in the "down" case,
 * the first call prevails.
 */
static void
RecordLinkUpDownReason2(Link l, int up, const char *key, const char *fmt, va_list args)
{
    char	**const cpp = up ? &l->upReason : &l->downReason;
    char	*buf;

    /* First reason overrides later ones */
    if (up) {
	if (l->upReasonValid) {
	    return;
	} else {
    	    l->upReasonValid = 1;
	}
    } else {
	if (l->downReasonValid) {
	    return;
	} else {
	    l->downReasonValid = 1;
	}
    }

    /* Allocate buffer if necessary */
    if (!*cpp)
	*cpp = Malloc(MB_LINK, RBUF_SIZE);
    buf = *cpp;

    /* Record reason */
    if (fmt) {
	snprintf(buf, RBUF_SIZE, "%s:", key);
	vsnprintf(buf + strlen(buf), RBUF_SIZE - strlen(buf), fmt, args);
    } else 
	strlcpy(buf, key, RBUF_SIZE);
}

void
RecordLinkUpDownReason(Bund b, Link l, int up, const char *key, const char *fmt, ...)
{
    va_list	args;
    int		k;

    if (l != NULL) {
	va_start(args, fmt);
	RecordLinkUpDownReason2(l, up, key, fmt, args);
	va_end(args);

    } else if (b != NULL) {
	for (k = 0; k < NG_PPP_MAX_LINKS; k++) {
    	    if (b->links[k]) {
		va_start(args, fmt);
		RecordLinkUpDownReason2(b->links[k], up, key, fmt, args);
		va_end(args);
    	    }
	}
    }
}

const char *
LinkMatchAction(Link l, int stage, char *login)
{
    struct linkaction *a;

    a = SLIST_FIRST(&l->actions);
    if (!a) {
	Log(LG_LINK, ("[%s] Link: No actions defined", l->name));
	return (NULL);
    }
    if (stage == 1) {
	if (SLIST_NEXT(a, next) == NULL && a->regex[0] == 0) {
	    if (a->action == LINK_ACTION_FORWARD) {
		    Log(LG_LINK, ("[%s] Link: Matched action 'forward \"%s\"'",
			l->name, a->arg));
		    return (a->arg);
	    }
	    if (a->action == LINK_ACTION_DROP) {
		    Log(LG_LINK, ("[%s] Link: Matched action 'drop'",
			l->name));
		    return ("##DROP##");
	    }
	}
	return (NULL);
    }
    SLIST_FOREACH(a, &l->actions, next) {
	if (!a->regex[0] || !regexec(&a->regexp, login, 0, NULL, 0))
	    break;
    }
    if (a) {
	if (a->action == LINK_ACTION_DROP) {
	    Log(LG_LINK, ("[%s] Link: Matched action 'drop'",
		l->name));
	    return ("##DROP##");
	}
	if ((stage == 2 && a->action == LINK_ACTION_FORWARD) ||
	    (stage == 3 && a->action == LINK_ACTION_BUNDLE)) {
	    Log(LG_LINK, ("[%s] Link: Matched action '%s \"%s\" \"%s\"'",
		l->name, (a->action == LINK_ACTION_FORWARD)?"forward":"bundle",
		a->arg, a->regex));
	    return (a->arg);
	}
    }
    return (NULL);
}

/*
 * LinkStat()
 */

int
LinkStat(Context ctx, int ac, char *av[], void *arg)
{
    Link 	l = ctx->lnk;
    struct linkaction *a;

    Printf("Link %s%s:\r\n", l->name, l->tmpl?" (template)":(l->stay?" (static)":""));

    Printf("Configuration:\r\n");
    Printf("\tDevice type    : %s\r\n", l->type?l->type->name:"");
    Printf("\tMRU            : %d bytes\r\n", l->conf.mru);
    Printf("\tMRRU           : %d bytes\r\n", l->conf.mrru);
    Printf("\tCtrl char map  : 0x%08x bytes\r\n", l->conf.accmap);
    Printf("\tRetry timeout  : %d seconds\r\n", l->conf.retry_timeout);
    Printf("\tMax redial     : ");
    if (l->conf.max_redial < 0)
	Printf("no redial\r\n");
    else if (l->conf.max_redial == 0) 
	Printf("unlimited, delay %ds\r\n", l->conf.redial_delay);
    else
	Printf("%d connect attempts, delay %ds\r\n",
	    l->conf.max_redial, l->conf.redial_delay);
    Printf("\tBandwidth      : %d bits/sec\r\n", l->bandwidth);
    Printf("\tLatency        : %d usec\r\n", l->latency);
    Printf("\tKeep-alive     : ");
    if (l->lcp.fsm.conf.echo_int == 0)
	Printf("disabled\r\n");
    else
	Printf("every %d secs, timeout %d\r\n",
    	    l->lcp.fsm.conf.echo_int, l->lcp.fsm.conf.echo_max);
    Printf("\tIdent string   : \"%s\"\r\n", l->conf.ident ? l->conf.ident : "");
    if (l->tmpl)
	Printf("\tMax children   : %d\r\n", l->conf.max_children);
    Printf("Link incoming actions:\r\n");
    SLIST_FOREACH(a, &l->actions, next) {
	Printf("\t%s\t%s\t%s\r\n", 
	    (a->action == LINK_ACTION_FORWARD)?"Forward":
	    (a->action == LINK_ACTION_BUNDLE)?"Bundle":"Drop",
	    a->arg, a->regex);
    }
    Printf("Link level options:\r\n");
    OptStat(ctx, &l->conf.options, gConfList);

    Printf("Link state:\r\n");
    if (l->tmpl)
	Printf("\tChildren       : %d\r\n", l->children);
    else {
	Printf("\tState          : %s\r\n", gPhysStateNames[l->state]);
	Printf("\tSession Id     : %s\r\n", l->session_id);
	Printf("\tPeer ident     : %s\r\n", l->lcp.peer_ident);
	if (l->state == PHYS_STATE_UP)
	    Printf("\tSession time   : %ld seconds\r\n", (long int)(time(NULL) - l->last_up));
    }
    if (!l->tmpl) {
	Printf("Up/Down stats:\r\n");
	if (l->downReason && (!l->downReasonValid))
	    Printf("\tDown Reason    : %s\r\n", l->downReason);
	if (l->upReason)
	    Printf("\tUp Reason      : %s\r\n", l->upReason);
	if (l->downReason && l->downReasonValid)
	    Printf("\tDown Reason    : %s\r\n", l->downReason);
  
	if (l->bund) {
	    LinkUpdateStats(l);
	    Printf("Traffic stats:\r\n");

	    Printf("\tInput octets   : %llu\r\n", (unsigned long long)l->stats.recvOctets);
	    Printf("\tInput frames   : %llu\r\n", (unsigned long long)l->stats.recvFrames);
	    Printf("\tOutput octets  : %llu\r\n", (unsigned long long)l->stats.xmitOctets);
	    Printf("\tOutput frames  : %llu\r\n", (unsigned long long)l->stats.xmitFrames);
	    Printf("\tBad protocols  : %llu\r\n", (unsigned long long)l->stats.badProtos);
	    Printf("\tRunts          : %llu\r\n", (unsigned long long)l->stats.runts);
	    Printf("\tDup fragments  : %llu\r\n", (unsigned long long)l->stats.dupFragments);
	    Printf("\tDrop fragments : %llu\r\n", (unsigned long long)l->stats.dropFragments);
	}
    }
    return(0);
}

/* 
 * LinkUpdateStats()
 */

void
LinkUpdateStats(Link l)
{
#ifndef NG_PPP_STATS64
    struct ng_ppp_link_stat	stats;

    if (NgFuncGetStats(l->bund, l->bundleIndex, &stats) != -1) {
	l->stats.xmitFrames += abs(stats.xmitFrames - l->oldStats.xmitFrames);
	l->stats.xmitOctets += abs(stats.xmitOctets - l->oldStats.xmitOctets);
	l->stats.recvFrames += abs(stats.recvFrames - l->oldStats.recvFrames);
	l->stats.recvOctets += abs(stats.recvOctets - l->oldStats.recvOctets);
        l->stats.badProtos  += abs(stats.badProtos - l->oldStats.badProtos);
        l->stats.runts	  += abs(stats.runts - l->oldStats.runts);
        l->stats.dupFragments += abs(stats.dupFragments - l->oldStats.dupFragments);
        l->stats.dropFragments += abs(stats.dropFragments - l->oldStats.dropFragments);
    }

    l->oldStats = stats;
#else
    NgFuncGetStats64(l->bund, l->bundleIndex, &l->stats);
#endif
}

/*
 * LinkResetStats()
 */

void
LinkResetStats(Link l)
{
    if (l->bund)
	NgFuncClrStats(l->bund, l->bundleIndex);
    memset(&l->stats, 0, sizeof(l->stats));
#ifndef NG_PPP_STATS64
    memset(&l->oldStats, 0, sizeof(l->oldStats));
#endif
}

/*
 * LinkSetCommand()
 */

static int
LinkSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    Link	l = ctx->lnk;
    int		val, nac = 0;
    const char	*name;
    char	*nav[ac];
    const char	*av2[] = { "chap-md5", "chap-msv1", "chap-msv2" };

    /* make "chap" as an alias for all chap-variants, this should keep BC */
    switch ((intptr_t)arg) {
	case SET_ACCEPT:
        case SET_DENY:
        case SET_ENABLE:
        case SET_DISABLE:
        case SET_YES:
        case SET_NO:
        {
	    int	i = 0;
            for ( ; i < ac; i++) {
    		if (strcasecmp(av[i], "chap") == 0) {
    		    LinkSetCommand(ctx, 3, (char **)av2, arg);
		} else {
		    nav[nac++] = av[i];
		} 
    	    }
    	    av = nav;
    	    ac = nac;
    	    break;
	}
    }

    switch ((intptr_t)arg) {
	case SET_BANDWIDTH:
	    if (ac != 1)
		return(-1);

    	    val = atoi(*av);
    	    if (val <= 0)
		Error("[%s] Bandwidth must be positive", l->name);
    	    else if (val > NG_PPP_MAX_BANDWIDTH * 10 * 8) {
		l->bandwidth = NG_PPP_MAX_BANDWIDTH * 10 * 8;
		Log(LG_ERR, ("[%s] Bandwidth truncated to %d bit/s", l->name, 
		    l->bandwidth));
    	    } else
		l->bandwidth = val;
    	    break;

	case SET_LATENCY:
	    if (ac != 1)
		return(-1);

    	    val = atoi(*av);
    	    if (val < 0)
		Error("[%s] Latency must be not negative", l->name);
    	    else if (val > NG_PPP_MAX_LATENCY * 1000) {
		Log(LG_ERR, ("[%s] Latency truncated to %d usec", l->name, 
		    NG_PPP_MAX_LATENCY * 1000));
		l->latency = NG_PPP_MAX_LATENCY * 1000;
    	    } else
    		l->latency = val;
    	    break;

	case SET_BUNDLE:
	case SET_FORWARD:
	case SET_DROP:
	    {
		struct linkaction	*n, *a;
	    
		if ((ac < 1 && (intptr_t)arg != SET_DROP) || ac > 2)
		    return(-1);

		n = Malloc(MB_LINK, sizeof(struct linkaction));
		if ((intptr_t)arg != SET_DROP) {
		    n->action = ((intptr_t)arg == SET_BUNDLE)?
		        LINK_ACTION_BUNDLE:LINK_ACTION_FORWARD;
		    strlcpy(n->arg, av[0], sizeof(n->arg));
		    if (ac == 2 && av[1][0]) {
		        strlcpy(n->regex, av[1], sizeof(n->regex));
		        if (regcomp(&n->regexp, n->regex, REG_EXTENDED)) {
		    	    Freee(n);
			    Error("regexp \"%s\" compilation error", av[0]);
			}
		    }
		} else {
		    n->action = LINK_ACTION_DROP;
		    if (ac == 1 && av[0][0]) {
		        strlcpy(n->regex, av[0], sizeof(n->regex));
		        if (regcomp(&n->regexp, n->regex, REG_EXTENDED)) {
		    	    Freee(n);
		    	    Error("regexp \"%s\" compilation error", av[0]);
			}
		    }
		}
	    
		a = SLIST_FIRST(&ctx->lnk->actions);
		if (a) {
		    while (SLIST_NEXT(a, next))
			a = SLIST_NEXT(a, next);
		    SLIST_INSERT_AFTER(a, n, next);
		} else {
		    SLIST_INSERT_HEAD(&ctx->lnk->actions, n, next);
		}
	    }
    	    break;

	case SET_CLEAR:
	    {
		struct linkaction	*a;
	    
		if (ac != 0)
		    return(-1);

	        while ((a = SLIST_FIRST(&l->actions)) != NULL) {
	    	    SLIST_REMOVE_HEAD(&l->actions, next);
    		    if (a->regex[0])
			regfree(&a->regexp);
		    Freee(a);
		}
	    }
    	    break;

	case SET_MRU:
	case SET_MTU:
	    if (ac != 1)
		return(-1);

    	    val = atoi(*av);
    	    name = ((intptr_t)arg == SET_MTU) ? "MTU" : "MRU";
    	    if (val < LCP_MIN_MRU)
		Error("min %s is %d", name, LCP_MIN_MRU);
    	    else if (l->type && (val > l->type->mru)) {
		Error("max %s on type \"%s\" links is %d",
		    name, l->type->name, l->type->mru);
    	    } else if ((intptr_t)arg == SET_MTU)
		l->conf.mtu = val;
    	    else
		l->conf.mru = val;
    	    break;

	case SET_MRRU:
	    if (ac != 1)
		return(-1);

    	    val = atoi(*av);
    	    if (val < MP_MIN_MRRU)
		Error("min MRRU is %d", MP_MIN_MRRU);
    	    else if (val > MP_MAX_MRRU)
		Error("max MRRU is %d", MP_MAX_MRRU);
    	    else
		l->conf.mrru = val;
    	    break;

	case SET_FSM_RETRY:
	    if (ac != 1)
		return(-1);

    	    val = atoi(*av);
    	    if (val < 1 || val > 10) {
		Error("incorrect fsm-timeout value %d", val);
	    } else {
		l->conf.retry_timeout = val;
	    }
    	    break;

	case SET_MAX_RETRY:
	    if (ac != 1)
		return(-1);

    	    l->conf.max_redial = atoi(*av);
    	    break;

	case SET_RETRY_DELAY:
	    if (ac != 1)
		return(-1);

	    l->conf.redial_delay = atoi(*av);
	    if (l->conf.redial_delay < 1)
		l->conf.redial_delay = 1;
	    break;

	case SET_MAX_CHILDREN:
	    if (ac != 1)
		return(-1);

	    if (!l->tmpl)
		Error("applicable only to templates");
	    val = atoi(*av);
	    if (val < 0 || val > 100000)
		Error("incorrect value %d", val);
    	    l->conf.max_children = val;
    	    break;

	case SET_ACCMAP:
	    if (ac != 1)
		return(-1);

    	    sscanf(*av, "%x", &val);
    	    l->conf.accmap = val;
    	    break;

	case SET_KEEPALIVE:
    	    if (ac != 2)
		return(-1);
    	    l->lcp.fsm.conf.echo_int = atoi(av[0]);
    	    l->lcp.fsm.conf.echo_max = atoi(av[1]);
    	    break;

	case SET_IDENT:
    	    if (ac != 1)
		return(-1);
    	    if (l->conf.ident != NULL) {
		Freee(l->conf.ident);
		l->conf.ident = NULL;
    	    }
    	    if (*av[0] != '\0')
	    strcpy(l->conf.ident = Malloc(MB_LINK, strlen(av[0]) + 1), av[0]);
    	    break;

	case SET_ACCEPT:
    	    AcceptCommand(ac, av, &l->conf.options, gConfList);
	    if (ctx->lnk->type->update)
		(ctx->lnk->type->update)(ctx->lnk);
    	    break;

	case SET_DENY:
    	    DenyCommand(ac, av, &l->conf.options, gConfList);
	    if (ctx->lnk->type->update)
		(ctx->lnk->type->update)(ctx->lnk);
    	    break;

	case SET_ENABLE:
    	    EnableCommand(ac, av, &l->conf.options, gConfList);
	    if (ctx->lnk->type->update)
		(ctx->lnk->type->update)(ctx->lnk);
    	    break;

	case SET_DISABLE:
    	    DisableCommand(ac, av, &l->conf.options, gConfList);
	    if (ctx->lnk->type->update)
		(ctx->lnk->type->update)(ctx->lnk);
    	    break;

	case SET_YES:
	    YesCommand(ac, av, &l->conf.options, gConfList);
	    if (ctx->lnk->type->update)
		(ctx->lnk->type->update)(ctx->lnk);
    	    break;

	case SET_NO:
    	    NoCommand(ac, av, &l->conf.options, gConfList);
	    if (ctx->lnk->type->update)
		(ctx->lnk->type->update)(ctx->lnk);
    	    break;

	default:
    	    assert(0);
    }

    return(0);
}

