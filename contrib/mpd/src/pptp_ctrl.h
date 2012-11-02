
/*
 * pptp_ctrl.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#if !defined(_PPTP_CTRL_H_) || defined(_WANT_PPTP_FIELDS)
#ifndef _WANT_PPTP_FIELDS
#define	_PPTP_CTRL_H_

/*
 * DEFINITIONS
 */

  /* Definitions per the spec */
  #define PPTP_PORT		1723
  #define PPTP_MTU		1532
  #define PPTP_PROTO_VERS	0x0100
  #define PPTP_MAGIC		0x1a2b3c4d
  #define PPTP_IDLE_TIMEOUT	60

  #define PPTP_HOSTNAME_LEN	64
  #define PPTP_VENDOR_LEN	64
  #define PPTP_PHONE_LEN	64
  #define PPTP_SUBADDR_LEN	64
  #define PPTP_STATS_LEN	128

  /* Control messages */
  #define PPTP_CTRL_MSG_TYPE	1

  enum {
    PPTP_StartCtrlConnRequest = 1,
    PPTP_StartCtrlConnReply = 2,
    PPTP_StopCtrlConnRequest = 3,
    PPTP_StopCtrlConnReply = 4,
    PPTP_EchoRequest = 5,
    PPTP_EchoReply = 6,
    PPTP_OutCallRequest = 7,
    PPTP_OutCallReply = 8,
    PPTP_InCallRequest = 9,
    PPTP_InCallReply = 10,
    PPTP_InCallConn = 11,
    PPTP_CallClearRequest = 12,
    PPTP_CallDiscNotify = 13,
    PPTP_WanErrorNotify = 14,
    PPTP_SetLinkInfo = 15
  };

  #define PPTP_MIN_CTRL_TYPE		1
  #define PPTP_MAX_CTRL_TYPE		16

  #define PPTP_VALID_CTRL_TYPE(x)	\
	((x) >= PPTP_MIN_CTRL_TYPE && (x) < PPTP_MAX_CTRL_TYPE)

  /* Framing capabilities */
  #define PPTP_FRAMECAP_ASYNC		0x01
  #define PPTP_FRAMECAP_SYNC		0x02
  #define PPTP_FRAMECAP_ANY		0x03

  /* Bearer capabilities */
  #define PPTP_BEARCAP_ANALOG		0x01
  #define PPTP_BEARCAP_DIGITAL		0x02
  #define PPTP_BEARCAP_ANY		0x03

  /* General error codes */
  #define PPTP_ERROR_NONE		0
  #define PPTP_ERROR_NOT_CONNECTED	1
  #define PPTP_ERROR_BAD_FORMAT		2
  #define PPTP_ERROR_BAD_VALUE		3
  #define PPTP_ERROR_NO_RESOURCE	4
  #define PPTP_ERROR_BAD_CALL_ID	5
  #define PPTP_ERROR_PAC_ERROR		6

  /* All reserved fields have this prefix */
  #define PPTP_RESV_PREF		"resv"

  /* Message structures */
  struct pptpMsgHead {
    u_int16_t	length;		/* total length */
    u_int16_t	msgType;	/* PPTP message type */
    u_int32_t	magic;		/* magic cookie */
    u_int16_t	type;		/* control message type */
    u_int16_t	resv0;		/* reserved */
  };
  typedef struct pptpMsgHead	*PptpMsgHead;

#else
  { { "len", 2 }, { "msgType", 2 }, { "magic", 4 }, { "type", 2 },
    { PPTP_RESV_PREF "0", 2 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpStartCtrlConnRequest {
    u_int16_t		vers;		/* protocol version */
    u_int16_t		resv0;		/* reserved */
    u_int32_t		frameCap;	/* framing capabilities */
    u_int32_t		bearCap;	/* bearer capabilities */
    u_int16_t		maxChan;	/* maximum # channels */
    u_int16_t		firmware;	/* firmware revision */
    char		host[PPTP_HOSTNAME_LEN];	/* host name */
    char		vendor[PPTP_VENDOR_LEN];	/* vendor name */
  };

#else
  { { "vers", 2 }, { PPTP_RESV_PREF "0", 2 }, { "frameCap", 4 },
    { "bearCap", 4 }, { "maxChan", 2 }, { "firm", 2 },
    { "host", PPTP_HOSTNAME_LEN }, { "vend", PPTP_VENDOR_LEN }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpStartCtrlConnReply {
    u_int16_t		vers;		/* protocol version */
    u_int8_t		result;		/* result code */
    u_int8_t		err;		/* error code */
    u_int32_t		frameCap;	/* framing capabilities */
    u_int32_t		bearCap;	/* bearer capabilities */
    u_int16_t		maxChan;	/* maximum # channels */
    u_int16_t		firmware;	/* firmware revision */
    char		host[PPTP_HOSTNAME_LEN];	/* host name */
    char		vendor[PPTP_VENDOR_LEN];	/* vendor name */
  };

#else
  { { "vers", 2 }, { "result", 1 }, { "err", 1 }, { "frameCap", 4 },
    { "bearCap", 4 }, { "maxChan", 2 }, { "firm", 2 },
    { "host", PPTP_HOSTNAME_LEN }, { "vend", PPTP_VENDOR_LEN }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_SCCR_RESL_OK	1	/* channel successfully established */
  #define PPTP_SCCR_RESL_ERR	2	/* general error; see error code */
  #define PPTP_SCCR_RESL_EXISTS	3	/* command channel already exists */
  #define PPTP_SCCR_RESL_AUTH	4	/* not authorized */
  #define PPTP_SCCR_RESL_VERS	5	/* incompatible protocol version */

  struct pptpStopCtrlConnRequest {
    u_int8_t		reason;		/* reason */
    u_int8_t		resv0;		/* reserved */
    u_int16_t		resv1;		/* reserved */
  };

#else
  { { "reason", 1 }, { PPTP_RESV_PREF "0", 1 }, { PPTP_RESV_PREF "1", 2 },
    { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_SCCR_REAS_NONE	1	/* general */
  #define PPTP_SCCR_REAS_PROTO	2	/* can't support protocol version */
  #define PPTP_SCCR_REAS_LOCAL	3	/* local shutdown */

  struct pptpStopCtrlConnReply {
    u_int8_t		result;		/* result code */
    u_int8_t		err;		/* error code */
    u_int16_t		resv0;		/* reserved */
  };

#else
  { { "result", 1 }, { "err", 1 }, { PPTP_RESV_PREF "0", 2 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpEchoRequest {
    u_int32_t		id;		/* identifier */
  };

#else
  { { "id", 4 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpEchoReply {
    u_int32_t		id;		/* identifier */
    u_int8_t		result;		/* result code */
    u_int8_t		err;		/* error code */
    u_int16_t		resv0;		/* reserved */
  };

#else
  { { "id", 4 }, { "result", 1 }, { "err", 1 },
    { "ignore", 2 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_ECHO_RESL_OK	1	/* echo reply is valid */
  #define PPTP_ECHO_RESL_ERR	2	/* general error; see error code */

  struct pptpOutCallRequest {
    u_int16_t		cid;		/* call id */
    u_int16_t		serno;		/* call serial # */
    u_int32_t		minBps;		/* minimum BPS */
    u_int32_t		maxBps;		/* maximum BPS */
    u_int32_t		bearType;	/* bearer type */
    u_int32_t		frameType;	/* framing type */
    u_int16_t		recvWin;	/* pkt receive window size */
    u_int16_t		ppd;		/* pkt processing delay */
    u_int16_t		numLen;		/* phone number length */
    u_int16_t		resv0;		/* reserved */
    char		phone[PPTP_PHONE_LEN];		/* phone number */
    char		subaddr[PPTP_SUBADDR_LEN];	/* sub-address */
  };

#else
  { { "cid", 2 }, { "serno", 2 }, { "minBPS", 4 }, { "maxBPS", 4 },
    { "bearType", 4 }, { "frameType", 4 },{ "recvWin", 2 }, { "ppd", 2 },
    { "numLen", 2 }, { PPTP_RESV_PREF "0", 2 },
    { "phone", PPTP_PHONE_LEN }, { "subaddr", PPTP_SUBADDR_LEN },
    { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpOutCallReply {
    u_int16_t		cid;		/* call id */
    u_int16_t		peerCid;	/* peer call id */
    u_int8_t		result;		/* result code */
    u_int8_t		err;		/* error code */
    u_int16_t		cause;		/* cause code */
    u_int32_t		speed;		/* cause code */
    u_int16_t		recvWin;	/* pkt receive window size code */
    u_int16_t		ppd;		/* pkt processing delay */
    u_int32_t		channel;	/* physical channel id */
  };

#else
  { { "cid", 2 }, { "peerCid", 2 }, { "result", 1 }, { "err", 1 },
    { "cause", 2 }, { "speed", 4 }, { "recvWin", 2 }, { "ppd", 2 },
    { "channel", 4 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_OCR_RESL_OK	1	/* call established OK */
  #define PPTP_OCR_RESL_ERR	2	/* general error; see error code */
  #define PPTP_OCR_RESL_NOCARR	3	/* no carrier */
  #define PPTP_OCR_RESL_BUSY	4	/* busy */
  #define PPTP_OCR_RESL_NODIAL	5	/* no dialtone */
  #define PPTP_OCR_RESL_TIMED	6	/* timed out */
  #define PPTP_OCR_RESL_ADMIN	7	/* administratvely prohibited */

  struct pptpInCallRequest {
    u_int16_t		cid;		/* call id */
    u_int16_t		serno;		/* call serial # */
    u_int32_t		bearType;	/* bearer type */
    u_int32_t		channel;	/* physical channel id */
    u_int16_t		dialedLen;	/* dialed number len */
    u_int16_t		dialingLen;	/* dialing number len */
    char		dialed[PPTP_PHONE_LEN];		/* dialed number */
    char		dialing[PPTP_PHONE_LEN];	/* dialing number */
    char		subaddr[PPTP_SUBADDR_LEN];	/* sub-address */
  };

#else
  { { "cid", 2 }, { "serno", 2 }, { "bearType", 4 }, { "channel", 4 },
    { "dialedLen", 2 }, { "dialingLen", 2 }, { "dialed", PPTP_PHONE_LEN },
    { "dialing", PPTP_PHONE_LEN }, { "subaddr", PPTP_SUBADDR_LEN },
    { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpInCallReply {
    u_int16_t		cid;		/* call id */
    u_int16_t		peerCid;	/* peer call id */
    u_int8_t		result;		/* result code */
    u_int8_t		err;		/* error code */
    u_int16_t		recvWin;	/* pkt receive window size code */
    u_int16_t		ppd;		/* pkt processing delay */
    u_int16_t		resv0;		/* reserved */
  };

#else
  { { "cid", 2 }, { "peerCid", 2 }, { "result", 1 }, { "err", 1 },
    { "recvWin", 2 }, { "ppd", 2 }, { PPTP_RESV_PREF "0", 2 },
    { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_ICR_RESL_OK	1	/* call established OK */
  #define PPTP_ICR_RESL_ERR	2	/* general error; see error code */
  #define PPTP_ICR_RESL_NAK	3	/* do not accept */

  struct pptpInCallConn {
    u_int16_t		peerCid;	/* peer call id */
    u_int16_t		resv0;		/* reserved */
    u_int32_t		speed;		/* connect speed */
    u_int16_t		recvWin;	/* pkt receive window size code */
    u_int16_t		ppd;		/* pkt processing delay */
    u_int32_t		frameType;	/* framing type */
  };

#else
  { { "peerCid", 2 }, { PPTP_RESV_PREF "0", 2 }, { "speed", 4 },
    { "recvWin", 2 }, { "ppd", 2 }, { "frameType", 4 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpCallClearRequest {
    u_int16_t		cid;		/* PNS assigned call id */
    u_int16_t		resv0;		/* reserved */
  };

#else
  { { "cid", 2 }, { PPTP_RESV_PREF "0", 2 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpCallDiscNotify {
    u_int16_t		cid;		/* PAC assigned call id */
    u_int8_t		result;		/* result code */
    u_int8_t		err;		/* error code */
    u_int16_t		cause;		/* cause code */
    u_int16_t		resv0;		/* reserved */
    char		stats[PPTP_STATS_LEN];	/* call stats */
  };

#else
  { { "cid", 2 }, { "result", 1 }, { "err", 1 }, { "cause", 2 },
    { PPTP_RESV_PREF "0", 2 }, { "stats", PPTP_STATS_LEN },
    { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_CDN_RESL_CARR	1	/* lost carrier */
  #define PPTP_CDN_RESL_ERR	2	/* general error; see error code */
  #define PPTP_CDN_RESL_ADMIN	3	/* administrative reason */
  #define PPTP_CDN_RESL_REQ	4	/* received disconnect request */

  struct pptpWanErrorNotify {
    u_int16_t		cid;		/* PNS assigned call id */
    u_int16_t		resv0;		/* reserved */
    u_int32_t		crc;		/* crc errors */
    u_int32_t		frame;		/* framing errors */
    u_int32_t		hdw;		/* hardware errors */
    u_int32_t		ovfl;		/* buffer overrun errors */
    u_int32_t		timeout;	/* timeout errors */
    u_int32_t		align;		/* alignment errors */
  };

#else
  { { "cid", 2 }, { PPTP_RESV_PREF "0", 2 }, { "crc", 4 },
    { "frame", 4 }, { "hdw", 4 }, { "ovfl", 4 }, { "timeout", 4 },
    { "align", 4 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  struct pptpSetLinkInfo {
    u_int16_t		cid;		/* call id */
    u_int16_t		resv0;		/* reserved */
    u_int32_t		sendAccm;	/* send ACCM */
    u_int32_t		recvAccm;	/* receive ACCM */
  };

#else
  { { "cid", 2 }, { PPTP_RESV_PREF "0", 2 }, { "sendAccm", 4 },
    { "recvAccm", 4 }, { NULL, 0 } },
#endif
#ifndef _WANT_PPTP_FIELDS

  #define PPTP_CTRL_MAX_FRAME	\
	(sizeof(struct pptpMsgHead) + sizeof(struct pptpInCallRequest))
  #define PPTP_CTRL_MAX_FIELDS	14

  /* Describes one field of a PPTP control message structure */
  struct pptpfield {
    const char	*name;
    u_short	length;
  };
  typedef struct pptpfield	*PptpField;

  /* Link <-> control liason structures and callback function types */
  struct pptplinkinfo {		/* PPTP's info for accessing link code */
    void	*cookie;	/* NULL indicates response is invalid */
    void	(*result)(void *cookie, const char *errmsg, int frameType);
    void	(*setLinkInfo)(void *cookie, u_int32_t sa, u_int32_t ra);
    void	(*cancel)(void *cookie);	/* cancel outgoing call */
  };
  typedef struct pptplinkinfo	*PptpLinkInfo;

  struct pptpctrlinfo {		/* Link's info for accessing PPTP code */
    void		*cookie;	/* NULL indicates response is invalid */
    struct u_addr	peer_addr;	/* Peer IP address and port */
    in_port_t		peer_port;
    void		(*close)(void *cookie, int result, int err, int cause);
    void		(*answer)(void *cookie, int rs, int er, int cs, int sp);
    void		(*connected)(void *cookie, int sp);
    void		(*setLinkInfo)(void *cookie, u_int32_t sa, u_int32_t ra);
  };
  typedef struct pptpctrlinfo	*PptpCtrlInfo;

  typedef struct pptplinkinfo	(*PptpGetInLink_t)(struct pptpctrlinfo *cinfo,
				  struct u_addr *self, struct u_addr *peer, in_port_t port, 
				  int bearType,
				  const char *callingNum,
				  const char *calledNum,
				  const char *subAddress);

  typedef struct pptplinkinfo	(*PptpGetOutLink_t)(struct pptpctrlinfo *cinfo,
				  struct u_addr *self, struct u_addr *peer, in_port_t port, 
				  int bearType, int frameType, int minBps, int maxBps,
				  const char *calledNum,
				  const char *subAddress);

/*
 * FUNCTIONS
 */

  extern int			PptpCtrlInit(PptpGetInLink_t getInLink,
				  PptpGetOutLink_t getOutLink);

  extern void*			PptpCtrlListen(struct u_addr *ip, in_port_t port);
  extern void			PptpCtrlUnListen(void *listener);

  extern void			PptpCtrlInCall(struct pptpctrlinfo *cinfo,
				  struct pptplinkinfo *linfo,
				  struct u_addr *locip, struct u_addr *ip,
				  in_port_t port, int bearType, int frameType,
				  int minBps, int maxBps,
				  const char *callingNum,
				  const char *calledNum,
				  const char *subAddress);

  extern void			PptpCtrlOutCall(struct pptpctrlinfo *cinfo,
				  struct pptplinkinfo *linfo,
				  struct u_addr *locip, struct u_addr *ip,
				  in_port_t port, int bearType, int frameType,
				  int minBps, int maxBps,
				  const char *calledNum,
				  const char *subAddress);

  extern int			PptpCtrlGetSessionInfo(struct pptpctrlinfo *cp,
				  struct u_addr *selfAddr,
				  struct u_addr *peerAddr,
				  u_int16_t *selfCid, u_int16_t *peerCid,
				  u_int16_t *peerWin, u_int16_t *peerPpd);

/*
 * Get local/remote hostnames.
 */
  extern int	PptpCtrlGetSelfName(struct pptpctrlinfo *cp,
			void *buf, size_t buf_len);
  extern int	PptpCtrlGetPeerName(struct pptpctrlinfo *cp,
			void *buf, size_t buf_len);

#endif	/* #ifndef _WANT_PPTP_FIELDS */
#endif	/* #if !defined(_PPTP_CTRL_H_) || defined(_WANT_PPTP_FIELDS) */

