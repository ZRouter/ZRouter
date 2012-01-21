
/*
 * phys.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "msg.h"
#include "link.h"
#include "devices.h"
#include "util.h"

#include <netgraph/ng_tee.h>

/*
 * The physical layer has four states: DOWN, OPENING, CLOSING, and UP.
 * Each device type must implement this set of standard methods:
 *
 *  init	Called once for each device to initialize it.
 *  open	Called in the DOWN state to initiate connection.
 *		Device should eventually call PhysUp() or PhysDown().
 *  close	Called in the OPENING or UP states.
 *		Device should eventually call PhysDown().
 *  update	Called when LCP reaches the UP state. Device should
 *		update its configuration based on LCP negotiated
 *		settings, if necessary.
 *  showstat	Display device statistics.
 *
 * The device should generate UP and DOWN events in response to OPEN
 * and CLOSE events. If the device goes down suddenly after being OPEN,
 * the close method will not be explicitly called to clean up.
 *
 * All device types must support MRU's of at least 1500.
 *
 * Each device is responsible for connecting the appropriate netgraph
 * node to the PPP node when the link comes up, and disconnecting it
 * when the link goes down (or is closed). The device should NOT send
 * any NGM_PPP_SET_CONFIG messsages to the ppp node.
 */

/*
 * INTERNAL FUNCTIONS
 */

  static void	PhysMsg(int type, void *arg);

/*
 * GLOBAL VARIABLES
 */

  const PhysType gPhysTypes[] = {
#define _WANT_DEVICE_TYPES
#include "devices.h"
    NULL,
  };

  const char *gPhysStateNames[] = {
    "DOWN",
    "CONNECTING",
    "READY",
    "UP",
  };

int
PhysInit(Link l)
{
    MsgRegister(&l->pmsgs, PhysMsg);

    /* Initialize type specific stuff */
    if ((l->type->init)(l) < 0) {
	Log(LG_ERR, ("[%s] type \"%s\" initialization failed",
    	    l->name, l->type->name));
	l->type = NULL;
	return (0);
    }

    return (0);
}

/*
 * PhysInst()
 */

int
PhysInst(Link l, Link lt)
{
    return ((l->type->inst)(l, lt));
}

/*
 * PhysOpenCmd()
 */

int
PhysOpenCmd(Context ctx)
{
    if (ctx->lnk->tmpl)
	Error("impossible to open template");
    RecordLinkUpDownReason(NULL, ctx->lnk, 1, STR_MANUALLY, NULL);
    PhysOpen(ctx->lnk);
    return (0);
}

/*
 * PhysOpen()
 */

void
PhysOpen(Link l)
{
    REF(l);
    MsgSend(&l->pmsgs, MSG_OPEN, l);
}

/*
 * PhysCloseCmd()
 */

int
PhysCloseCmd(Context ctx)
{
    if (ctx->lnk->tmpl)
	Error("impossible to close template");
    RecordLinkUpDownReason(NULL, ctx->lnk, 0, STR_MANUALLY, NULL);
    PhysClose(ctx->lnk);
    return (0);
}

/*
 * PhysClose()
 */

void
PhysClose(Link l)
{
    REF(l);
    MsgSend(&l->pmsgs, MSG_CLOSE, l);
}

/*
 * PhysUp()
 */

void
PhysUp(Link l)
{
    Log(LG_PHYS2, ("[%s] device: UP event", l->name));
    l->last_up = time(NULL);
    if (!l->rep) {
	LinkUp(l);
    } else {
	RepUp(l);
    }
}

/*
 * PhysDown()
 */

void
PhysDown(Link l, const char *reason, const char *details)
{
    Log(LG_PHYS2, ("[%s] device: DOWN event", l->name));
    if (!l->rep) {
	RecordLinkUpDownReason(NULL, l, 0, reason, details);
	l->upReasonValid=0;
	LinkDown(l);
	LinkShutdownCheck(l, l->lcp.fsm.state);

    } else {
	RepDown(l);
    }
}

/*
 * PhysIncoming()
 */

void
PhysIncoming(Link l)
{
    const char	*rept;
    
    rept = LinkMatchAction(l, 1, NULL);
    if (rept) {
	if (strcmp(rept,"##DROP##") == 0) {
	    /* Action told we must drop this connection */
	    Log(LG_PHYS, ("[%s] Drop connection", l->name));
	    PhysClose(l);
	    return;
	}
	if (RepCreate(l, rept)) {
	    Log(LG_ERR, ("[%s] Repeater to \"%s\" creation error", l->name, rept));
	    PhysClose(l);
	    return;
	}
    }

    if (!l->rep) {
	RecordLinkUpDownReason(NULL, l, 1, STR_INCOMING_CALL, NULL);
	LinkOpen(l);
    } else {
        RepIncoming(l);
    }
}

/*
 * PhysSetAccm()
 */

int
PhysSetAccm(Link l, uint32_t xmit, u_int32_t recv)
{
    if (l->type && l->type->setaccm)
	return (*l->type->setaccm)(l, xmit, recv);
    else 
	return (0);
}

/*
 * PhysGetUpperHook()
 */

int
PhysGetUpperHook(Link l, char *path, char *hook)
{
    if (!l->rep) {
	snprintf(path, NG_PATHSIZ, "[%lx]:", (u_long)l->nodeID);
	strcpy(hook, NG_TEE_HOOK_LEFT);
	return 1;
    } else {
	return RepGetHook(l, path, hook);
    }
    return 0;
}

/*
 * PhysGetOriginate()
 *
 * This returns one of LINK_ORIGINATE_{UNKNOWN, LOCAL, REMOTE}
 */

int
PhysGetOriginate(Link l)
{
  PhysType	const pt = l->type;

  return((pt && pt->originate) ? (*pt->originate)(l) : LINK_ORIGINATE_UNKNOWN);
}

/*
 * PhysIsSync()
 *
 * This returns 1 if link is synchronous
 */

int
PhysIsSync(Link l)
{
  PhysType	const pt = l->type;

  return((pt && pt->issync) ? (*pt->issync)(l) : 0);
}

/*
 * PhysSetCalledNum()
 */

int
PhysSetCallingNum(Link l, char *buf)
{
    PhysType	const pt = l->type;

    if (pt && pt->setcallingnum)
	return ((*pt->setcallingnum)(l, buf));
    else
	return (0);
}

/*
 * PhysSetCalledNum()
 */

int
PhysSetCalledNum(Link l, char *buf)
{
    PhysType	const pt = l->type;

    if (pt && pt->setcallednum)
	return ((*pt->setcallednum)(l, buf));
    else
	return (0);
}

/*
 * PhysGetSelfAddr()
 */

int
PhysGetSelfName(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->selfname)
	return ((*pt->selfname)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetPeerAddr()
 */

int
PhysGetPeerName(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->peername)
	return ((*pt->peername)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetSelfAddr()
 */

int
PhysGetSelfAddr(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->selfaddr)
	return ((*pt->selfaddr)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetPeerAddr()
 */

int
PhysGetPeerAddr(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->peeraddr)
	return ((*pt->peeraddr)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetPeerPort()
 */

int
PhysGetPeerPort(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->peerport)
	return ((*pt->peerport)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetPeerMacAddr()
 */

int
PhysGetPeerMacAddr(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->peermacaddr)
	return ((*pt->peermacaddr)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetPeerIface()
 */

int
PhysGetPeerIface(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->peeriface)
	return ((*pt->peeriface)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysGetCallingNum()
 */

int
PhysGetCallingNum(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt) {
	/* For untrusted peers use peeraddr as calling */
	if (Enabled(&l->conf.options, LINK_CONF_PEER_AS_CALLING) && pt->peeraddr)
	    (*pt->peeraddr)(l, buf, buf_len);
	else if (pt->callingnum)
	    (*pt->callingnum)(l, buf, buf_len);
	if (Enabled(&l->conf.options, LINK_CONF_REPORT_MAC)) {
	    char tmp[64];
	    strlcat(buf, " / ", buf_len);
	    if (pt->peermacaddr) {
		(*pt->peermacaddr)(l, tmp, sizeof(tmp));
		strlcat(buf, tmp, buf_len);
	    }
	    strlcat(buf, " / ", buf_len);
	    if (pt->peeriface) {
		(*pt->peeriface)(l, tmp, sizeof(tmp));
		strlcat(buf, tmp, buf_len);
	    }
	}
    }
    return (0);
}

/*
 * PhysGetCalledNum()
 */

int
PhysGetCalledNum(Link l, char *buf, size_t buf_len)
{
    PhysType	const pt = l->type;

    buf[0] = 0;

    if (pt && pt->callednum)
	return ((*pt->callednum)(l, buf, buf_len));
    else
	return (0);
}

/*
 * PhysIsBusy()
 *
 * This returns 1 if link is busy
 */

int
PhysIsBusy(Link l)
{
    return (l->die || l->rep || l->state != PHYS_STATE_DOWN ||
	l->lcp.fsm.state != ST_INITIAL || l->lcp.auth.acct_thread != NULL ||
	(l->tmpl && (l->children >= l->conf.max_children || gChildren >= gMaxChildren)));
}

/*
 * PhysShutdown()
 */

void
PhysShutdown(Link l)
{
    PhysType	const pt = l->type;

    MsgUnRegister(&l->pmsgs);

    if (pt && pt->shutdown)
	(*pt->shutdown)(l);
}

/*
 * PhysSetDeviceType()
 */

void
PhysSetDeviceType(Link l, char *typename)
{
  PhysType	pt;
  int		k;

  /* Make sure device type not already set */
  if (l->type) {
    Log(LG_ERR, ("[%s] device type already set to %s",
      l->name, l->type->name));
    return;
  }

  /* Locate type */
  for (k = 0; (pt = gPhysTypes[k]); k++) {
    if (!strcmp(pt->name, typename))
      break;
  }
  if (pt == NULL) {
    Log(LG_ERR, ("[%s] device type \"%s\" unknown", l->name, typename));
    return;
  }
  l->type = pt;

  /* Initialize type specific stuff */
  if ((l->type->init)(l) < 0) {
    Log(LG_ERR, ("[%s] type \"%s\" initialization failed",
      l->name, l->type->name));
    l->type = NULL;
    return;
  }
}

/*
 * PhysMsg()
 */

static void
PhysMsg(int type, void *arg)
{
    Link	const l = (Link)arg;

    if (l->dead) {
	UNREF(l);
	return;
    }
    Log(LG_PHYS2, ("[%s] device: %s event",
	l->name, MsgName(type)));
    switch (type) {
    case MSG_OPEN:
    	l->downReasonValid=0;
        if (l->rep && l->state == PHYS_STATE_UP) {
	    PhysUp(l);
	    break;
	}
        (*l->type->open)(l);
        break;
    case MSG_CLOSE:
        (*l->type->close)(l);
        break;
    default:
        assert(FALSE);
    }
    UNREF(l);
}

/*
 * PhysStat()
 */

int
PhysStat(Context ctx, int ac, char *av[], void *arg)
{
    Link	const l = ctx->lnk;

    Printf("Device '%s' (%s)\r\n", l->name, (l->tmpl)?"template":"instance");
    Printf("\tType         : %s\r\n", l->type->name);
    if (!l->tmpl) {
	Printf("\tState        : %s\r\n", gPhysStateNames[l->state]);
	if (l->state == PHYS_STATE_UP)
    	    Printf("\tSession time : %ld seconds\r\n", (long int)(time(NULL) - l->last_up));
    }

    if (l->type->showstat)
	(*l->type->showstat)(ctx);
    return 0;
}

