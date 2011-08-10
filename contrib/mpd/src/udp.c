
/*
 * udp.c
 *
 * Written by  Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "phys.h"
#include "mbuf.h"
#include "udp.h"
#include "ngfunc.h"
#include "util.h"
#include "log.h"

#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_ksocket.h>
#include <netgraph.h>

/*
 * XXX this device type not completely correct, 
 * as it can deliver out-of-order frames. This can make problems 
 * for different compression and encryption protocols.
 */

/*
 * DEFINITIONS
 */

  #define UDP_MTU		2048
  #define UDP_MRU		2048

  #define UDP_MAXPARENTIFS	256

  struct udpinfo {
    struct {
	struct optinfo	options;
	struct u_addr	self_addr;	/* Configured local IP address */
	struct u_range	peer_addr;	/* Configured peer IP address */
	in_port_t	self_port;	/* Configured local port */
	in_port_t	peer_port;	/* Configured peer port */
	char		*fqdn_peer_addr; /* FQDN Peer address */
    } conf;

    /* State */
    u_char		incoming;		/* incoming vs. outgoing */
    struct UdpIf 	*If;
    struct u_addr	peer_addr;
    in_port_t		peer_port;
    ng_ID_t		node_id;
  };
  typedef struct udpinfo	*UdpInfo;

/* Set menu options */

  enum {
    SET_PEERADDR,
    SET_SELFADDR,
    SET_ENABLE,
    SET_DISABLE
  };

  /* Binary options */
  enum {
    UDP_CONF_RESOLVE_ONCE	/* Only once resolve peer_addr */
  };

/*
 * INTERNAL FUNCTIONS
 */

  static int	UdpInit(Link l);
  static int	UdpInst(Link l, Link lt);
  static void	UdpOpen(Link l);
  static void	UdpClose(Link l);
  static void	UdpStat(Context ctx);
  static int	UdpOrigination(Link l);
  static int	UdpIsSync(Link l);
  static int	UdpSelfAddr(Link l, void *buf, size_t buf_len);
  static int	UdpPeerAddr(Link l, void *buf, size_t buf_len);
  static int	UdpPeerPort(Link l, void *buf, size_t buf_len);
  static int	UdpCallingNum(Link l, void *buf, size_t buf_len);
  static int	UdpCalledNum(Link l, void *buf, size_t buf_len);

  static void	UdpDoClose(Link l);
  static void	UdpShutdown(Link l);
  static int	UdpSetCommand(Context ctx, int ac, char *av[], void *arg);
  static void	UdpNodeUpdate(Link l);
  static int	UdpListen(Link l);
  static int	UdpUnListen(Link l);

/*
 * GLOBAL VARIABLES
 */

  const struct phystype gUdpPhysType = {
    .name		= "udp",
    .descr		= "PPP over UDP",
    .mtu		= UDP_MTU,
    .mru		= UDP_MRU,
    .tmpl		= 1,
    .init		= UdpInit,
    .inst		= UdpInst,
    .open		= UdpOpen,
    .close		= UdpClose,
    .update		= UdpNodeUpdate,
    .shutdown		= UdpShutdown,
    .showstat		= UdpStat,
    .originate		= UdpOrigination,
    .issync		= UdpIsSync,
    .selfaddr		= UdpSelfAddr,
    .peeraddr		= UdpPeerAddr,
    .peerport		= UdpPeerPort,
    .callingnum		= UdpCallingNum,
    .callednum		= UdpCalledNum,
  };

  const struct cmdtab UdpSetCmds[] = {
    { "self {ip} [{port}]",		"Set local IP address",
	UdpSetCommand, NULL, 2, (void *) SET_SELFADDR },
    { "peer {ip} [{port}]",		"Set remote IP address",
	UdpSetCommand, NULL, 2, (void *) SET_PEERADDR },
    { "enable [opt ...]",		"Enable option",
	UdpSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",		"Disable option",
	UdpSetCommand, NULL, 2, (void *) SET_DISABLE },
    { NULL },
  };

struct UdpIf {
    struct u_addr	self_addr;
    in_port_t	self_port;
    int		refs;
    int		csock;                  /* netgraph Control socket */
    EventRef	ctrlEvent;		/* listen for ctrl messages */
};
struct UdpIf UdpIfs[UDP_MAXPARENTIFS];

int UdpListenUpdateSheduled=0;
struct pppTimer UdpListenUpdateTimer;

 /*
 * INTERNAL VARIABLES
 */

  static struct confinfo	gConfList[] = {
    { 0,	UDP_CONF_RESOLVE_ONCE,	"resolve-once"	},
    { 0,	0,			NULL		},
  };


/*
 * UdpInit()
 */

static int
UdpInit(Link l)
{
    UdpInfo	pi;

    pi = (UdpInfo) (l->info = Malloc(MB_PHYS, sizeof(*pi)));

    u_addrclear(&pi->conf.self_addr);
    u_rangeclear(&pi->conf.peer_addr);
    pi->conf.self_port=0;
    pi->conf.peer_port=0;

    pi->incoming = 0;
    pi->If = NULL;

    u_addrclear(&pi->peer_addr);
    pi->peer_port=0;
    pi->conf.fqdn_peer_addr = NULL;
    Enable(&pi->conf.options, UDP_CONF_RESOLVE_ONCE);

    return(0);
}

/*
 * UdpInst()
 */

static int
UdpInst(Link l, Link lt)
{
    UdpInfo	pi;
    l->info = Mdup(MB_PHYS, lt->info, sizeof(struct udpinfo));
    pi = (UdpInfo) l->info;
    
    if (pi->If)
	pi->If->refs++;

    return(0);
}

/*
 * UdpOpen()
 */

static void
UdpOpen(Link l)
{
	UdpInfo			const pi = (UdpInfo) l->info;
	char        		path[NG_PATHSIZ];
	char        		hook[NG_HOOKSIZ];
	struct ngm_mkpeer	mkp;
	struct ngm_name         nm;
	struct sockaddr_storage	addr;
        union {
            u_char buf[sizeof(struct ng_ksocket_sockopt) + sizeof(int)];
            struct ng_ksocket_sockopt ksso;
        } u;
        struct ng_ksocket_sockopt *const ksso = &u.ksso;
	int			csock;

	/* Create a new netgraph node to control TCP ksocket node. */
	if (NgMkSockNode(NULL, &csock, NULL) < 0) {
		Log(LG_ERR, ("[%s] TCP can't create control socket: %s",
		    l->name, strerror(errno)));
		goto fail;
	}
	(void)fcntl(csock, F_SETFD, 1);

        if (!PhysGetUpperHook(l, path, hook)) {
		Log(LG_PHYS, ("[%s] UDP: can't get upper hook", l->name));
    		goto fail;
        }

	/* Attach ksocket node to PPP node */
	strcpy(mkp.type, NG_KSOCKET_NODE_TYPE);
	strlcpy(mkp.ourhook, hook, sizeof(mkp.ourhook));
	if ((pi->conf.self_addr.family==AF_INET6) || 
	    (pi->conf.self_addr.family==AF_UNSPEC && pi->conf.peer_addr.addr.family==AF_INET6)) {
	        snprintf(mkp.peerhook, sizeof(mkp.peerhook), "%d/%d/%d", PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	} else {
	    snprintf(mkp.peerhook, sizeof(mkp.peerhook), "inet/dgram/udp");
	}
	if (NgSendMsg(csock, path, NGM_GENERIC_COOKIE,
	    NGM_MKPEER, &mkp, sizeof(mkp)) < 0) {
	        Log(LG_ERR, ("[%s] can't attach %s node: %s",
	    	    l->name, NG_KSOCKET_NODE_TYPE, strerror(errno)));
		goto fail;
	}

	strlcat(path, ".", sizeof(path));
	strlcat(path, hook, sizeof(path));

	/* Give it a name */
	snprintf(nm.name, sizeof(nm.name), "mpd%d-%s-kso", gPid, l->name);
	if (NgSendMsg(csock, path,
	    NGM_GENERIC_COOKIE, NGM_NAME, &nm, sizeof(nm)) < 0) {
		Log(LG_ERR, ("[%s] can't name %s node: %s",
		    l->name, NG_KSOCKET_NODE_TYPE, strerror(errno)));
	}

	if ((pi->node_id = NgGetNodeID(csock, path)) == 0) {
	    Log(LG_ERR, ("[%s] Cannot get %s node id: %s",
		l->name, NG_KSOCKET_NODE_TYPE, strerror(errno)));
	    goto fail;
	};

    if ((pi->incoming) || (pi->conf.self_port != 0)) {
	/* Setsockopt socket. */
	ksso->level=SOL_SOCKET;
	ksso->name=SO_REUSEPORT;
	((int *)(ksso->value))[0]=1;
	if (NgSendMsg(csock, path, NGM_KSOCKET_COOKIE,
    	    NGM_KSOCKET_SETOPT, &u, sizeof(u)) < 0) {
    	    Log(LG_ERR, ("[%s] can't setsockopt() %s node: %s",
    		l->name, NG_KSOCKET_NODE_TYPE, strerror(errno)));
	    goto fail;
	}

	if ((!Enabled(&pi->conf.options, UDP_CONF_RESOLVE_ONCE)) &&
	    (pi->conf.fqdn_peer_addr != NULL)) {
	    struct u_range	rng;
	    if (ParseRange(pi->conf.fqdn_peer_addr, &rng, ALLOW_IPV4|ALLOW_IPV6))
		pi->conf.peer_addr = rng;
	}

	/* Bind socket */
	u_addrtosockaddr(&pi->conf.self_addr, pi->conf.self_port, &addr);
	if (NgSendMsg(csock, path, NGM_KSOCKET_COOKIE,
	  NGM_KSOCKET_BIND, &addr, addr.ss_len) < 0) {
	    Log(LG_ERR, ("[%s] can't bind() %s node: %s",
    		l->name, NG_KSOCKET_NODE_TYPE, strerror(errno)));
	    goto fail;
	}
    }

    if (!pi->incoming) {
	if ((!u_rangeempty(&pi->conf.peer_addr)) && (pi->conf.peer_port != 0)) {
	    u_addrcopy(&pi->conf.peer_addr.addr,&pi->peer_addr);
	    pi->peer_port = pi->conf.peer_port;
	} else {
	    Log(LG_ERR, ("[%s] Can't connect without peer specified", l->name));
	    goto fail;
	}
    }
    u_addrtosockaddr(&pi->peer_addr, pi->peer_port, &addr);

    /* Connect socket if peer address and port is specified */
    if (NgSendMsg(csock, path, NGM_KSOCKET_COOKIE,
      NGM_KSOCKET_CONNECT, &addr, addr.ss_len) < 0) {
	Log(LG_ERR, ("[%s] can't connect() %s node: %s",
	    l->name, NG_KSOCKET_NODE_TYPE, strerror(errno)));
	goto fail;
    }
  
    close(csock);

    /* OK */
    l->state = PHYS_STATE_UP;
    PhysUp(l);
    return;

fail:
    UdpDoClose(l);
    pi->incoming=0;
    l->state = PHYS_STATE_DOWN;
    u_addrclear(&pi->peer_addr);
    pi->peer_port=0;
    if (csock>0)
	close(csock);

    PhysDown(l, STR_ERROR, NULL);
}

/*
 * UdpClose()
 */

static void
UdpClose(Link l)
{
    UdpInfo const pi = (UdpInfo) l->info;
    if (l->state != PHYS_STATE_DOWN) {
	UdpDoClose(l);
	pi->incoming=0;
	l->state = PHYS_STATE_DOWN;
	u_addrclear(&pi->peer_addr);
	pi->peer_port=0;
	PhysDown(l, STR_MANUALLY, NULL);
    }
}

/*
 * UdpShutdown()
 */

static void
UdpShutdown(Link l)
{
    UdpInfo const pi = (UdpInfo) l->info;

    if (pi->conf.fqdn_peer_addr)
        Freee(pi->conf.fqdn_peer_addr);

	UdpDoClose(l);
	UdpUnListen(l);
	Freee(l->info);
}

/*
 * UdpDoClose()
 */

static void
UdpDoClose(Link l)
{
	UdpInfo	const pi = (UdpInfo) l->info;
	char	path[NG_PATHSIZ];
	int	csock;

	if (pi->node_id == 0)
		return;

	/* Get a temporary netgraph socket node */
	if (NgMkSockNode(NULL, &csock, NULL) == -1) {
		Log(LG_ERR, ("UDP: NgMkSockNode: %s", strerror(errno)));
		return;
	}
	
	/* Disconnect session hook. */
	snprintf(path, sizeof(path), "[%lx]:", (u_long)pi->node_id);
	NgFuncShutdownNode(csock, l->name, path);
	
	close(csock);
	
	pi->node_id = 0;
}

/*
 * UdpOrigination()
 */

static int
UdpOrigination(Link l)
{
    UdpInfo	const pi = (UdpInfo) l->info;

    return (pi->incoming ? LINK_ORIGINATE_REMOTE : LINK_ORIGINATE_LOCAL);
}

/*
 * UdpIsSync()
 */

static int
UdpIsSync(Link l)
{
    return (1);
}

static int
UdpSelfAddr(Link l, void *buf, size_t buf_len)
{
    UdpInfo	const pi = (UdpInfo) l->info;

    if (!u_addrempty(&pi->conf.self_addr)) {
	if (u_addrtoa(&pi->conf.self_addr, buf, buf_len))
	    return (0);
  	else {
	    ((char*)buf)[0]=0;
	    return (-1);
	}
    }
    ((char*)buf)[0]=0;
    return (0);
}

static int
UdpPeerAddr(Link l, void *buf, size_t buf_len)
{
    UdpInfo	const pi = (UdpInfo) l->info;

    if (u_addrtoa(&pi->peer_addr, buf, buf_len))
	return(0);
    else
	return(-1);
}

static int
UdpPeerPort(Link l, void *buf, size_t buf_len)
{
    UdpInfo	const pi = (UdpInfo) l->info;

    if (snprintf(buf, buf_len, "%d", pi->peer_port))
	return(0);
    else
	return(-1);
}

static int
UdpCallingNum(Link l, void *buf, size_t buf_len)
{
	UdpInfo const pi = (UdpInfo) l->info;

	if (pi->incoming) {
	    if (u_addrtoa(&pi->peer_addr, buf, buf_len))
	    	return (0);
  	    else
		return (-1);
	} else {
	    if (u_addrtoa(&pi->conf.self_addr, buf, buf_len))
	    	return (0);
  	    else
		return (-1);
	}
}

static int
UdpCalledNum(Link l, void *buf, size_t buf_len)
{
	UdpInfo const pi = (UdpInfo) l->info;

	if (!pi->incoming) {
	    if (u_addrtoa(&pi->peer_addr, buf, buf_len))
	    	return (0);
  	    else
		return (-1);
	} else {
	    if (u_addrtoa(&pi->conf.self_addr, buf, buf_len))
	    	return (0);
  	    else
		return (-1);
	}
}

/*
 * UdpStat()
 */

void
UdpStat(Context ctx)
{
	UdpInfo const pi = (UdpInfo) ctx->lnk->info;
	char	buf[48];

	Printf("UDP configuration:\r\n");
	Printf("\tPeer FQDN    : %s\r\n", pi->conf.fqdn_peer_addr);
	Printf("\tSelf address : %s, port %u\r\n",
	    u_addrtoa(&pi->conf.self_addr, buf, sizeof(buf)), pi->conf.self_port);
	Printf("\tPeer address : %s, port %u\r\n",
	    u_rangetoa(&pi->conf.peer_addr, buf, sizeof(buf)), pi->conf.peer_port);
	Printf("UDP state:\r\n");
	if (ctx->lnk->state != PHYS_STATE_DOWN) {
	    Printf("\tIncoming     : %s\r\n", (pi->incoming?"YES":"NO"));
	    Printf("\tCurrent peer : %s, port %u\r\n",
		u_addrtoa(&pi->peer_addr, buf, sizeof(buf)), pi->peer_port);
	}
}

/*
 * UdpAcceptEvent() triggers when we accept incoming connection.
 */

static void
UdpAcceptEvent(int type, void *cookie)
{
	struct sockaddr_storage saddr;
	socklen_t	saddrlen;
	struct u_addr	addr;
	in_port_t	port;
	char		buf[48];
	char		buf1[48];
	int 		k;
	struct UdpIf 	*If=(struct UdpIf *)(cookie);
	Link		l = NULL;
	UdpInfo		pi = NULL;

	char		pktbuf[UDP_MRU+100];
	ssize_t		pktlen;

	assert(type == EVENT_READ);

	saddrlen = sizeof(saddr);
	if ((pktlen = recvfrom(If->csock, pktbuf, sizeof(pktbuf), MSG_DONTWAIT, (struct sockaddr *)(&saddr), &saddrlen)) < 0) {
	    Log(LG_PHYS, ("recvfrom() error: %s", strerror(errno)));
	}

	sockaddrtou_addr(&saddr, &addr, &port);

	Log(LG_PHYS, ("Incoming UDP connection from %s %u to %s %u",
	    u_addrtoa(&addr, buf, sizeof(buf)), port,
	    u_addrtoa(&If->self_addr, buf1, sizeof(buf1)), If->self_port));

	if (gShutdownInProgress) {
		Log(LG_PHYS, ("Shutdown sequence in progress, ignoring request."));
		goto failed;
	}

	if (OVERLOAD()) {
		Log(LG_PHYS, ("Daemon overloaded, ignoring request."));
		goto failed;
	}

	/* Examine all UDP links. */
	for (k = 0; k < gNumLinks; k++) {
		Link l2;
	        UdpInfo pi2;

		if (!gLinks[k] || gLinks[k]->type != &gUdpPhysType)
			continue;

		l2 = gLinks[k];
		pi2 = (UdpInfo)l2->info;

		if ((!PhysIsBusy(l2)) &&
		    Enabled(&l2->conf.options, LINK_CONF_INCOMING) &&
		    (pi2->If == If) &&
		    IpAddrInRange(&pi2->conf.peer_addr, &addr) &&
		    (pi2->conf.peer_port == 0 || pi2->conf.peer_port == port)) {

			if (pi == NULL || pi2->conf.peer_addr.width > pi->conf.peer_addr.width) {
				l = l2;
				pi = pi2;
				if (u_rangehost(&pi->conf.peer_addr)) {
					break;	/* Nothing could be better */
				}
			}
		}
	}
	if (l != NULL && l->tmpl)
    		l = LinkInst(l, NULL, 0, 0);

	if (l != NULL) {
    		pi = (UdpInfo)l->info;
		Log(LG_PHYS, ("[%s] Accepting UDP connection from %s %u to %s %u",
		    l->name, u_addrtoa(&addr, buf, sizeof(buf)), port,
		    u_addrtoa(&If->self_addr, buf1, sizeof(buf1)), If->self_port));

		sockaddrtou_addr(&saddr, &pi->peer_addr, &pi->peer_port);

		pi->incoming=1;
		l->state = PHYS_STATE_READY;

		PhysIncoming(l);
	} else {
		Log(LG_PHYS, ("No free UDP link with requested parameters "
	    	    "was found"));
	}

failed:
	EventRegister(&If->ctrlEvent, EVENT_READ, If->csock,
	    0, UdpAcceptEvent, If);
}

static int 
UdpListen(Link l)
{
	UdpInfo const pi = (UdpInfo) l->info;
	struct sockaddr_storage addr;
	char buf[48];
	int opt, i, j = -1, free = -1;
	
	if (pi->If)
	    return(1);

	for (i = 0; i < UDP_MAXPARENTIFS; i++) {
	    if (UdpIfs[i].self_port == 0)
	        free = i;
	    else if ((u_addrcompare(&UdpIfs[i].self_addr, &pi->conf.self_addr) == 0) &&
	        (UdpIfs[i].self_port == pi->conf.self_port)) {
	            j = i;
	    	    break;
	    }
	}

	if (j >= 0) {
	    UdpIfs[j].refs++;
	    pi->If=&UdpIfs[j];
	    return(1);
	}

	if (free < 0) {
	    Log(LG_ERR, ("[%s] UDP: Too many different listening ports! ", 
		l->name));
	    return (0);
	}

	UdpIfs[free].refs = 1;
	pi->If=&UdpIfs[free];
	
	u_addrcopy(&pi->conf.self_addr,&pi->If->self_addr);
	pi->If->self_port=pi->conf.self_port;
	
	/* Make listening UDP socket. */
	if (pi->If->self_addr.family==AF_INET6) {
	    pi->If->csock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	} else {
	    pi->If->csock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	(void)fcntl(pi->If->csock, F_SETFD, 1);

	/* Setsockopt socket. */
	opt = 1;
	if (setsockopt(pi->If->csock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
		Log(LG_ERR, ("UDP: can't setsockopt socket: %s",
		    strerror(errno)));
		goto fail2;
	};

	/* Bind socket. */
	u_addrtosockaddr(&pi->If->self_addr, pi->If->self_port, &addr);
	if (bind(pi->If->csock, (struct sockaddr *)(&addr), addr.ss_len)) {
		Log(LG_ERR, ("UDP: can't bind socket: %s",
		    strerror(errno)));
		goto fail2;
	}

	Log(LG_PHYS, ("UDP: waiting for connection on %s %u",
	    u_addrtoa(&pi->If->self_addr, buf, sizeof(buf)), pi->If->self_port));
	EventRegister(&pi->If->ctrlEvent, EVENT_READ, pi->If->csock,
	    0, UdpAcceptEvent, pi->If);

	return (1);
fail2:
	close(pi->If->csock);
	pi->If->csock = -1;
	pi->If->self_port = 0;
	pi->If = NULL;
	return (0);
}


static int 
UdpUnListen(Link l)
{
	UdpInfo const pi = (UdpInfo) l->info;
	char buf[48];
	
	if (!pi->If)
	    return(1);

	pi->If->refs--;
	if (pi->If->refs == 0) {
	    Log(LG_PHYS, ("UDP: stop waiting for connection on %s %u",
		u_addrtoa(&pi->If->self_addr, buf, sizeof(buf)), pi->If->self_port));
	    EventUnRegister(&pi->If->ctrlEvent);
	    close(pi->If->csock);
	    pi->If->csock = -1;
	    pi->If->self_port = 0;
	    pi->If = NULL;
	}

	return (1);
}

/*
 * UdpNodeUpdate()
 */

static void
UdpNodeUpdate(Link l)
{
    UdpInfo const pi = (UdpInfo) l->info;
    if (!pi->If) {
	if (Enabled(&l->conf.options, LINK_CONF_INCOMING))
	    UdpListen(l);
    } else {
	if (!Enabled(&l->conf.options, LINK_CONF_INCOMING))
	    UdpUnListen(l);
    }
}

/*
 * UdpSetCommand()
 */

static int
UdpSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    UdpInfo		const pi = (UdpInfo) ctx->lnk->info;
    char		**fqdn_peer_addr = &pi->conf.fqdn_peer_addr;
    struct u_range	rng;
    int			port;
	
    switch ((intptr_t)arg) {
	case SET_PEERADDR:
	case SET_SELFADDR:
	    if ((ac == 1 || ac == 2) && (intptr_t)arg == SET_PEERADDR) {
		if (*fqdn_peer_addr)
		    Freee(*fqdn_peer_addr);
		*fqdn_peer_addr = Mstrdup(MB_PHYS, av[0]);
	    }
    	    if (ac < 1 || ac > 2 || !ParseRange(av[0], &rng, ALLOW_IPV4|ALLOW_IPV6))
		return(-1);
    	    if (ac > 1) {
		if ((port = atoi(av[1])) < 0 || port > 0xffff)
		    return(-1);
    	    } else {
		port = 0;
    	    }
    	    if ((intptr_t)arg == SET_SELFADDR) {
		pi->conf.self_addr = rng.addr;
		pi->conf.self_port = port;
    	    } else {
		pi->conf.peer_addr = rng;
		pi->conf.peer_port = port;
    	    }
	    if (pi->If) {
		UdpUnListen(ctx->lnk);
		UdpListen(ctx->lnk);
	    }
    	    break;
	case SET_ENABLE:
	    EnableCommand(ac, av, &pi->conf.options, gConfList);
	    UdpNodeUpdate(ctx->lnk);
	    break;
	case SET_DISABLE:
	    DisableCommand(ac, av, &pi->conf.options, gConfList);
	    UdpNodeUpdate(ctx->lnk);
	    break;
	default:
    	    assert(0);
    }
    return(0);
}

