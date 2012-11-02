
/*
 * fsm.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _FSM_H_
#define _FSM_H_

#include <netinet/in.h>
#include "mbuf.h"
#include "timer.h"
#include <netgraph/ng_ppp.h>

/*
 * DEFINITIONS
 */

    /* States: don't change these! */
    enum fsm_state {
	ST_INITIAL = 0,
	ST_STARTING,
	ST_CLOSED,
	ST_STOPPED,
	ST_CLOSING,
	ST_STOPPING,
	ST_REQSENT,
	ST_ACKRCVD,
	ST_ACKSENT,
	ST_OPENED
    };

  #define OPEN_STATE(s)		((s) > ST_CLOSING || ((s) & 1))

  #define MODE_REQ	0
  #define MODE_NAK	1
  #define MODE_REJ	2
  #define MODE_NOP	3

  /* Codes */
  #define CODE_VENDOR		0
  #define CODE_CONFIGREQ	1
  #define CODE_CONFIGACK	2
  #define CODE_CONFIGNAK	3
  #define CODE_CONFIGREJ	4
  #define CODE_TERMREQ		5
  #define CODE_TERMACK		6
  #define CODE_CODEREJ		7
  #define CODE_PROTOREJ		8
  #define CODE_ECHOREQ		9
  #define CODE_ECHOREP		10
  #define CODE_DISCREQ		11
  #define CODE_IDENT		12
  #define CODE_TIMEREM		13
  #define CODE_RESETREQ		14
  #define CODE_RESETACK		15

  /* All the various ways that the FSM can fail */
  /* XXX This should be extended to contain more descriptive information
     XXX about the cause of the failure, like what the rejected protocol
     XXX or option was, etc. */
  enum fsmfail {
    FAIL_NEGOT_FAILURE,		/* option negotiation failed */
    FAIL_RECD_BADMAGIC,		/* rec'd bad magic number */
    FAIL_RECD_CODEREJ,		/* rec'd fatal code reject */
    FAIL_RECD_PROTREJ,		/* rec'd fatal protocol reject */
    FAIL_WAS_PROTREJ,		/* protocol was rejected */
    FAIL_ECHO_TIMEOUT,		/* peer not responding to echo requests */
    FAIL_CANT_ENCRYPT		/* failed to negotiate required encryption */
  };

  /* FSM descriptor */
  struct fsm;
  typedef struct fsm			*Fsm;
  struct fsmoption;
  typedef struct fsmoption		*FsmOption;
  struct fsmoptinfo;
  typedef const struct fsmoptinfo	*FsmOptInfo;

  struct fsmconf {
    short	maxconfig;	/* "Max-Configure" initial value */
    short	maxterminate;	/* "Max-Terminate" initial value */
    short	maxfailure;	/* "Max-Failure" initial value */
    short	echo_int;	/* LCP echo interval (zero disables) */
    short	echo_max;	/* LCP max quiet timeout */
    u_char	check_magic;	/* Validate any magic numbers seen */
    u_char	passive;	/* Passive option (see rfc 1661) */
  };
  typedef struct fsmconf	*FsmConf;

  struct fsmtype {
    const char		*name;		/* Name of protocol */
    uint16_t		proto;		/* Protocol number */
    uint16_t		known_codes;	/* Accepted FSM codes */
    u_char		link_layer;	/* Link level FSM */
    int			log, log2;	/* Log levels for FSM events */

    void		(*NewState)(Fsm f, enum fsm_state old, enum fsm_state new);
    void		(*LayerUp)(Fsm f);
    void		(*LayerDown)(Fsm f);
    void		(*LayerStart)(Fsm f);
    void		(*LayerFinish)(Fsm f);
    u_char *		(*BuildConfigReq)(Fsm f, u_char *cp);
    void		(*DecodeConfig)(Fsm f, FsmOption a, int num, int mode);
    void		(*Configure)(Fsm f);
    void		(*UnConfigure)(Fsm f);
    void		(*SendTerminateReq)(Fsm f);
    void		(*SendTerminateAck)(Fsm f);
    int			(*RecvCodeRej)(Fsm f, int code, Mbuf bp);
    int			(*RecvProtoRej)(Fsm f, int proto, Mbuf bp);
    void		(*Failure)(Fsm f, enum fsmfail reason);
    void		(*RecvResetReq)(Fsm f, int id, Mbuf bp);
    void		(*RecvResetAck)(Fsm f, int id, Mbuf bp);
    void		(*RecvIdent)(Fsm f, Mbuf bp);
    void		(*RecvDiscReq)(Fsm f, Mbuf bp);
    void		(*RecvTimeRemain)(Fsm f, Mbuf bp);
    void		(*RecvVendor)(Fsm f, Mbuf bp);
  };
  typedef const struct fsmtype	*FsmType;

  struct fsm {
    FsmType		type;		/* FSM constant stuff */
    void		*arg;		/* Context (Link or Bund) */
    int			log;		/* Current log level */
    int			log2;		/* Current log2 level */
    struct fsmconf	conf;		/* FSM parameters */
    enum fsm_state	state;		/* State of the machine */
    u_char		reqid;		/* Next request id */
    u_char		rejid;		/* Next reject id */
    u_char		echoid;		/* Next echo request id */
    short		restart;	/* Restart counter value */
    short		failure;	/* How many failures left */
    short		config;		/* How many configs left */
    short		quietCount;	/* How long peer has been silent */
    struct pppTimer	timer;		/* Restart Timer */
    struct pppTimer	echoTimer;	/* Keep-alive timer */
    struct ng_ppp_link_stat
			idleStats;	/* Stats for echo timeout */
  };

  /* Packet header */
  struct fsmheader {
    u_char	code;		/* Request code */
    u_char	id;		/* Identification */
    u_short	length;		/* Length of packet */
  };
  typedef struct fsmheader	*FsmHeader;

  /* One config option */
  struct fsmoption {
    u_char	type;
    u_char	len;
    u_char	*data;
  };

  /* Fsm option descriptor */
  struct fsmoptinfo {
    const char	*name;
    u_char	type;
    u_char	minLen;
    u_char	maxLen;
    u_char	supported;
  };

/*
 * VARIABLES
 */

  extern u_int		gAckSize, gNakSize, gRejSize;

/*
 * FUNCTIONS
 */

  extern void		FsmInit(Fsm f, FsmType t, void *arg);
  extern void		FsmInst(Fsm fp, Fsm fpt, void *arg);
  extern void		FsmOpen(Fsm f);
  extern void		FsmClose(Fsm f);
  extern void		FsmUp(Fsm f);
  extern void		FsmDown(Fsm f);
  extern void		FsmInput(Fsm f, Mbuf bp);
  extern void		FsmOutput(Fsm, u_int, u_int, u_char *, int);
  extern void		FsmOutputMbuf(Fsm, u_int, u_int, Mbuf);
  extern void		FsmSendEchoReq(Fsm fp, Mbuf payload);
  extern void		FsmSendIdent(Fsm fp, const char *ident);
  extern void		FsmSendTimeRemaining(Fsm fp, u_int seconds);
  extern u_char		*FsmConfValue(u_char *cp, int ty,
				int len, const void *data);
  extern void		FsmFailure(Fsm fp, enum fsmfail reason);
  extern const char	*FsmFailureStr(enum fsmfail reason);

  extern void		FsmAck(Fsm fp, const struct fsmoption *opt);
  extern void		FsmNak(Fsm fp, const struct fsmoption *opt);
  extern void		FsmRej(Fsm fp, const struct fsmoption *opt);

  extern FsmOptInfo	FsmFindOptInfo(FsmOptInfo list, u_char type);
  extern const char	*FsmStateName(enum fsm_state state);
  extern const char	*FsmCodeName(int code);

#define Pref(fp)	 ( (fp)->type->link_layer ? ((Link)((fp)->arg))->name : ((Bund)((fp)->arg))->name )
#define Fsm(fp)		 ( (fp)->type->name )

#endif	/* _FSM_H_ */

