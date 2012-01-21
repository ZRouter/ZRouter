
/*
 * modem.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include <termios.h>
#include "chat.h"
#include "phys.h"
#include "modem.h"
#include "ngfunc.h"
#include "lcp.h"
#include "event.h"
#include "util.h"
#include "log.h"

#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_async.h>
#include <netgraph/ng_tty.h>
#include <netgraph.h>

/*
 * DEFINITIONS
 */

#ifndef NETGRAPHDISC
  #define NETGRAPHDISC			7	/* XXX */
#endif

  #define MODEM_MTU			1600
  #define MODEM_MRU			1600

  #define MODEM_MIN_CLOSE_TIME		3
  #define MODEM_CHECK_INTERVAL		1
  #define MODEM_DEFAULT_SPEED		115200
  #define MODEM_ERR_REPORT_INTERVAL	60

  #define MODEM_IDLE_RESULT_ANSWER	"answer"
  #define MODEM_IDLE_RESULT_RINGBACK	"ringback"

  /* Special chat script variables we set/use */
  #define CHAT_VAR_LOGIN		"$Login"
  #define CHAT_VAR_PASSWORD		"$Password"
  #define CHAT_VAR_DEVICE		"$modemDevice"
  #define CHAT_VAR_IDLE_RESULT		"$IdleResult"
  #define CHAT_VAR_CONNECT_SPEED	"$ConnectionSpeed"
  #define CHAT_VAR_CALLING		"$CallingID"
  #define CHAT_VAR_CALLED		"$CalledID"

  /* Nominal link parameters */
  #define MODEM_DEFAULT_BANDWIDTH	28800	/* ~33.6 modem */
  #define MODEM_DEFAULT_LATENCY		10000	/* 10ms */

  /* Modem device state */
  struct modeminfo {
    int			fd;			/* Device file desc, or -1 */
    int			csock;			/* netgraph control socket */
    int			speed;			/* Port speed */
    u_int		watch;			/* Signals to watch */
    char		device[20];		/* Serial device name */
    char		ttynode[NG_NODESIZ];	/* TTY node name */
    char		connScript[CHAT_MAX_LABEL];	/* Connect script */
    char		idleScript[CHAT_MAX_LABEL];	/* Idle script */
    struct pppTimer	checkTimer;		/* Timer to check pins */
    struct pppTimer	reportTimer;		/* Timer to report errs */
    struct pppTimer	startTimer;		/* Timer for ModemStart() */
    struct optinfo	options;		/* Binary options */
    struct ng_async_cfg	acfg;			/* ng_async node config */
    ChatInfo		chat;			/* Chat script state */
    time_t		lastClosed;		/* Last time device closed */
    u_char		opened:1;		/* We have been opened */
    u_char		originated:1;		/* We originated current call */
    u_char		answering:1;		/* $IdleResult was "answer" */
  };
  typedef struct modeminfo	*ModemInfo;

  /* Set menu options */
  enum {
    SET_DEVICE,
    SET_SPEED,
    SET_CSCRIPT,
    SET_ISCRIPT,
    SET_SCRIPT_VAR,
    SET_WATCH
  };

/*
 * INTERNAL FUNCTIONS
 */

  static int		ModemInit(Link l);
  static void		ModemOpen(Link l);
  static void		ModemClose(Link l);
  static void		ModemUpdate(Link l);
  static int		ModemSetAccm(Link l, u_int32_t xmit, u_int32_t recv);
  static void		ModemStat(Context ctx);
  static int		ModemOriginated(Link l);
  static int		ModemIsSync(Link l);
  static int		ModemSelfAddr(Link l, void *buf, size_t buf_len);
  static int		ModemIface(Link l, void *buf, size_t buf_len);
  static int		ModemCallingNum(Link l, void *buf, size_t buf_len);
  static int		ModemCalledNum(Link l, void *buf, size_t buf_len);

  static void		ModemStart(void *arg);
  static void		ModemDoClose(Link l, int opened);

  /* Chat callbacks */
  static int		ModemChatSetBaudrate(void *arg, int baud);
  static void		ModemChatConnectResult(void *arg,
				int rslt, const char *msg);
  static void		ModemChatIdleResult(void *arg, int rslt,
				const char *msg);

  static int		ModemSetCommand(Context ctx, int ac, char *av[], void *arg);
  static int		ModemInstallNodes(Link l);
  static int		ModemGetNgStats(Link l, struct ng_async_stat *sp);

  static void		ModemCheck(void *arg);
  static void		ModemErrorCheck(void *arg);

/*
 * GLOBAL VARIABLES
 */

  const struct phystype gModemPhysType = {
    .name		= "modem",
    .descr		= "Serial port modem",
    .mtu		= MODEM_MTU,
    .mru		= MODEM_MRU,
    .tmpl		= 0,
    .init		= ModemInit,
    .open		= ModemOpen,
    .close		= ModemClose,
    .update		= ModemUpdate,
    .showstat		= ModemStat,
    .originate		= ModemOriginated,
    .issync		= ModemIsSync,
    .setaccm 		= ModemSetAccm,
    .selfaddr		= ModemSelfAddr,
    .peeraddr		= ModemSelfAddr,
    .peeriface		= ModemIface,
    .callingnum		= ModemCallingNum,
    .callednum		= ModemCalledNum,
  };

  const struct cmdtab ModemSetCmds[] = {
    { "device {name}",			"Set modem device",
      ModemSetCommand, NULL, 2, (void *) SET_DEVICE },
    { "speed {port-speed}",		"Set modem speed",
      ModemSetCommand, NULL, 2, (void *) SET_SPEED },
    { "script [{label}]",		"Set connect script",
      ModemSetCommand, NULL, 2, (void *) SET_CSCRIPT },
    { "idle-script [{label}]",		"Set idle script",
      ModemSetCommand, NULL, 2, (void *) SET_ISCRIPT },
    { "var ${var} {string}",		"Set script variable",
      ModemSetCommand, NULL, 2, (void *) SET_SCRIPT_VAR },
    { "watch [+|-cd] [+|-dsr]", 	"Set signals to monitor",
      ModemSetCommand, NULL, 2, (void *) SET_WATCH },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static int	gSpeedList[] = {
    50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 
    38400, 7200, 14400, 28800, 57600, 76800, 115200, 230400, 460800, 614400,
    921600, 1228800, 1843200, 2000000, 3000000, -1
  };

/*
 * ModemInit()
 *
 * Allocate and initialize device private info
 */

static int
ModemInit(Link l)
{
    char	defSpeed[32];
    ModemInfo	m;

    m = (ModemInfo) (l->info = Malloc(MB_PHYS, sizeof(*m)));
    m->watch = TIOCM_CAR;
    m->chat = ChatInit(l, ModemChatSetBaudrate);
    m->fd = -1;
    m->opened = FALSE;

    /* Set nominal link speed and bandwith for a modem connection */
    l->latency = MODEM_DEFAULT_LATENCY;
    l->bandwidth = MODEM_DEFAULT_BANDWIDTH;

    /* Set default speed */
    m->speed = MODEM_DEFAULT_SPEED;
    snprintf(defSpeed, sizeof(defSpeed), "%d", m->speed);
    ChatPresetVar(m->chat, CHAT_VAR_BAUDRATE, defSpeed);
    return(0);
}

/*
 * ModemOpen()
 */

static void
ModemOpen(Link l)
{
    ModemInfo	const m = (ModemInfo) l->info;

    assert(!m->opened);
    m->opened = TRUE;
    if (m->fd >= 0) {			/* Device is already open.. */
	if (m->answering) {			/* We just answered a call */
    	    m->originated = FALSE;
    	    m->answering = FALSE;
    	    ModemChatConnectResult(l, TRUE, NULL);
	} else
    	    ModemDoClose(l, TRUE);		/* Stop idle script then dial back */
    } else
	ModemStart(l);			/* Open device and try to dial */
}

/*
 * ModemStart()
 */

static void
ModemStart(void *arg)
{
    Link		const l = (Link) arg;
    ModemInfo		const m = (ModemInfo) l->info;
    const time_t	now = time(NULL);
    char		password[AUTH_MAX_PASSWORD];
    FILE		*scriptfp;

    /* If we're idle, and there's no idle script, there's nothing to do */
    assert(!m->answering);
    TimerStop(&m->startTimer);
    if (!m->opened &&
      (!*m->idleScript || !Enabled(&l->conf.options, LINK_CONF_INCOMING) ||
       gShutdownInProgress))
	return;

    /* Avoid brief hang from kernel enforcing minimum DTR hold time */
    if (now - m->lastClosed < MODEM_MIN_CLOSE_TIME) {
	TimerInit(&m->startTimer, "ModemStart",
    	  (MODEM_MIN_CLOSE_TIME - (now - m->lastClosed)) * SECONDS, ModemStart, l);
	TimerStart(&m->startTimer);
	return;
    }

    /* Open and configure serial port */
    if ((m->fd = OpenSerialDevice(l->name, m->device, m->speed)) < 0)
        goto fail;

    /* If connecting, but no connect script, then skip chat altogether */
    if (m->opened && !*m->connScript) {
	ModemChatConnectResult(l, TRUE, NULL);
	return;
    }

    /* Open chat script file */
    if ((scriptfp = OpenConfFile(SCRIPT_FILE, NULL)) == NULL) {
	Log(LG_ERR, ("[%s] MODEM: can't open chat script file", l->name));
	ExclusiveCloseDevice(l->name, m->fd, m->device);
	m->fd = -1;
fail:
	m->opened = FALSE;
	m->lastClosed = time(NULL);
	l->state = PHYS_STATE_DOWN;
	PhysDown(l, STR_ERROR, STR_DEV_NOT_READY);
	return;
    }

    /* Preset some special chat variables */
    ChatPresetVar(m->chat, CHAT_VAR_DEVICE, m->device);
    ChatPresetVar(m->chat, CHAT_VAR_LOGIN, l->lcp.auth.conf.authname);
    if (l->lcp.auth.conf.password[0] != 0) {
	ChatPresetVar(m->chat, CHAT_VAR_PASSWORD, l->lcp.auth.conf.password);
    } else if (AuthGetData(l->lcp.auth.conf.authname,
        password, sizeof(password), NULL, NULL) >= 0) {
    	    ChatPresetVar(m->chat, CHAT_VAR_PASSWORD, password);
    }

    /* Run connect or idle script as appropriate */
    if (!m->opened) {
	ChatPresetVar(m->chat, CHAT_VAR_IDLE_RESULT, "<unknown>");
	ChatStart(m->chat, m->fd, scriptfp, m->idleScript, ModemChatIdleResult);
    } else {
	m->originated = TRUE;
	l->state = PHYS_STATE_CONNECTING;
	ChatStart(m->chat, m->fd, scriptfp, m->connScript, ModemChatConnectResult);
    }
}

/*
 * ModemClose()
 */

static void
ModemClose(Link l)
{
    ModemInfo	const m = (ModemInfo) l->info;

    if (!m->opened)
	return;
    ModemDoClose(l, FALSE);
    l->state = PHYS_STATE_DOWN;
    PhysDown(l, STR_MANUALLY, NULL);
}

static void
ModemUpdate(Link l)
{
    ModemInfo	const m = (ModemInfo) l->info;

    if (m->opened || TimerRemain(&m->startTimer) >= 0)
	return;		/* nothing needs to be done right now */
    if (m->fd >= 0 &&
      (!*m->idleScript || !Enabled(&l->conf.options, LINK_CONF_INCOMING)))
        ModemDoClose(l, FALSE);
    else if (m->fd < 0 &&
      (*m->idleScript && Enabled(&l->conf.options, LINK_CONF_INCOMING)))
        ModemStart(l);
}

/*
 * ModemDoClose()
 */

static void
ModemDoClose(Link l, int opened)
{
    ModemInfo	const m = (ModemInfo) l->info;
    const char	ch = ' ';

    /* Shutdown everything */
    assert(m->fd >= 0);
    ChatAbort(m->chat);
    TimerStop(&m->checkTimer);
    TimerStop(&m->startTimer);
    TimerStop(&m->reportTimer);
    if (*m->ttynode != '\0') {
	char	path[NG_PATHSIZ];

	snprintf(path, sizeof(path), "%s:%s", m->ttynode, NG_TTY_HOOK);
	NgFuncShutdownNode(m->csock, l->name, path);
	snprintf(path, sizeof(path), "%s:", m->ttynode);
	NgFuncShutdownNode(m->csock, l->name, path);
	*m->ttynode = '\0';
    }
    (void) write(m->fd, &ch, 1);	/* USR kludge to prevent dial lockup */
    if (m->csock > 0) {
	close(m->csock);
	m->csock = -1;
    }
    ExclusiveCloseDevice(l->name, m->fd, m->device);
    m->lastClosed = time(NULL);
    m->answering = FALSE;
    m->fd = -1;
    m->opened = opened;
    ModemStart(l);
}

/*
 * ModemSetAccm()
 */

static int
ModemSetAccm(Link l, u_int32_t xmit, u_int32_t recv)
{
    ModemInfo		const m = (ModemInfo) l->info;
    char       		path[NG_PATHSIZ];

    /* Update async config */
    m->acfg.accm = xmit|recv;
    snprintf(path, sizeof(path), "%s:%s", m->ttynode, NG_TTY_HOOK);
    if (NgSendMsg(m->csock, path, NGM_ASYNC_COOKIE,
      NGM_ASYNC_CMD_SET_CONFIG, &m->acfg, sizeof(m->acfg)) < 0) {
	Log(LG_PHYS, ("[%s] MODEM: can't update config for %s: %s",
    	    l->name, path, strerror(errno)));
        return (-1);
    }
    return (0);
}

/*
 * ModemChatConnectResult()
 *
 * Connect chat script returns here when finished.
 */

static void
ModemChatConnectResult(void *arg, int result, const char *msg)
{
    Link	const l = (Link) arg;
    ModemInfo	const m = (ModemInfo) l->info;
    char	*cspeed;
    int		bw;

    /* Was the connect script successful? */
    Log(LG_PHYS, ("[%s] MODEM: chat script %s",
	l->name, result ? "succeeded" : "failed"));
    if (!result) {
failed:
	ModemDoClose(l, FALSE);
	l->state = PHYS_STATE_DOWN;
	PhysDown(l, STR_ERROR, msg);
	return;
    }

    /* Set modem's reported connection speed (if any) as the link bandwidth */
    if ((cspeed = ChatGetVar(m->chat, CHAT_VAR_CONNECT_SPEED)) != NULL) {
	if ((bw = (int) strtoul(cspeed, NULL, 10)) > 0) {
	    l->bandwidth = bw;
	}
	Freee(cspeed);
    }

    /* Do async <-> sync conversion via netgraph node */
    if (ModemInstallNodes(l) < 0) {
	msg = STR_DEV_NOT_READY;
	goto failed;
    }

    /* Start pin check and report timers */
    TimerInit(&m->checkTimer, "ModemCheck",
	MODEM_CHECK_INTERVAL * SECONDS, ModemCheck, l);
    TimerStart(&m->checkTimer);
    TimerStop(&m->reportTimer);
    TimerInit(&m->reportTimer, "ModemReport",
	MODEM_ERR_REPORT_INTERVAL * SECONDS, ModemErrorCheck, l);
    TimerStart(&m->reportTimer);

    l->state = PHYS_STATE_UP;
    PhysUp(l);
}

/*
 * ModemChatIdleResult()
 *
 * Idle chat script returns here when finished. If the script returned
 * successfully, then one of two things happened: either we answered
 * an incoming call, or else we got a ring and want to do ringback.
 * We tell the difference by checking $IdleResult.
 */

static void
ModemChatIdleResult(void *arg, int result, const char *msg)
{
    Link		const l = (Link) arg;
    ModemInfo	const m = (ModemInfo) l->info;
    char	*idleResult;

    /* If script failed, then do nothing */
    if (!result) {
	ModemDoClose(l, FALSE);
	return;
    }

    /* See what script wants us to do now by checking variable $IdleResult */
    if ((idleResult = ChatGetVar(m->chat, CHAT_VAR_IDLE_RESULT)) == NULL) {
	Log(LG_ERR, ("[%s] MODEM: idle script succeeded, but %s not defined",
    	    l->name, CHAT_VAR_IDLE_RESULT));
	ModemDoClose(l, FALSE);
	return;
    }

    /* Do whatever */
    Log(LG_PHYS, ("[%s] MODEM: idle script succeeded, action=%s",
	l->name, idleResult));

    if (gShutdownInProgress) {
        Log(LG_PHYS, ("Shutdown sequence in progress, ignoring"));
	ModemDoClose(l, FALSE);
    } else if (strcasecmp(idleResult, MODEM_IDLE_RESULT_ANSWER) == 0) {
        Log(LG_PHYS, ("[%s] MODEM: opening link in %s mode", l->name, "answer"));
        RecordLinkUpDownReason(NULL, l, 1, STR_INCOMING_CALL, msg ? "%s" : NULL, msg);
        m->answering = TRUE;
        l->state = PHYS_STATE_READY;
        PhysIncoming(l);
    } else if (strcasecmp(idleResult, MODEM_IDLE_RESULT_RINGBACK) == 0) {
        Log(LG_PHYS, ("[%s] MODEM: opening link in %s mode", l->name, "ringback"));
        RecordLinkUpDownReason(NULL, l, 1, STR_RINGBACK, msg ? "%s" : NULL, msg);
        m->answering = FALSE;
        PhysIncoming(l);
    } else {
        Log(LG_ERR, ("[%s] MODEM: idle script succeeded, but action \"%s\" unknown",
    	  l->name, idleResult));
        ModemDoClose(l, FALSE);
    }
    Freee(idleResult);
}

/*
 * ModemInstallNodes()
 */

static int
ModemInstallNodes(Link l)
{
    ModemInfo 		m = (ModemInfo) l->info;
    struct ngm_mkpeer	ngm;
    struct ngm_connect	cn;
    char       		path[NG_PATHSIZ];
    int			hotchar = PPP_FLAG;
#if NGM_TTY_COOKIE < 1226109660
    struct nodeinfo	ngtty;
    int			ldisc = NETGRAPHDISC;
#else
    struct ngm_rmhook	rm;
    union {
	u_char buf[sizeof(struct ng_mesg) + sizeof(struct nodeinfo)];
	struct ng_mesg reply;
    } repbuf;
    struct ng_mesg *const reply = &repbuf.reply;
    struct nodeinfo *ninfo = (struct nodeinfo *)&reply->data;
    int	tty[2];
#endif

    /* Get a temporary netgraph socket node */
    if (NgMkSockNode(NULL, &m->csock, NULL) == -1) {
    	Log(LG_ERR, ("MODEM: NgMkSockNode: %s", strerror(errno)));
    	return(-1);
    }

#if NGM_TTY_COOKIE < 1226109660
    /* Install ng_tty line discipline */
    if (ioctl(m->fd, TIOCSETD, &ldisc) < 0) {

	/* Installation of the tty node type should be automatic, but isn't yet.
    	   The 'mkpeer' below will fail, because you can only create a ng_tty
           node via TIOCSETD; however, this will force a load of the node type. */
	if (errno == ENODEV) {
    	    (void)NgSendAsciiMsg(m->csock, ".:",
		"mkpeer { type=\"%s\" ourhook=\"dummy\" peerhook=\"%s\" }",
		NG_TTY_NODE_TYPE, NG_TTY_HOOK);
	}
	if (ioctl(m->fd, TIOCSETD, &ldisc) < 0) {
    	    Log(LG_ERR, ("[%s] ioctl(TIOCSETD, %d): %s",
		l->name, ldisc, strerror(errno))); 
    	    close(m->csock);
    	    return(-1);
	}
    }

    /* Get the name of the ng_tty node */
    if (ioctl(m->fd, NGIOCGINFO, &ngtty) < 0) {
	Log(LG_ERR, ("[%s] MODEM: ioctl(NGIOCGINFO): %s", l->name, strerror(errno))); 
	return(-1);
    }
    strlcpy(m->ttynode, ngtty.name, sizeof(m->ttynode));
#else
    /* Attach a TTY node */
    snprintf(ngm.type, sizeof(ngm.type), "%s", NG_TTY_NODE_TYPE);
    snprintf(ngm.ourhook, sizeof(ngm.ourhook), "%s", NG_TTY_HOOK);
    snprintf(ngm.peerhook, sizeof(ngm.peerhook), "%s", NG_TTY_HOOK);
    if (NgSendMsg(m->csock, ".", NGM_GENERIC_COOKIE,
    	    NGM_MKPEER, &ngm, sizeof(ngm)) < 0) {
        Log(LG_ERR, ("[%s] MODEM: can't connect %s node on %s", l->name,
    	    NG_TTY_NODE_TYPE, "."));
        close(m->csock);
	return(-1);
    }
    snprintf(path, sizeof(path), ".:%s", NG_TTY_HOOK);
    if (NgSendMsg(m->csock, path,
	    NGM_GENERIC_COOKIE, NGM_NODEINFO, NULL, 0) != -1) {
        if (NgRecvMsg(m->csock, reply, sizeof(repbuf), NULL) < 0) {
            Log(LG_ERR, ("[%s] MODEM: can't locate %s node on %s (%d)", l->name,
        	NG_TTY_NODE_TYPE, path, errno));
	    close(m->csock);
	    return(-1);
	}
    }
    snprintf(m->ttynode, sizeof(m->ttynode), "[%x]", ninfo->id);
    /* Attach to the TTY */
    tty[0] = gPid;
    tty[1] = m->fd;
    if (NgSendMsg(m->csock, path, NGM_TTY_COOKIE,
          NGM_TTY_SET_TTY, &tty, sizeof(tty)) < 0) {
        Log(LG_ERR, ("[%s] MODEM: can't hook tty to fd %d", l->name, m->fd));
        close(m->csock);
	return(-1);
    }
    /* Disconnect temporary hook. */
    snprintf(rm.ourhook, sizeof(rm.ourhook), "%s", NG_TTY_HOOK);
    if (NgSendMsg(m->csock, ".",
    	    NGM_GENERIC_COOKIE, NGM_RMHOOK, &rm, sizeof(rm)) < 0) {
        Log(LG_ERR, ("[%s] MODEM: can't remove hook %s: %s", l->name,
	    NG_TTY_HOOK, strerror(errno)));
    	close(m->csock);
    	return(-1);
    }
#endif

    /* Set the ``hot char'' on the TTY node */
    snprintf(path, sizeof(path), "%s:", m->ttynode);
    if (NgSendMsg(m->csock, path, NGM_TTY_COOKIE,
      NGM_TTY_SET_HOTCHAR, &hotchar, sizeof(hotchar)) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't set hotchar", l->name));
	close(m->csock);
	return(-1);
    }

    /* Attach an async converter node */
    strcpy(ngm.type, NG_ASYNC_NODE_TYPE);
    strcpy(ngm.ourhook, NG_TTY_HOOK);
    strcpy(ngm.peerhook, NG_ASYNC_HOOK_ASYNC);
    if (NgSendMsg(m->csock, path, NGM_GENERIC_COOKIE,
      NGM_MKPEER, &ngm, sizeof(ngm)) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't connect %s node", l->name, NG_ASYNC_NODE_TYPE));
	close(m->csock);
	return(-1);
    }

    /* Configure the async converter node */
    snprintf(path, sizeof(path), "%s:%s", m->ttynode, NG_TTY_HOOK);
    memset(&m->acfg, 0, sizeof(m->acfg));
    m->acfg.enabled = TRUE;
    m->acfg.accm = ~0;
    m->acfg.amru = MODEM_MRU;
    m->acfg.smru = MODEM_MTU;
    if (NgSendMsg(m->csock, path, NGM_ASYNC_COOKIE,
      NGM_ASYNC_CMD_SET_CONFIG, &m->acfg, sizeof(m->acfg)) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't config %s", l->name, path));
	close(m->csock);
	return(-1);
    }

    /* Attach async node to PPP node */
    if (!PhysGetUpperHook(l, cn.path, cn.peerhook)) {
        Log(LG_PHYS, ("[%s] MODEM: can't get upper hook", l->name));
	close(m->csock);
	return (-1);
    }
    snprintf(cn.ourhook, sizeof(cn.ourhook), NG_ASYNC_HOOK_SYNC);
    if (NgSendMsg(m->csock, path, NGM_GENERIC_COOKIE, NGM_CONNECT, 
        &cn, sizeof(cn)) < 0) {
    	    Log(LG_ERR, ("[%s] MODEM: can't connect \"%s\"->\"%s\" and \"%s\"->\"%s\": %s",
	        l->name, path, cn.ourhook, cn.path, cn.peerhook, strerror(errno)));
	    close(m->csock);
	    return (-1);
    }

    return(0);
}

/*
 * ModemChatSetBaudrate()
 *
 * This callback changes the actual baudrate of the serial port.
 * Should only be called once the device is already open.
 * Returns -1 on failure.
 */

static int
ModemChatSetBaudrate(void *arg, int baud)
{
    Link		const l = (Link) arg;
    ModemInfo		const m = (ModemInfo) l->info;
    struct termios	attr;

    /* Change baud rate */
    if (tcgetattr(m->fd, &attr) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't tcgetattr \"%s\": %s",
    	    l->name, m->device, strerror(errno)));
	return(-1);
    }
    if (cfsetspeed(&attr, (speed_t) baud) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't set speed %d: %s",
    	    l->name, baud, strerror(errno)));
	return(-1);
    }
    if (tcsetattr(m->fd, TCSANOW, &attr) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't tcsetattr \"%s\": %s",
    	    l->name, m->device, strerror(errno)));
	return(-1);
    }
    return(0);
}

/*
 * ModemGetVar()
 */

char *
ModemGetVar(Link l, const char *name)
{
    ModemInfo	const m = (ModemInfo) l->info;

    return ChatGetVar(m->chat, name);
}

/*
 * ModemCheck()
 */

static void
ModemCheck(void *arg)
{
    Link	const l = (Link)arg;
    ModemInfo	const m = (ModemInfo) l->info;
    int		state;

    if (ioctl(m->fd, TIOCMGET, &state) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't ioctl(%s) %s: %s",
    	    l->name, "TIOCMGET", m->device, strerror(errno)));
	l->state = PHYS_STATE_DOWN;
	ModemDoClose(l, FALSE);
	PhysDown(l, STR_ERROR, strerror(errno));
	return;
    }
    if ((m->watch & TIOCM_CAR) && !(state & TIOCM_CAR)) {
	Log(LG_PHYS, ("[%s] MODEM: carrier detect (CD) signal lost", l->name));
	l->state = PHYS_STATE_DOWN;
	ModemDoClose(l, FALSE);
	PhysDown(l, STR_DROPPED, STR_LOST_CD);
	return;
    }
    if ((m->watch & TIOCM_DSR) && !(state & TIOCM_DSR)) {
	Log(LG_PHYS, ("[%s] MODEM: data-set ready (DSR) signal lost", l->name));
	l->state = PHYS_STATE_DOWN;
	ModemDoClose(l, FALSE);
	PhysDown(l, STR_DROPPED, STR_LOST_DSR);
	return;
    }
    TimerStart(&m->checkTimer);
}

/*
 * ModemErrorCheck()
 *
 * Called every second to record errors to the log
 */

static void
ModemErrorCheck(void *arg)
{
    Link			const l = (Link) arg;
    ModemInfo			const m = (ModemInfo) l->info;
    char			path[NG_PATHSIZ];
    struct ng_async_stat	stats;

    /* Check for errors */
    snprintf(path, sizeof(path), "%s:%s", m->ttynode, NG_TTY_HOOK);
    if (ModemGetNgStats(l, &stats) >= 0
      && (stats.asyncBadCheckSums
       || stats.asyncRunts || stats.asyncOverflows)) {
	Log(LG_PHYS, ("[%s] NEW FRAME ERRS: FCS %u RUNT %u OVFL %u",
        l->name, stats.asyncBadCheckSums,
        stats.asyncRunts, stats.asyncOverflows));
	(void) NgSendMsg(m->csock, path,
    	    NGM_ASYNC_COOKIE, NGM_ASYNC_CMD_CLR_STATS, NULL, 0);
    }

    /* Restart timer */
    TimerStop(&m->reportTimer);
    TimerStart(&m->reportTimer);
}

/*
 * ModemGetNgStats()
 */

static int
ModemGetNgStats(Link l, struct ng_async_stat *sp)
{
    ModemInfo           const m = (ModemInfo) l->info;
    char		path[NG_PATHSIZ];
    union {
	u_char		buf[sizeof(struct ng_mesg) + sizeof(*sp)];
	struct ng_mesg	resp;
    } u;

    /* Get stats */
    snprintf(path, sizeof(path), "%s:%s", m->ttynode, NG_TTY_HOOK);
    if (NgFuncSendQuery(path, NGM_ASYNC_COOKIE, NGM_ASYNC_CMD_GET_STATS,
      NULL, 0, &u.resp, sizeof(u), NULL) < 0) {
	Log(LG_ERR, ("[%s] MODEM: can't get stats: %s", l->name, strerror(errno)));
	return(-1);
    }

    memcpy(sp, u.resp.data, sizeof(*sp));
    return(0);
}

/*
 * ModemSetCommand()
 */

static int
ModemSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    Link	const l = ctx->lnk;
    ModemInfo	const m = (ModemInfo) l->info;

    switch ((intptr_t)arg) {
	case SET_DEVICE:
    	    if (ac == 1)
		strlcpy(m->device, av[0], sizeof(m->device));
    	    break;
	case SET_SPEED:
    	    {
		int	k, baud;

		if (ac != 1)
		    return(-1);
		baud = atoi(*av);
		for (k = 0; gSpeedList[k] != -1 && baud != gSpeedList[k]; k++);
		if (gSpeedList[k] == -1)
		    Error("invalid speed \'%s\'", *av);
		else {
		    char	buf[32];

		    m->speed = baud;
		    snprintf(buf, sizeof(buf), "%d", m->speed);
		    ChatPresetVar(m->chat, CHAT_VAR_BAUDRATE, buf);
		}
    	    }
    	    break;
	case SET_CSCRIPT:
    	    if (ac != 1)
		return(-1);
    	    *m->connScript = 0;
    	    strlcpy(m->connScript, av[0], sizeof(m->connScript));
    	    break;
	case SET_ISCRIPT:
    	    if (ac != 1)
		return(-1);
    	    *m->idleScript = 0;
    	    strlcpy(m->idleScript, av[0], sizeof(m->idleScript));
    	    if (m->opened || TimerRemain(&m->startTimer) >= 0)
		break;		/* nothing needs to be done right now */
    	    if (m->fd >= 0 && !*m->idleScript)
	        ModemDoClose(l, FALSE);
    	    else if (m->fd < 0 && *m->idleScript)
	        ModemStart(l);
    	    break;
	case SET_SCRIPT_VAR:
    	    if (ac != 2)
		return(-1);
    	    ChatPresetVar(m->chat, av[0], av[1]);
    	    break;
	case SET_WATCH:
    	    {
		int	bit, add;

		while (ac--) {
		    switch (**av) {
			case '+':
	    		    (*av)++;
			default:
	    		    add = TRUE;
	    		    break;
			case '-':
	    		    add = FALSE;
	    		    (*av)++;
	    		    break;
		    }
		    if (!strcasecmp(*av, "cd"))
			bit = TIOCM_CAR;
		    else if (!strcasecmp(*av, "dsr"))
			bit = TIOCM_DSR;
		    else {
			Printf("[%s] modem signal \"%s\" is unknown\r\n", l->name, *av);
			bit = 0;
		    }
		    if (add)
			m->watch |= bit;
		    else
			m->watch &= ~bit;
		    av++;
		}
    	    }
    	    break;
	default:
    	    assert(0);
    }
    return(0);
}

/*
 * ModemOriginated()
 */

static int
ModemOriginated(Link l)
{
    ModemInfo	const m = (ModemInfo) l->info;

    return(m->originated ? LINK_ORIGINATE_LOCAL : LINK_ORIGINATE_REMOTE);
}

/*
 * ModemIsSync()
 */

static int
ModemIsSync(Link l)
{
    return (0);
}

static int
ModemSelfAddr(Link l, void *buf, size_t buf_len)
{
    ModemInfo	const m = (ModemInfo) l->info;

    strlcpy(buf, m->ttynode, buf_len);
    return(0);
}

static int
ModemIface(Link l, void *buf, size_t buf_len)
{
    ModemInfo	const m = (ModemInfo) l->info;

    strlcpy(buf, m->device, buf_len);
    return(0);
}

static int
ModemCallingNum(Link l, void *buf, size_t buf_len)
{
    ModemInfo	const m = (ModemInfo) l->info;
    char	*tmp;

    if ((tmp = ChatGetVar(m->chat, CHAT_VAR_CALLING)) == NULL) {
	((char *)buf)[0] = 0;
	return (-1);
    }
    strlcpy((char*)buf, tmp, buf_len);
    Freee(tmp);
    return(0);
}

static int
ModemCalledNum(Link l, void *buf, size_t buf_len)
{
    ModemInfo	const m = (ModemInfo) l->info;
    char	*tmp;

    if ((tmp = ChatGetVar(m->chat, CHAT_VAR_CALLED)) == NULL) {
	((char *)buf)[0] = 0;
	return (-1);
    }
    strlcpy((char*)buf, tmp, buf_len);
    Freee(tmp);
    return(0);
}

/*
 * ModemStat()
 */

void
ModemStat(Context ctx)
{
    ModemInfo			const m = (ModemInfo) ctx->lnk->info;
    struct ng_async_stat	stats;
    char			*tmp;

    Printf("Modem info:\r\n");
    Printf("\tDevice       : %s\r\n", m->device);
    Printf("\tPort speed   : %d baud\r\n", m->speed);
    Printf("\tConn. script : \"%s\"\r\n", m->connScript);
    Printf("\tIdle script  : \"%s\"\r\n", m->idleScript);
    Printf("\tPins to watch: %s%s\r\n",
	(m->watch & TIOCM_CAR) ? "CD " : "",
	(m->watch & TIOCM_DSR) ? "DSR" : "");

    Printf("Modem status:\r\n");
    if (ctx->lnk->state != PHYS_STATE_DOWN) {
	Printf("\tOpened       : %s\r\n", (m->opened?"YES":"NO"));
	Printf("\tIncoming     : %s\r\n", (m->originated?"NO":"YES"));

	/* Set modem's reported connection speed (if any) as the link bandwidth */
	if ((tmp = ChatGetVar(m->chat, CHAT_VAR_CONNECT_SPEED)) != NULL) {
	    Printf("\tConnect speed: %s baud\r\n", tmp);
	    Freee(tmp);
	}

	if ((tmp = ChatGetVar(m->chat, CHAT_VAR_CALLING)) != NULL) {
	    Printf("\tCalling      : %s\r\n", tmp);
	    Freee(tmp);
	}
	if ((tmp = ChatGetVar(m->chat, CHAT_VAR_CALLED)) != NULL) {
	    Printf("\tCalled       : %s\r\n", tmp);
	    Freee(tmp);
	}

	if (ctx->lnk->state == PHYS_STATE_UP && 
    		ModemGetNgStats(ctx->lnk, &stats) >= 0) {
    	    Printf("Async stats:\r\n");
    	    Printf("\t       syncOctets: %8u\r\n", stats.syncOctets);
    	    Printf("\t       syncFrames: %8u\r\n", stats.syncFrames);
    	    Printf("\t    syncOverflows: %8u\r\n", stats.syncOverflows);
	    Printf("\t      asyncOctets: %8u\r\n", stats.asyncOctets);
    	    Printf("\t      asyncFrames: %8u\r\n", stats.asyncFrames);
    	    Printf("\t       asyncRunts: %8u\r\n", stats.asyncRunts);
    	    Printf("\t   asyncOverflows: %8u\r\n", stats.asyncOverflows);
    	    Printf("\tasyncBadCheckSums: %8u\r\n", stats.asyncBadCheckSums);
        }
    }
}

