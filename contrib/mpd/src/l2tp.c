
/*
 * l2tp.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "phys.h"
#include "mbuf.h"
#include "ngfunc.h"
#include "l2tp.h"
#include "l2tp_avp.h"
#include "l2tp_ctrl.h"
#include "log.h"
#include "util.h"

#include <sys/types.h>
#ifdef NOLIBPDEL
#include "contrib/libpdel/util/ghash.h"
#else
#include <pdel/util/ghash.h>
#endif

#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_ksocket.h>
#include <netgraph/ng_l2tp.h>
#include <netgraph.h>

/*
 * DEFINITIONS
 */

  #define L2TP_MTU              1600
  #define L2TP_MRU		L2TP_MTU
  
  #define L2TP_PORT		1701

  #define L2TP_CALL_MIN_BPS	56000
  #define L2TP_CALL_MAX_BPS	64000

  struct l2tp_server {
    struct u_addr	self_addr;	/* self IP address */
    in_port_t		self_port;	/* self port */
    int			refs;
    int			sock;		/* server listen socket */
    EventRef		event;		/* listen for data messages */
  };
  
  struct l2tp_tun {
    struct u_addr	self_addr;	/* self IP address */
    struct u_addr	peer_addr;	/* peer IP address */
    char                peer_iface[IFNAMSIZ];	/* Peer iface */
    u_char		peer_mac_addr[6];	/* Peer MAC address */
    in_port_t		self_port;	/* self port */
    in_port_t		peer_port;	/* peer port */
    u_char		connected;	/* control connection is connected */
    u_char		alive;		/* control connection is not dying */
    u_int		active_sessions;/* number of calls in this sunnels */
    struct ppp_l2tp_ctrl *ctrl;		/* control connection for this tunnel */
  };
  
  struct l2tpinfo {
    struct {
	struct u_addr	self_addr;	/* self IP address */
	struct u_range	peer_addr;	/* Peer IP addresses allowed */
	in_port_t	self_port;	/* self port */
	in_port_t	peer_port;	/* Peer port required (or zero) */
	struct optinfo	options;
	char		callingnum[64];	/* L2TP phone number to use */
	char		callednum[64];	/* L2TP phone number to use */
	char 		hostname[MAXHOSTNAMELEN]; /* L2TP local hostname */
	char		secret[64];	/* L2TP tunnel secret */
    } conf;
    u_char		opened;		/* L2TP opened by phys */
    u_char		incoming;	/* Call is incoming vs. outgoing */
    u_char		outcall;	/* incall or outcall */
    u_char		sync;		/* sync or async call */
    struct l2tp_server	*server;	/* server associated with link */
    struct l2tp_tun	*tun;		/* tunnel associated with link */
    struct ppp_l2tp_sess *sess;		/* current session for this link */
    char		callingnum[64];	/* current L2TP phone number */
    char		callednum[64];	/* current L2TP phone number */
  };
  typedef struct l2tpinfo	*L2tpInfo;

  /* Set menu options */
  enum {
    SET_SELFADDR,
    SET_PEERADDR,
    SET_CALLINGNUM,
    SET_CALLEDNUM,
    SET_HOSTNAME,
    SET_SECRET,
    SET_ENABLE,
    SET_DISABLE
  };

  /* Binary options */
  enum {
    L2TP_CONF_OUTCALL,		/* when originating, calls are "outgoing" */
    L2TP_CONF_HIDDEN,		/* enable AVP hidding */
    L2TP_CONF_LENGTH,		/* enable Length field in data packets */
    L2TP_CONF_DATASEQ		/* enable sequence fields in data packets */
  };

/*
 * INTERNAL FUNCTIONS
 */

  static int	L2tpTInit(void);
  static void	L2tpTShutdown(void);
  static int	L2tpInit(Link l);
  static int	L2tpInst(Link l, Link lt);
  static void	L2tpOpen(Link l);
  static void	L2tpClose(Link l);
  static void	L2tpShutdown(Link l);
  static void	L2tpStat(Context ctx);
  static int	L2tpOriginated(Link l);
  static int	L2tpIsSync(Link l);
  static int	L2tpSetAccm(Link l, u_int32_t xmit, u_int32_t recv);
  static int	L2tpSelfName(Link l, void *buf, size_t buf_len);
  static int	L2tpPeerName(Link l, void *buf, size_t buf_len);
  static int	L2tpSelfAddr(Link l, void *buf, size_t buf_len);
  static int	L2tpPeerAddr(Link l, void *buf, size_t buf_len);
  static int	L2tpPeerPort(Link l, void *buf, size_t buf_len);
  static int	L2tpPeerMacAddr(Link l, void *buf, size_t buf_len);
  static int	L2tpPeerIface(Link l, void *buf, size_t buf_len);
  static int	L2tpCallingNum(Link l, void *buf, size_t buf_len);
  static int	L2tpCalledNum(Link l, void *buf, size_t buf_len);
  static int	L2tpSetCallingNum(Link l, void *buf);
  static int	L2tpSetCalledNum(Link l, void *buf);

  static void	L2tpHookUp(Link l);
  static void	L2tpUnhook(Link l);

  static void	L2tpNodeUpdate(Link l);
  static int	L2tpListen(Link l);
  static void	L2tpUnListen(Link l);
  static int	L2tpSetCommand(Context ctx, int ac, char *av[], void *arg);

  /* L2TP control callbacks */
  static ppp_l2tp_ctrl_connected_t	ppp_l2tp_ctrl_connected_cb;
  static ppp_l2tp_ctrl_terminated_t	ppp_l2tp_ctrl_terminated_cb;
  static ppp_l2tp_ctrl_destroyed_t	ppp_l2tp_ctrl_destroyed_cb;
  static ppp_l2tp_initiated_t		ppp_l2tp_initiated_cb;
  static ppp_l2tp_connected_t		ppp_l2tp_connected_cb;
  static ppp_l2tp_terminated_t		ppp_l2tp_terminated_cb;
  static ppp_l2tp_set_link_info_t	ppp_l2tp_set_link_info_cb;

  static const struct ppp_l2tp_ctrl_cb ppp_l2tp_server_ctrl_cb = {
	ppp_l2tp_ctrl_connected_cb,
	ppp_l2tp_ctrl_terminated_cb,
	ppp_l2tp_ctrl_destroyed_cb,
	ppp_l2tp_initiated_cb,
	ppp_l2tp_connected_cb,
	ppp_l2tp_terminated_cb,
	ppp_l2tp_set_link_info_cb,
	NULL,
  };

/*
 * GLOBAL VARIABLES
 */

  const struct phystype	gL2tpPhysType = {
    .name		= "l2tp",
    .descr		= "Layer Two Tunneling Protocol",
    .mtu		= L2TP_MTU,
    .mru		= L2TP_MRU,
    .tmpl		= 1,
    .tinit		= L2tpTInit,
    .tshutdown		= L2tpTShutdown,
    .init		= L2tpInit,
    .inst		= L2tpInst,
    .open		= L2tpOpen,
    .close		= L2tpClose,
    .update		= L2tpNodeUpdate,
    .shutdown		= L2tpShutdown,
    .showstat		= L2tpStat,
    .originate		= L2tpOriginated,
    .issync		= L2tpIsSync,
    .setaccm 		= L2tpSetAccm,
    .setcallingnum	= L2tpSetCallingNum,
    .setcallednum	= L2tpSetCalledNum,
    .selfname		= L2tpSelfName,
    .peername		= L2tpPeerName,
    .selfaddr		= L2tpSelfAddr,
    .peeraddr		= L2tpPeerAddr,
    .peerport		= L2tpPeerPort,
    .peermacaddr	= L2tpPeerMacAddr,
    .peeriface		= L2tpPeerIface,
    .callingnum		= L2tpCallingNum,
    .callednum		= L2tpCalledNum,
  };

  const struct cmdtab	L2tpSetCmds[] = {
    { "self {ip} [{port}]",		"Set local IP address",
	L2tpSetCommand, NULL, 2, (void *) SET_SELFADDR },
    { "peer {ip} [{port}]",		"Set remote IP address",
	L2tpSetCommand, NULL, 2, (void *) SET_PEERADDR },
    { "callingnum {number}",		"Set calling L2TP telephone number",
	L2tpSetCommand, NULL, 2, (void *) SET_CALLINGNUM },
    { "callednum {number}",		"Set called L2TP telephone number",
	L2tpSetCommand, NULL, 2, (void *) SET_CALLEDNUM },
    { "hostname {name}",		"Set L2TP local hostname",
	L2tpSetCommand, NULL, 2, (void *) SET_HOSTNAME },
    { "secret {sec}",			"Set L2TP tunnel secret",
	L2tpSetCommand, NULL, 2, (void *) SET_SECRET },
    { "enable [opt ...]",		"Enable option",
	L2tpSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",		"Disable option",
	L2tpSetCommand, NULL, 2, (void *) SET_DISABLE },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static struct confinfo	gConfList[] = {
    { 0,	L2TP_CONF_OUTCALL,	"outcall"	},
    { 0,	L2TP_CONF_HIDDEN,	"hidden"	},
    { 0,	L2TP_CONF_LENGTH,	"length"	},
    { 0,	L2TP_CONF_DATASEQ,	"dataseq"	},
    { 0,	0,			NULL		},
  };

int L2tpListenUpdateSheduled = 0;
struct pppTimer L2tpListenUpdateTimer;

struct ghash	*gL2tpServers;
struct ghash	*gL2tpTuns;
int		one = 1;

/*
 * L2tpTInit()
 */

static int
L2tpTInit(void)
{
    if ((gL2tpServers = ghash_create(NULL, 0, 0, MB_PHYS, NULL, NULL, NULL, NULL))
	  == NULL)
	return(-1);
    if ((gL2tpTuns = ghash_create(NULL, 0, 0, MB_PHYS, NULL, NULL, NULL, NULL))
	  == NULL)
	return(-1);
    return(0);
}

/*
 * L2tpTShutdown()
 */

static void
L2tpTShutdown(void)
{
    struct ghash_walk walk;
    struct l2tp_tun *tun;

    Log(LG_PHYS2, ("L2TP: Total shutdown"));
    ghash_walk_init(gL2tpTuns, &walk);
    while ((tun = ghash_walk_next(gL2tpTuns, &walk)) != NULL) {
        if (tun->ctrl) {
    	    if (tun->alive)
	        ppp_l2tp_ctrl_shutdown(tun->ctrl,
		    L2TP_RESULT_SHUTDOWN, 0, NULL);
	    ppp_l2tp_ctrl_destroy(&tun->ctrl);
	}
    }
    ghash_destroy(&gL2tpServers);
    ghash_destroy(&gL2tpTuns);
}

/*
 * L2tpInit()
 */

static int
L2tpInit(Link l)
{
    L2tpInfo	l2tp;

    /* Initialize this link */
    l2tp = (L2tpInfo) (l->info = Malloc(MB_PHYS, sizeof(*l2tp)));
  
    u_addrclear(&l2tp->conf.self_addr);
    l2tp->conf.self_addr.family = AF_INET;
    l2tp->conf.self_port = 0;
    u_rangeclear(&l2tp->conf.peer_addr);
    l2tp->conf.peer_addr.addr.family = AF_INET;
    l2tp->conf.peer_addr.width = 0;
    l2tp->conf.peer_port = 0;

    Enable(&l2tp->conf.options, L2TP_CONF_DATASEQ);
  
    return(0);
}

/*
 * L2tpInst()
 */

static int
L2tpInst(Link l, Link lt)
{
	L2tpInfo pi;
	l->info = Mdup(MB_PHYS, lt->info, sizeof(struct l2tpinfo));
	pi = (L2tpInfo) l->info;
    
	if (pi->server)
	    pi->server->refs++;
	
	return(0);
}

/*
 * L2tpOpen()
 */

static void
L2tpOpen(Link l)
{
	L2tpInfo const pi = (L2tpInfo) l->info;

	struct l2tp_tun *tun = NULL;
	struct ppp_l2tp_sess *sess;
	struct ppp_l2tp_avp_list *avps = NULL;
	union {
	    u_char buf[sizeof(struct ng_ksocket_sockopt) + sizeof(int)];
	    struct ng_ksocket_sockopt sockopt;
	} sockopt_buf;
	struct ng_ksocket_sockopt *const sockopt = &sockopt_buf.sockopt;
	union {
	    u_char	buf[sizeof(struct ng_mesg) + sizeof(struct sockaddr_storage)];
	    struct ng_mesg	reply;
	} ugetsas;
	struct sockaddr_storage	*const getsas = (struct sockaddr_storage *)(void *)ugetsas.reply.data;
	struct ngm_mkpeer mkpeer;
	struct sockaddr_storage peer_sas;
	struct sockaddr_storage sas;
	char hook[NG_HOOKSIZ];
	char namebuf[64];
	char buf[32], buf2[32];
	char hostname[MAXHOSTNAMELEN];
	ng_ID_t node_id;
	int csock = -1;
	int dsock = -1;
	struct ghash_walk walk;
	u_int32_t       cap;
	u_int16_t	win;

	pi->opened=1;
	
	if (pi->incoming == 1) {
		Log(LG_PHYS2, ("[%s] L2tpOpen() on incoming call", l->name));
		if (l->state==PHYS_STATE_READY) {
		    l->state = PHYS_STATE_UP;
		    if (pi->outcall) {
			pi->sync = 1;
			if (l->rep) {
			    uint32_t fr;
			    avps = ppp_l2tp_avp_list_create();
			    if (RepIsSync(l)) {
				fr = htonl(L2TP_FRAMING_SYNC);
			    } else {
				fr = htonl(L2TP_FRAMING_ASYNC);
				pi->sync = 0;
			    }
			    if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_FRAMING_TYPE,
	        	        &fr, sizeof(fr)) == -1) {
			    	    Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
				        l->name, strerror(errno)));
			    }
			} else {
			    avps = NULL;
			}
			Log(LG_PHYS, ("[%s] L2TP: Call #%u connected", l->name, 
			    ppp_l2tp_sess_get_serial(pi->sess)));
			ppp_l2tp_connected(pi->sess, avps);
			if (avps)
			    ppp_l2tp_avp_list_destroy(&avps);
		    }
		    L2tpHookUp(l);
		    PhysUp(l);
		}
		return;
	}

	/* Sanity check. */
	if (l->state != PHYS_STATE_DOWN) {
		Log(LG_PHYS, ("[%s] L2TP: allready active", l->name));
		return;
	};

	l->state = PHYS_STATE_CONNECTING;
	strlcpy(pi->callingnum, pi->conf.callingnum, sizeof(pi->callingnum));
	strlcpy(pi->callednum, pi->conf.callednum, sizeof(pi->callednum));

	ghash_walk_init(gL2tpTuns, &walk);
	while ((tun = ghash_walk_next(gL2tpTuns, &walk)) != NULL) {
	    if (tun->ctrl && tun->alive && tun->active_sessions < gL2TPtunlimit &&
		(IpAddrInRange(&pi->conf.peer_addr, &tun->peer_addr)) &&
		(u_addrempty(&pi->conf.self_addr) || u_addrempty(&tun->self_addr) ||
		    u_addrcompare(&pi->conf.self_addr, &tun->self_addr) == 0) &&
		(pi->conf.peer_port == 0 || pi->conf.peer_port == tun->peer_port)) {
		    pi->tun = tun;
		    tun->active_sessions++;
		    if (tun->connected) { /* if tun is connected then just initiate */
		    
			/* Create number AVPs */
			avps = ppp_l2tp_avp_list_create();
			if (pi->conf.callingnum[0]) {
			  if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_CALLING_NUMBER,
	        	    pi->conf.callingnum, strlen(pi->conf.callingnum)) == -1) {
				Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
				    l->name, strerror(errno)));
			  }
			}
			if (pi->conf.callednum[0]) {
			  if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_CALLED_NUMBER,
	        	    pi->conf.callednum, strlen(pi->conf.callednum)) == -1) {
				Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
				    l->name, strerror(errno)));
			  }
			}
			if ((sess = ppp_l2tp_initiate(tun->ctrl, 
				Enabled(&pi->conf.options, L2TP_CONF_OUTCALL)?1:0,
				Enabled(&pi->conf.options, L2TP_CONF_LENGTH)?1:0,
				Enabled(&pi->conf.options, L2TP_CONF_DATASEQ)?1:0,
				avps)) == NULL) {
			    Log(LG_ERR, ("[%s] ppp_l2tp_initiate: %s", 
				l->name, strerror(errno)));
			    ppp_l2tp_avp_list_destroy(&avps);
			    pi->sess = NULL;
			    pi->tun = NULL;
			    tun->active_sessions--;
			    l->state = PHYS_STATE_DOWN;
			    PhysDown(l, STR_ERROR, NULL);
			    return;
			};
			ppp_l2tp_avp_list_destroy(&avps);
			pi->sess = sess;
			pi->outcall = Enabled(&pi->conf.options, L2TP_CONF_OUTCALL);
			Log(LG_PHYS, ("[%s] L2TP: %s call #%u via control connection %p initiated", 
			    l->name, (pi->outcall?"Outgoing":"Incoming"), 
			    ppp_l2tp_sess_get_serial(sess), tun->ctrl));
			ppp_l2tp_sess_set_cookie(sess, l);
			if (!pi->outcall) {
			    pi->sync = 1;
			    if (l->rep) {
				uint32_t fr;
				avps = ppp_l2tp_avp_list_create();
				if (RepIsSync(l)) {
				    fr = htonl(L2TP_FRAMING_SYNC);
				} else {
				    fr = htonl(L2TP_FRAMING_ASYNC);
				    pi->sync = 0;
				}
				if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_FRAMING_TYPE,
	        		    &fr, sizeof(fr)) == -1) {
				        Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
				    	    l->name, strerror(errno)));
				}
			    } else {
				avps = NULL;
			    }
			    ppp_l2tp_connected(pi->sess, avps);
			    if (avps)
				ppp_l2tp_avp_list_destroy(&avps);
			}
		    } /* Else wait while it will be connected */
		    return;
	    }
	}

	/* There is no tun which we need. Create a new one. */
	tun = Malloc(MB_PHYS, sizeof(*tun));
	sockaddrtou_addr(&peer_sas,&tun->peer_addr,&tun->peer_port);
	u_addrcopy(&pi->conf.peer_addr.addr, &tun->peer_addr);
	tun->peer_port = pi->conf.peer_port?pi->conf.peer_port:L2TP_PORT;
	u_addrcopy(&pi->conf.self_addr, &tun->self_addr);
	tun->self_port = pi->conf.self_port;
	tun->alive = 1;
	tun->connected = 0;

	/* Create vendor name AVP */
	avps = ppp_l2tp_avp_list_create();

	if (pi->conf.hostname[0] != 0) {
	    strlcpy(hostname, pi->conf.hostname, sizeof(hostname));
	} else {
	    (void)gethostname(hostname, sizeof(hostname) - 1);
	    hostname[sizeof(hostname) - 1] = '\0';
	}
	cap = htonl(L2TP_BEARER_DIGITAL|L2TP_BEARER_ANALOG);
	win = htons(8); /* XXX: this value is empirical. */
	if ((ppp_l2tp_avp_list_append(avps, 1, 0, AVP_HOST_NAME,
	      hostname, strlen(hostname)) == -1) ||
	    (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_VENDOR_NAME,
	      MPD_VENDOR, strlen(MPD_VENDOR)) == -1) ||
	    (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_BEARER_CAPABILITIES,
	      &cap, sizeof(cap)) == -1) ||
	    (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_RECEIVE_WINDOW_SIZE,
	      &win, sizeof(win)) == -1)) {
		Log(LG_ERR, ("L2TP: ppp_l2tp_avp_list_append: %s", strerror(errno)));
		goto fail;
	}

	/* Create a new control connection */
	if ((tun->ctrl = ppp_l2tp_ctrl_create(gPeventCtx, &gGiantMutex,
	    &ppp_l2tp_server_ctrl_cb, u_addrtoid(&tun->peer_addr),
	    &node_id, hook, avps, 
	    pi->conf.secret, strlen(pi->conf.secret),
	    Enabled(&pi->conf.options, L2TP_CONF_HIDDEN))) == NULL) {
		Log(LG_ERR, ("[%s] ppp_l2tp_ctrl_create: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}
	ppp_l2tp_ctrl_set_cookie(tun->ctrl, tun);

	Log(LG_PHYS, ("L2TP: Initiating control connection %p %s %u <-> %s %u",
	    tun->ctrl, u_addrtoa(&tun->self_addr,buf,sizeof(buf)), tun->self_port,
	    u_addrtoa(&tun->peer_addr,buf2,sizeof(buf2)), tun->peer_port));

	/* Get a temporary netgraph socket node */
	if (NgMkSockNode(NULL, &csock, &dsock) == -1) {
		Log(LG_ERR, ("[%s] NgMkSockNode: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}

	/* Attach a new UDP socket to "lower" hook */
	snprintf(namebuf, sizeof(namebuf), "[%lx]:", (u_long)node_id);
	memset(&mkpeer, 0, sizeof(mkpeer));
	strlcpy(mkpeer.type, NG_KSOCKET_NODE_TYPE, sizeof(mkpeer.type));
	strlcpy(mkpeer.ourhook, hook, sizeof(mkpeer.ourhook));
	if (tun->peer_addr.family==AF_INET6) {
		snprintf(mkpeer.peerhook, sizeof(mkpeer.peerhook), "%d/%d/%d", PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	} else {
	        snprintf(mkpeer.peerhook, sizeof(mkpeer.peerhook), "inet/dgram/udp");
	}
	if (NgSendMsg(csock, namebuf, NGM_GENERIC_COOKIE,
	    NGM_MKPEER, &mkpeer, sizeof(mkpeer)) == -1) {
		Log(LG_ERR, ("[%s] mkpeer: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}

	/* Point name at ksocket node */
	strlcat(namebuf, hook, sizeof(namebuf));

	/* Make UDP port reusable */
	memset(&sockopt_buf, 0, sizeof(sockopt_buf));
	sockopt->level = SOL_SOCKET;
	sockopt->name = SO_REUSEADDR;
	memcpy(sockopt->value, &one, sizeof(int));
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	    NGM_KSOCKET_SETOPT, sockopt, sizeof(sockopt_buf)) == -1) {
		Log(LG_ERR, ("[%s] setsockopt: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}
	sockopt->name = SO_REUSEPORT;
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	    NGM_KSOCKET_SETOPT, sockopt, sizeof(sockopt_buf)) == -1) {
		Log(LG_ERR, ("[%s] setsockopt: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}

	if (!u_addrempty(&tun->self_addr)) {
	    /* Bind socket to a new port */
	    u_addrtosockaddr(&tun->self_addr,tun->self_port,&sas);
	    if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
		NGM_KSOCKET_BIND, &sas, sas.ss_len) == -1) {
		    Log(LG_ERR, ("[%s] bind: %s", 
			l->name, strerror(errno)));
		    goto fail;
	    }
	}
	/* Connect socket to remote peer's IP and port */
	u_addrtosockaddr(&tun->peer_addr,tun->peer_port,&sas);
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	      NGM_KSOCKET_CONNECT, &sas, sas.ss_len) == -1
	    && errno != EINPROGRESS) {
		Log(LG_ERR, ("[%s] connect: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}

	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	    NGM_KSOCKET_GETNAME, NULL, 0) == -1) {
		Log(LG_ERR, ("[%s] getname send: %s", 
		    l->name, strerror(errno)));
	} else 
	if (NgRecvMsg(csock, &ugetsas.reply, sizeof(ugetsas), NULL) == -1) {
		Log(LG_ERR, ("[%s] getname recv: %s", 
		    l->name, strerror(errno)));
	} else {
	    sockaddrtou_addr(getsas,&tun->self_addr,&tun->self_port);
	}

	/* Add peer to our hash table */
	if (ghash_put(gL2tpTuns, tun) == -1) {
		Log(LG_ERR, ("[%s] ghash_put: %s", 
		    l->name, strerror(errno)));
		goto fail;
	}
	pi->tun = tun;
	tun->active_sessions++;
	Log(LG_PHYS2, ("L2TP: Control connection %p %s %u <-> %s %u initiated",
	    tun->ctrl, u_addrtoa(&tun->self_addr,buf,sizeof(buf)), tun->self_port,
	    u_addrtoa(&tun->peer_addr,buf2,sizeof(buf2)), tun->peer_port));
	ppp_l2tp_ctrl_initiate(tun->ctrl);

	/* Clean up and return */
	ppp_l2tp_avp_list_destroy(&avps);
	(void)close(csock);
	(void)close(dsock);
	return;

fail:
	/* Clean up after failure */
	if (csock != -1)
		(void)close(csock);
	if (dsock != -1)
		(void)close(dsock);
	if (tun != NULL) {
		ppp_l2tp_ctrl_destroy(&tun->ctrl);
		Freee(tun);
	}
	l->state = PHYS_STATE_DOWN;
	PhysDown(l, STR_ERROR, NULL);
}

/*
 * L2tpClose()
 */

static void
L2tpClose(Link l)
{
    L2tpInfo      const pi = (L2tpInfo) l->info;

    pi->opened = 0;
    pi->incoming = 0;
    pi->outcall = 0;
    if (l->state == PHYS_STATE_DOWN)
    	return;
    L2tpUnhook(l);
    if (pi->sess) {
	Log(LG_PHYS, ("[%s] L2TP: Call #%u terminated locally", l->name, 
	    ppp_l2tp_sess_get_serial(pi->sess)));
	ppp_l2tp_terminate(pi->sess, L2TP_RESULT_ADMIN, 0, NULL);
	pi->sess = NULL;
    }
    if (pi->tun)
	pi->tun->active_sessions--;
    pi->tun = NULL;
    pi->callingnum[0]=0;
    pi->callednum[0]=0;
    l->state = PHYS_STATE_DOWN;
    PhysDown(l, STR_MANUALLY, NULL);
}

/*
 * L2tpShutdown()
 */

static void
L2tpShutdown(Link l)
{
    L2tpUnListen(l);
    Freee(l->info);
}

/*
 * L2tpUnhook()
 */

static void
L2tpUnhook(Link l)
{
    int		csock = -1;
    L2tpInfo	const pi = (L2tpInfo) l->info;
    const char	*hook;
    ng_ID_t	node_id;
    char	path[NG_PATHSIZ];
	
    if (pi->sess) {		/* avoid double close */

	/* Get this link's node and hook */
	ppp_l2tp_sess_get_hook(pi->sess, &node_id, &hook);
	
	if (node_id != 0) {

	    /* Get a temporary netgraph socket node */
	    if (NgMkSockNode(NULL, &csock, NULL) == -1) {
		Log(LG_ERR, ("L2TP: NgMkSockNode: %s", strerror(errno)));
		return;
	    }
	
	    /* Disconnect session hook. */
	    snprintf(path, sizeof(path), "[%lx]:", (u_long)node_id);
	    NgFuncDisconnect(csock, l->name, path, hook);
	
	    close(csock);
	}
    }
}

/*
 * L2tpOriginated()
 */

static int
L2tpOriginated(Link l)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    return(l2tp->incoming ? LINK_ORIGINATE_REMOTE : LINK_ORIGINATE_LOCAL);
}

/*
 * L2tpIsSync()
 */

static int
L2tpIsSync(Link l)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    return (l2tp->sync);
}

static int
L2tpSetAccm(Link l, u_int32_t xmit, u_int32_t recv)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;
    
    if (!l2tp->sess)
	    return (-1);

    return (ppp_l2tp_set_link_info(l2tp->sess, xmit, recv));
}

static int
L2tpSetCallingNum(Link l, void *buf)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    strlcpy(l2tp->conf.callingnum, buf, sizeof(l2tp->conf.callingnum));
    return(0);
}

static int
L2tpSetCalledNum(Link l, void *buf)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    strlcpy(l2tp->conf.callednum, buf, sizeof(l2tp->conf.callednum));
    return(0);
}

static int
L2tpSelfName(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun && l2tp->tun->ctrl)
	return (ppp_l2tp_ctrl_get_self_name(l2tp->tun->ctrl, buf, buf_len));
    ((char*)buf)[0]=0;
    return (0);
}

static int
L2tpPeerName(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun && l2tp->tun->ctrl)
	return (ppp_l2tp_ctrl_get_peer_name(l2tp->tun->ctrl, buf, buf_len));
    ((char*)buf)[0]=0;
    return (0);
}

static int
L2tpSelfAddr(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun && !u_addrempty(&l2tp->tun->self_addr)) {
	if (u_addrtoa(&l2tp->tun->self_addr, buf, buf_len))
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
L2tpPeerAddr(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun) {
	if (u_addrtoa(&l2tp->tun->peer_addr, buf, buf_len))
	    return(0);
	else {
	    ((char*)buf)[0]=0;
	    return(-1);
	}
    }
    ((char*)buf)[0]=0;
    return(0);
}

static int
L2tpPeerPort(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun) {
	if (snprintf(buf, buf_len, "%d", l2tp->tun->peer_port))
	    return(0);
	else {
	    ((char*)buf)[0]=0;
	    return(-1);
	}
    }
    ((char*)buf)[0]=0;
    return(0);
}

static int
L2tpPeerMacAddr(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun && l2tp->tun->peer_iface[0]) {
	snprintf(buf, buf_len, "%02x:%02x:%02x:%02x:%02x:%02x",
	    l2tp->tun->peer_mac_addr[0], l2tp->tun->peer_mac_addr[1],
	    l2tp->tun->peer_mac_addr[2], l2tp->tun->peer_mac_addr[3],
	    l2tp->tun->peer_mac_addr[4], l2tp->tun->peer_mac_addr[5]);
	return (0);
    }
    ((char*)buf)[0]=0;
    return(0);
}

static int
L2tpPeerIface(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    if (l2tp->tun && l2tp->tun->peer_iface[0]) {
	strlcpy(buf, l2tp->tun->peer_iface, buf_len);
	return (0);
    }
    ((char*)buf)[0]=0;
    return(0);
}

static int
L2tpCallingNum(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    strlcpy((char*)buf, l2tp->callingnum, buf_len);
    return(0);
}

static int
L2tpCalledNum(Link l, void *buf, size_t buf_len)
{
    L2tpInfo	const l2tp = (L2tpInfo) l->info;

    strlcpy((char*)buf, l2tp->callednum, buf_len);
    return(0);
}

/*
 * L2tpStat()
 */

void
L2tpStat(Context ctx)
{
    L2tpInfo	const l2tp = (L2tpInfo) ctx->lnk->info;
    char	buf[32];

    Printf("L2TP configuration:\r\n");
    Printf("\tSelf addr    : %s, port %u",
	u_addrtoa(&l2tp->conf.self_addr, buf, sizeof(buf)), l2tp->conf.self_port);
    Printf("\r\n");
    Printf("\tPeer range   : %s",
	u_rangetoa(&l2tp->conf.peer_addr, buf, sizeof(buf)));
    if (l2tp->conf.peer_port)
	Printf(", port %u", l2tp->conf.peer_port);
    Printf("\r\n");
    Printf("\tHostname     : %s\r\n", l2tp->conf.hostname);
    Printf("\tSecret       : %s\r\n", (l2tp->conf.callingnum[0])?"******":"");
    Printf("\tCalling number: %s\r\n", l2tp->conf.callingnum);
    Printf("\tCalled number: %s\r\n", l2tp->conf.callednum);
    Printf("L2TP options:\r\n");
    OptStat(ctx, &l2tp->conf.options, gConfList);
    Printf("L2TP status:\r\n");
    if (ctx->lnk->state != PHYS_STATE_DOWN) {
	Printf("\tIncoming     : %s\r\n", (l2tp->incoming?"YES":"NO"));
	if (l2tp->tun) {
	    Printf("\tCurrent self : %s, port %u",
		u_addrtoa(&l2tp->tun->self_addr, buf, sizeof(buf)), l2tp->tun->self_port);
	    L2tpSelfName(ctx->lnk, buf, sizeof(buf));
	    Printf(" (%s)\r\n", buf);
	    Printf("\tCurrent peer : %s, port %u",
		u_addrtoa(&l2tp->tun->peer_addr, buf, sizeof(buf)), l2tp->tun->peer_port);
	    L2tpPeerName(ctx->lnk, buf, sizeof(buf));
	    Printf(" (%s)\r\n", buf);
	    if (l2tp->tun->peer_iface[0]) {
		Printf("\tCurrent peer : %02x:%02x:%02x:%02x:%02x:%02x at %s\r\n",
		    l2tp->tun->peer_mac_addr[0], l2tp->tun->peer_mac_addr[1],
		    l2tp->tun->peer_mac_addr[2], l2tp->tun->peer_mac_addr[3],
		    l2tp->tun->peer_mac_addr[4], l2tp->tun->peer_mac_addr[5],
		    l2tp->tun->peer_iface);
	    }

	    Printf("\tFraming      : %s\r\n", (l2tp->sync?"Sync":"Async"));
	}
	Printf("\tCalling number: %s\r\n", l2tp->callingnum);
	Printf("\tCalled number: %s\r\n", l2tp->callednum);
    }
}

/*
 * This is called when a control connection gets opened.
 */
static void
ppp_l2tp_ctrl_connected_cb(struct ppp_l2tp_ctrl *ctrl)
{
	struct l2tp_tun *tun = ppp_l2tp_ctrl_get_cookie(ctrl);
	struct ppp_l2tp_sess *sess;
	struct ppp_l2tp_avp_list *avps = NULL;
	struct sockaddr_dl  hwa;
	char	buf[32], buf2[32];
	int	k;

	Log(LG_PHYS, ("L2TP: Control connection %p %s %u <-> %s %u connected",
	    ctrl, u_addrtoa(&tun->self_addr,buf,sizeof(buf)), tun->self_port,
	    u_addrtoa(&tun->peer_addr,buf2,sizeof(buf2)), tun->peer_port));
	
	if (GetPeerEther(&tun->peer_addr, &hwa)) {
	    if_indextoname(hwa.sdl_index, tun->peer_iface);
	    memcpy(tun->peer_mac_addr, LLADDR(&hwa), sizeof(tun->peer_mac_addr));
	};

	/* Examine all L2TP links. */
	for (k = 0; k < gNumLinks; k++) {
		Link l;
	        L2tpInfo pi;

		if (!gLinks[k] || gLinks[k]->type != &gL2tpPhysType)
			continue;

		l = gLinks[k];
		pi = (L2tpInfo)l->info;

		if (pi->tun != tun)
			continue;

		tun->connected = 1;
		/* Create number AVPs */
		avps = ppp_l2tp_avp_list_create();
		if (pi->conf.callingnum[0]) {
		   if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_CALLING_NUMBER,
	            pi->conf.callingnum, strlen(pi->conf.callingnum)) == -1) {
			Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
			    l->name, strerror(errno)));
		   }
		}
		if (pi->conf.callednum[0]) {
		   if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_CALLED_NUMBER,
	            pi->conf.callednum, strlen(pi->conf.callednum)) == -1) {
			Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
			    l->name, strerror(errno)));
		   }
		}
		if ((sess = ppp_l2tp_initiate(tun->ctrl,
			    Enabled(&pi->conf.options, L2TP_CONF_OUTCALL)?1:0, 
			    Enabled(&pi->conf.options, L2TP_CONF_LENGTH)?1:0,
			    Enabled(&pi->conf.options, L2TP_CONF_DATASEQ)?1:0,
			    avps)) == NULL) {
			Log(LG_ERR, ("ppp_l2tp_initiate: %s", strerror(errno)));
			pi->sess = NULL;
			pi->tun = NULL;
			tun->active_sessions--;
			l->state = PHYS_STATE_DOWN;
			PhysDown(l, STR_ERROR, NULL);
			continue;
		};
		ppp_l2tp_avp_list_destroy(&avps);
		pi->sess = sess;
		pi->outcall = Enabled(&pi->conf.options, L2TP_CONF_OUTCALL);
		Log(LG_PHYS, ("[%s] L2TP: %s call #%u via control connection %p initiated", 
		    l->name, (pi->outcall?"Outgoing":"Incoming"), 
		    ppp_l2tp_sess_get_serial(sess), tun->ctrl));
		ppp_l2tp_sess_set_cookie(sess, l);
		if (!pi->outcall) {
		    pi->sync = 1;
		    if (l->rep) {
			uint32_t fr;
			avps = ppp_l2tp_avp_list_create();
			if (RepIsSync(l)) {
			    fr = htonl(L2TP_FRAMING_SYNC);
			} else {
			    fr = htonl(L2TP_FRAMING_ASYNC);
			    pi->sync = 0;
			}
			if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_FRAMING_TYPE,
	        	    &fr, sizeof(fr)) == -1) {
			        Log(LG_ERR, ("[%s] ppp_l2tp_avp_list_append: %s", 
			    	    l->name, strerror(errno)));
			}
		    } else {
			avps = NULL;
		    }
		    ppp_l2tp_connected(pi->sess, avps);
		    if (avps)
			ppp_l2tp_avp_list_destroy(&avps);
		}
	};
}

/*
 * This is called when a control connection is terminated for any reason
 * other than a call ppp_l2tp_ctrl_destroy().
 */
static void
ppp_l2tp_ctrl_terminated_cb(struct ppp_l2tp_ctrl *ctrl,
	u_int16_t result, u_int16_t error, const char *errmsg)
{
	struct l2tp_tun *tun = ppp_l2tp_ctrl_get_cookie(ctrl);
	int	k;

	Log(LG_PHYS, ("L2TP: Control connection %p terminated: %d (%s)", 
	    ctrl, error, errmsg));

	/* Examine all L2TP links. */
	for (k = 0; k < gNumLinks; k++) {
		Link l;
	        L2tpInfo pi;

		if (!gLinks[k] || gLinks[k]->type != &gL2tpPhysType)
			continue;

		l = gLinks[k];
		pi = (L2tpInfo)l->info;

		if (pi->tun != tun)
			continue;

		l->state = PHYS_STATE_DOWN;
		L2tpUnhook(l);
		pi->sess = NULL;
		pi->tun = NULL;
		tun->active_sessions--;
		pi->callingnum[0]=0;
	        pi->callednum[0]=0;
		PhysDown(l, STR_DROPPED, NULL);
	};
	
	tun->alive = 0;
}

/*
 * This is called before control connection is destroyed for any reason
 * other than a call ppp_l2tp_ctrl_destroy().
 */
static void
ppp_l2tp_ctrl_destroyed_cb(struct ppp_l2tp_ctrl *ctrl)
{
	struct l2tp_tun *tun = ppp_l2tp_ctrl_get_cookie(ctrl);

	Log(LG_PHYS, ("L2TP: Control connection %p destroyed", ctrl));

	ghash_remove(gL2tpTuns, tun);
	Freee(tun);
}

/*
 * This callback is used to report the peer's initiating a new incoming
 * or outgoing call.
 */
static void
ppp_l2tp_initiated_cb(struct ppp_l2tp_ctrl *ctrl,
	struct ppp_l2tp_sess *sess, int out,
	const struct ppp_l2tp_avp_list *avps,
	u_char *include_length, u_char *enable_dseq)
{
	struct	l2tp_tun *const tun = ppp_l2tp_ctrl_get_cookie(ctrl);
	struct	ppp_l2tp_avp_ptrs *ptrs = NULL;
	Link 	l = NULL;
	L2tpInfo pi = NULL;
	int	k;

	/* Convert AVP's to friendly form */
	if ((ptrs = ppp_l2tp_avp_list2ptrs(avps)) == NULL) {
		Log(LG_ERR, ("L2TP: error decoding AVP list: %s", strerror(errno)));
		ppp_l2tp_terminate(sess, L2TP_RESULT_ERROR,
		    L2TP_ERROR_GENERIC, strerror(errno));
		return;
	}

	Log(LG_PHYS, ("L2TP: %s call #%u via connection %p received", 
	    (out?"Outgoing":"Incoming"), 
	    ppp_l2tp_sess_get_serial(sess), ctrl));

	if (gShutdownInProgress) {
		Log(LG_PHYS, ("Shutdown sequence in progress, ignoring request."));
		goto failed;
	}

	if (OVERLOAD()) {
		Log(LG_PHYS, ("Daemon overloaded, ignoring request."));
		goto failed;
	}

	/* Examine all L2TP links. */
	for (k = 0; k < gNumLinks; k++) {
		Link l2;
	        L2tpInfo pi2;

		if (!gLinks[k] || gLinks[k]->type != &gL2tpPhysType)
			continue;

		l2 = gLinks[k];
		pi2 = (L2tpInfo)l2->info;

		if ((!PhysIsBusy(l2)) &&
		    Enabled(&l2->conf.options, LINK_CONF_INCOMING) &&
		    ((u_addrempty(&pi2->conf.self_addr)) || (u_addrcompare(&pi2->conf.self_addr, &tun->self_addr) == 0)) &&
		    (pi2->conf.self_port == 0 || pi2->conf.self_port == tun->self_port) &&
		    (IpAddrInRange(&pi2->conf.peer_addr, &tun->peer_addr)) &&
		    (pi2->conf.peer_port == 0 || pi2->conf.peer_port == tun->peer_port)) {
			
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
    		pi = (L2tpInfo)l->info;
		Log(LG_PHYS, ("[%s] L2TP: %s call #%u via control connection %p accepted", 
		    l->name, (out?"Outgoing":"Incoming"), 
		    ppp_l2tp_sess_get_serial(sess), ctrl));

		if (out)
		    l->state = PHYS_STATE_READY;
		else
		    l->state = PHYS_STATE_CONNECTING;
		pi->incoming = 1;
		pi->outcall = out;
		pi->tun = tun;
		tun->active_sessions++;
		pi->sess = sess;
		if (ptrs->callingnum && ptrs->callingnum->number)
		    strlcpy(pi->callingnum, ptrs->callingnum->number, sizeof(pi->callingnum));
		if (ptrs->callednum && ptrs->callednum->number)
		    strlcpy(pi->callednum, ptrs->callednum->number, sizeof(pi->callednum));
		    
		*include_length = (Enabled(&pi->conf.options, L2TP_CONF_LENGTH)?1:0);
		*enable_dseq = (Enabled(&pi->conf.options, L2TP_CONF_DATASEQ)?1:0);

		PhysIncoming(l);

		ppp_l2tp_sess_set_cookie(sess, l);
		ppp_l2tp_avp_ptrs_destroy(&ptrs);
		return;
	}
	Log(LG_PHYS, ("L2TP: No free link with requested parameters "
	    "was found"));
failed:
	ppp_l2tp_terminate(sess, L2TP_RESULT_AVAIL_TEMP, 0, NULL);
	ppp_l2tp_avp_ptrs_destroy(&ptrs);
}

/*
 * This callback is used to report successful connection of a remotely
 * initiated incoming call (see ppp_l2tp_initiated_t) or a locally initiated
 * outgoing call (see ppp_l2tp_initiate()).
 */
static void
ppp_l2tp_connected_cb(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps)
{
	Link l;
	L2tpInfo pi;
	struct ppp_l2tp_avp_ptrs *ptrs = NULL;

	l = ppp_l2tp_sess_get_cookie(sess);
	pi = (L2tpInfo)l->info;

	Log(LG_PHYS, ("[%s] L2TP: Call #%u connected", l->name, 
	    ppp_l2tp_sess_get_serial(sess)));

	if ((pi->incoming != pi->outcall) && avps != NULL) {
		/* Convert AVP's to friendly form */
		if ((ptrs = ppp_l2tp_avp_list2ptrs(avps)) == NULL) {
			Log(LG_ERR, ("L2TP: error decoding AVP list: %s", strerror(errno)));
		} else {
			if (ptrs->framing && ptrs->framing->sync) {
				pi->sync = 1;
			} else {
				pi->sync = 0;
			}
			ppp_l2tp_avp_ptrs_destroy(&ptrs);
		}
	}

	if (pi->opened) {
	    l->state = PHYS_STATE_UP;
	    L2tpHookUp(l);
	    PhysUp(l);
	} else {
	    l->state = PHYS_STATE_READY;
	}
}

/*
 * This callback is called when any call, whether successfully connected
 * or not, is terminated for any reason other than explict termination
 * from the link side (via a call to either ppp_l2tp_terminate() or
 * ppp_l2tp_ctrl_destroy()).
 */
static void
ppp_l2tp_terminated_cb(struct ppp_l2tp_sess *sess,
	u_int16_t result, u_int16_t error, const char *errmsg)
{
	char buf[128];
	Link l;
	L2tpInfo pi;

	l = ppp_l2tp_sess_get_cookie(sess);
	pi = (L2tpInfo) l->info;

	/* Control side is notifying us session is down */
	snprintf(buf, sizeof(buf), "result=%u error=%u errmsg=\"%s\"",
	    result, error, (errmsg != NULL) ? errmsg : "");
	Log(LG_PHYS, ("[%s] L2TP: call #%u terminated: %s", l->name, 
	    ppp_l2tp_sess_get_serial(sess), buf));

	l->state = PHYS_STATE_DOWN;
	L2tpUnhook(l);
	pi->sess = NULL;
	if (pi->tun)
	    pi->tun->active_sessions--;
	pi->tun = NULL;
	pi->callingnum[0]=0;
	pi->callednum[0]=0;
	PhysDown(l, STR_DROPPED, NULL);
}

/*
 * This callback called on receiving link info from peer.
 */
void
ppp_l2tp_set_link_info_cb(struct ppp_l2tp_sess *sess,
			u_int32_t xmit, u_int32_t recv)
{
	Link l = ppp_l2tp_sess_get_cookie(sess);

	if (l->rep != NULL) {
		RepSetAccm(l, xmit, recv);
	}
}

/*
 * Connect L2TP and link hooks.
 */
 
static void
L2tpHookUp(Link l)
{
	int		csock = -1;
	L2tpInfo	pi = (L2tpInfo)l->info;
        const char 	*hook;
        ng_ID_t		node_id;
	char		path[NG_PATHSIZ];
	struct ngm_connect      cn;

	/* Get a temporary netgraph socket node */
	if (NgMkSockNode(NULL, &csock, NULL) == -1) {
		Log(LG_ERR, ("L2TP: NgMkSockNode: %s", strerror(errno)));
		goto fail;
	}

	/* Get this link's node and hook */
	ppp_l2tp_sess_get_hook(pi->sess, &node_id, &hook);

	/* Connect our ng_ppp(4) node link hook and ng_l2tp(4) node. */
	if (!PhysGetUpperHook(l, cn.path, cn.peerhook)) {
	    Log(LG_PHYS, ("[%s] L2TP: can't get upper hook", l->name));
	    goto fail;
	}
	snprintf(path, sizeof(path), "[%lx]:", (u_long)node_id);
	strlcpy(cn.ourhook, hook, sizeof(cn.ourhook));
	if (NgSendMsg(csock, path, NGM_GENERIC_COOKIE, NGM_CONNECT, 
	    &cn, sizeof(cn)) < 0) {
		Log(LG_ERR, ("[%s] L2TP: can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
    		    l->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
		goto fail;
	}
	ppp_l2tp_sess_hooked(pi->sess);
	close(csock);
	return;

fail:
	/* Clean up after failure */
	ppp_l2tp_terminate(pi->sess, L2TP_RESULT_ERROR,
	    L2TP_ERROR_GENERIC, strerror(errno));
	if (csock != -1)
		(void)close(csock);
}

/*
 * Read an incoming packet that might be a new L2TP connection.
 */
 
static void
L2tpServerEvent(int type, void *arg)
{
	struct l2tp_server *const s = arg;
	L2tpInfo pi = NULL;
	struct ppp_l2tp_avp_list *avps = NULL;
	struct l2tp_tun *tun = NULL;
	union {
	    u_char buf[sizeof(struct ng_ksocket_sockopt) + sizeof(int)];
	    struct ng_ksocket_sockopt sockopt;
	} sockopt_buf;
	struct ng_ksocket_sockopt *const sockopt = &sockopt_buf.sockopt;
	struct ngm_connect connect;
	struct ngm_rmhook rmhook;
	struct ngm_mkpeer mkpeer;
	struct sockaddr_storage peer_sas;
	struct sockaddr_storage sas;
	const size_t bufsize = 8192;
	u_int16_t *buf = NULL;
	char hook[NG_HOOKSIZ];
	char hostname[MAXHOSTNAMELEN];
	socklen_t sas_len;
	char namebuf[64];
	char buf1[32], buf2[32];
	ng_ID_t node_id;
	int csock = -1;
	int dsock = -1;
	int len;
	u_int32_t	cap;
	u_int16_t	win;
	int	k;

	/* Allocate buffer */
	buf = Malloc(MB_PHYS, bufsize);

	/* Read packet */
	sas_len = sizeof(peer_sas);
	if ((len = recvfrom(s->sock, buf, bufsize, 0,
	    (struct sockaddr *)&peer_sas, &sas_len)) == -1) {
		Log(LG_ERR, ("L2TP: recvfrom: %s", strerror(errno)));
		goto fail;
	}

	/* Drop it if it's not an initial L2TP packet */
	if (len < 12)
		goto fail;
	if ((ntohs(buf[0]) & 0xcb0f) != 0xc802 || ntohs(buf[1]) < 12
	    || buf[2] != 0 || buf[3] != 0 || buf[4] != 0 || buf[5] != 0)
		goto fail;

	/* Create a new tun */
	tun = Malloc(MB_PHYS, sizeof(*tun));
	sockaddrtou_addr(&peer_sas,&tun->peer_addr,&tun->peer_port);
	u_addrcopy(&s->self_addr, &tun->self_addr);
	tun->self_port = s->self_port;
	tun->alive = 1;

	Log(LG_PHYS, ("Incoming L2TP packet from %s %d", 
		u_addrtoa(&tun->peer_addr, namebuf, sizeof(namebuf)), tun->peer_port));

	/* Examine all L2TP links to get best possible fit tunnel parameters. */
	for (k = 0; k < gNumLinks; k++) {
		Link l2;
	        L2tpInfo pi2;

		if (!gLinks[k] || gLinks[k]->type != &gL2tpPhysType)
			continue;

		l2 = gLinks[k];
		pi2 = (L2tpInfo)l2->info;

		/* Simplified comparation as it is not a final one. */
		if ((!PhysIsBusy(l2)) &&
		    (pi2->server == s) &&
		    (IpAddrInRange(&pi2->conf.peer_addr, &tun->peer_addr)) &&
		    (pi2->conf.peer_port == 0 || pi2->conf.peer_port == tun->peer_port)) {
			
			if (pi == NULL || pi2->conf.peer_addr.width > pi->conf.peer_addr.width) {
				pi = pi2;
				if (u_rangehost(&pi->conf.peer_addr)) {
					break;	/* Nothing could be better */
				}
			}
		}
	}
	if (pi == NULL) {
		Log(LG_PHYS, ("L2TP: No link with requested parameters "
		    "was found"));
		goto fail;
	}

	/* Create vendor name AVP */
	avps = ppp_l2tp_avp_list_create();

	if (pi->conf.hostname[0] != 0) {
	    strlcpy(hostname, pi->conf.hostname, sizeof(hostname));
	} else {
	    (void)gethostname(hostname, sizeof(hostname) - 1);
	    hostname[sizeof(hostname) - 1] = '\0';
	}
	cap = htonl(L2TP_BEARER_DIGITAL|L2TP_BEARER_ANALOG);
	win = htons(8); /* XXX: this value is empirical. */
	if ((ppp_l2tp_avp_list_append(avps, 1, 0, AVP_HOST_NAME,
	      hostname, strlen(hostname)) == -1) ||
	    (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_VENDOR_NAME,
	      MPD_VENDOR, strlen(MPD_VENDOR)) == -1) ||
	    (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_BEARER_CAPABILITIES,
	      &cap, sizeof(cap)) == -1) ||
	    (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_RECEIVE_WINDOW_SIZE,
	      &win, sizeof(win)) == -1)) {
		Log(LG_ERR, ("L2TP: ppp_l2tp_avp_list_append: %s", strerror(errno)));
		goto fail;
	}

	/* Create a new control connection */
	if ((tun->ctrl = ppp_l2tp_ctrl_create(gPeventCtx, &gGiantMutex,
	    &ppp_l2tp_server_ctrl_cb, u_addrtoid(&tun->peer_addr),
	    &node_id, hook, avps, 
	    pi->conf.secret, strlen(pi->conf.secret),
	    Enabled(&pi->conf.options, L2TP_CONF_HIDDEN))) == NULL) {
		Log(LG_ERR, ("L2TP: ppp_l2tp_ctrl_create: %s", strerror(errno)));
		goto fail;
	}
	ppp_l2tp_ctrl_set_cookie(tun->ctrl, tun);

	/* Get a temporary netgraph socket node */
	if (NgMkSockNode(NULL, &csock, &dsock) == -1) {
		Log(LG_ERR, ("L2TP: NgMkSockNode: %s", strerror(errno)));
		goto fail;
	}

	/* Connect to l2tp netgraph node "lower" hook */
	snprintf(namebuf, sizeof(namebuf), "[%lx]:", (u_long)node_id);
	memset(&connect, 0, sizeof(connect));
	strlcpy(connect.path, namebuf, sizeof(connect.path));
	strlcpy(connect.ourhook, hook, sizeof(connect.ourhook));
	strlcpy(connect.peerhook, hook, sizeof(connect.peerhook));
	if (NgSendMsg(csock, ".:", NGM_GENERIC_COOKIE,
	    NGM_CONNECT, &connect, sizeof(connect)) == -1) {
		Log(LG_ERR, ("L2TP: %s: %s", "connect", strerror(errno)));
		goto fail;
	}

	/* Write the received packet to the node */
	if (NgSendData(dsock, hook, (u_char *)buf, len) == -1) {
		Log(LG_ERR, ("L2TP: %s: %s", "NgSendData", strerror(errno)));
		goto fail;
	}

	/* Disconnect from netgraph node "lower" hook */
	memset(&rmhook, 0, sizeof(rmhook));
	strlcpy(rmhook.ourhook, hook, sizeof(rmhook.ourhook));
	if (NgSendMsg(csock, ".:", NGM_GENERIC_COOKIE,
	    NGM_RMHOOK, &rmhook, sizeof(rmhook)) == -1) {
		Log(LG_ERR, ("L2TP: %s: %s", "rmhook", strerror(errno)));
		goto fail;
	}

	/* Attach a new UDP socket to "lower" hook */
	memset(&mkpeer, 0, sizeof(mkpeer));
	strlcpy(mkpeer.type, NG_KSOCKET_NODE_TYPE, sizeof(mkpeer.type));
	strlcpy(mkpeer.ourhook, hook, sizeof(mkpeer.ourhook));
	if (s->self_addr.family==AF_INET6) {
		snprintf(mkpeer.peerhook, sizeof(mkpeer.peerhook), "%d/%d/%d", PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	} else {
	        snprintf(mkpeer.peerhook, sizeof(mkpeer.peerhook), "inet/dgram/udp");
	}
	if (NgSendMsg(csock, namebuf, NGM_GENERIC_COOKIE,
	    NGM_MKPEER, &mkpeer, sizeof(mkpeer)) == -1) {
		Log(LG_ERR, ("L2TP: %s: %s", "mkpeer", strerror(errno)));
		goto fail;
	}

	/* Point name at ksocket node */
	strlcat(namebuf, hook, sizeof(namebuf));

	/* Make UDP port reusable */
	memset(&sockopt_buf, 0, sizeof(sockopt_buf));
	sockopt->level = SOL_SOCKET;
	sockopt->name = SO_REUSEADDR;
	memcpy(sockopt->value, &one, sizeof(int));
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	    NGM_KSOCKET_SETOPT, sockopt, sizeof(sockopt_buf)) == -1) {
		Log(LG_ERR, ("L2TP: setsockopt: %s", strerror(errno)));
		goto fail;
	}
	sockopt->name = SO_REUSEPORT;
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	    NGM_KSOCKET_SETOPT, sockopt, sizeof(sockopt_buf)) == -1) {
		Log(LG_ERR, ("L2TP: setsockopt: %s", strerror(errno)));
		goto fail;
	}

	/* Bind socket to a new port */
	u_addrtosockaddr(&s->self_addr,s->self_port,&sas);
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	    NGM_KSOCKET_BIND, &sas, sas.ss_len) == -1) {
		Log(LG_ERR, ("L2TP: bind: %s", strerror(errno)));
		goto fail;
	}

	/* Connect socket to remote peer's IP and port */
	if (NgSendMsg(csock, namebuf, NGM_KSOCKET_COOKIE,
	      NGM_KSOCKET_CONNECT, &peer_sas, peer_sas.ss_len) == -1
	    && errno != EINPROGRESS) {
		Log(LG_ERR, ("L2TP: connect: %s", strerror(errno)));
		goto fail;
	}

	/* Add peer to our hash table */
	if (ghash_put(gL2tpTuns, tun) == -1) {
		Log(LG_ERR, ("L2TP: %s: %s", "ghash_put", strerror(errno)));
		goto fail;
	}

	Log(LG_PHYS2, ("L2TP: Control connection %p %s %u <-> %s %u accepted",
	    tun->ctrl, u_addrtoa(&tun->self_addr,buf1,sizeof(buf1)), tun->self_port,
	    u_addrtoa(&tun->peer_addr,buf2,sizeof(buf2)), tun->peer_port));

	/* Clean up and return */
	ppp_l2tp_avp_list_destroy(&avps);
	(void)close(csock);
	(void)close(dsock);
	Freee(buf);
	return;

fail:
	/* Clean up after failure */
	if (csock != -1)
		(void)close(csock);
	if (dsock != -1)
		(void)close(dsock);
	if (tun != NULL) {
		ppp_l2tp_ctrl_destroy(&tun->ctrl);
		Freee(tun);
	}
	ppp_l2tp_avp_list_destroy(&avps);
	Freee(buf);
}


/*
 * L2tpListen()
 */

static int
L2tpListen(Link l)
{
	L2tpInfo	p = (L2tpInfo)l->info;
	struct l2tp_server *s;
	struct sockaddr_storage sa;
	char buf[48];
	struct ghash_walk walk;

	if (p->server)
	    return(1);

	ghash_walk_init(gL2tpServers, &walk);
	while ((s = ghash_walk_next(gL2tpServers, &walk)) != NULL) {
	    if ((u_addrcompare(&s->self_addr, &p->conf.self_addr) == 0) && 
		s->self_port == (p->conf.self_port?p->conf.self_port:L2TP_PORT)) {
		    s->refs++;
		    p->server = s;
		    return(1);
	    }
	}

	s = Malloc(MB_PHYS, sizeof(struct l2tp_server));
	s->refs = 1;
	u_addrcopy(&p->conf.self_addr, &s->self_addr);
	s->self_port = p->conf.self_port?p->conf.self_port:L2TP_PORT;
	
	/* Setup UDP socket that listens for new connections */
	if (s->self_addr.family==AF_INET6) {
		s->sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		s->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	if (s->sock == -1) {
		Log(LG_ERR, ("L2TP: socket: %s", strerror(errno)));
		goto fail;
	}
	if (setsockopt(s->sock, SOL_SOCKET,
	    SO_REUSEADDR, &one, sizeof(one)) == -1) {
		Log(LG_ERR, ("L2TP: setsockopt: %s", strerror(errno)));
		goto fail;
	}
	if (setsockopt(s->sock, SOL_SOCKET,
	    SO_REUSEPORT, &one, sizeof(one)) == -1) {
		Log(LG_ERR, ("L2TP: setsockopt: %s", strerror(errno)));
		goto fail;
	}
	u_addrtosockaddr(&s->self_addr, s->self_port, &sa);
	if (bind(s->sock, (struct sockaddr *)&sa, sa.ss_len) == -1) {
		Log(LG_ERR, ("L2TP: bind: %s", strerror(errno)));
		goto fail;
	}

	EventRegister(&s->event, EVENT_READ, s->sock,
	    EVENT_RECURRING, L2tpServerEvent, s);

	Log(LG_PHYS, ("L2TP: waiting for connection on %s %u",
	    u_addrtoa(&s->self_addr, buf, sizeof(buf)), s->self_port));
	
	p->server = s;
	ghash_put(gL2tpServers, s);
	return (1);
fail:
	if (s->sock)
	    close(s->sock);
	Freee(s);
	return (0);
}

/*
 * L2tpUnListen()
 */

static void
L2tpUnListen(Link l)
{
	L2tpInfo	p = (L2tpInfo)l->info;
	struct l2tp_server *s = p->server;
	char buf[48];

	if (!s)
	    return;

	s->refs--;
	if (s->refs == 0) {
	    Log(LG_PHYS, ("L2TP: stop waiting for connection on %s %u",
		u_addrtoa(&s->self_addr, buf, sizeof(buf)), s->self_port));
	
	    ghash_remove(gL2tpServers, s);
	    EventUnRegister(&s->event);
	    if (s->sock)
		close(s->sock);
	    Freee(s);
	    p->server = NULL;
	}
	return;
}

/*
 * L2tpNodeUpdate()
 */

static void
L2tpNodeUpdate(Link l)
{
    L2tpInfo const pi = (L2tpInfo) l->info;
    if (!pi->server) {
	if (Enabled(&l->conf.options, LINK_CONF_INCOMING))
	    L2tpListen(l);
    } else {
	if (!Enabled(&l->conf.options, LINK_CONF_INCOMING))
	    L2tpUnListen(l);
    }
}

/*
 * L2tpSetCommand()
 */

static int
L2tpSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    L2tpInfo		const l2tp = (L2tpInfo) ctx->lnk->info;
    struct u_range	rng;
    int			port;

    switch ((intptr_t)arg) {
	case SET_SELFADDR:
	case SET_PEERADDR:
    	    if (ac < 1 || ac > 2 || !ParseRange(av[0], &rng, ALLOW_IPV4|ALLOW_IPV6))
		return(-1);
    	    if (ac > 1) {
		if ((port = atoi(av[1])) < 0 || port > 0xffff)
		    return(-1);
    	    } else {
		port = 0;
    	    }
    	    if ((intptr_t)arg == SET_SELFADDR) {
		l2tp->conf.self_addr = rng.addr;
		l2tp->conf.self_port = port;
		if (l2tp->server) {
		    L2tpUnListen(ctx->lnk);
		    L2tpListen(ctx->lnk);
		}
    	    } else {
		l2tp->conf.peer_addr = rng;
		l2tp->conf.peer_port = port;
    	    }
    	    break;
	case SET_CALLINGNUM:
    	    if (ac != 1)
		return(-1);
    	    strlcpy(l2tp->conf.callingnum, av[0], sizeof(l2tp->conf.callingnum));
    	    break;
	case SET_CALLEDNUM:
    	    if (ac != 1)
		return(-1);
    	    strlcpy(l2tp->conf.callednum, av[0], sizeof(l2tp->conf.callednum));
    	    break;
	case SET_HOSTNAME:
    	    if (ac != 1)
		return(-1);
    	    strlcpy(l2tp->conf.hostname, av[0], sizeof(l2tp->conf.hostname));
    	    break;
	case SET_SECRET:
    	    if (ac != 1)
		return(-1);
    	    strlcpy(l2tp->conf.secret, av[0], sizeof(l2tp->conf.secret));
    	    break;
	case SET_ENABLE:
    	    EnableCommand(ac, av, &l2tp->conf.options, gConfList);
    	    break;
	case SET_DISABLE:
    	    DisableCommand(ac, av, &l2tp->conf.options, gConfList);
    	    break;
	default:
    	    assert(0);
    }
    return(0);
}

/*
 * L2tpsStat()
 */

int
L2tpsStat(Context ctx, int ac, char *av[], void *arg)
{
    struct l2tp_tun	*tun;
    struct ghash_walk	walk;
    char	buf1[64], buf2[64], buf3[64];

    Printf("Active L2TP tunnels:\r\n");
    ghash_walk_init(gL2tpTuns, &walk);
    while ((tun = ghash_walk_next(gL2tpTuns, &walk)) != NULL) {

	u_addrtoa(&tun->self_addr, buf1, sizeof(buf1));
	u_addrtoa(&tun->peer_addr, buf2, sizeof(buf2));
	ppp_l2tp_ctrl_stats(tun->ctrl, buf3, sizeof(buf3));
	Printf("%p\t %s %d <=> %s %d\t%s %d calls\r\n",
    	    tun->ctrl, buf1, tun->self_port, buf2, tun->peer_port,
	    buf3, tun->active_sessions);
    }

    return 0;
}
