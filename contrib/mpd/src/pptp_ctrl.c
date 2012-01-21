
/*
 * pptp_ctrl.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "log.h"
#include "pptp_ctrl.h"
#include "util.h"

#include <netinet/tcp.h>

/*
 * DEFINITIONS
 */

  #define PPTP_FIRMWARE_REV		0x0101

  #define PPTP_STR_INTERNAL_CALLING	"Internally originated VPN call"

  /* Limits on how long we wait for replies to things */
  #define PPTP_DFL_REPLY_TIME		PPTP_IDLE_TIMEOUT
  #define PPTP_OUTCALLREQ_REPLY_TIME	90
  #define PPTP_INCALLREP_REPLY_TIME	90
  #define PPTP_STOPCCR_REPLY_TIME	3

  /* Retry for binding to listen socket */
  #define PPTP_LISTEN_RETRY		10

  /* This describes how/if a reply is required */
  struct pptpreqrep {
    u_char	reply;			/* required reply (or zero) */
    u_char	killCtrl;		/* fatal to ctrl or just to channel */
    u_short	timeout;		/* max time to wait for reply */
  };
  typedef struct pptpreqrep	*PptpReqRep;

  /* This represents a pending reply we're waiting for */
  struct pptppendrep {
    const struct pptpmsginfo	*request;	/* original message info */
    struct pptpctrl		*ctrl;		/* control channel */
    struct pptpchan		*chan;		/* channel (NULL if none) */
    struct pppTimer		timer;		/* reply timeout timer */
    struct pptppendrep		*next;		/* next in list */
  };
  typedef struct pptppendrep	*PptpPendRep;

  /* This describes how to match a message to the corresponding channel */
  struct pptpchanid {
    u_char		findIn;		/* how to find channel (incoming) */
    u_char		findOut;	/* how to find channel (outgoing) */
    const char		*inField;	/* field used to find channel (in) */
    const char		*outField;	/* field used to find channel (out) */
  };
  typedef struct pptpchanid	*PptpChanId;

  #define PPTP_FIND_CHAN_MY_CID		1	/* match field vs. my cid */
  #define PPTP_FIND_CHAN_PEER_CID	2	/* match field vs. peer cid */
  #define PPTP_FIND_CHAN_PNS_CID	3	/* match field vs. PNS cid */
  #define PPTP_FIND_CHAN_PAC_CID	4	/* match field vs. PAC cid */

  /* Total info about a message type (except field layout) */
  struct pptpmsginfo {
    const char		*name;		/* name for this message type */
    void		(*handler)();	/* message handler function */
    u_char		isReply;	/* this is always a reply message */
    u_char		length;		/* length of message (sans header) */
    u_short		states;		/* states which admit this message */
    struct pptpchanid	match;		/* how to find corresponding channel */
    struct pptpreqrep	reqrep;		/* what kind of reply we expect */
  };
  typedef const struct pptpmsginfo	*PptpMsgInfo;

  /* Receive window size XXX */
  #define PPTP_RECV_WIN			16

  /* Packet processing delay XXX */
  #define PPTP_PPD			1

  /* Channel state */
  struct pptpchan {
    uint16_t		id;		/* channel index */
    u_char		state;		/* channel state */
    u_char		orig;		/* call originated from us */
    u_char		incoming;	/* call is incoming, not outgoing */
    u_int16_t		cid;		/* my call id */
    u_int16_t		serno;		/* call serial number */
    u_int16_t		peerCid;	/* peer call id */
    u_int16_t		peerPpd;	/* peer's packet processing delay */
    u_int16_t		recvWin;	/* peer's recv window size */
    u_int32_t		bearType;	/* call bearer type */
    u_int32_t		frameType;	/* call framing type */
    u_int32_t		minBps;		/* minimum acceptable speed */
    u_int32_t		maxBps;		/* maximum acceptable speed */
    struct pptplinkinfo	linfo;		/* info about corresponding link */
    struct pptpctrl	*ctrl;		/* my control channel */
    char		callingNum[PPTP_PHONE_LEN + 1];	/* calling number */
    char		calledNum[PPTP_PHONE_LEN + 1];	/* called number */
    char		subAddress[PPTP_SUBADDR_LEN + 1];/* sub-address */
    struct pppTimer	killTimer;	/* kill timer */
  };
  typedef struct pptpchan	*PptpChan;

  #define PPTP_CHAN_IS_PNS(ch)		(!(ch)->orig ^ !(ch)->incoming)

  /* Control channel state */
  struct pptpctrl {
    u_int32_t		id;		/* channel index */
    u_char		state;		/* state */
    u_char		orig;		/* we originated connection */
    union {
	u_char			buf[PPTP_CTRL_MAX_FRAME];
	struct pptpMsgHead	hdr;
    }			frame;
    u_int16_t		flen;		/* length of partial frame */
    int			csock;		/* peer control messages */
    struct u_addr	self_addr;	/* local IP address */
    struct u_addr	peer_addr;	/* peer we're talking to */
    in_port_t		self_port;
    in_port_t		peer_port;
    EventRef		connEvent;	/* connection event */
    EventRef		ctrlEvent;	/* control connection input */
    struct pppTimer	idleTimer;	/* idle timer */
    struct pppTimer	killTimer;	/* kill timer */
    u_int32_t		echoId;		/* last echo id # sent */
    PptpPendRep		reps;		/* pending replies to msgs */
    PptpChan		*channels;	/* array of channels */
    int			numChannels;	/* length of channels array */
    u_int		active_sessions;	/* # non-dying sessns */
    char 		self_name[MAXHOSTNAMELEN]; /* local hostname */
    char		peer_name[MAXHOSTNAMELEN]; /* remote hostname */
  };
  typedef struct pptpctrl	*PptpCtrl;

  struct pptplis {
    struct u_addr	self_addr;	/* local IP address */
    in_port_t		self_port;
    int			ref;
    int			sock;
    EventRef		retry;
    EventRef		event;
  };
  typedef struct pptplis	*PptpLis;

  /* Our physical channel ID */
  #define PHYS_CHAN(ch)		(((ch)->ctrl->id << 16) | (ch)->id)

  int	PptpsStat(Context ctx, int ac, char *av[], void *arg);

/*
 * INTERNAL FUNCTIONS
 */

  /* Methods for each control message type */
  static void	PptpStartCtrlConnRequest(PptpCtrl c,
			struct pptpStartCtrlConnRequest *m);
  static void	PptpStartCtrlConnReply(PptpCtrl c,
			struct pptpStartCtrlConnReply *m);
  static void	PptpStopCtrlConnRequest(PptpCtrl c,
			struct pptpStopCtrlConnRequest *m);
  static void	PptpStopCtrlConnReply(PptpCtrl c,
			struct pptpStopCtrlConnReply *m);
  static void	PptpEchoRequest(PptpCtrl c, struct pptpEchoRequest *m);
  static void	PptpEchoReply(PptpCtrl c, struct pptpEchoReply *m);
  static void	PptpOutCallRequest(PptpCtrl c, struct pptpOutCallRequest *m);
  static void	PptpOutCallReply(PptpChan ch, struct pptpOutCallReply *m);
  static void	PptpInCallRequest(PptpCtrl c, struct pptpInCallRequest *m);
  static void	PptpInCallReply(PptpChan ch, struct pptpInCallReply *m);
  static void	PptpInCallConn(PptpChan ch, struct pptpInCallConn *m);
  static void	PptpCallClearRequest(PptpChan ch,
			struct pptpCallClearRequest *m);
  static void	PptpCallDiscNotify(PptpChan ch, struct pptpCallDiscNotify *m);
  static void	PptpWanErrorNotify(PptpChan ch, struct pptpWanErrorNotify *m);
  static void	PptpSetLinkInfo(PptpChan ch, struct pptpSetLinkInfo *m);

  /* Link layer callbacks */
  static void	PptpCtrlCloseChan(PptpChan ch,
		  int result, int error, int cause);
  static void	PptpCtrlKillChan(PptpChan ch, const char *errmsg);
  static void	PptpCtrlFreeChan(PptpChan ch);
  static void	PptpCtrlDialResult(void *cookie,
		  int result, int error, int cause, int speed);
  static void	PptpCtrlConected(void *cookie, int speed);
  static void	PptpCtrlSetLinkInfo(void *cookie, u_int32_t sa, u_int32_t ra);

  /* Internal event handlers */
  static void	PptpCtrlListenEvent(int type, void *cookie);
  static void	PptpCtrlListenRetry(int type, void *cookie);
  static void	PptpCtrlConnEvent(int type, void *cookie);
  static void	PptpCtrlReadCtrl(int type, void *cookie);

  /* Shutdown routines */
  static void	PptpCtrlCloseCtrl(PptpCtrl c);
  static void	PptpCtrlKillCtrl(PptpCtrl c);
  static void	PptpCtrlFreeCtrl(PptpCtrl c);

  /* Timer routines */
  static void	PptpCtrlResetIdleTimer(PptpCtrl c);
  static void	PptpCtrlIdleTimeout(void *arg);
  static void	PptpCtrlReplyTimeout(void *arg);

  /* Misc routines */
  static void	PptpCtrlInitCtrl(PptpCtrl c, int orig);
  static void	PptpCtrlMsg(PptpCtrl c, int type, void *msg);
  static int	PptpCtrlWriteMsg(PptpCtrl c, int type, void *msg);

  static void	PptpCtrlSwap(int type, void *buf);
  static void	PptpCtrlDump(int level, int type, void *msg);
  static int	PptpCtrlFindField(int type, const char *name, u_int *offset);
  static void	PptpCtrlInitCinfo(PptpChan ch, PptpCtrlInfo ci);

  static void	PptpCtrlNewCtrlState(PptpCtrl c, int new);
  static void	PptpCtrlNewChanState(PptpChan ch, int new);

  static void		PptpCtrlOrigCall(int incoming, struct pptpctrlinfo *cinfo,
			  struct pptplinkinfo *linfo, struct u_addr *locip,
			  struct u_addr *ip, in_port_t port, int bearType,
			  int frameType, int minBps, int maxBps,
			  const char *callingNum, const char *calledNum,
			  const char *subAddress);

  static PptpCtrl	PptpCtrlGetCtrl(int orig, struct u_addr *self_addr,
			  struct u_addr *peer_addr, in_port_t peer_port,
			  char *buf, size_t bsiz);
  static PptpChan	PptpCtrlGetChan(PptpCtrl c, int chanState, int orig,
			  int incoming, int bearType, int frameType, int minBps,
			  int maxBps, const char *callingNum,
			  const char *calledNum, const char *subAddress);
  static PptpChan	PptpCtrlFindChan(PptpCtrl c, int type,
			  void *msg, int incoming);
  static void		PptpCtrlCheckConn(PptpCtrl c);

/*
 * INTERNAL VARIABLES
 */

  static u_char			gInitialized = 0;
  static u_int16_t		gLastCallId;
  static u_char			gCallIds[65536];
  static PptpGetInLink_t	gGetInLink;
  static PptpGetOutLink_t	gGetOutLink;

  static PptpCtrl		*gPptpCtrl;	/* array of control channels */
  static int			gNumPptpCtrl;	/* length of gPptpCtrl array */

  static PptpLis		*gPptpLis;	/* array of listeners */
  static int			gNumPptpLis;	/* length of gPptpLis array */

  /* Control message field layout */
  static struct pptpfield
    gPptpMsgLayout[PPTP_MAX_CTRL_TYPE][PPTP_CTRL_MAX_FIELDS] =
  {
#define _WANT_PPTP_FIELDS
#include "pptp_ctrl.h"
#undef _WANT_PPTP_FIELDS
  };

  /* Control channel and call state names */
  static const char		*gPptpCtrlStates[] = {
#define PPTP_CTRL_ST_FREE		0
		    "FREE",
#define PPTP_CTRL_ST_IDLE		1
		    "IDLE",
#define PPTP_CTRL_ST_WAIT_CTL_REPLY	2
		    "WAIT_CTL_REPLY",
#define PPTP_CTRL_ST_WAIT_STOP_REPLY	3
		    "WAIT_STOP_REPLY",
#define PPTP_CTRL_ST_ESTABLISHED	4
		    "ESTABLISHED",
#define PPTP_CTRL_ST_DYING		5
		    "DYING",
  };

  static const char		*gPptpChanStates[] = {
#define PPTP_CHAN_ST_FREE		0
		    "FREE",
#define PPTP_CHAN_ST_WAIT_IN_REPLY	1
		    "WAIT_IN_REPLY",
#define PPTP_CHAN_ST_WAIT_OUT_REPLY	2
		    "WAIT_OUT_REPLY",
#define PPTP_CHAN_ST_WAIT_CONNECT	3
		    "WAIT_CONNECT",
#define PPTP_CHAN_ST_WAIT_DISCONNECT	4
		    "WAIT_DISCONNECT",
#define PPTP_CHAN_ST_WAIT_ANSWER	5
		    "WAIT_ANSWER",
#define PPTP_CHAN_ST_ESTABLISHED	6
		    "ESTABLISHED",
#define PPTP_CHAN_ST_WAIT_CTRL		7
		    "WAIT_CTRL",
#define PPTP_CHAN_ST_DYING		8
		    "DYING",
  };

  /* Control message descriptors */
#define CL(s)	(1 << (PPTP_CTRL_ST_ ## s))
#define CH(s)	((1 << (PPTP_CHAN_ST_ ## s)) | 0x8000)

  static const struct pptpmsginfo	gPptpMsgInfo[PPTP_MAX_CTRL_TYPE] = {
    { "PptpMsgHead", NULL,			/* placeholder */
      FALSE, sizeof(struct pptpMsgHead),
    },
    { "StartCtrlConnRequest", PptpStartCtrlConnRequest,
      FALSE, sizeof(struct pptpStartCtrlConnRequest),
      CL(IDLE),
      { 0, 0 },					/* no associated channel */
      { PPTP_StartCtrlConnReply, TRUE, PPTP_DFL_REPLY_TIME },
    },
    { "StartCtrlConnReply", PptpStartCtrlConnReply,
      TRUE, sizeof(struct pptpStartCtrlConnReply),
      CL(WAIT_CTL_REPLY),
      { 0, 0 },					/* no associated channel */
      { 0 },					/* no reply expected */
    },
    { "StopCtrlConnRequest", PptpStopCtrlConnRequest,
      FALSE, sizeof(struct pptpStopCtrlConnRequest),
      CL(WAIT_CTL_REPLY)|CL(WAIT_STOP_REPLY)|CL(ESTABLISHED),
      { 0, 0 },					/* no associated channel */
      { PPTP_StopCtrlConnReply, TRUE, PPTP_STOPCCR_REPLY_TIME },
    },
    { "StopCtrlConnReply", PptpStopCtrlConnReply,
      TRUE, sizeof(struct pptpStopCtrlConnReply),
      CL(WAIT_STOP_REPLY),
      { 0, 0 },					/* no associated channel */
      { 0 },					/* no reply expected */
    },
    { "EchoRequest", PptpEchoRequest,
      FALSE, sizeof(struct pptpEchoRequest),
      CL(ESTABLISHED),
      { 0, 0 },					/* no associated channel */
      { PPTP_EchoReply, TRUE, PPTP_DFL_REPLY_TIME },
    },
    { "EchoReply", PptpEchoReply,
      TRUE, sizeof(struct pptpEchoReply),
      CL(ESTABLISHED),
      { 0, 0 },					/* no associated channel */
      { 0 },					/* no reply expected */
    },
    { "OutCallRequest", PptpOutCallRequest,
      FALSE, sizeof(struct pptpOutCallRequest),
      CL(ESTABLISHED),
      { 0, PPTP_FIND_CHAN_MY_CID, NULL, "cid" },
      { PPTP_OutCallReply, TRUE, PPTP_OUTCALLREQ_REPLY_TIME },
    },
    { "OutCallReply", PptpOutCallReply,
      TRUE, sizeof(struct pptpOutCallReply),
      CH(WAIT_OUT_REPLY),
      { PPTP_FIND_CHAN_MY_CID, PPTP_FIND_CHAN_MY_CID, "peerCid", "cid" },
      { 0 },					/* no reply expected */
    },
    { "InCallRequest", PptpInCallRequest,
      FALSE, sizeof(struct pptpInCallRequest),
      CL(ESTABLISHED),
      { 0, PPTP_FIND_CHAN_MY_CID, NULL, "cid" },
      { PPTP_InCallReply, FALSE, PPTP_DFL_REPLY_TIME },
    },
    { "InCallReply", PptpInCallReply,
      TRUE, sizeof(struct pptpInCallReply),
      CH(WAIT_IN_REPLY),
      { PPTP_FIND_CHAN_MY_CID, PPTP_FIND_CHAN_MY_CID, "peerCid", "cid" },
      { PPTP_InCallConn, FALSE, PPTP_INCALLREP_REPLY_TIME },
    },
    { "InCallConn", PptpInCallConn,
      TRUE, sizeof(struct pptpInCallConn),
      CH(WAIT_CONNECT),
      { PPTP_FIND_CHAN_MY_CID, PPTP_FIND_CHAN_PEER_CID, "peerCid", "peerCid" },
      { 0 },					/* no reply expected */
    },
    { "CallClearRequest", PptpCallClearRequest,
      FALSE, sizeof(struct pptpCallClearRequest),
      CH(WAIT_IN_REPLY)|CH(WAIT_ANSWER)|CH(ESTABLISHED),
      { PPTP_FIND_CHAN_PNS_CID, PPTP_FIND_CHAN_PNS_CID, "cid", "cid" },
      { PPTP_CallDiscNotify, TRUE, PPTP_DFL_REPLY_TIME },
    },
    { "CallDiscNotify", PptpCallDiscNotify,
      FALSE, sizeof(struct pptpCallDiscNotify),
      CH(WAIT_OUT_REPLY)|CH(WAIT_CONNECT)|CH(WAIT_DISCONNECT)|CH(ESTABLISHED),
      { PPTP_FIND_CHAN_PAC_CID, PPTP_FIND_CHAN_PAC_CID, "cid", "cid" },
      { 0 },					/* no reply expected */
    },
    { "WanErrorNotify", PptpWanErrorNotify,
      FALSE, sizeof(struct pptpWanErrorNotify),
      CH(ESTABLISHED),
      { PPTP_FIND_CHAN_PNS_CID, PPTP_FIND_CHAN_PNS_CID, "cid", "cid" },
      { 0 },					/* no reply expected */
    },
    { "SetLinkInfo", PptpSetLinkInfo,
      FALSE, sizeof(struct pptpSetLinkInfo),
      CH(ESTABLISHED),
      { PPTP_FIND_CHAN_PAC_CID, PPTP_FIND_CHAN_PAC_CID, "cid", "cid" },
      { 0 },					/* no reply expected */
    },
  };

#undef CL
#undef CH

  /* Error code to string converters */
  #define DECODE(a, n)	((u_int)(n) < (sizeof(a) / sizeof(*(a))) ? \
			    (a)[(u_int)(n)] : "[out of range]")

  static const char	*const gPptpErrorCodes[] = {
    "none",
    "not connected",
    "bad format",
    "bad value",
    "no resource",
    "bad call ID",
    "pac error",
  };
  #define PPTP_ERROR_CODE(n)		DECODE(gPptpErrorCodes, (n))

  static const char	*const gPptpSccrReslCodes[] = {
    "zero?",
    "OK",
    "general error",
    "channel exists",
    "not authorized",
    "bad protocol version",
  };
  #define PPTP_SCCR_RESL_CODE(n)	DECODE(gPptpSccrReslCodes, (n))

  static const char	*const gPptpSccrReasCodes[] = {
    "zero?",
    "none",
    "bad protocol version",
    "local shutdown",
  };
  #define PPTP_SCCR_REAS_CODE(n)	DECODE(gPptpSccrReasCodes, (n))

  static const char	*const gPptpEchoReslCodes[] = {
    "zero?",
    "OK",
    "general error",
  };
  #define PPTP_ECHO_RESL_CODE(n)	DECODE(gPptpEchoReslCodes, (n))

  static const char	*const gPptpOcrReslCodes[] = {
    "zero?",
    "OK",
    "general error",
    "no carrier",
    "busy",
    "no dialtone",
    "timed out",
    "admin prohib",
  };
  #define PPTP_OCR_RESL_CODE(n)		DECODE(gPptpOcrReslCodes, (n))

  static const char	*const gPptpIcrReslCodes[] = {
    "zero?",
    "OK",
    "general error",
    "not accepted",
  };
  #define PPTP_ICR_RESL_CODE(n)		DECODE(gPptpIcrReslCodes, (n))

  static const char	*const gPptpCdnReslCodes[] = {
    "zero?",
    "lost carrier",
    "general error",
    "admin action",
    "disconnect request",
  };
  #define PPTP_CDN_RESL_CODE(n)		DECODE(gPptpCdnReslCodes, (n))

/*************************************************************************
			EXPORTED FUNCTIONS
*************************************************************************/

/*
 * PptpCtrlInit()
 *
 * Initialize PPTP state and set up callbacks. This must be called
 * first, and any calls after the first will ignore the ip parameter.
 * Returns 0 if successful, -1 otherwise.
 *
 * Parameters:
 *   getInLink	Function to call when a peer has requested to establish
 *		an incoming call. If returned cookie is NULL, call failed.
 *		This pointer may be NULL to deny all incoming calls.
 *   getOutLink	Function to call when a peer has requested to establish
 *		an outgoming call. If returned cookie is NULL, call failed.
 *		This pointer may be NULL to deny all outgoing calls.
 *   ip		The IP address for my server to use (cannot be zero).
 */

int
PptpCtrlInit(PptpGetInLink_t getInLink, PptpGetOutLink_t getOutLink)
{
    int	type;

    /* Save callbacks */
    gGetInLink = getInLink;
    gGetOutLink = getOutLink;
    if (gInitialized)
	return(0);

    /* Generate semi-random call ID */
#ifdef RANDOMIZE_CID
    gLastCallId = (u_short) (time(NULL) ^ (gPid << 5));
#endif
    bzero(gCallIds, sizeof(gCallIds));

    /* Sanity check structure lengths and valid state bits */
    for (type = 0; type < PPTP_MAX_CTRL_TYPE; type++) {
	PptpMsgInfo	const mi = &gPptpMsgInfo[type];
	PptpField	field = gPptpMsgLayout[type];
	int		total;

	assert((mi->match.inField != NULL) ^ !(mi->states & 0x8000));
	for (total = 0; field->name; field++)
    	    total += field->length;
	assert(total == gPptpMsgInfo[type].length);
    }

    /* Done */
    gInitialized = TRUE;
    return(0);
}

/*
 * PptpCtrlListen()
 *
 * Enable incoming PPTP TCP connections.
 * Returns not-NULL if successful, NULL otherwise.
 */

void *
PptpCtrlListen(struct u_addr *ip, in_port_t port)
{
    char	buf[48];
    PptpLis	l;
    int		k;

    assert(gInitialized);
    port = port ? port : PPTP_PORT;
    
    /* See if we're already have a listener matching this address and port */
    for (k = 0; k < gNumPptpLis; k++) {
	PptpLis	const l = gPptpLis[k];

	if (l != NULL
	    && (!u_addrcompare (&l->self_addr, ip))
	    && l->self_port == port) {
		l->ref++;
		return(l);
        }
    }

    /* Find/create a free one */
    for (k = 0; k < gNumPptpLis && gPptpLis[k] != NULL; k++);
    if (k == gNumPptpLis)
	LengthenArray(&gPptpLis, sizeof(*gPptpLis), &gNumPptpLis, MB_PPTP);

    l = Malloc(MB_PPTP, sizeof(*l));
    l->ref = 1;
    l->self_addr = *ip;
    l->self_port = port;
    if ((l->sock = TcpGetListenPort(ip, port, FALSE)) < 0) {
        if (errno == EADDRINUSE || errno == EADDRNOTAVAIL) {
	    EventRegister(&l->retry, EVENT_TIMEOUT, PPTP_LISTEN_RETRY * 1000,
		0, PptpCtrlListenRetry, l);
        } else {
	    Freee(l);
    	    Log(LG_ERR, ("PPTP: can't get listening socket"));
    	    return(NULL);
	}
    } else {
	EventRegister(&l->event, EVENT_READ,
    	    l->sock, EVENT_RECURRING, PptpCtrlListenEvent, l);
    }
    gPptpLis[k] = l;
    Log(LG_PHYS, ("PPTP: waiting for connection on %s %u",
	u_addrtoa(&l->self_addr, buf, sizeof(buf)), l->self_port));
    return(l);
}

/*
 * PptpCtrlUnListen()
 *
 * Disable incoming PPTP TCP connections.
 */

void
PptpCtrlUnListen(void *listener)
{
    PptpLis	l = (PptpLis)listener;
    char	buf[48];
    int		k;

    assert(l);
    
    l->ref--;
    if (l->ref > 0)
	return;

    Log(LG_PHYS, ("PPTP: stop waiting for connection on %s %u",
	u_addrtoa(&l->self_addr, buf, sizeof(buf)), l->self_port));

    for (k = 0; k < gNumPptpLis && gPptpLis[k] != l; k++);
    assert(k != gNumPptpLis);
    
    gPptpLis[k] = NULL;
    EventUnRegister(&l->retry);
    EventUnRegister(&l->event);
    close(l->sock);
    Freee(l);
}

/*
 * PptpCtrlListenRetry()
 *
 * Socket address was temporarily unavailable; try again.
 */

static void
PptpCtrlListenRetry(int type, void *cookie)
{
    PptpLis const	l = (PptpLis)cookie;

    if ((l->sock = TcpGetListenPort(&l->self_addr, l->self_port, FALSE)) < 0) {
	EventRegister(&l->retry, EVENT_TIMEOUT, PPTP_LISTEN_RETRY * 1000,
	    0, PptpCtrlListenRetry, l);
    } else {
	EventRegister(&l->event, EVENT_READ,
    	    l->sock, EVENT_RECURRING, PptpCtrlListenEvent, l);
    }
}

/*
 * PptpCtrlInCall()
 *
 * Initiate an incoming call
 */

void
PptpCtrlInCall(struct pptpctrlinfo *cinfo, struct pptplinkinfo *linfo,
	struct u_addr *locip, struct u_addr *ip, in_port_t port,
	int bearType, int frameType, int minBps, int maxBps, const char *callingNum,
	const char *calledNum, const char *subAddress)
{
    PptpCtrlOrigCall(TRUE, cinfo, linfo, locip, ip, port,
	bearType, frameType, minBps, maxBps,
	callingNum, calledNum, subAddress);
}

/*
 * PptpCtrlOutCall()
 *
 * Initiate an outgoing call
 */

void
PptpCtrlOutCall(struct pptpctrlinfo *cinfo, struct pptplinkinfo *linfo,
	struct u_addr *locip, struct u_addr *ip, in_port_t port, int bearType,
	int frameType, int minBps, int maxBps,
	const char *calledNum, const char *subAddress)
{
    PptpCtrlOrigCall(FALSE, cinfo, linfo, locip, ip, port,
	bearType, frameType, minBps, maxBps,
        PPTP_STR_INTERNAL_CALLING, calledNum, subAddress);
}

/*
 * PptpCtrlOrigCall()
 *
 * Request from the PPTP peer at ip:port the establishment of an
 * incoming or outgoing call (as viewed by the peer). The "result"
 * callback will be called when the connection has been established
 * or failed to do so. This initiates a TCP control connection if
 * needed; otherwise it uses the existing connection. If port is
 * zero, then use the normal PPTP port.
 */

static void
PptpCtrlOrigCall(int incoming, struct pptpctrlinfo *cinfo, struct pptplinkinfo *linfo,
	struct u_addr *locip, struct u_addr *ip, in_port_t port, int bearType,
	int frameType, int minBps, int maxBps, const char *callingNum,
	const char *calledNum, const char *subAddress)
{
  PptpCtrl		c;
  PptpChan		ch;
  char			ebuf[64];

  /* Init */
  assert(gInitialized);
  port = port ? port : PPTP_PORT;
  memset(cinfo, 0, sizeof(cinfo));

  /* Find/create control block */
  if ((c = PptpCtrlGetCtrl(TRUE, locip, ip, port,
      ebuf, sizeof(ebuf))) == NULL) {
    Log(LG_PHYS2, ("%s", ebuf));
    return;
  }

  /* Get new channel */
  if ((ch = PptpCtrlGetChan(c, PPTP_CHAN_ST_WAIT_CTRL, TRUE, incoming,
      bearType, frameType, minBps, maxBps,
      callingNum, calledNum, subAddress)) == NULL) {
    PptpCtrlKillCtrl(c);
    return;
  }
  ch->linfo = *linfo;

  /* Control channel may be ready already; start channel if so */
  PptpCtrlCheckConn(c);

  /* Return OK */
  PptpCtrlInitCinfo(ch, cinfo);
}

/*
 * PptpCtrlGetSessionInfo()
 *
 * Returns information associated with a call.
 */

int
PptpCtrlGetSessionInfo(struct pptpctrlinfo *cp,
	struct u_addr *selfAddr, struct u_addr *peerAddr,
	u_int16_t *selfCid, u_int16_t *peerCid,
	u_int16_t *peerWin, u_int16_t *peerPpd)
{
  PptpChan	const ch = (PptpChan)cp->cookie;

  switch (ch->state) {
    case PPTP_CHAN_ST_WAIT_IN_REPLY:
    case PPTP_CHAN_ST_WAIT_OUT_REPLY:
    case PPTP_CHAN_ST_WAIT_CONNECT:
    case PPTP_CHAN_ST_WAIT_DISCONNECT:
    case PPTP_CHAN_ST_WAIT_ANSWER:
    case PPTP_CHAN_ST_ESTABLISHED:
    case PPTP_CHAN_ST_WAIT_CTRL:
      {
	PptpCtrl	const c = ch->ctrl;

	if (selfAddr != NULL)
	  *selfAddr = c->self_addr;
	if (peerAddr != NULL)
	  *peerAddr = c->peer_addr;
	if (selfCid != NULL)
	  *selfCid = ch->cid;
	if (peerCid != NULL)
	  *peerCid = ch->peerCid;
	if (peerWin != NULL)
	  *peerWin = ch->recvWin;
	if (peerPpd != NULL)
	  *peerPpd = ch->peerPpd;
	return(0);
     }
    case PPTP_CHAN_ST_FREE:
    case PPTP_CHAN_ST_DYING:
      return(-1);
      break;
    default:
      assert(0);
  }
  return(-1);	/* NOTREACHED */
}

int
PptpCtrlGetSelfName(struct pptpctrlinfo *cp, void *buf, size_t buf_len) {
    PptpChan	const ch = (PptpChan)cp->cookie;
    PptpCtrl	const c = ch->ctrl;
    
    strlcpy(buf, c->self_name, buf_len);
    return (0);
};

int
PptpCtrlGetPeerName(struct pptpctrlinfo *cp, void *buf, size_t buf_len) {
    PptpChan	const ch = (PptpChan)cp->cookie;
    PptpCtrl	const c = ch->ctrl;
    
    strlcpy(buf, c->peer_name, buf_len);
    return (0);
};

/*************************************************************************
			CONTROL CONNECTION SETUP
*************************************************************************/

/*
 * PptpCtrlListenEvent()
 *
 * Someone has connected to our TCP socket on which we were listening.
 */

static void
PptpCtrlListenEvent(int type, void *cookie)
{
    PptpLis const		l = (PptpLis)cookie;
  struct sockaddr_storage	peerst, selfst;
  struct u_addr			peer_addr, self_addr;
  in_port_t			peer_port, self_port;
  char				ebuf[64];
  PptpCtrl			c;
  int				sock;
  char				buf[48], buf2[48];
  socklen_t			addrLen;
  
    /* Accept connection */
    if ((sock = TcpAcceptConnection(l->sock, &peerst, FALSE)) < 0)
	return;
    sockaddrtou_addr(&peerst,&peer_addr,&peer_port);

    /* Get local IP address */
    addrLen = sizeof(selfst);
    if (getsockname(sock, (struct sockaddr *) &selfst, &addrLen) < 0) {
	Log(LG_ERR, ("PPTP: %s getsockname(): %s", __func__, strerror(errno)));
	u_addrclear(&self_addr);
	self_port = 0;
    } else {
	sockaddrtou_addr(&selfst, &self_addr, &self_port);
    }
    Log(LG_PHYS2, ("PPTP: Incoming control connection from %s %u to %s %u",
	u_addrtoa(&peer_addr, buf, sizeof(buf)), peer_port,
	u_addrtoa(&self_addr, buf2, sizeof(buf2)), self_port));

    /* Initialize a new control block */
    if ((c = PptpCtrlGetCtrl(FALSE, &self_addr, &peer_addr, peer_port,
    	    ebuf, sizeof(ebuf))) == NULL) {
	Log(LG_PHYS2, ("PPTP: Control connection failed: %s", ebuf));
	close(sock);
	return;
    }
    c->csock = sock;

    /* Initialize the session */
    PptpCtrlInitCtrl(c, FALSE);
}

/*
 * PptpCtrlConnEvent()
 *
 * We are trying to make a TCP connection to the peer. When this
 * either succeeds or fails, we jump to here.
 */

static void
PptpCtrlConnEvent(int type, void *cookie)
{
  PptpCtrl		const c = (PptpCtrl) cookie;
  struct sockaddr_storage	addr;
  socklen_t		addrLen = sizeof(addr);
  char			buf[48];

  /* Get event */
  assert(c->state == PPTP_CTRL_ST_IDLE);

  /* Check whether the connection was successful or not */
  if (getpeername(c->csock, (struct sockaddr *) &addr, &addrLen) < 0) {
    Log(LG_PHYS2, ("pptp%d: connection to %s %d failed",
      c->id, u_addrtoa(&c->peer_addr,buf,sizeof(buf)), c->peer_port));
    PptpCtrlKillCtrl(c);
    return;
  }

  /* Initialize the session */
  Log(LG_PHYS2, ("pptp%d: connected to %s %u",
    c->id, u_addrtoa(&c->peer_addr,buf,sizeof(buf)), c->peer_port));
  PptpCtrlInitCtrl(c, TRUE);
}

/*
 * PptpCtrlInitCtrl()
 *
 * A TCP connection has just been established. Initialize the
 * control block for this connection and initiate the session.
 */

static void
PptpCtrlInitCtrl(PptpCtrl c, int orig)
{
  struct sockaddr_storage	self, peer;
  static const int	one = 1;
  int			k;
  socklen_t		addrLen;
  char			buf[48];

  /* Good time for a sanity check */
  assert(c->state == PPTP_CTRL_ST_IDLE);
  assert(!c->reps);
  for (k = 0; k < c->numChannels; k++) {
    PptpChan	const ch = c->channels[k];

    assert(ch == NULL || ch->state == PPTP_CHAN_ST_WAIT_CTRL);
  }

  /* Initialize control state */
  c->orig = orig;
  c->echoId = 0;
  c->flen = 0;

  /* Get local IP address */
  addrLen = sizeof(self);
  if (getsockname(c->csock, (struct sockaddr *) &self, &addrLen) < 0) {
    Log(LG_PHYS2, ("pptp%d: %s: %s", c->id, "getsockname", strerror(errno)));
abort:
    PptpCtrlKillCtrl(c);
    return;
  }
  sockaddrtou_addr(&self, &c->self_addr, &c->self_port);

  /* Get remote IP address */
  addrLen = sizeof(peer);
  if (getpeername(c->csock, (struct sockaddr *) &peer, &addrLen) < 0) {
    Log(LG_PHYS2, ("pptp%d: %s: %s", c->id, "getpeername", strerror(errno)));
    goto abort;
  }
  sockaddrtou_addr(&peer, &c->peer_addr, &c->peer_port);

  /* Log which control block */
  Log(LG_PHYS2, ("pptp%d: attached to connection with %s %u",
    c->id, u_addrtoa(&c->peer_addr,buf,sizeof(buf)), c->peer_port));

  /* Turn of Nagle algorithm on the TCP socket, since we are going to
     be writing complete control frames one at a time */
  if (setsockopt(c->csock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
    Log(LG_PHYS2, ("pptp%d: %s: %s", c->id, "setsockopt", strerror(errno)));

  /* Register for events on control and data sockets */
  EventRegister(&c->ctrlEvent, EVENT_READ,
    c->csock, EVENT_RECURRING, PptpCtrlReadCtrl, c);

  /* Start echo keep-alive timer */
  PptpCtrlResetIdleTimer(c);

  /* If we originated the call, we start the conversation */
  if (c->orig) {
    struct pptpStartCtrlConnRequest	msg;

    memset(&msg, 0, sizeof(msg));
    msg.vers = PPTP_PROTO_VERS;
    msg.frameCap = PPTP_FRAMECAP_ANY;
    msg.bearCap = PPTP_BEARCAP_ANY;
    msg.firmware = PPTP_FIRMWARE_REV;
    gethostname(c->self_name, sizeof(c->self_name) - 1);
    c->self_name[sizeof(c->self_name) - 1] = '\0';
    strlcpy(msg.host, c->self_name, sizeof(msg.host));
    strncpy(msg.vendor, MPD_VENDOR, sizeof(msg.vendor));
    PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_WAIT_CTL_REPLY);
    PptpCtrlWriteMsg(c, PPTP_StartCtrlConnRequest, &msg);
  }
}

/*************************************************************************
		CONTROL CONNECTION MESSAGE HANDLING
*************************************************************************/

/*
 * PptpCtrlReadCtrl()
 */

static void
PptpCtrlReadCtrl(int type, void *cookie)
{
  PptpCtrl	const c = (PptpCtrl) cookie;
  PptpMsgHead	const hdr = &c->frame.hdr;
  int		nread;

  /* Figure how much to read and read it */
  nread = (c->flen < sizeof(*hdr) ? sizeof(*hdr) : hdr->length) - c->flen;
  if ((nread = read(c->csock, c->frame.buf + c->flen, nread)) <= 0) {
    if (nread < 0) {
      if (errno == EAGAIN)
	return;
      Log(LG_PHYS2, ("pptp%d: %s: %s", c->id, "read", strerror(errno)));
    } else
      Log(LG_PHYS2, ("pptp%d: ctrl connection closed by peer", c->id));
    goto abort;
  }
  LogDumpBuf(LG_FRAME, c->frame.buf + c->flen, nread,
    "pptp%d: read %d bytes ctrl data", c->id, nread);
  c->flen += nread;

  /* Do whatever with what we got */
  if (c->flen < sizeof(*hdr))			/* incomplete header */
    return;
  if (c->flen == sizeof(*hdr)) {		/* complete header */
    PptpCtrlSwap(0, hdr);		/* byte swap header */
    Log(LG_FRAME, ("pptp%d: got hdr", c->id));
    PptpCtrlDump(LG_FRAME, 0, hdr);
    if (hdr->msgType != PPTP_CTRL_MSG_TYPE) {
      Log(LG_PHYS2, ("pptp%d: invalid msg type %d", c->id, hdr->msgType));
      goto abort;
    }
    if (hdr->magic != PPTP_MAGIC) {
      Log(LG_PHYS2, ("pptp%d: invalid magic %x", c->id, hdr->type));
      goto abort;
    }
    if (!PPTP_VALID_CTRL_TYPE(hdr->type)) {
      Log(LG_PHYS2, ("pptp%d: invalid ctrl type %d", c->id, hdr->type));
      goto abort;
    }
    if (hdr->resv0 != 0) {
      Log(LG_PHYS2, ("pptp%d: non-zero reserved field in header", c->id));
#if 0
      goto abort;
#endif
    }
    if (hdr->length != sizeof(*hdr) + gPptpMsgInfo[hdr->type].length) {
      Log(LG_PHYS2, ("pptp%d: invalid length %d for type %d",
	c->id, hdr->length, hdr->type));
abort:
      PptpCtrlKillCtrl(c);
      return;
    }
    return;
  }
  if (c->flen == hdr->length) {			/* complete message */
    void	*const msg = ((u_char *) hdr) + sizeof(*hdr);

    PptpCtrlSwap(hdr->type, msg);		/* byte swap message */
    Log(LG_PHYS3, ("pptp%d: recv %s", c->id, gPptpMsgInfo[hdr->type].name));
    PptpCtrlDump(LG_PHYS3, hdr->type, msg);
    c->flen = 0;
    PptpCtrlResetIdleTimer(c);
    PptpCtrlMsg(c, hdr->type, msg);
  }
}

/*
 * PptpCtrlMsg()
 *
 * We read a complete control message. Sanity check it and handle it.
 */

static void
PptpCtrlMsg(PptpCtrl c, int type, void *msg)
{
  PptpMsgInfo	const mi = &gPptpMsgInfo[type];
  PptpField	field = gPptpMsgLayout[type];
  PptpChan	ch = NULL;
  PptpPendRep	*pp;
  u_int		off;
  static u_char	zeros[4];

  /* Make sure all reserved fields are zero */
  for (off = 0; field->name; off += field->length, field++) {
    if (!strncmp(field->name, PPTP_RESV_PREF, strlen(PPTP_RESV_PREF))
	&& memcmp((u_char *) msg + off, zeros, field->length)) {
      Log(LG_PHYS2, ("pptp%d: non-zero reserved field %s in %s",
	c->id, field->name, mi->name));
#if 0
      PptpCtrlKillCtrl(c);
      return;
#endif
    }
  }

  /* Find channel this message corresponds to (if any) */
  if (mi->match.inField && !(ch = PptpCtrlFindChan(c, type, msg, TRUE)))
    return;

  /* See if this message qualifies as the reply to a previously sent message */
  for (pp = &c->reps; *pp; pp = &(*pp)->next) {
    if ((*pp)->request->reqrep.reply == type && (*pp)->chan == ch)
      break;
  }

  /* If not, and this message is *always* a reply, ignore it */
  if (*pp == NULL && mi->isReply) {
    Log(LG_PHYS2, ("pptp%d: rec'd spurious %s", c->id, mi->name));
    return;
  }

  /* If so, cancel the matching pending reply */
  if (*pp) {
    PptpPendRep	const prep = *pp;

    TimerStop(&prep->timer);
    *pp = prep->next;
    Freee(prep);
  }

  /* Check for invalid message and call or control state combinations */
  if (!ch && ((1 << c->state) & mi->states) == 0) {
    Log(LG_PHYS2, ("pptp%d: got %s in state %s",
      c->id, gPptpMsgInfo[type].name, gPptpCtrlStates[c->state]));
    PptpCtrlKillCtrl(c);
    return;
  }
  if (ch && ((1 << ch->state) & mi->states) == 0) {
    Log(LG_PHYS2, ("pptp%d-%d: got %s in state %s",
      c->id, ch->id, gPptpMsgInfo[type].name, gPptpChanStates[ch->state]));
    PptpCtrlKillCtrl(c);
    return;
  }

  /* Things look OK; process message */
  (*mi->handler)(ch ? (void *) ch : (void *) c, msg);
}

/*
 * PptpCtrlWriteMsg()
 *
 * Write out a control message. If we should expect a reply,
 * register a matching pending reply for it.
 *
 * NOTE: calling this function can result in the connection being shutdown!
 */

static int
PptpCtrlWriteMsg(PptpCtrl c, int type, void *msg)
{
  PptpMsgInfo		const mi = &gPptpMsgInfo[type];
  union {
      u_char			buf[PPTP_CTRL_MAX_FRAME];
      struct pptpMsgHead	hdr;
  }			frame;
  PptpMsgHead		const hdr = &frame.hdr;
  u_char		*const payload = (u_char *) (hdr + 1);
  const int		totlen = sizeof(*hdr) + gPptpMsgInfo[type].length;
  int			nwrote;

  /* Build message */
  assert(PPTP_VALID_CTRL_TYPE(type));
  memset(hdr, 0, sizeof(*hdr));
  hdr->length = totlen;
  hdr->msgType = PPTP_CTRL_MSG_TYPE;
  hdr->magic = PPTP_MAGIC;
  hdr->type = type;
  memcpy(payload, msg, gPptpMsgInfo[type].length);
  Log(LG_PHYS3, ("pptp%d: send %s msg", c->id, gPptpMsgInfo[hdr->type].name));
  PptpCtrlDump(LG_PHYS3, 0, hdr);
  PptpCtrlDump(LG_PHYS3, type, msg);

  /* Byte swap it */
  PptpCtrlSwap(0, hdr);
  PptpCtrlSwap(type, payload);

  /* Send it; if TCP buffer is full, we abort the connection */
  if ((nwrote = write(c->csock, frame.buf, totlen)) != totlen) {
    if (nwrote < 0)
      Log(LG_PHYS2, ("pptp%d: %s: %s", c->id, "write", strerror(errno)));
    else
      Log(LG_PHYS2, ("pptp%d: only wrote %d/%d", c->id, nwrote, totlen));
    PptpCtrlKillCtrl(c);
    return -1;
  }
  LogDumpBuf(LG_FRAME, frame.buf, totlen, "pptp%d: wrote %d bytes ctrl data", c->id, totlen);

  /* If we expect a reply to this message, start expecting it now */
  if (PPTP_VALID_CTRL_TYPE(mi->reqrep.reply)) {
    PptpPendRep	const prep = Malloc(MB_PPTP, sizeof(*prep));

    prep->ctrl = c;
    prep->chan = PptpCtrlFindChan(c, type, msg, FALSE);
    prep->request = mi;
    TimerInit(&prep->timer, "PptpReply",
      mi->reqrep.timeout * SECONDS, PptpCtrlReplyTimeout, prep);
    TimerStart(&prep->timer);
    prep->next = c->reps;
    c->reps = prep;
  }

  /* Done */
  return 0;
}

/*************************************************************************
		CONTROL AND CHANNEL ALLOCATION FUNCTIONS
*************************************************************************/

/*
 * PptpCtrlGetCtrl()
 *
 * Get existing or create new control bock for given peer and return it.
 * Returns NULL if there was some problem, and puts an error message
 * into the buffer.
 *
 * If "orig" is TRUE, and we currently have no TCP connection to the peer,
 * then initiate one. Otherwise, make sure we don't already have one,
 * because that would mean we'd have two connections to the same peer.
 */

static PptpCtrl
PptpCtrlGetCtrl(int orig, struct u_addr *self_addr,
	struct u_addr *peer_addr, in_port_t peer_port, char *buf, size_t bsiz)
{
    PptpCtrl			c;
    int				k;
    struct sockaddr_storage	peer;
    char			buf1[48];

    /* For incoming any control is new! */
    if (orig) {
	/* See if we're already have a control block matching this address and port */
	  for (k = 0; k < gNumPptpCtrl; k++) {
		PptpCtrl	const c = gPptpCtrl[k];
	
		if (c != NULL
		    && (c->active_sessions < gPPTPtunlimit)
		    && (u_addrcompare(&c->peer_addr, peer_addr) == 0)
		    && (c->peer_port == peer_port || c->orig != orig)
		    && (u_addrempty(self_addr) || 
		      (u_addrcompare(&c->self_addr, self_addr) == 0))) {
			return(c);
		}
	  }
    }

    /* Find/create a free one */
    for (k = 0; k < gNumPptpCtrl && gPptpCtrl[k] != NULL; k++);
    if (k == gNumPptpCtrl)
	LengthenArray(&gPptpCtrl, sizeof(*gPptpCtrl), &gNumPptpCtrl, MB_PPTP);
    c = Malloc(MB_PPTP, sizeof(*c));
    gPptpCtrl[k] = c;

  /* Initialize it */
  c->id = k;
  c->orig = orig;
  c->csock = -1;
  c->self_addr = *self_addr;
  c->peer_addr = *peer_addr;
  c->peer_port = peer_port;
  PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_IDLE);

  /* If not doing the connecting, return here */
  if (!orig)
    return(c);

  /* Connect to peer */
  if ((c->csock = GetInetSocket(SOCK_STREAM, self_addr, 0, FALSE, buf, bsiz)) < 0) {
    gPptpCtrl[k] = NULL;
    PptpCtrlFreeCtrl(c);
    return(NULL);
  }
  u_addrtosockaddr(&c->peer_addr, c->peer_port, &peer);
  if (connect(c->csock, (struct sockaddr *) &peer, peer.ss_len) < 0
      && errno != EINPROGRESS) {
    (void) close(c->csock);
    c->csock = -1;
    snprintf(buf, bsiz, "pptp: connect to %s %u failed: %s",
      u_addrtoa(&c->peer_addr,buf1,sizeof(buf1)), c->peer_port, strerror(errno));
    gPptpCtrl[k] = NULL;
    PptpCtrlFreeCtrl(c);
    return(NULL);
  }

  /* Wait for it to go through */
  EventRegister(&c->connEvent, EVENT_WRITE, c->csock,
    0, PptpCtrlConnEvent, c);
  Log(LG_PHYS2, ("pptp%d: connecting to %s %u",
    c->id, u_addrtoa(&c->peer_addr,buf1,sizeof(buf1)), c->peer_port));
  return(c);
}

/*
 * PptpCtrlGetChan()
 *
 * Find a free data channel and attach it to the control channel.
 */

static PptpChan
PptpCtrlGetChan(PptpCtrl c, int chanState, int orig, int incoming,
	int bearType, int frameType, int minBps, int maxBps,
	const char *callingNum, const char *calledNum, const char *subAddress)
{
    PptpChan	ch;
    int		k;

    TimerStop(&c->killTimer);

    /* Get a free data channel */
    for (k = 0; k < c->numChannels && c->channels[k] != NULL; k++);
    if (k == c->numChannels)
	LengthenArray(&c->channels, sizeof(*c->channels), &c->numChannels, MB_PPTP);
    ch = Malloc(MB_PPTP, sizeof(*ch));
    c->channels[k] = ch;
    c->active_sessions++;
    ch->id = k;
    while (gCallIds[gLastCallId])
	gLastCallId++;
    gCallIds[gLastCallId] = 1;
    ch->cid = gLastCallId;
    ch->ctrl = c;
    ch->orig = orig;
    ch->incoming = incoming;
    ch->minBps = minBps;
    ch->maxBps = maxBps;
    ch->bearType = bearType;
    ch->frameType = frameType;
    strlcpy(ch->calledNum, calledNum, sizeof(ch->calledNum));
    strlcpy(ch->callingNum, callingNum, sizeof(ch->callingNum));
    strlcpy(ch->subAddress, subAddress, sizeof(ch->subAddress));
    PptpCtrlNewChanState(ch, chanState);
    return(ch);
}

/*
 * PptpCtrlDialResult()
 *
 * Link layer calls this to let us know whether an outgoing call
 * has been successfully completed or has failed.
 */

static void
PptpCtrlDialResult(void *cookie, int result, int error, int cause, int speed)
{
  PptpChan			const ch = (PptpChan) cookie;
  PptpCtrl			const c = ch->ctrl;
  struct pptpOutCallReply	rep;

  memset(&rep, 0, sizeof(rep));
  rep.cid = ch->cid;
  rep.peerCid = ch->peerCid;
  rep.result = result;
  if (rep.result == PPTP_OCR_RESL_ERR)
    rep.err = error;
  rep.cause = cause;
  rep.speed = speed;
  rep.ppd = PPTP_PPD;		/* XXX should get this value from link layer */
  rep.recvWin = PPTP_RECV_WIN;	/* XXX */
  rep.channel = PHYS_CHAN(ch);
  if (rep.result == PPTP_OCR_RESL_OK)
    PptpCtrlNewChanState(ch, PPTP_CHAN_ST_ESTABLISHED);
  else
    PptpCtrlKillChan(ch, "local outgoing call failed");
  PptpCtrlWriteMsg(c, PPTP_OutCallReply, &rep);
}

/*
 * PptpCtrlConected()
 *
 * Link layer calls this to let us know whether an incoming call
 * has been successfully connected.
 */

static void
PptpCtrlConected(void *cookie, int speed)
{
  PptpChan			const ch = (PptpChan) cookie;
  PptpCtrl			const c = ch->ctrl;
  struct pptpInCallConn con;
  
  /* Send back connected message */
  memset(&con, 0, sizeof(con));
  con.peerCid = ch->peerCid;
  con.speed = speed;
  con.recvWin = PPTP_RECV_WIN;		/* XXX */
  con.ppd = PPTP_PPD;			/* XXX */
  con.frameType = ch->frameType;
  PptpCtrlWriteMsg(c, PPTP_InCallConn, &con);
}

static void
PptpCtrlSetLinkInfo(void *cookie, u_int32_t sa, u_int32_t ra)
{
  PptpChan			const ch = (PptpChan) cookie;
  PptpCtrl			const c = ch->ctrl;
  struct pptpSetLinkInfo	rep;

  /* Only PNS should send SLI */
  if (!PPTP_CHAN_IS_PNS(ch))
	return;

  memset(&rep, 0, sizeof(rep));
  rep.cid = ch->peerCid;
  rep.sendAccm = sa;
  rep.recvAccm = ra;
  PptpCtrlWriteMsg(c, PPTP_SetLinkInfo, &rep);
}

/*************************************************************************
		    SHUTDOWN FUNCTIONS
*************************************************************************/

/*
 * PptpCtrlCloseCtrl()
 */

static void
PptpCtrlCloseCtrl(PptpCtrl c)
{
  char	buf[48];

  Log(LG_PHYS2, ("pptp%d: closing connection with %s %u",
    c->id, u_addrtoa(&c->peer_addr,buf,sizeof(buf)), c->peer_port));
  switch (c->state) {
    case PPTP_CTRL_ST_IDLE:
    case PPTP_CTRL_ST_WAIT_STOP_REPLY:
    case PPTP_CTRL_ST_WAIT_CTL_REPLY:
      PptpCtrlKillCtrl(c);
      return;
    case PPTP_CTRL_ST_ESTABLISHED:
      {
	struct pptpStopCtrlConnRequest	req;

	memset(&req, 0, sizeof(req));
	req.reason = PPTP_SCCR_REAS_LOCAL;
	PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_WAIT_STOP_REPLY);
	PptpCtrlWriteMsg(c, PPTP_StopCtrlConnRequest, &req);
	return;
      }
      break;
    case PPTP_CTRL_ST_DYING:
      break;
    default:
      assert(0);
  }
}

/*
 * PptpCtrlKillCtrl()
 */

static void
PptpCtrlKillCtrl(PptpCtrl c)
{
  int		k;
  PptpPendRep	prep, next;
  char		buf[48];

  /* Don't recurse */
  assert(c);
  if (c->state == PPTP_CTRL_ST_DYING)
    return;
  PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_DYING);

  /* Do ungraceful shutdown */
  Log(LG_PHYS2, ("pptp%d: killing connection with %s %u",
    c->id, u_addrtoa(&c->peer_addr,buf,sizeof(buf)), c->peer_port));
  for (k = 0; k < c->numChannels; k++) {
    PptpChan	const ch = c->channels[k];

    if (ch != NULL)
      PptpCtrlKillChan(ch, "control channel shutdown");
  }
  gPptpCtrl[c->id] = NULL;
  if (c->csock >= 0) {
    close(c->csock);
    c->csock = -1;
  }
  EventUnRegister(&c->connEvent);
  EventUnRegister(&c->ctrlEvent);
  TimerStop(&c->idleTimer);
  TimerStop(&c->killTimer);
  for (prep = c->reps; prep; prep = next) {
    next = prep->next;
    TimerStop(&prep->timer);
    Freee(prep);
  }
  c->reps = NULL;
  
  /* Delay Free call to avoid "Modify after free" case */
  TimerInit(&c->killTimer, "PptpKill", 0, (void (*)(void *))PptpCtrlFreeCtrl, c);
  TimerStart(&c->killTimer);
}

static void
PptpCtrlFreeCtrl(PptpCtrl c)
{
    Freee(c->channels);
    memset(c, 0, sizeof(*c));
    Freee(c);
}

/*
 * PptpCtrlCloseChan()
 *
 * Gracefully clear a call.
 */

static void
PptpCtrlCloseChan(PptpChan ch, int result, int error, int cause)
{
  PptpCtrl	const c = ch->ctrl;

  /* Check call state */
  switch (ch->state) {
    case PPTP_CHAN_ST_ESTABLISHED:
      if (PPTP_CHAN_IS_PNS(ch))
	goto pnsClear;
      else
	goto pacClear;
      break;
    case PPTP_CHAN_ST_WAIT_ANSWER:
      {
	struct pptpOutCallReply	reply;

	memset(&reply, 0, sizeof(reply));
	reply.peerCid = ch->peerCid;
	reply.result = PPTP_OCR_RESL_ADMIN;
	PptpCtrlWriteMsg(c, PPTP_OutCallReply, &reply);
	PptpCtrlKillChan(ch, "link layer shutdown");	/* XXX errmsg */
	return;
      }
      break;
    case PPTP_CHAN_ST_WAIT_IN_REPLY:		/* we are the PAC */
pacClear:
      {
	struct pptpCallDiscNotify	disc;

	Log(LG_PHYS2, ("pptp%d-%d: clearing call", c->id, ch->id));
	memset(&disc, 0, sizeof(disc));
	disc.cid = ch->cid;
	disc.result = result;
	if (disc.result == PPTP_CDN_RESL_ERR)
	  disc.err = error;
	disc.cause = cause;
	/* XXX stats? */
	PptpCtrlWriteMsg(c, PPTP_CallDiscNotify, &disc);
	PptpCtrlKillChan(ch, "link layer shutdown");	/* XXX errmsg */
      }
      break;
    case PPTP_CHAN_ST_WAIT_OUT_REPLY:		/* we are the PNS */
    case PPTP_CHAN_ST_WAIT_CONNECT:		/* we are the PNS */
pnsClear:
      {
	struct pptpCallClearRequest	req;

	Log(LG_PHYS2, ("pptp%d-%d: clearing call", c->id, ch->id));
	memset(&req, 0, sizeof(req));
	req.cid = ch->cid;
	PptpCtrlWriteMsg(c, PPTP_CallClearRequest, &req);
	PptpCtrlNewChanState(ch, PPTP_CHAN_ST_WAIT_DISCONNECT);
      }
      break;
    case PPTP_CHAN_ST_WAIT_DISCONNECT:		/* call was already cleared */
      return;
    case PPTP_CHAN_ST_WAIT_CTRL:
      PptpCtrlKillChan(ch, "link layer shutdown");
      return;
    case PPTP_CHAN_ST_DYING:
      break;
    default:
      assert(0);
  }
}

/*
 * PptpCtrlKillChan()
 */

static void
PptpCtrlKillChan(PptpChan ch, const char *errmsg)
{
  PptpCtrl	const c = ch->ctrl;
  PptpPendRep	*pp;

  assert(ch);
  /* Don't recurse */
  if (ch->state == PPTP_CHAN_ST_DYING)
    return;

  /* If link layer needs notification, tell it */
  Log(LG_PHYS2, ("pptp%d-%d: killing channel", c->id, ch->id));
  switch (ch->state) {
    case PPTP_CHAN_ST_WAIT_IN_REPLY:
    case PPTP_CHAN_ST_WAIT_OUT_REPLY:
    case PPTP_CHAN_ST_WAIT_CONNECT:
    case PPTP_CHAN_ST_ESTABLISHED:
    case PPTP_CHAN_ST_WAIT_CTRL:
    case PPTP_CHAN_ST_WAIT_DISCONNECT:
	if (ch->linfo.cookie != NULL)
    	    (*ch->linfo.result)(ch->linfo.cookie, errmsg, 0);
      break;
    case PPTP_CHAN_ST_WAIT_ANSWER:
	if (ch->linfo.cookie != NULL)
    	    (*ch->linfo.cancel)(ch->linfo.cookie);
      break;
    case PPTP_CHAN_ST_DYING:				/* should never happen */
    default:
      assert(0);
  }
  PptpCtrlNewChanState(ch, PPTP_CHAN_ST_DYING);

  /* Nuke any pending replies pertaining to this channel */
  for (pp = &c->reps; *pp; ) {
    PptpPendRep	const prep = *pp;

    if (prep->chan == ch) {
      TimerStop(&prep->timer);
      *pp = prep->next;
      Freee(prep);
    } else
      pp = &prep->next;
  }

    /* Free channel */
    gCallIds[ch->cid] = 0;
    c->channels[ch->id] = NULL;
    c->active_sessions--;
    /* Delay Free call to avoid "Modify after free" case */
    TimerInit(&ch->killTimer, "PptpKillCh", 0, (void (*)(void *))PptpCtrlFreeChan, ch);
    TimerStart(&ch->killTimer);

    /* When the last channel is closed, close the control channel too. */
    if (c->active_sessions == 0 && c->state <= PPTP_CTRL_ST_ESTABLISHED) {
	/* Delay control close as it may be be needed soon */
	TimerInit(&c->killTimer, "PptpUnused", gPPTPto * SECONDS,
	    (void (*)(void *))PptpCtrlCloseCtrl, c);
	TimerStart(&c->killTimer);
    }
}

static void
PptpCtrlFreeChan(PptpChan ch)
{
    memset(ch, 0, sizeof(*ch));
    Freee(ch);
}

/*************************************************************************
		    TIMER RELATED FUNCTIONS
*************************************************************************/

/*
 * PptpCtrlReplyTimeout()
 */

static void
PptpCtrlReplyTimeout(void *arg)
{
  PptpPendRep	const prep = (PptpPendRep) arg;
  PptpPendRep	*pp;
  PptpChan	const ch = prep->chan;
  PptpCtrl	const c = prep->ctrl;

  /* Log it */
  if (ch) {
    Log(LG_PHYS2, ("pptp%d-%d: no reply to %s after %d sec",
      c->id, ch->id, prep->request->name, prep->request->reqrep.timeout));
  } else {
    Log(LG_PHYS2, ("pptp%d: no reply to %s after %d sec",
      c->id, prep->request->name, prep->request->reqrep.timeout));
  }

  /* Unlink pending reply */
  for (pp = &c->reps; *pp != prep; pp = &(*pp)->next);
  assert(*pp);
  *pp = prep->next;

  /* Either close this channel or kill entire control connection */
  if (prep->request->reqrep.killCtrl)
    PptpCtrlKillCtrl(c);
  else
    PptpCtrlCloseChan(ch, PPTP_CDN_RESL_ERR, PPTP_ERROR_PAC_ERROR, 0);

  /* Done */
  Freee(prep);
}

/*
 * PptpCtrlIdleTimeout()
 *
 * We've heard PPTP_IDLE_TIMEOUT seconds of silence from the peer.
 * Send an echo request to make sure it's alive.
 */

static void
PptpCtrlIdleTimeout(void *arg)
{
  PptpCtrl			const c = (PptpCtrl) arg;
  struct pptpEchoRequest	msg;

  /* Send echo request */
  memset(&msg, 0, sizeof(msg));
  msg.id = ++c->echoId;
  PptpCtrlWriteMsg(c, PPTP_EchoRequest, &msg);
}

/*
 * PptpCtrlResetIdleTimer()
 *
 * Reset the idle timer back up to PPTP_IDLE_TIMEOUT seconds.
 */

static void
PptpCtrlResetIdleTimer(PptpCtrl c)
{
  TimerStop(&c->idleTimer);
  TimerInit(&c->idleTimer, "PptpIdle",
    PPTP_IDLE_TIMEOUT * SECONDS, PptpCtrlIdleTimeout, c);
  TimerStart(&c->idleTimer);
}

/*************************************************************************
		    CHANNEL MAINTENANCE FUNCTIONS
*************************************************************************/

/*
 * PptpCtrlCheckConn()
 *
 * Check whether we have any pending connection requests, and whether
 * we can send them yet, or what.
 */

static void
PptpCtrlCheckConn(PptpCtrl c)
{
  int	k;

  switch (c->state) {
    case PPTP_CTRL_ST_IDLE:
    case PPTP_CTRL_ST_WAIT_CTL_REPLY:
    case PPTP_CTRL_ST_WAIT_STOP_REPLY:
      break;
    case PPTP_CTRL_ST_ESTABLISHED:
      for (k = 0; k < c->numChannels; k++) {
	PptpChan	const ch = c->channels[k];

	if (ch == NULL || ch->state != PPTP_CHAN_ST_WAIT_CTRL)
	  continue;
	if (ch->incoming) {
	  struct pptpInCallRequest	req;

	  memset(&req, 0, sizeof(req));
	  req.cid = ch->cid;
	  req.serno = req.cid;
	  req.bearType = PPTP_BEARCAP_DIGITAL;
	  req.channel = PHYS_CHAN(ch);
	  req.dialingLen = strlen(ch->callingNum);
	  strncpy(req.dialing, ch->callingNum, sizeof(req.dialing));
	  req.dialedLen = strlen(ch->calledNum);
	  strncpy(req.dialed, ch->calledNum, sizeof(req.dialed));
	  strncpy(req.subaddr, ch->subAddress, sizeof(req.subaddr));
	  PptpCtrlNewChanState(ch, PPTP_CHAN_ST_WAIT_IN_REPLY);
	  PptpCtrlWriteMsg(c, PPTP_InCallRequest, &req);
	} else {
	  struct pptpOutCallRequest	req;

	  memset(&req, 0, sizeof(req));
	  req.cid = ch->cid;
	  req.serno = req.cid;
	  req.minBps = ch->minBps;
	  req.maxBps = ch->maxBps;
	  req.bearType = ch->bearType;
	  req.frameType = ch->frameType;
	  req.recvWin = PPTP_RECV_WIN;		/* XXX */
	  req.ppd = PPTP_PPD;			/* XXX */
	  req.numLen = strlen(ch->calledNum);
	  strncpy(req.phone, ch->calledNum, sizeof(req.phone));
	  strncpy(req.subaddr, ch->subAddress, sizeof(req.subaddr));
	  PptpCtrlNewChanState(ch, PPTP_CHAN_ST_WAIT_OUT_REPLY);
	  PptpCtrlWriteMsg(c, PPTP_OutCallRequest, &req);
	}
      }
      break;
    case PPTP_CTRL_ST_DYING:
      break;
    default:
      assert(0);
  }
}

/*
 * PptpCtrlFindChan()
 *
 * Find the channel identified by this message. Returns NULL if
 * the message is not channel specific, or the channel was not found.
 */

static PptpChan
PptpCtrlFindChan(PptpCtrl c, int type, void *msg, int incoming)
{
  PptpMsgInfo	const mi = &gPptpMsgInfo[type];
  const char	*fname = incoming ? mi->match.inField : mi->match.outField;
  const int	how = incoming ? mi->match.findIn : mi->match.findOut;
  u_int16_t	cid;
  int		k;
  u_int		off = 0;

  /* Get the identifying CID field */
  if (!fname)
    return(NULL);
  (void) PptpCtrlFindField(type, fname, &off);		/* we know len == 2 */
  cid = *((u_int16_t *)(void *)((u_char *) msg + off));

  /* Match the CID against our list of active channels */
  for (k = 0; k < c->numChannels; k++) {
    PptpChan	const ch = c->channels[k];
    u_int16_t	tryCid = 0;

    if (ch == NULL)
      continue;
    switch (how) {
      case PPTP_FIND_CHAN_MY_CID:
	tryCid = ch->cid;
	break;
      case PPTP_FIND_CHAN_PEER_CID:
	tryCid = ch->peerCid;
	break;
      case PPTP_FIND_CHAN_PNS_CID:
	tryCid = PPTP_CHAN_IS_PNS(ch) ? ch->cid : ch->peerCid;
	break;
      case PPTP_FIND_CHAN_PAC_CID:
	tryCid = !PPTP_CHAN_IS_PNS(ch) ? ch->cid : ch->peerCid;
	break;
      default:
	assert(0);
    }
    if (cid == tryCid)
      return(ch);
  }

  /* Not found */
  Log(LG_PHYS2, ("pptp%d: CID 0x%04x in %s not found", c->id, cid, mi->name));
  return(NULL);
}

/*************************************************************************
			  MISC FUNCTIONS
*************************************************************************/

/*
 * PptpCtrlNewCtrlState()
 */

static void
PptpCtrlNewCtrlState(PptpCtrl c, int new)
{
  Log(LG_PHYS3, ("pptp%d: ctrl state %s --> %s",
    c->id, gPptpCtrlStates[c->state], gPptpCtrlStates[new]));
  assert(c->state != new);
  c->state = new;
}

/*
 * PptpCtrlNewChanState()
 */

static void
PptpCtrlNewChanState(PptpChan ch, int new)
{
  PptpCtrl	const c = ch->ctrl;

  Log(LG_PHYS3, ("pptp%d-%d: chan state %s --> %s",
    c->id, ch->id, gPptpChanStates[ch->state], gPptpChanStates[new]));
  assert(ch->state != new);
  ch->state = new;
}

/*
 * PptpCtrlSwap()
 *
 * Byteswap message between host order <--> network order. Note:
 * this relies on the fact that ntohs == htons and ntohl == htonl.
 */

static void
PptpCtrlSwap(int type, void *buf)
{
  PptpField	field = gPptpMsgLayout[type];
  int		off;

  for (off = 0; field->name; off += field->length, field++) {
    void *const ptr = (u_char *)buf + off;

    switch (field->length) {
      case 4:
	{
	  u_int32_t value;

	  memcpy(&value, ptr, field->length);
	  value = ntohl(value);
	  memcpy(ptr, &value, field->length);
	}
	break;
      case 2:
	{
	  u_int16_t value;

	  memcpy(&value, ptr, field->length);
	  value = ntohs(value);
	  memcpy(ptr, &value, field->length);
	}
	break;
    }
  }
}

/*
 * PptpCtrlDump()
 *
 * Debugging display of a control message.
 */

#define DUMP_DING	65
#define DUMP_MAX_DEC	100
#define DUMP_MAX_BUF	200

static void
PptpCtrlDump(int level, int type, void *msg)
{
  PptpField	field = gPptpMsgLayout[type];
  char		line[DUMP_MAX_BUF];
  int		off;

  if (!(gLogOptions & level))
    return;
  for (*line = off = 0; field->name; off += field->length, field++) {
    u_char	*data = (u_char *) msg + off;
    char	buf[DUMP_MAX_BUF];

    if (!strncmp(field->name, PPTP_RESV_PREF, strlen(PPTP_RESV_PREF)))
      continue;
    snprintf(buf, sizeof(buf), " %s=", field->name);
    switch (field->length) {
      case 4:
      {
	u_int32_t value;

	memcpy(&value, data, field->length);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
	  (value <= DUMP_MAX_DEC) ? "%d" : "0x%x", value);
	break;
      }
      case 2:
      {
	u_int16_t value;

	memcpy(&value, data, field->length);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
	  (value <= DUMP_MAX_DEC) ? "%d" : "0x%x", value);
	break;
      }
      case 1:
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
	  "%d", *((u_int8_t *) data));
	break;
      default:
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
	  "\"%s\"", (char *) data);
	break;
    }
    if (*line && strlen(line) + strlen(buf) > DUMP_DING) {
      Log(level, (" %s", line));
      *line = 0;
    }
    strlcpy(line + strlen(line), buf, sizeof(line) - strlen(line));
  }
  if (*line)
    Log(level, (" %s", line));
}

/*
 * PptpCtrlFindField()
 *
 * Locate a field in a structure, returning length and offset (*offset).
 * Die if field was not found.
 */

static int
PptpCtrlFindField(int type, const char *name, u_int *offp)
{
  PptpField	field = gPptpMsgLayout[type];
  u_int		off;

  for (off = 0; field->name; off += field->length, field++) {
    if (!strcmp(field->name, name)) {
      *offp = off;
      return(field->length);
    }
  }
  assert(0);
  return(0);
}

/*
 * PptpCtrlInitCinfo()
 */

static void
PptpCtrlInitCinfo(PptpChan ch, PptpCtrlInfo ci)
{
  PptpCtrl	const c = ch->ctrl;

  memset(ci, 0, sizeof(*ci));
  ci->cookie = ch;
  ci->peer_addr = c->peer_addr;
  ci->peer_port = c->peer_port;
  ci->close =(void (*)(void *, int, int, int)) PptpCtrlCloseChan;
  ci->answer = (void (*)(void *, int, int, int, int)) PptpCtrlDialResult;
  ci->connected = PptpCtrlConected;
  ci->setLinkInfo = PptpCtrlSetLinkInfo;
}

/*************************************************************************
		      CONTROL MESSAGE FUNCTIONS
*************************************************************************/

/*
 * PptpStartCtrlConnRequest()
 */

static void
PptpStartCtrlConnRequest(PptpCtrl c, struct pptpStartCtrlConnRequest *req)
{
    struct pptpStartCtrlConnReply	reply;

    /* Initialize reply */
    memset(&reply, 0, sizeof(reply));
    reply.vers = PPTP_PROTO_VERS;
    reply.frameCap = PPTP_FRAMECAP_ANY;
    reply.bearCap = PPTP_BEARCAP_ANY;
    reply.firmware = PPTP_FIRMWARE_REV;
    reply.result = PPTP_SCCR_RESL_OK;
    gethostname(c->self_name, sizeof(c->self_name) - 1);
    c->self_name[sizeof(c->self_name) - 1] = '\0';
    strlcpy(reply.host, c->self_name, sizeof(reply.host));
    strncpy(reply.vendor, MPD_VENDOR, sizeof(reply.vendor));

#ifdef LOOK_LIKE_NT
    reply.firmware = 0;
    reply.maxChan = 2;
    memset(reply.host, 0, sizeof(reply.host));
    snprintf(reply.host, sizeof(reply.host), "local");
    memset(reply.vendor, 0, sizeof(reply.vendor));
    snprintf(reply.vendor, sizeof(reply.vendor), "NT");
#endif

    /* Check protocol version */
    if (req->vers != PPTP_PROTO_VERS) {
	Log(LG_PHYS2, ("pptp%d: incompatible protocol 0x%04x", c->id, req->vers));
	reply.result = PPTP_SCCR_RESL_VERS;
	if (PptpCtrlWriteMsg(c, PPTP_StartCtrlConnReply, &reply) == -1)
	    return;
	PptpCtrlKillCtrl(c);
	return;
    }

    strlcpy(c->peer_name, req->host, sizeof(c->peer_name));

    /* OK */
    PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_ESTABLISHED);
    PptpCtrlWriteMsg(c, PPTP_StartCtrlConnReply, &reply);
}

/*
 * PptpStartCtrlConnReply()
 */

static void
PptpStartCtrlConnReply(PptpCtrl c, struct pptpStartCtrlConnReply *rep)
{

    /* Is peer happy? */
    if (rep->result != 0 && rep->result != PPTP_SCCR_RESL_OK) {
	Log(LG_PHYS2, ("pptp%d: my %s failed, result=%s err=%s",
    	    c->id, gPptpMsgInfo[PPTP_StartCtrlConnRequest].name,
    	    PPTP_SCCR_RESL_CODE(rep->result), PPTP_ERROR_CODE(rep->err)));
	PptpCtrlKillCtrl(c);
	return;
    }

    /* Check peer's protocol version */
    if (rep->vers != PPTP_PROTO_VERS) {
	struct pptpStopCtrlConnRequest	req;

	Log(LG_PHYS2, ("pptp%d: incompatible protocol 0x%04x", c->id, rep->vers));
	memset(&req, 0, sizeof(req));
	req.reason = PPTP_SCCR_REAS_PROTO;
	PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_WAIT_STOP_REPLY);
	PptpCtrlWriteMsg(c, PPTP_StopCtrlConnRequest, &req);
	return;
    }

    strlcpy(c->peer_name, rep->host, sizeof(c->peer_name));

    /* OK */
    PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_ESTABLISHED);
    PptpCtrlCheckConn(c);
}

/*
 * PptpStopCtrlConnRequest()
 */

static void
PptpStopCtrlConnRequest(PptpCtrl c, struct pptpStopCtrlConnRequest *req)
{
  struct pptpStopCtrlConnReply	rep;

  Log(LG_PHYS2, ("pptp%d: got %s: reason=%s",
    c->id, gPptpMsgInfo[PPTP_StopCtrlConnRequest].name,
    PPTP_SCCR_REAS_CODE(req->reason)));
  memset(&rep, 0, sizeof(rep));
  rep.result = PPTP_SCCR_RESL_OK;
  PptpCtrlNewCtrlState(c, PPTP_CTRL_ST_IDLE);
  if (PptpCtrlWriteMsg(c, PPTP_StopCtrlConnReply, &rep) == -1)
    return;
  PptpCtrlKillCtrl(c);
}

/*
 * PptpStopCtrlConnReply()
 */

static void
PptpStopCtrlConnReply(PptpCtrl c, struct pptpStopCtrlConnReply *rep)
{
  PptpCtrlKillCtrl(c);
}

/*
 * PptpEchoRequest()
 */

static void
PptpEchoRequest(PptpCtrl c, struct pptpEchoRequest *req)
{
  struct pptpEchoReply	reply;

  memset(&reply, 0, sizeof(reply));
  reply.id = req->id;
  reply.result = PPTP_ECHO_RESL_OK;
  PptpCtrlWriteMsg(c, PPTP_EchoReply, &reply);
}

/*
 * PptpEchoReply()
 */

static void
PptpEchoReply(PptpCtrl c, struct pptpEchoReply *rep)
{
  if (rep->result != PPTP_ECHO_RESL_OK) {
    Log(LG_PHYS2, ("pptp%d: echo reply failed: res=%s err=%s",
      c->id, PPTP_ECHO_RESL_CODE(rep->result), PPTP_ERROR_CODE(rep->err)));
    PptpCtrlKillCtrl(c);
  } else if (rep->id != c->echoId) {
    Log(LG_PHYS2, ("pptp%d: bogus echo reply: %u != %u",
      c->id, rep->id, c->echoId));
    PptpCtrlKillCtrl(c);
  }
}

/*
 * PptpOutCallRequest()
 */

static void
PptpOutCallRequest(PptpCtrl c, struct pptpOutCallRequest *req)
{
  struct pptpOutCallReply	reply;
  struct pptpctrlinfo		cinfo;
  struct pptplinkinfo		linfo;
  PptpChan			ch = NULL;
  char				calledNum[PPTP_PHONE_LEN + 1];
  char				subAddress[PPTP_SUBADDR_LEN + 1];

  /* Does link allow this? */
  if (gGetOutLink == NULL)
    goto denied;

  /* Copy out fields */
  strncpy(calledNum, req->phone, sizeof(calledNum) - 1);
  calledNum[sizeof(calledNum) - 1] = 0;
  strncpy(subAddress, req->subaddr, sizeof(subAddress) - 1);
  subAddress[sizeof(subAddress) - 1] = 0;

  /* Get a data channel */
  if ((ch = PptpCtrlGetChan(c, PPTP_CHAN_ST_WAIT_ANSWER, FALSE, FALSE,
      req->bearType, req->frameType, req->minBps, req->maxBps,
      PPTP_STR_INTERNAL_CALLING, calledNum, subAddress)) == NULL) {
    Log(LG_PHYS2, ("pptp%d: no free channels for outgoing call", c->id));
    goto chFail;
  }

  /* Ask link layer about making the outgoing call */
  PptpCtrlInitCinfo(ch, &cinfo);
  linfo = (*gGetOutLink)(&cinfo, &c->self_addr, &c->peer_addr, c->peer_port, req->bearType,
    req->frameType, req->minBps, req->maxBps, calledNum, subAddress);
  if (linfo.cookie == NULL)
    goto denied;

  /* Link layer says it's OK; wait for link layer to report back later */
  ch->serno = req->serno;
  ch->peerCid = req->cid;
  ch->peerPpd = req->ppd;
  ch->recvWin = req->recvWin;
  ch->linfo = linfo;
  return;

  /* Failed */
denied:
  Log(LG_PHYS2, ("pptp%d: peer's outgoing call request denied", c->id));
  if (ch)
    PptpCtrlKillChan(ch, "peer's outgoing call request denied");
chFail:
  memset(&reply, 0, sizeof(reply));
  reply.peerCid = req->cid;
  reply.result = PPTP_OCR_RESL_ADMIN;
  PptpCtrlWriteMsg(c, PPTP_OutCallReply, &reply);
}

/*
 * PptpOutCallReply()
 */

static void
PptpOutCallReply(PptpChan ch, struct pptpOutCallReply *reply)
{
  PptpCtrl	const c = ch->ctrl;

  /* Did call go through? */
  if (reply->result != PPTP_OCR_RESL_OK) {
    char	errmsg[100];

    snprintf(errmsg, sizeof(errmsg),
      "pptp%d-%d: outgoing call failed: res=%s err=%s",
      c->id, ch->id, PPTP_OCR_RESL_CODE(reply->result),
      PPTP_ERROR_CODE(reply->err));
    Log(LG_PHYS2, ("%s", errmsg));
    (*ch->linfo.result)(ch->linfo.cookie, errmsg, 0);
    PptpCtrlKillChan(ch, "remote outgoing call failed");
    return;
  }

  /* Call succeeded */
  ch->peerPpd = reply->ppd;
  ch->recvWin = reply->recvWin;
  ch->peerCid = reply->cid;
  Log(LG_PHYS2, ("pptp%d-%d: outgoing call connected at %d bps",
    c->id, ch->id, reply->speed));
  PptpCtrlNewChanState(ch, PPTP_CHAN_ST_ESTABLISHED);
  (*ch->linfo.result)(ch->linfo.cookie, NULL, ch->frameType);
}

/*
 * PptpInCallRequest()
 */

static void
PptpInCallRequest(PptpCtrl c, struct pptpInCallRequest *req)
{
  struct pptpInCallReply	reply;
  struct pptpctrlinfo		cinfo;
  struct pptplinkinfo		linfo;
  PptpChan			ch;
  char				callingNum[PPTP_PHONE_LEN + 1];
  char				calledNum[PPTP_PHONE_LEN + 1];
  char				subAddress[PPTP_SUBADDR_LEN + 1];

  /* Copy out fields */
  if (req->dialedLen > PPTP_PHONE_LEN)
    req->dialedLen = PPTP_PHONE_LEN;
  if (req->dialingLen > PPTP_PHONE_LEN)
    req->dialingLen = PPTP_PHONE_LEN;
  strncpy(callingNum, req->dialing, sizeof(callingNum) - 1);
  callingNum[req->dialingLen] = 0;
  strncpy(calledNum, req->dialed, sizeof(calledNum) - 1);
  calledNum[req->dialedLen] = 0;
  strncpy(subAddress, req->subaddr, sizeof(subAddress) - 1);
  subAddress[sizeof(subAddress) - 1] = 0;

  Log(LG_PHYS2, ("pptp%d: peer incoming call to \"%s\" from \"%s\"",
    c->id, calledNum, callingNum));

  /* Initialize reply */
  memset(&reply, 0, sizeof(reply));
  reply.peerCid = req->cid;
  reply.recvWin = PPTP_RECV_WIN;	/* XXX */
  reply.ppd = PPTP_PPD;			/* XXX */

  /* Get a data channel */
  if ((ch = PptpCtrlGetChan(c, PPTP_CHAN_ST_WAIT_CONNECT, FALSE, TRUE,
      req->bearType, 0, 0, INT_MAX,
      callingNum, calledNum, subAddress)) == NULL) {
    Log(LG_PHYS2, ("pptp%d: no free channels for incoming call", c->id));
    reply.result = PPTP_ICR_RESL_ERR;
    reply.err = PPTP_ERROR_NO_RESOURCE;
    goto done;
  }
  reply.cid = ch->cid;

  /* Ask link layer about accepting the incoming call */
  PptpCtrlInitCinfo(ch, &cinfo);
  linfo.cookie = NULL;
  linfo.result = NULL;
  linfo.setLinkInfo = NULL;
  linfo.cancel = NULL;
  if (gGetInLink)
    linfo = (*gGetInLink)(&cinfo, &c->self_addr, &c->peer_addr, c->peer_port,
      req->bearType, callingNum, calledNum, subAddress);
  ch->linfo = linfo;
  if (linfo.cookie == NULL) {
    Log(LG_PHYS2, ("pptp%d: incoming call request denied", c->id));
    reply.result = PPTP_ICR_RESL_NAK;
    goto done;
  }

  /* Link layer says it's OK */
  Log(LG_PHYS2, ("pptp%d-%d: accepting incoming call to \"%s\" from \"%s\"",
    c->id, ch->id, calledNum, callingNum));
  reply.result = PPTP_ICR_RESL_OK;
  ch->serno = req->serno;
  ch->peerCid = req->cid;
  ch->bearType = req->bearType;
  strncpy(ch->callingNum, req->dialing, sizeof(ch->callingNum));
  strncpy(ch->calledNum, req->dialed, sizeof(ch->calledNum));
  strncpy(ch->subAddress, req->subaddr, sizeof(ch->subAddress));

  /* Return result */
done:
  if (PptpCtrlWriteMsg(c, PPTP_InCallReply, &reply) == -1)
    return;
  if (reply.result != PPTP_ICR_RESL_OK)
    PptpCtrlKillChan(ch, "peer incoming call failed");
}

/*
 * PptpInCallReply()
 */

static void
PptpInCallReply(PptpChan ch, struct pptpInCallReply *reply)
{
  PptpCtrl		const c = ch->ctrl;

  /* Did call go through? */
  if (reply->result != PPTP_ICR_RESL_OK) {
    char	errmsg[100];

    snprintf(errmsg, sizeof(errmsg),
      "pptp%d-%d: peer denied incoming call: res=%s err=%s",
      c->id, ch->id, PPTP_ICR_RESL_CODE(reply->result),
      PPTP_ERROR_CODE(reply->err));
    Log(LG_PHYS2, ("%s", errmsg));
    (*ch->linfo.result)(ch->linfo.cookie, errmsg, 0);
    PptpCtrlKillChan(ch, "peer denied incoming call");
    return;
  }

  /* Call succeeded */
  Log(LG_PHYS2, ("pptp%d-%d: incoming call accepted by peer", c->id, ch->id));
  ch->peerCid = reply->cid;
  ch->peerPpd = reply->ppd;
  ch->recvWin = reply->recvWin;
  PptpCtrlNewChanState(ch, PPTP_CHAN_ST_ESTABLISHED);
  (*ch->linfo.result)(ch->linfo.cookie, NULL, ch->frameType);
}

/*
 * PptpInCallConn()
 */

static void
PptpInCallConn(PptpChan ch, struct pptpInCallConn *con)
{
  PptpCtrl	const c = ch->ctrl;

  Log(LG_PHYS2, ("pptp%d-%d: peer incoming call connected at %d bps",
    c->id, ch->id, con->speed));
  ch->peerPpd = con->ppd;
  ch->recvWin = con->recvWin;
  ch->frameType = con->frameType;
  (*ch->linfo.result)(ch->linfo.cookie, NULL, ch->frameType);
  PptpCtrlNewChanState(ch, PPTP_CHAN_ST_ESTABLISHED);
}

/*
 * PptpCallClearRequest()
 */

static void
PptpCallClearRequest(PptpChan ch, struct pptpCallClearRequest *req)
{
  struct pptpCallDiscNotify	notify;
  PptpCtrl			const c = ch->ctrl;

  if (PPTP_CHAN_IS_PNS(ch)) {
    Log(LG_PHYS2, ("pptp%d-%d: got %s, but we are PNS for this call",
      c->id, ch->id, gPptpMsgInfo[PPTP_CallClearRequest].name));
    PptpCtrlKillCtrl(c);
    return;
  }
  Log(LG_PHYS2, ("pptp%d-%d: call cleared by peer", c->id, ch->id));
  memset(&notify, 0, sizeof(notify));
  notify.cid = ch->cid;			/* we are the PAC, use our CID */
  notify.result = PPTP_CDN_RESL_REQ;
  /* XXX stats? */
  PptpCtrlKillChan(ch, "cleared by peer");
  PptpCtrlWriteMsg(c, PPTP_CallDiscNotify, &notify);
}

/*
 * PptpCallDiscNotify()
 */

static void
PptpCallDiscNotify(PptpChan ch, struct pptpCallDiscNotify *notify)
{
  PptpCtrl	const c = ch->ctrl;

  Log(LG_PHYS2, ("pptp%d-%d: peer call disconnected res=%s err=%s",
    c->id, ch->id, PPTP_CDN_RESL_CODE(notify->result),
    PPTP_ERROR_CODE(notify->err)));
  PptpCtrlKillChan(ch, "disconnected by peer");
}

/*
 * PptpWanErrorNotify()
 */

static void
PptpWanErrorNotify(PptpChan ch, struct pptpWanErrorNotify *notif)
{
  PptpCtrl	const c = ch->ctrl;

  Log(LG_PHYS2, ("pptp%d-%d: ignoring %s",
    c->id, ch->id, gPptpMsgInfo[PPTP_WanErrorNotify].name));
}

/*
 * PptpSetLinkInfo()
 */

static void
PptpSetLinkInfo(PptpChan ch, struct pptpSetLinkInfo *info)
{
  PptpCtrl	const c = ch->ctrl;

  if (ch->linfo.setLinkInfo)
    (*ch->linfo.setLinkInfo)(ch->linfo.cookie, info->sendAccm, info->recvAccm);
  else {
    Log(LG_PHYS2, ("pptp%d-%d: ignoring %s",
      c->id, ch->id, gPptpMsgInfo[PPTP_SetLinkInfo].name));
  }
}

/*
 * PptpsStat()
 */

int
PptpsStat(Context ctx, int ac, char *av[], void *arg)
{
    int		k;
    char	buf1[48], buf2[48];

    Printf("Active PPTP tunnels:\r\n");
    for (k = 0; k < gNumPptpCtrl; k++) {
	PptpCtrl        const c = gPptpCtrl[k];
	if (c) {

	    u_addrtoa(&c->self_addr, buf1, sizeof(buf1));
	    u_addrtoa(&c->peer_addr, buf2, sizeof(buf2));
	    Printf("pptp%d\t %s %d <=> %s %d\t%s\t%d calls\r\n",
    		c->id, buf1, c->self_port, buf2, c->peer_port,
		gPptpCtrlStates[c->state], c->active_sessions);
	}
    }

    return 0;
}
