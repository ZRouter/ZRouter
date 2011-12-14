
/*
 * ngfunc.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 *
 * TCP MSSFIX contributed by Sergey Korolew <dsATbittu.org.ru>
 *
 * Routines for doing netgraph stuff
 *
 */

#include "defs.h"
#include "ppp.h"
#include "bund.h"
#include "ngfunc.h"
#include "input.h"
#include "ccp.h"
#include "netgraph.h"
#include "command.h"
#include "util.h"

#ifdef USE_NG_BPF
#include <net/bpf.h>
#endif
#include <arpa/inet.h>

#include <netgraph/ng_message.h>

#include <netgraph/ng_socket.h>
#include <netgraph/ng_ksocket.h>
#include <netgraph/ng_iface.h>
#ifdef USE_NG_VJC
#include <netgraph/ng_vjc.h>
#endif
#ifdef USE_NG_BPF
#include <netgraph/ng_bpf.h>
#endif
#include <netgraph/ng_tee.h>
#ifdef USE_NG_TCPMSS
#include <netgraph/ng_tcpmss.h>
#endif
#ifdef USE_NG_NETFLOW
#include <netgraph/netflow/ng_netflow.h>
#endif
#ifdef USE_NG_PRED1
#include <netgraph/ng_pred1.h>
#endif

#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>

/*
 * DEFINITIONS
 */

  #define TEMPHOOK		"temphook"
  #define MAX_IFACE_CREATE	128

  /* Set menu options */
  enum {
    SET_PEER,
    SET_SELF,
    SET_TIMEOUTS,
    SET_NODE,
    SET_HOOK
  };

/*
 * INTERNAL FUNCTIONS
 */

#ifdef USE_NG_NETFLOW
  static int	NetflowSetCommand(Context ctx, int ac, char *av[], void *arg);
#endif

/*
 * GLOBAL VARIABLES
 */

#ifdef USE_NG_NETFLOW
  const struct cmdtab NetflowSetCmds[] = {
    { "peer {ip} {port}",	"Set export destination" ,
        NetflowSetCommand, NULL, 2, (void *) SET_PEER },
    { "self {ip} [{port}]",	"Set export source" ,
        NetflowSetCommand, NULL, 2, (void *) SET_SELF },
    { "timeouts {inactive} {active}", "Set NetFlow timeouts" ,
        NetflowSetCommand, NULL, 2, (void *) SET_TIMEOUTS },
    { "node {name}", "Set node name to use" ,
        NetflowSetCommand, NULL, 2, (void *) SET_NODE },
    { "hook {number}", "Set initial hook number" ,
        NetflowSetCommand, NULL, 2, (void *) SET_HOOK },
    { NULL },
  };
#endif

/*
 * INTERNAL VARIABLES
 */

#ifdef USE_NG_TCPMSS
  u_char gTcpMSSNode = FALSE;
#endif
#ifdef USE_NG_NETFLOW
  u_char gNetflowNode = FALSE;
  u_char gNetflowNodeShutdown = TRUE;
  char gNetflowNodeName[64] = "mpd-nf";
  ng_ID_t gNetflowNodeID = 0;
  u_int gNetflowIface = 0;
  struct sockaddr_storage gNetflowExport;
  struct sockaddr_storage gNetflowSource;
  uint32_t gNetflowInactive = 0;
  uint32_t gNetflowActive = 0;
#endif
  
  static int	gNgStatSock=0;


#ifdef USE_NG_NETFLOW
int
NgFuncInitGlobalNetflow(void)
{
    char		path[NG_PATHSIZ];
    struct ngm_mkpeer	mp;
    struct ngm_rmhook	rm;
    struct ngm_name	nm;
    int			csock;

    /* Create a netgraph socket node */
    if (NgMkSockNode(NULL, &csock, NULL) < 0) {
	Log(LG_ERR, ("NETFLOW: Can't create %s node: %s",
    	    NG_SOCKET_NODE_TYPE, strerror(errno)));
        return (-1);
    }

    /* If node exist just get it's ID. */
    if (gNetflowNode) {
	snprintf(path, sizeof(path), "%s:", gNetflowNodeName);
	gNetflowNodeID = NgGetNodeID(csock, path);
	close(csock);
	return (0);
    }

    snprintf(gNetflowNodeName, sizeof(gNetflowNodeName), "mpd%d-nf", gPid);

    /* Create a global netflow node. */
    strcpy(mp.type, NG_NETFLOW_NODE_TYPE);
    strcpy(mp.ourhook, TEMPHOOK);
    strcpy(mp.peerhook, NG_NETFLOW_HOOK_DATA "0");
    if (NgSendMsg(csock, ".:",
      NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Log(LG_ERR, ("NETFLOW: Can't create %s node at \"%s\"->\"%s\": %s", 
	    mp.type, ".:", mp.ourhook, strerror(errno)));
	goto fail;
    }
    
    /* Get new node ID. */
    gNetflowNodeID = NgGetNodeID(csock, TEMPHOOK);

    /* Set the new node's name. */
    strcpy(nm.name, gNetflowNodeName);
    if (NgSendMsg(csock, TEMPHOOK,
      NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
	Log(LG_ERR, ("NETFLOW: Can't name %s node: %s", NG_NETFLOW_NODE_TYPE,
            strerror(errno)));
	goto fail;
    }

    /* Connect ng_ksocket(4) node for export. */
    strcpy(mp.type, NG_KSOCKET_NODE_TYPE);
    strcpy(mp.ourhook, NG_NETFLOW_HOOK_EXPORT);
    if (gNetflowExport.ss_family==AF_INET6) {
	snprintf(mp.peerhook, sizeof(mp.peerhook), "%d/%d/%d", PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    } else {
        snprintf(mp.peerhook, sizeof(mp.peerhook), "inet/dgram/udp");
    }
    snprintf(path, sizeof(path), "[%x]:", gNetflowNodeID);
    if (NgSendMsg(csock, path,
      NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Log(LG_ERR, ("NETFLOW: Can't create %s node at \"%s\"->\"%s\": %s", 
	    mp.type, path, mp.ourhook, strerror(errno)));
	goto fail;
    }

    /* Configure timeouts for ng_netflow(4). */
    if (gNetflowInactive != 0 && gNetflowActive != 0) {
	struct ng_netflow_settimeouts nf_settime;

	nf_settime.inactive_timeout = gNetflowInactive;
	nf_settime.active_timeout = gNetflowActive;

	if (NgSendMsg(csock, path, NGM_NETFLOW_COOKIE,
	  NGM_NETFLOW_SETTIMEOUTS, &nf_settime, sizeof(nf_settime)) < 0) {
	    Log(LG_ERR, ("NETFLOW: Can't set timeouts on netflow %s node: %s",
		NG_NETFLOW_NODE_TYPE, strerror(errno)));
	    goto fail2;
	}
    }

    /* Configure export destination and source on ng_ksocket(4). */
    strlcat(path, NG_NETFLOW_HOOK_EXPORT, sizeof(path));
    if (gNetflowSource.ss_len != 0) {
	if (NgSendMsg(csock, path, NGM_KSOCKET_COOKIE,
	  NGM_KSOCKET_BIND, &gNetflowSource, sizeof(gNetflowSource)) < 0) {
	    Log(LG_ERR, ("NETFLOW: Can't bind export %s node: %s",
		NG_KSOCKET_NODE_TYPE, strerror(errno)));
	    goto fail2;
	}
    }
    if (gNetflowExport.ss_len != 0) {
	if (NgSendMsg(csock, path, NGM_KSOCKET_COOKIE,
	  NGM_KSOCKET_CONNECT, &gNetflowExport, sizeof(gNetflowExport)) < 0) {
	    Log(LG_ERR, ("NETFLOW: Can't connect export %s node: %s",
		NG_KSOCKET_NODE_TYPE, strerror(errno)));
	    goto fail2;
	}
    }

    /* Set the new node's name. */
    snprintf(nm.name, sizeof(nm.name), "mpd%d-nfso", gPid);
    if (NgSendMsg(csock, path,
      NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
	Log(LG_ERR, ("NETFLOW: Can't name %s node: %s", NG_KSOCKET_NODE_TYPE,
            strerror(errno)));
	goto fail2;
    }

    /* Disconnect temporary hook. */
    strcpy(rm.ourhook, TEMPHOOK);
    if (NgSendMsg(csock, ".:",
      NGM_GENERIC_COOKIE, NGM_RMHOOK, &rm, sizeof(rm)) < 0) {
	Log(LG_ERR, ("can't remove hook %s: %s", TEMPHOOK, strerror(errno)));
	goto fail2;
    }
    gNetflowNode = TRUE;
    close(csock);

    return (0);
fail2:
    snprintf(path, sizeof(path), "[%x]:", gNetflowNodeID);
    NgFuncShutdownNode(csock, "netflow", path);
fail:
    gNetflowNodeID = 0;
    close(csock);
    return (-1);
}
#endif

/*
 * NgFuncCreateIface()
 *
 * Create a new netgraph interface.
 */

int
NgFuncCreateIface(Bund b, char *buf, int max)
{
    union {
        u_char		buf[sizeof(struct ng_mesg) + sizeof(struct nodeinfo)];
        struct ng_mesg	reply;
    }			u;
    struct nodeinfo	*const ni = (struct nodeinfo *)(void *)u.reply.data;
    struct ngm_rmhook	rm;
    struct ngm_mkpeer	mp;
    int			rtn = 0;

    /* Create iface node (as a temporary peer of the socket node) */
    strcpy(mp.type, NG_IFACE_NODE_TYPE);
    strcpy(mp.ourhook, TEMPHOOK);
    strcpy(mp.peerhook, NG_IFACE_HOOK_INET);
    if (NgSendMsg(gLinksCsock, ".:",
      NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Log(LG_ERR, ("[%s] can't create %s node at \"%s\"->\"%s\": %s %d",
    	    b->name, NG_IFACE_NODE_TYPE, ".:", mp.ourhook, strerror(errno), gLinksCsock));
	return(-1);
    }

    /* Get the new node's name */
    if (NgSendMsg(gLinksCsock, TEMPHOOK,
      NGM_GENERIC_COOKIE, NGM_NODEINFO, NULL, 0) < 0) {
	Log(LG_ERR, ("[%s] %s: %s", b->name, "NGM_NODEINFO", strerror(errno)));
	rtn = -1;
	goto done;
    }
    if (NgRecvMsg(gLinksCsock, &u.reply, sizeof(u), NULL) < 0) {
	Log(LG_ERR, ("[%s] reply from %s: %s",
    	    b->name, NG_IFACE_NODE_TYPE, strerror(errno)));
	rtn = -1;
	goto done;
    }
    strlcpy(buf, ni->name, max);

done:
    /* Disconnect temporary hook */
    strcpy(rm.ourhook, TEMPHOOK);
    if (NgSendMsg(gLinksCsock, ".:",
      NGM_GENERIC_COOKIE, NGM_RMHOOK, &rm, sizeof(rm)) < 0) {
	Log(LG_ERR, ("[%s] can't remove hook %s: %s",
    	    b->name, TEMPHOOK, strerror(errno)));
	rtn = -1;
    }

    /* Done */
    return(rtn);
}

/*
 * NgFuncShutdownGlobal()
 *
 * Shutdown nodes, that are shared between bundles.
 */

void
NgFuncShutdownGlobal(void)
{
#ifdef USE_NG_NETFLOW
    char	path[NG_PATHSIZ];
    int		csock;

    if (gNetflowNode == FALSE || gNetflowNodeShutdown==FALSE)
	return;

    /* Create a netgraph socket node */
    if (NgMkSockNode(NULL, &csock, NULL) < 0) {
	Log(LG_ERR, ("NgFuncShutdownGlobal: can't create %s node: %s",
    	    NG_SOCKET_NODE_TYPE, strerror(errno)));
        return;
    }

    snprintf(path, sizeof(path), "[%x]:", gNetflowNodeID);
    NgFuncShutdownNode(csock, "netflow", path);
    
    close(csock);
#endif
}

/*
 * NgFuncShutdownNode()
 */

int
NgFuncShutdownNode(int csock, const char *label, const char *path)
{
    int rtn, retry = 10, delay = 1000;

retry:
    if ((rtn = NgSendMsg(csock, path,
      NGM_GENERIC_COOKIE, NGM_SHUTDOWN, NULL, 0)) < 0) {
	if (errno == ENOBUFS && retry > 0) {
    	    Log(LG_ERR, ("[%s] shutdown \"%s\": %s, retrying...",
	      label, path, strerror(errno)));
	    usleep(delay);
	    retry--;
	    delay *= 2;
	    goto retry;
	}
	if (errno != ENOENT) {
    	    Log(LG_ERR, ("[%s] can't shutdown \"%s\": %s",
	      label, path, strerror(errno)));
	}
    }
    return(rtn);
}

/*
 * NgFuncSetConfig()
 */

void
NgFuncSetConfig(Bund b)
{
    char	path[NG_PATHSIZ];
    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    if (NgSendMsg(gLinksCsock, path, NGM_PPP_COOKIE,
    	    NGM_PPP_SET_CONFIG, &b->pppConfig, sizeof(b->pppConfig)) < 0) {
	Log(LG_ERR, ("[%s] can't config %s: %s",
    	    b->name, path, strerror(errno)));
	DoExit(EX_ERRDEAD);
    }
}

/*
 * NgFuncSendQuery()
 */

int
NgFuncSendQuery(const char *path, int cookie, int cmd, const void *args,
	size_t arglen, struct ng_mesg *rbuf, size_t replen, char *raddr)
{

    if (!gNgStatSock) {
	char		name[NG_NODESIZ];
	
	/* Create a netgraph socket node */
	snprintf(name, sizeof(name), "mpd%d-stats", gPid);
	if (NgMkSockNode(name, &gNgStatSock, NULL) < 0) {
    	    Log(LG_ERR, ("NgFuncSendQuery: can't create %s node: %s",
    		NG_SOCKET_NODE_TYPE, strerror(errno)));
    	    return(-1);
	}
	(void) fcntl(gNgStatSock, F_SETFD, 1);
    }

    /* Send message */
    if (NgSendMsg(gNgStatSock, path, cookie, cmd, args, arglen) < 0)
	return (-1);

    /* Read message */
    if (NgRecvMsg(gNgStatSock, rbuf, replen, raddr) < 0) {
	Log(LG_ERR, ("NgFuncSendQuery: can't read unexpected message: %s",
    	    strerror(errno)));
	return (-1);
    }

    return (0);
}

/*
 * NgFuncConnect()
 */

int
NgFuncConnect(int csock, char *label, const char *path, const char *hook,
	const char *path2, const char *hook2)
{
    struct ngm_connect	cn;

    strlcpy(cn.path, path2, sizeof(cn.path));
    strlcpy(cn.ourhook, hook, sizeof(cn.ourhook));
    strlcpy(cn.peerhook, hook2, sizeof(cn.peerhook));
    if (NgSendMsg(csock, path,
      NGM_GENERIC_COOKIE, NGM_CONNECT, &cn, sizeof(cn)) < 0) {
        Log(LG_ERR, ("[%s] can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    	    label, path, hook, path2, hook2, strerror(errno)));
	return(-1);
    }
    return(0);
}

/*
 * NgFuncDisconnect()
 */

int
NgFuncDisconnect(int csock, char *label, const char *path, const char *hook)
{
    struct ngm_rmhook	rm;
    int		retry = 10, delay = 1000;

    /* Disconnect hook */
    strlcpy(rm.ourhook, hook, sizeof(rm.ourhook));
retry:
    if (NgSendMsg(csock, path,
      NGM_GENERIC_COOKIE, NGM_RMHOOK, &rm, sizeof(rm)) < 0) {
	if (errno == ENOBUFS && retry > 0) {
    	    Log(LG_ERR, ("[%s] remove hook %s from node \"%s\": %s, retrying...",
	      label, hook, path, strerror(errno)));
	    usleep(delay);
	    retry--;
	    delay *= 2;
	    goto retry;
	}
	Log(LG_ERR, ("[%s] can't remove hook %s from node \"%s\": %s",
    	  label, hook, path, strerror(errno)));
	return(-1);
    }
    return(0);
}

/*
 * NgFuncWritePppFrame()
 *
 * Consumes the mbuf.
 */

int
NgFuncWritePppFrame(Bund b, int linkNum, int proto, Mbuf bp)
{
    u_int16_t	temp;

    /* Prepend ppp node bypass header */
    temp = htons(linkNum);
    bp = mbcopyback(bp, -4, &temp, 2);
    temp = htons(proto);
    bp = mbcopyback(bp, 2, &temp, 2);

    /* Debugging */
    LogDumpBp(LG_FRAME, bp,
	"[%s] xmit bypass frame link=%d proto=0x%04x",
	b->name, (int16_t)linkNum, proto);

    if ((linkNum == NG_PPP_BUNDLE_LINKNUM && b->n_up == 0) ||
	(linkNum != NG_PPP_BUNDLE_LINKNUM &&
	    (b->links[linkNum] == NULL ||
	    b->links[linkNum]->state != PHYS_STATE_UP))) {
	Log(LG_FRAME, ("[%s] Bundle: No links ready to send packet", b->name));
	mbfree(bp);
	return (-1);
    }

    /* Write frame */
    return NgFuncWriteFrame(gLinksDsock, b->hook, b->name, bp);
}

/*
 * NgFuncWritePppFrameLink()
 *
 * Consumes the mbuf.
 */

int
NgFuncWritePppFrameLink(Link l, int proto, Mbuf bp)
{
    u_int16_t	temp;

    if (l->joined_bund) {
	return (NgFuncWritePppFrame(l->bund, l->bundleIndex, proto, bp));
    }

    /* Prepend framing */
    temp = htons(0xff03);
    bp = mbcopyback(bp, -4, &temp, 2);
    temp = htons(proto);
    bp = mbcopyback(bp, 2, &temp, 2);

    /* Debugging */
    LogDumpBp(LG_FRAME, bp,
	"[%s] xmit frame to link proto=0x%04x",
	l->name, proto);

    if (l->state != PHYS_STATE_UP) {
	Log(LG_FRAME, ("[%s] Link: Not ready to send packet", l->name));
	mbfree(bp);
	return (-1);
    }

    /* Write frame */
    return NgFuncWriteFrame(gLinksDsock, l->hook, l->name, bp);
}

/*
 * NgFuncWriteFrame()
 *
 * Consumes the mbuf.
 */

int
NgFuncWriteFrame(int dsock, const char *hookname, const char *label, Mbuf bp)
{
    union {
        u_char          buf[sizeof(struct sockaddr_ng) + NG_HOOKSIZ];
	struct sockaddr_ng sa_ng;
    }                   u;
    struct sockaddr_ng	*ng = &u.sa_ng;
    int			rtn;

    /* Write frame */
    if (bp == NULL)  
	return (-1);

    /* Set dest address */
    memset(&u.buf, 0, sizeof(u.buf));
    strlcpy(ng->sg_data, hookname, NG_HOOKSIZ);
    ng->sg_family = AF_NETGRAPH;
    ng->sg_len = 3 + strlen(ng->sg_data);

    rtn = sendto(dsock, MBDATAU(bp), MBLEN(bp),
	0, (struct sockaddr *)ng, ng->sg_len);

    /* ENOBUFS can be expected on some links, e.g., ng_pptpgre(4) */
    if (rtn < 0 && errno != ENOBUFS) {
	Log(LG_ERR, ("[%s] error writing len %d frame to %s: %s",
    	    label, MBLEN(bp), hookname, strerror(errno)));
    }
    mbfree(bp);
    return (rtn);
}

/*
 * NgFuncClrStats()
 *
 * Clear link or whole bundle statistics
 */

int
NgFuncClrStats(Bund b, u_int16_t linkNum)
{
    char	path[NG_PATHSIZ];

    /* Get stats */
    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    if (NgSendMsg(gLinksCsock, path, 
	NGM_PPP_COOKIE, NGM_PPP_CLR_LINK_STATS, &linkNum, sizeof(linkNum)) < 0) {
	    Log(LG_ERR, ("[%s] can't clear stats, link=%d: %s",
    		b->name, linkNum, strerror(errno)));
	    return (-1);
    }
    return(0);
}

/*
 * NgFuncGetStats()
 *
 * Get link or whole bundle statistics
 */

int
NgFuncGetStats(Bund b, u_int16_t linkNum, struct ng_ppp_link_stat *statp)
{
    union {
        u_char			buf[sizeof(struct ng_mesg)
				  + sizeof(struct ng_ppp_link_stat)];
        struct ng_mesg		reply;
    }				u;
    char			path[NG_PATHSIZ];

    /* Get stats */
    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    if (NgFuncSendQuery(path, NGM_PPP_COOKIE, NGM_PPP_GET_LINK_STATS,
      &linkNum, sizeof(linkNum), &u.reply, sizeof(u), NULL) < 0) {
	Log(LG_ERR, ("[%s] can't get stats, link=%d: %s",
    	    b->name, linkNum, strerror(errno)));
	return -1;
    }
    if (statp != NULL)
	memcpy(statp, u.reply.data, sizeof(*statp));
    return(0);
}

#ifdef NG_PPP_STATS64
/*
 * NgFuncGetStats64()
 *
 * Get 64bit link or whole bundle statistics
 */

int
NgFuncGetStats64(Bund b, u_int16_t linkNum, struct ng_ppp_link_stat64 *statp)
{
    union {
        u_char			buf[sizeof(struct ng_mesg)
				  + sizeof(struct ng_ppp_link_stat64)];
        struct ng_mesg		reply;
    }				u;
    char			path[NG_PATHSIZ];

    /* Get stats */
    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    if (NgFuncSendQuery(path, NGM_PPP_COOKIE, NGM_PPP_GET_LINK_STATS64,
      &linkNum, sizeof(linkNum), &u.reply, sizeof(u), NULL) < 0) {
	Log(LG_ERR, ("[%s] can't get stats, link=%d: %s",
    	    b->name, linkNum, strerror(errno)));
	return -1;
    }
    if (statp != NULL)
	memcpy(statp, u.reply.data, sizeof(*statp));
    return(0);
}
#endif

/*
 * NgFuncErrx()
 */

void
NgFuncErrx(const char *fmt, ...)
{
    char	buf[100];
    va_list	args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Log(LG_ERR, ("netgraph: %s", buf));
}

/*
 * NgFuncErr()
 */

void
NgFuncErr(const char *fmt, ...)
{
    char	buf[100];
    va_list	args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Log(LG_ERR, ("netgraph: %s: %s", buf, strerror(errno)));
}

#ifdef USE_NG_NETFLOW
/*
 * NetflowSetCommand()
 */
       
static int
NetflowSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    struct sockaddr_storage *sin;

    switch ((intptr_t)arg) {
	case SET_PEER: 
    	    if (ac != 2)
		return (-1);
    	    if ((sin = ParseAddrPort(ac, av, ALLOW_IPV4|ALLOW_IPV6)) == NULL)
		return (-1);
    	    gNetflowExport = *sin;
    	    break;
	case SET_SELF:
    	    if ((sin = ParseAddrPort(ac, av, ALLOW_IPV4|ALLOW_IPV6)) == NULL)
		return (-1);
    	    gNetflowSource = *sin;
    	    break;
	case SET_TIMEOUTS:
    	    if (ac != 2)
		return (-1);
    	    if (atoi(av[0]) <= 0 || atoi(av[1]) <= 0)
		Error("Bad netflow timeouts \"%s %s\"", av[0], av[1]);
    	    gNetflowInactive = atoi(av[0]);
    	    gNetflowActive = atoi(av[1]);
    	    break;
	case SET_NODE:
    	    if (ac != 1)
		return (-1);
    	    if (strlen(av[0]) == 0 || strlen(av[0]) > 63)
		Error("Bad netflow node name \"%s\"", av[0]);
    	    strlcpy(gNetflowNodeName, av[0], sizeof(gNetflowNodeName));
    	    gNetflowNode=TRUE;
    	    gNetflowNodeShutdown=FALSE;
    	    break;
	case SET_HOOK:
    	    if (ac != 1)
		return (-1);
    	    if (atoi(av[0]) <= 0)
		Error("Bad netflow hook number \"%s\"", av[0]);
    	    gNetflowIface = atoi(av[0])-1;
    	    break;

	default:
	    return (-1);
    }

    return (0);
}
#endif /* USE_NG_NETFLOW */

ng_ID_t
NgGetNodeID(int csock, const char *path)
{
    union {
        u_char          buf[sizeof(struct ng_mesg) + sizeof(struct nodeinfo)];
	struct ng_mesg  reply;
    }                   u;
    struct nodeinfo     *const ni = (struct nodeinfo *)(void *)u.reply.data;
    
    if (csock < 0) {
	if (!gNgStatSock) {
	    char		name[NG_NODESIZ];
	
	    /* Create a netgraph socket node */
	    snprintf(name, sizeof(name), "mpd%d-stats", gPid);
	    if (NgMkSockNode(name, &gNgStatSock, NULL) < 0) {
    		Log(LG_ERR, ("NgFuncSendQuery: can't create %s node: %s",
    	    	    NG_SOCKET_NODE_TYPE, strerror(errno)));
    		return(-1);
	    }
	    (void) fcntl(gNgStatSock, F_SETFD, 1);
	}
	csock = gNgStatSock;
    }

    if (NgSendMsg(csock, path,
      NGM_GENERIC_COOKIE, NGM_NODEINFO, NULL, 0) < 0)
        return (0);
    if (NgRecvMsg(csock, &u.reply, sizeof(u), NULL) < 0)
	return (0);
    
    return (ni->id);
}

