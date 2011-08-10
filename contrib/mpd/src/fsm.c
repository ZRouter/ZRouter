
/*
 * fsm.c
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "fsm.h"
#include "ngfunc.h"
#include "util.h"

/*
 * DEFINITIONS
 */

  #define FSM_MAXCONFIG		10
  #define FSM_MAXNAK		10
  #define FSM_MAX_OPTS		100
  #define FSM_MAXTERMINATE	2
  #define FSM_MAXFAILURE	5

/* FSM restart options */

  /* #define RESTART_OPENED */
  /* #define RESTART_CLOSING */
  /* #define RESTART_STOPPED */

  struct fsmcodedesc {
    void	(*action)(Fsm fp, FsmHeader hdr, Mbuf bp);
    const char	*name;
  };

/* Size of REQ, ACK, NAK, and REJ buffers */

  #define REQ_BUFFER_SIZE	256
  #define REJ_BUFFER_SIZE	256
  #define ACK_BUFFER_SIZE	256
  #define NAK_BUFFER_SIZE	256

/*
 * INTERNAL FUNCTIONS
 */

  static void	FsmNewState(Fsm f, enum fsm_state state);

  static void	FsmSendConfigReq(Fsm fp);
  static void	FsmSendTerminateReq(Fsm fp);
  static void	FsmSendTerminateAck(Fsm fp);
  static void	FsmInitRestartCounter(Fsm fp, int value);
  static void	FsmInitMaxFailure(Fsm fp, int value);
  static void	FsmInitMaxConfig(Fsm fp, int value);
  static void	FsmTimeout(void *arg);

  static void	FsmRecvConfigReq(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvConfigAck(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvConfigNak(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvTermReq(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvTermAck(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvConfigRej(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvCodeRej(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvProtoRej(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvEchoReq(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvEchoRep(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvDiscReq(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvIdent(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvTimeRemain(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvResetReq(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvResetAck(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvVendor(Fsm fp, FsmHeader lhp, Mbuf bp);
  static void	FsmRecvRxjPlus(Fsm fp);

  static void	FsmLayerUp(Fsm fp);
  static void	FsmLayerDown(Fsm fp);
  static void	FsmLayerStart(Fsm fp);
  static void	FsmLayerFinish(Fsm fp);

  static Mbuf	FsmCheckMagic(Fsm fp, Mbuf bp);
  static void	FsmEchoTimeout(void *arg);
  static void	FsmDecodeBuffer(Fsm fp, u_char *buf, int size, int mode);
  static int	FsmExtractOptions(Fsm fp, u_char *data,
		  int dlen, FsmOption opts, int max);

/*
 * GLOBAL VARIABLES
 */

  u_int		gAckSize, gNakSize, gRejSize;

/*
 * INTERNAL VARIABLES
 */

/*
 * It's OK for these to be statically allocated, because we always
 * do our work with them all in one whole step without stopping.
 */

  static u_char	gRejBuf[REJ_BUFFER_SIZE];
  static u_char	gAckBuf[ACK_BUFFER_SIZE];
  static u_char	gNakBuf[NAK_BUFFER_SIZE];

  static const struct fsmcodedesc FsmCodes[] = {
   { FsmRecvVendor,	"Vendor Packet" },
   { FsmRecvConfigReq,  "Configure Request" },
   { FsmRecvConfigAck,  "Configure Ack" },
   { FsmRecvConfigNak,  "Configure Nak" },
   { FsmRecvConfigRej,  "Configure Reject" },
   { FsmRecvTermReq,    "Terminate Request" },
   { FsmRecvTermAck,    "Terminate Ack" },
   { FsmRecvCodeRej,    "Code Reject" },
   { FsmRecvProtoRej,   "Protocol Reject" },
   { FsmRecvEchoReq,    "Echo Request" },
   { FsmRecvEchoRep,    "Echo Reply" },
   { FsmRecvDiscReq,    "Discard Request" },
   { FsmRecvIdent,      "Ident" },
   { FsmRecvTimeRemain, "Time Remain" },
   { FsmRecvResetReq,   "Reset Request" },
   { FsmRecvResetAck,   "Reset Ack" },
  };

  #define NUM_FSM_CODES	(sizeof(FsmCodes) / sizeof(*FsmCodes))

/*
 * FsmInit()
 *
 * Initialize FSM structure and install some default values
 */

void
FsmInit(Fsm fp, FsmType type, void *arg)
{
  memset(fp, 0, sizeof(*fp));
  fp->type = type;
  fp->arg = arg;
  fp->log = LG_FSM | type->log;
  fp->log2 = LG_FSM | type->log2;
  fp->conf.maxconfig = FSM_MAXCONFIG;
  fp->conf.maxterminate = FSM_MAXTERMINATE;
  fp->conf.maxfailure = FSM_MAXFAILURE;
  fp->conf.check_magic = FALSE;
  fp->conf.passive = FALSE;
  fp->state = ST_INITIAL;
  fp->reqid = 1;
  fp->rejid = 1;
  fp->echoid = 1;
}

/*
 * FsmInst()
 *
 * Instantiate FSM structure from template
 */

void
FsmInst(Fsm fp, Fsm fpt, void *arg)
{
    memcpy(fp, fpt, sizeof(*fp));
    fp->arg = arg;
}

/*
 * FsmNewState()
 *
 * Change state of a FSM. Also, call the configuration routine
 * if this is an appropriate time.
 */

void
FsmNewState(Fsm fp, enum fsm_state new)
{
  enum fsm_state	old;

  /* Log it */
  Log(fp->log2, ("[%s] %s: state change %s --> %s",
    Pref(fp), Fsm(fp), FsmStateName(fp->state), FsmStateName(new)));

  /* Change state and call protocol's own handler, if any */
  old = fp->state;
  if (fp->type->NewState)
    (*fp->type->NewState)(fp, old, new);
  fp->state = new;
  if ((new >= ST_INITIAL && new <= ST_STOPPED) || (new == ST_OPENED))
    TimerStop(&fp->timer);

  /* Turn on/off keep-alive echo packets (if so configured) */
  if (old == ST_OPENED)
    TimerStop(&fp->echoTimer);
  if (new == ST_OPENED && fp->conf.echo_int != 0) {
    fp->quietCount = 0;
    memset(&fp->idleStats, 0, sizeof(fp->idleStats));
    TimerInit(&fp->echoTimer, "FsmKeepAlive",
      fp->conf.echo_int * SECONDS, FsmEchoTimeout, fp);
    TimerStartRecurring(&fp->echoTimer);
  }
}

/*
 * FsmOutputMbuf()
 *
 * Send out a control packet with specified contents. Consumes the mbuf.
 */

void
FsmOutputMbuf(Fsm fp, u_int code, u_int id, Mbuf payload)
{
    Bund		b;
    Mbuf		bp;
    struct fsmheader	hdr;

    if (fp->type->link_layer)
	b = ((Link)fp->arg)->bund;
    else
	b = (Bund)fp->arg;

    /* Build header */
    hdr.id = id;
    hdr.code = code;
    hdr.length = htons(sizeof(hdr) + MBLEN(payload));

    /* Prepend to payload */
    bp = mbcopyback(payload, -(int)sizeof(hdr), &hdr, sizeof(hdr));

    /* Send it out */
    if (fp->type->link_layer) {
	NgFuncWritePppFrameLink((Link)fp->arg, fp->type->proto, bp);
    } else {
	NgFuncWritePppFrame(b, NG_PPP_BUNDLE_LINKNUM, fp->type->proto, bp);
    }
}

/*
 * FsmOutput()
 *
 * Send out a control packet with specified contents
 */

void
FsmOutput(Fsm fp, u_int code, u_int id, u_char *ptr, int len)
{
  Mbuf	bp;

  bp = (len > 0) ? mbcopyback(NULL, 0, ptr, len) : NULL;
  FsmOutputMbuf(fp, code, id, bp);
}

/*
 * FsmOpen()
 *
 * XXX The use of the restart option should probably be
 * XXX configured per FSM via the initialization structure.
 * XXX For now, we just make it mandatory via #ifdef's.
 */

void
FsmOpen(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: Open event", Pref(fp), Fsm(fp)));
  switch (fp->state) {
    case ST_INITIAL:
      FsmNewState(fp, ST_STARTING);
      FsmLayerStart(fp);
      break;
    case ST_STARTING:
      break;
    case ST_CLOSED:
      if (fp->type->Configure)
	(*fp->type->Configure)(fp);
      FsmNewState(fp, ST_REQSENT);
      FsmLayerStart(fp);		/* Missing in RFC 1661 */
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmInitMaxFailure(fp, fp->conf.maxfailure);
      FsmInitMaxConfig(fp, fp->conf.maxconfig);
      FsmSendConfigReq(fp);
      break;
    case ST_REQSENT:
    case ST_ACKRCVD:
    case ST_ACKSENT:
      break;

    case ST_OPENED:
#ifdef RESTART_OPENED
      FsmDown(fp);
      FsmUp(fp);
#endif
      break;

    case ST_CLOSING:
      FsmNewState(fp, ST_STOPPING);
    case ST_STOPPING:
#ifdef RESTART_CLOSING
      FsmDown(fp);
      FsmUp(fp);
#endif
      break;

    case ST_STOPPED:
#ifdef RESTART_STOPPED
      FsmDown(fp);
      FsmUp(fp);
#endif
      break;
  }
}

void
FsmUp(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: Up event", Pref(fp), Fsm(fp)));
  switch (fp->state) {
    case ST_INITIAL:
      FsmNewState(fp, ST_CLOSED);
      break;
    case ST_STARTING:
      if (fp->type->Configure)
	(*fp->type->Configure)(fp);
      FsmNewState(fp, ST_REQSENT);
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmInitMaxFailure(fp, fp->conf.maxfailure);
      FsmInitMaxConfig(fp, fp->conf.maxconfig);
      FsmSendConfigReq(fp);
      break;
    default:
      Log(fp->log2, ("[%s] %s: Oops, UP at %s",
	Pref(fp), Fsm(fp), FsmStateName(fp->state)));
      break;
  }
}

void
FsmDown(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: Down event", Pref(fp), Fsm(fp)));
  switch (fp->state) {
    case ST_CLOSING:
      FsmLayerFinish(fp);		/* Missing in RFC 1661 */
      /* fall through */
    case ST_CLOSED:
      FsmNewState(fp, ST_INITIAL);
      break;
    case ST_STOPPED:
      FsmNewState(fp, ST_STARTING);
      FsmLayerStart(fp);
      break;
    case ST_STOPPING:
    case ST_REQSENT:
    case ST_ACKRCVD:
    case ST_ACKSENT:
      FsmNewState(fp, ST_STARTING);
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;
    case ST_OPENED:
      FsmNewState(fp, ST_STARTING);
      FsmLayerDown(fp);
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;
    default:
      break;
  }
}

void
FsmClose(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: Close event", Pref(fp), Fsm(fp)));
  switch (fp->state) {
    case ST_STARTING:
      FsmNewState(fp, ST_INITIAL);
      FsmLayerFinish(fp);
      break;
    case ST_STOPPED:
      FsmNewState(fp, ST_CLOSED);
      break;
    case ST_STOPPING:
      FsmNewState(fp, ST_CLOSING);
      break;
    case ST_OPENED:
      FsmNewState(fp, ST_CLOSING);
      FsmInitRestartCounter(fp, fp->conf.maxterminate);
      FsmSendTerminateReq(fp);
      FsmLayerDown(fp);
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;
    case ST_REQSENT:
    case ST_ACKRCVD:
    case ST_ACKSENT:
      FsmNewState(fp, ST_CLOSING);
      FsmInitRestartCounter(fp, fp->conf.maxterminate);
      FsmSendTerminateReq(fp);
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;
    default:
      break;
  }
}

/*
 * Send functions
 */

static void
FsmSendConfigReq(Fsm fp)
{
  u_char	reqBuf[REQ_BUFFER_SIZE];
  u_char	*cp;

  /* Build and display config request */
  Log(fp->log, ("[%s] %s: SendConfigReq #%d", Pref(fp), Fsm(fp), fp->reqid));
  cp = (*fp->type->BuildConfigReq)(fp, reqBuf);
  FsmDecodeBuffer(fp, reqBuf, cp - reqBuf, MODE_NOP);

  /* Send it */
  FsmOutput(fp, CODE_CONFIGREQ, fp->reqid++, reqBuf, cp - reqBuf);

  /* Restart restart timer and decrement restart counter */
  TimerStart(&fp->timer);
  fp->restart--;
  fp->config--;
}

static void
FsmSendTerminateReq(Fsm fp)
{
  Log(fp->log, ("[%s] %s: SendTerminateReq #%d", Pref(fp), Fsm(fp), fp->reqid));
  FsmOutput(fp, CODE_TERMREQ, fp->reqid++, NULL, 0);
  if (fp->type->SendTerminateReq)
    (*fp->type->SendTerminateReq)(fp);
  TimerStart(&fp->timer);	/* Restart restart timer */
  fp->restart--;		/* Decrement restart counter */
}

static void
FsmSendTerminateAck(Fsm fp)
{
  Log(fp->log, ("[%s] %s: SendTerminateAck #%d", Pref(fp), Fsm(fp), fp->reqid));
  FsmOutput(fp, CODE_TERMACK, fp->reqid++, NULL, 0);
  if (fp->type->SendTerminateAck)
    (*fp->type->SendTerminateAck)(fp);
}

static void
FsmSendConfigAck(Fsm fp, FsmHeader lhp, u_char *option, int count)
{
  Log(fp->log, ("[%s] %s: SendConfigAck #%d", Pref(fp), Fsm(fp), lhp->id));
  FsmDecodeBuffer(fp, option, count, MODE_NOP);
  FsmOutput(fp, CODE_CONFIGACK, lhp->id, option, count);
}

static void
FsmSendConfigRej(Fsm fp, FsmHeader lhp, u_char *option, int count)
{
  Log(fp->log, ("[%s] %s: SendConfigRej #%d", Pref(fp), Fsm(fp), lhp->id));
  FsmDecodeBuffer(fp, option, count, MODE_NOP);
  FsmOutput(fp, CODE_CONFIGREJ, lhp->id, option, count);
  fp->failure--;
}

static void
FsmSendConfigNak(Fsm fp, FsmHeader lhp, u_char *option, int count)
{
  Log(fp->log, ("[%s] %s: SendConfigNak #%d", Pref(fp), Fsm(fp), lhp->id));
  FsmDecodeBuffer(fp, option, count, MODE_NOP);
  FsmOutput(fp, CODE_CONFIGNAK, lhp->id, option, count);
  fp->failure--;
}

/*
 * Timeout actions
 */

static void
FsmTimeout(void *arg)
{
  Fsm	fp = (Fsm) arg;

  if (fp->restart > 0) {	/* TO+ */
    switch (fp->state) {
      case ST_CLOSING:
      case ST_STOPPING:
	FsmSendTerminateReq(fp);
	break;
      case ST_REQSENT:
      case ST_ACKSENT:
	FsmSendConfigReq(fp);
	break;
      case ST_ACKRCVD:
	FsmNewState(fp, ST_REQSENT);
	FsmSendConfigReq(fp);
	break;
      default:
	break;
    }
  } else {				/* TO- */
    switch (fp->state) {
      case ST_CLOSING:
	FsmNewState(fp, ST_CLOSED);
	FsmLayerFinish(fp);
	break;
      case ST_STOPPING:
	FsmNewState(fp, ST_STOPPED);
	FsmLayerFinish(fp);
	break;
      case ST_REQSENT:
      case ST_ACKSENT:
      case ST_ACKRCVD:
	FsmFailure(fp, FAIL_NEGOT_FAILURE);
	break;
      default:
        break;
    }
  }
}

void
FsmInitRestartCounter(Fsm fp, int value)
{
  const int	retry = fp->type->link_layer ?
			((Link)(fp->arg))->conf.retry_timeout : 
			((Bund)(fp->arg))->conf.retry_timeout;

  TimerStop(&fp->timer);
  TimerInit(&fp->timer,
    fp->type->name, retry * SECONDS, FsmTimeout, (void *) fp);
  fp->restart = value;
}

void
FsmInitMaxFailure(Fsm fp, int value)
{
  fp->failure = value;
}

void
FsmInitMaxConfig(Fsm fp, int value)
{
  fp->config = value;
}

/*
 * Actions that happen when we receive packets
 */

/* RCR */

static void
FsmRecvConfigReq(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  int	fullyAcked;

  /* Check and process easy cases */
  switch (fp->state) {
    case ST_INITIAL:
    case ST_STARTING:
      Log(fp->log2, ("[%s] %s: Oops, RCR in %s", Pref(fp), Fsm(fp), FsmStateName(fp->state)));
      mbfree(bp);
      return;
    case ST_CLOSED:
      FsmSendTerminateAck(fp);
      mbfree(bp);
      return;
    case ST_STOPPED:
      if (fp->type->Configure)
	(*fp->type->Configure)(fp);
      break;
    case ST_CLOSING:
    case ST_STOPPING:
      mbfree(bp);
      return;
    default:
      break;
  }

  /* Decode packet */
  FsmDecodeBuffer(fp, MBDATA(bp), MBLEN(bp), MODE_REQ);

  /* State specific actions */
  switch (fp->state) {
    case ST_OPENED:
      FsmLayerDown(fp);
      FsmSendConfigReq(fp);
      break;
    case ST_STOPPED:
      FsmLayerStart(fp);		/* Missing in RFC 1661 */
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmInitMaxFailure(fp, fp->conf.maxfailure);
      FsmInitMaxConfig(fp, fp->conf.maxconfig);
      FsmSendConfigReq(fp);
      break;
    default:
      break;
  }

  /* What did we think of packet? */
  fullyAcked = (gNakSize == 0 && gRejSize == 0);
  if (fullyAcked)
    FsmSendConfigAck(fp, lhp, gAckBuf, gAckSize);
  else {
    if (fp->failure <= 0) {
      Log(fp->log, ("[%s] %s: not converging", Pref(fp), Fsm(fp)));
      FsmFailure(fp, FAIL_NEGOT_FAILURE);
      mbfree(bp);
      return;
    } else {
      if (gRejSize)
	FsmSendConfigRej(fp, lhp, gRejBuf, gRejSize);
      else if (gNakSize)
	FsmSendConfigNak(fp, lhp, gNakBuf, gNakSize);
    }
  }

  /* Continue with state transition */
  switch (fp->state) {
    case ST_STOPPED:
    case ST_OPENED:
      if (fullyAcked)
	FsmNewState(fp, ST_ACKSENT);
      else
	FsmNewState(fp, ST_REQSENT);
      break;
    case ST_REQSENT:
      if (fullyAcked)
	FsmNewState(fp, ST_ACKSENT);
      break;
    case ST_ACKRCVD:
      if (fullyAcked)
      {
	FsmNewState(fp, ST_OPENED);
	FsmLayerUp(fp);
      }
      break;
    case ST_ACKSENT:
      if (!fullyAcked)
	FsmNewState(fp, ST_REQSENT);
      break;
    default:
      break;
  }
  mbfree(bp);
}

/* RCA */

static void
FsmRecvConfigAck(Fsm fp, FsmHeader lhp, Mbuf bp)
{

  /* Check sequence number */
  if (lhp->id != (u_char) (fp->reqid - 1)) {
    Log(fp->log, ("[%s]   Wrong id#, expecting %d", Pref(fp), (u_char) (fp->reqid - 1)));
    mbfree(bp);
    return;
  }

  /* XXX We should verify the contents are equal to our last sent config-req */

  /* Decode packet */
  FsmDecodeBuffer(fp, MBDATA(bp), MBLEN(bp), MODE_NOP);

  /* Do whatever */
  switch (fp->state) {
    case ST_CLOSED:
    case ST_STOPPED:
      FsmSendTerminateAck(fp);
      break;
    case ST_CLOSING:
    case ST_STOPPING:
      break;
    case ST_REQSENT:
      FsmNewState(fp, ST_ACKRCVD);
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmInitMaxFailure(fp, fp->conf.maxfailure);
      FsmInitMaxConfig(fp, fp->conf.maxconfig);
      TimerStart(&fp->timer);		/* Start restart timer */
      break;
    case ST_ACKRCVD:
      FsmNewState(fp, ST_REQSENT);
      FsmSendConfigReq(fp);
      break;
    case ST_ACKSENT:
      FsmNewState(fp, ST_OPENED);
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmInitMaxFailure(fp, fp->conf.maxfailure);
      FsmInitMaxConfig(fp, fp->conf.maxconfig);
      FsmLayerUp(fp);
      break;
    case ST_OPENED:
      FsmNewState(fp, ST_REQSENT);
      FsmLayerDown(fp);
      FsmSendConfigReq(fp);
      break;
    default:
      break;
  }
  mbfree(bp);
}

/* RCN */

static void
FsmRecvConfigNak(Fsm fp, FsmHeader lhp, Mbuf bp)
{

  /* Check sequence number */
  if (lhp->id != (u_char) (fp->reqid - 1)) {
    Log(fp->log, ("[%s]   Wrong id#, expecting %d", Pref(fp), (u_char) (fp->reqid - 1)));
    mbfree(bp);
    return;
  }

  /* Check and process easy cases */
  switch (fp->state) {
    case ST_INITIAL:
    case ST_STARTING:
      Log(fp->log, ("[%s] %s: Oops, RCN in %s", Pref(fp), Fsm(fp), FsmStateName(fp->state)));
      mbfree(bp);
      return;
    case ST_CLOSED:
    case ST_STOPPED:
      FsmSendTerminateAck(fp);
      mbfree(bp);
      return;
    case ST_CLOSING:
    case ST_STOPPING:
      mbfree(bp);
      return;
    default:
      break;
  }

  /* Decode packet */
  FsmDecodeBuffer(fp, MBDATA(bp), MBLEN(bp), MODE_NAK);

  /* Not converging? */
  if (fp->config <= 0) {
    Log(fp->log, ("[%s] %s: not converging", Pref(fp), Fsm(fp)));
    FsmFailure(fp, FAIL_NEGOT_FAILURE);
    mbfree(bp);
    return;
  }

  /* Do whatever */
  switch (fp->state) {
    case ST_REQSENT:
    case ST_ACKSENT:
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmSendConfigReq(fp);
      break;
    case ST_OPENED:
      FsmLayerDown(fp);
      /* fall through */
    case ST_ACKRCVD:
      FsmNewState(fp, ST_REQSENT);
      FsmSendConfigReq(fp);
      break;
    default:
      break;
  }
  mbfree(bp);
}

/* RCJ */

static void
FsmRecvConfigRej(Fsm fp, FsmHeader lhp, Mbuf bp)
{

  /* Check sequence number */
  if (lhp->id != (u_char) (fp->reqid - 1)) {
    Log(fp->log, ("[%s]   Wrong id#, expecting %d", Pref(fp), (u_char) (fp->reqid - 1)));
    mbfree(bp);
    return;
  }

  /* XXX should verify contents are a subset of previously sent config-req */

  /* Check and process easy cases */
  switch (fp->state) {
    case ST_INITIAL:
    case ST_STARTING:
      Log(fp->log, ("[%s] %s: Oops, RCJ in %s", Pref(fp), Fsm(fp), FsmStateName(fp->state)));
      mbfree(bp);
      return;
    case ST_CLOSED:
    case ST_STOPPED:
      FsmSendTerminateAck(fp);
      mbfree(bp);
      return;
    case ST_CLOSING:
    case ST_STOPPING:
      mbfree(bp);
      return;
    default:
      break;
  }

  /* Decode packet */
  FsmDecodeBuffer(fp, MBDATA(bp), MBLEN(bp), MODE_REJ);

  /* Not converging? */
  if (fp->config <= 0) {
    Log(fp->log, ("[%s] %s: not converging", Pref(fp), Fsm(fp)));
    FsmFailure(fp, FAIL_NEGOT_FAILURE);
    mbfree(bp);
    return;
  }

  /* Do whatever */
  switch (fp->state) {
    case ST_REQSENT:
    case ST_ACKSENT:
      FsmInitRestartCounter(fp, fp->conf.maxconfig);
      FsmSendConfigReq(fp);
      break;
    case ST_OPENED:
      FsmNewState(fp, ST_REQSENT);
      FsmLayerDown(fp);
      FsmSendConfigReq(fp);
      break;
    case ST_ACKRCVD:
      FsmNewState(fp, ST_REQSENT);
      FsmSendConfigReq(fp);
      break;
    default:
      break;
  }
  mbfree(bp);
}

/* RTR */

static void
FsmRecvTermReq(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  if (fp->type->link_layer) {
    RecordLinkUpDownReason(NULL, (Link)(fp->arg), 0, STR_PEER_DISC, NULL);
  }
  switch (fp->state) {
    case ST_INITIAL:
    case ST_STARTING:
      Log(fp->log, ("[%s] %s: Oops, RTR in %s", Pref(fp), Fsm(fp), FsmStateName(fp->state)));
      break;
    case ST_CLOSED:
    case ST_STOPPED:
    case ST_CLOSING:
    case ST_STOPPING:
    case ST_REQSENT:
      FsmSendTerminateAck(fp);
      break;
    case ST_ACKRCVD:
    case ST_ACKSENT:
      FsmNewState(fp, ST_REQSENT);
      FsmSendTerminateAck(fp);
      break;
    case ST_OPENED:
      FsmNewState(fp, ST_STOPPING);
      FsmSendTerminateAck(fp);
      FsmLayerDown(fp);
      FsmInitRestartCounter(fp, 0);	/* Zero restart counter */
      TimerStart(&fp->timer);		/* Start restart timer */
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;
  }
  mbfree(bp);
}

/* RTA */

static void
FsmRecvTermAck(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  switch (fp->state) {
    case ST_CLOSING:
      FsmNewState(fp, ST_CLOSED);
      FsmLayerFinish(fp);
      break;
    case ST_STOPPING:
      FsmNewState(fp, ST_STOPPED);
      FsmLayerFinish(fp);
      break;
    case ST_ACKRCVD:
      FsmNewState(fp, ST_REQSENT);
      break;
    case ST_OPENED:
      FsmNewState(fp, ST_REQSENT);
      FsmLayerDown(fp);
      FsmSendConfigReq(fp);
      break;
    default:
      break;
  }
  mbfree(bp);
}

/*
 * FsmRecvCodeRej()
 *
 * By default, this is fatal for most codes
 */

static void
FsmRecvCodeRej(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  u_char	code = 0;
  int		fatal;

  /* Get code and log it */
  bp = mbread(bp, &code, sizeof(code));
  Log(fp->log, ("[%s] %s: code %s was rejected", Pref(fp), Fsm(fp), FsmCodeName(code)));

  /* Determine fatalness */
  if (fp->type->RecvCodeRej)
    fatal = (*fp->type->RecvCodeRej)(fp, code, bp);
  else {
    switch (code) {
      case CODE_CONFIGREQ:
      case CODE_CONFIGACK:
      case CODE_CONFIGNAK:
      case CODE_CONFIGREJ:
      case CODE_TERMREQ:
      case CODE_TERMACK:
      case CODE_CODEREJ:
      case CODE_PROTOREJ:
      case CODE_ECHOREQ:
      case CODE_ECHOREP:
      case CODE_RESETREQ:
      case CODE_RESETACK:
	fatal = TRUE;
	break;
      case CODE_VENDOR:
      case CODE_DISCREQ:
      case CODE_IDENT:
      case CODE_TIMEREM:
      default:		/* if we don't know it, that makes two of us */
	fatal = FALSE;
	break;
    }
  }

  /* Possibly shut down as a result */
  if (fatal)
    FsmFailure(fp, FAIL_RECD_CODEREJ);		/* RXJ- */
  else
    FsmRecvRxjPlus(fp);
  mbfree(bp);
}

/*
 * FsmRecvProtoRej()
 *
 * By default, this is non-fatal
 */

static void
FsmRecvProtoRej(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  u_short	proto = 0;
  int		fatal = FALSE;

  bp = mbread(bp, &proto, sizeof(proto));
  proto = ntohs(proto);
  Log(fp->log, ("[%s] %s: protocol %s was rejected", Pref(fp), Fsm(fp), ProtoName(proto)));
  if (fp->state == ST_OPENED && fp->type->RecvProtoRej)
    fatal = (*fp->type->RecvProtoRej)(fp, proto, bp);
  if (fatal)
    FsmFailure(fp, FAIL_RECD_PROTREJ);		/* RXJ- */
  else
    FsmRecvRxjPlus(fp);
  mbfree(bp);
}

/*
 * FsmRecvRxjPlus()
 */

static void
FsmRecvRxjPlus(Fsm fp)				/* RXJ+ */
{
  switch (fp->state) {
    case ST_ACKRCVD:
      FsmNewState(fp, ST_REQSENT);
      break;
    default:
      break;
  }
}

/*
 * FsmFailure()
 *
 * Call this at any point if something fatal happens that is inherent
 * to the FSM itself, like a RXJ- event or negotiation failed to converge.
 * The action taken is like the RXJ- event.
 */

void
FsmFailure(Fsm fp, enum fsmfail reason)
{
  Log(fp->log, ("[%s] %s: %s", Pref(fp), Fsm(fp), FsmFailureStr(reason)));

  /* Let layer do any special handling of error code */
  if (fp->type->Failure)
    (*fp->type->Failure)(fp, reason);

  /* Shut this state machine down */
  switch (fp->state) {
    case ST_CLOSING:
      FsmNewState(fp, ST_CLOSED);
      /* fall through */
    case ST_CLOSED:
      FsmLayerFinish(fp);
      break;
    case ST_STOPPING:
      FsmNewState(fp, ST_STOPPED);
      FsmLayerFinish(fp);
      break;
    case ST_ACKRCVD:
    case ST_ACKSENT:
    case ST_REQSENT:
      FsmNewState(fp, ST_STOPPED);
      if (!fp->conf.passive)
	FsmLayerFinish(fp);
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;

    /*
     * In the opened state, the peer FSM has somehow died or did something
     * horribly wrong to us. So the common sense action in this case is to 
     * bring the whole link down, rather than restarting just the FSM.
     * We do this by telling the lower layer to restart using tlf and tls
     * while also pretending that we got a DOWN event.
     */
    case ST_OPENED:
      FsmNewState(fp, ST_STOPPING);
      FsmInitRestartCounter(fp, fp->conf.maxterminate);
      FsmSendTerminateReq(fp);
      FsmLayerDown(fp);
      if (fp->type->UnConfigure)
	(*fp->type->UnConfigure)(fp);
      break;

    /*
     * In the STOPPED state, then most likely the negotiation timed out
     * or didn't converge. Just wait for the DOWN event from the previously
     * issued tlf action.
     */
    case ST_STOPPED:
      if (!fp->conf.passive)
	FsmLayerFinish(fp);
      break;
    default:
      break;
  }
}

/*
 * FsmFailureStr()
 */

const char *
FsmFailureStr(enum fsmfail reason)
{
  const char	*string = NULL;

  switch (reason) {
    case FAIL_NEGOT_FAILURE:
      string = STR_FAIL_NEGOT_FAILURE;
      break;
    case FAIL_RECD_BADMAGIC:
      string = STR_FAIL_RECD_BADMAGIC;
      break;
    case FAIL_RECD_CODEREJ:
      string = STR_FAIL_RECD_CODEREJ;
      break;
    case FAIL_RECD_PROTREJ:
      string = STR_FAIL_RECD_PROTREJ;
      break;
    case FAIL_WAS_PROTREJ:
      string = STR_FAIL_WAS_PROTREJ;
      break;
    case FAIL_ECHO_TIMEOUT:
      string = STR_FAIL_ECHO_TIMEOUT;
      break;
    case FAIL_CANT_ENCRYPT:
      string = STR_FAIL_CANT_ENCRYPT;
      break;
    default:
      assert(0);
  }
  return(string);
}

/*
 * FsmSendEchoReq()
 *
 * Send an echo request containing the supplied payload data.
 * Consumes the mbuf.
 */

void
FsmSendEchoReq(Fsm fp, Mbuf payload)
{
  u_int32_t	const self_magic = htonl(((Link)(fp->arg))->lcp.want_magic);
  Mbuf		bp;

  if (fp->state != ST_OPENED)
    return;

  /* Prepend my magic number */
  bp = mbcopyback(payload, -(int)sizeof(self_magic), &self_magic, sizeof(self_magic));

  /* Send it */
  Log(LG_ECHO, ("[%s] %s: SendEchoReq #%d", Pref(fp), Fsm(fp), fp->echoid));
  FsmOutputMbuf(fp, CODE_ECHOREQ, fp->echoid++, bp);
}

/*
 * FsmSendIdent()
 *
 * Send an LCP ident packet.
 */

void
FsmSendIdent(Fsm fp, const char *ident)
{
  u_int32_t	self_magic = htonl(((Link)(fp->arg))->lcp.want_magic);
  const int	len = strlen(ident) + 1;	/* include NUL to be nice */
  Mbuf		bp;

  /* Leave magic zero unless fully opened, as IDENT can be sent anytime */
  if (fp->state != ST_OPENED)
    self_magic = 0;

  /* Prepend my magic number */
  bp = mbcopyback(NULL, 0, (u_char *) &self_magic, sizeof(self_magic));
  bp = mbcopyback(bp, sizeof(self_magic), ident, len);

  /* Send it */
  Log(LG_FSM, ("[%s] %s: SendIdent #%d", Pref(fp), Fsm(fp), fp->echoid));
  ShowMesg(LG_FSM, Pref(fp), ident, len);
  FsmOutputMbuf(fp, CODE_IDENT, fp->echoid++, bp);
}

/*
 * FsmSendTimeRemaining()
 *
 * Send an LCP Time-Remaining packet.
 */

void
FsmSendTimeRemaining(Fsm fp, u_int seconds)
{
  u_int32_t	self_magic = htonl(((Link)(fp->arg))->lcp.want_magic);
  u_int32_t	data = htonl(seconds);
  Mbuf		bp;

  /* Leave magic zero unless fully opened, as IDENT can be sent anytime */
  if (fp->state != ST_OPENED)
    self_magic = 0;

  /* Prepend my magic number */
  bp = mbcopyback(NULL, 0, (u_char *) &self_magic, sizeof(self_magic));
  bp = mbcopyback(bp, sizeof(self_magic), &data, 4);

  /* Send it */
  Log(LG_FSM, ("[%s] %s: SendTimeRemaining #%d", Pref(fp), Fsm(fp), fp->echoid));
  Log(LG_FSM, ("[%s]   %u seconds remain", Pref(fp), seconds));
  FsmOutputMbuf(fp, CODE_TIMEREM, fp->echoid++, bp);
}

/*
 * FsmRecvEchoReq()
 */

static void
FsmRecvEchoReq(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  u_int32_t	self_magic;

  /* Validate magic number */
  bp = FsmCheckMagic(fp, bp);

  /* If not opened, do nothing */
  if (fp->state != ST_OPENED) {
    mbfree(bp);
    return;
  }

  /* Stick my magic number in there instead */
  self_magic = htonl(((Link)(fp->arg))->lcp.want_magic);
  bp = mbcopyback(bp, -(int)sizeof(self_magic), &self_magic, sizeof(self_magic));

  /* Send it back, preserving everything else */
  Log(LG_ECHO, ("[%s] %s: SendEchoRep #%d", Pref(fp), Fsm(fp), lhp->id));
  FsmOutputMbuf(fp, CODE_ECHOREP, lhp->id, bp);
}

/*
 * FsmRecvEchoRep()
 */

static void
FsmRecvEchoRep(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  bp = FsmCheckMagic(fp, bp);
  mbfree(bp);
}

/*
 * FsmRecvDiscReq()
 */

static void
FsmRecvDiscReq(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  bp = FsmCheckMagic(fp, bp);
  if (fp->type->RecvDiscReq)
    (*fp->type->RecvDiscReq)(fp, bp);
  mbfree(bp);
}

/*
 * FsmRecvIdent()
 */

static void
FsmRecvIdent(Fsm fp, FsmHeader lhp, Mbuf bp)
{
    bp = FsmCheckMagic(fp, bp);
    if (bp)
        ShowMesg(fp->log, Pref(fp), (char *) MBDATA(bp), MBLEN(bp));
    if (fp->type->RecvIdent)
	(*fp->type->RecvIdent)(fp, bp);
    mbfree(bp);
}

/*
 * FsmRecvVendor()
 */

static void
FsmRecvVendor(Fsm fp, FsmHeader lhp, Mbuf bp)
{
    bp = FsmCheckMagic(fp, bp);
    if (fp->type->RecvVendor)
	(*fp->type->RecvVendor)(fp, bp);
    mbfree(bp);
}

/*
 * FsmRecvTimeRemain()
 */

static void
FsmRecvTimeRemain(Fsm fp, FsmHeader lhp, Mbuf bp)
{
    bp = FsmCheckMagic(fp, bp);
    if (bp) {
	u_int32_t	remain = 0;
	mbcopy(bp, 0, &remain, sizeof(remain));
	remain = ntohl(remain);
	Log(fp->log, ("[%s]   %u seconds remain", Pref(fp), remain));
    }
    if (fp->type->RecvTimeRemain)
	(*fp->type->RecvTimeRemain)(fp, bp);
    mbfree(bp);
}

/*
 * FsmRecvResetReq()
 */

static void
FsmRecvResetReq(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  if (fp->type->RecvResetReq)
    (*fp->type->RecvResetReq)(fp, lhp->id, bp);
  mbfree(bp);
}

/*
 * FsmRecvResetAck()
 */

static void
FsmRecvResetAck(Fsm fp, FsmHeader lhp, Mbuf bp)
{
  if (fp->type->RecvResetAck)
    (*fp->type->RecvResetAck)(fp, lhp->id, bp);
  mbfree(bp);
}

/*
 * FsmLayerUp()
 */

static void
FsmLayerUp(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: LayerUp", Pref(fp), Fsm(fp)));
  if (fp->type->LayerUp)
    (*fp->type->LayerUp)(fp);
}

/*
 * FsmLayerDown()
 */

static void
FsmLayerDown(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: LayerDown", Pref(fp), Fsm(fp)));
  if (fp->type->LayerDown)
    (*fp->type->LayerDown)(fp);
}

/*
 * FsmLayerStart()
 */

static void
FsmLayerStart(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: LayerStart", Pref(fp), Fsm(fp)));
  if (fp->type->LayerStart)
    (*fp->type->LayerStart)(fp);
}

/*
 * FsmLayerFinish()
 */

static void
FsmLayerFinish(Fsm fp)
{
  Log(fp->log2, ("[%s] %s: LayerFinish", Pref(fp), Fsm(fp)));
  if (fp->type->LayerFinish)
    (*fp->type->LayerFinish)(fp);
}

/*
 * FsmCheckMagic()
 */

static Mbuf
FsmCheckMagic(Fsm fp, Mbuf bp)
{
  u_int32_t	peer_magic;
  u_int32_t	peer_magic_ought;

  /* Read magic number */
  bp = mbread(bp, &peer_magic, sizeof(peer_magic));
  peer_magic = ntohl(peer_magic);

  /* What should magic number be? */
  if (PROT_LINK_LAYER(fp->type->proto))
    peer_magic_ought = ((Link)(fp->arg))->lcp.peer_magic;
  else
    peer_magic_ought = 0;

  /* Verify */
  if (fp->conf.check_magic && peer_magic != 0
     && peer_magic != peer_magic_ought) {
    Log(fp->log, ("[%s] %s: magic number is wrong: 0x%08x != 0x%08x",
      Pref(fp), Fsm(fp), peer_magic, peer_magic_ought));
    FsmFailure(fp, FAIL_RECD_BADMAGIC);
  }
  return(bp);
}

/*
 * FsmEchoTimeout()
 */

static void
FsmEchoTimeout(void *arg)
{
    Fsm			const fp = (Fsm) arg;
    Bund		b;
    Link		l;
    struct ng_ppp_link_stat	oldStats;

    if (fp->type->link_layer) {
	l = (Link)fp->arg;
	b = l->bund;
    } else {
	b = (Bund)fp->arg;
	l = NULL;
    }
    
    if (!b) {
	/* We can't get link stat without bundle present */
	return;
    }

    /* See if there was any traffic since last time */
    oldStats = fp->idleStats;
    NgFuncGetStats(b, l ?
	l->bundleIndex : NG_PPP_BUNDLE_LINKNUM, &fp->idleStats);
    if (fp->idleStats.recvFrames > oldStats.recvFrames)
	fp->quietCount = 0;
    else
	fp->quietCount++;

    /* See if peer hasn't responded for too many requests */
    switch (fp->quietCount) {

    /* Peer failed to reply to previous echo request */
    default:
        Log(LG_ECHO|fp->log,
	    ("[%s] %s: no reply to %d echo request(s)",
	    Pref(fp), Fsm(fp), fp->quietCount - 1));

        /* Has peer failed to reply for maximum allowable interval? */
        if (fp->quietCount * fp->conf.echo_int >= fp->conf.echo_max) {
	    TimerStop(&fp->echoTimer);
	    FsmFailure(fp, FAIL_ECHO_TIMEOUT);
	    break;
        }
        /* fall through */
    case 1:	/* one interval of silence elapsed; send first echo request */
        FsmSendEchoReq(fp, NULL);
        /* fall through */
    case 0:
        break;
    }
}

/*
 * FsmInput()
 *
 * Deal with an incoming packet for FSM pointed to by "fp"
 */

void
FsmInput(Fsm fp, Mbuf bp)
{
    int			log, recd_len, length;
    struct fsmheader	hdr;

    /* Check for runt frames; discard them */
    if ((recd_len = MBLEN(bp)) < sizeof(hdr)) {
	Log(fp->log, ("[%s] %s: runt packet: %d bytes", Pref(fp), Fsm(fp), recd_len));
	mbfree(bp);
	return;
    }

    /* Read in the header */
    bp = mbread(bp, &hdr, sizeof(hdr));
    length = ntohs(hdr.length);

    /* Make sure length is sensible; discard otherwise */
    if (length < sizeof(hdr) || length > recd_len) {
	Log(fp->log, ("[%s] %s: bad length: says %d, rec'd %d",
    	    Pref(fp), Fsm(fp), length, recd_len));
	mbfree(bp);
	return;
    }

    /* Truncate off any padding bytes */
    if (length < recd_len)
	bp = mbtrunc(bp, length - sizeof(hdr));

    /* Check for a valid code byte -- if not, send code-reject */
    if ((hdr.code >= NUM_FSM_CODES) ||
	    (((1 << hdr.code) & fp->type->known_codes) == 0)) {	/* RUC */
	Log(fp->log, ("[%s] %s: unknown code %d", Pref(fp), Fsm(fp), hdr.code));
	FsmOutputMbuf(fp, CODE_CODEREJ, fp->rejid++, bp);
	return;
    }

    /* Log it */
    if (hdr.code == CODE_ECHOREQ || hdr.code == CODE_ECHOREP)
	log = LG_ECHO;
    else if (hdr.code == CODE_RESETREQ || hdr.code == CODE_RESETACK)
	log = fp->log2;
    else
	log = fp->log;
    Log(log, ("[%s] %s: rec'd %s #%d (%s)",
	Pref(fp), Fsm(fp), FsmCodeName(hdr.code), (int) hdr.id,
	FsmStateName(fp->state)));

    /* Do whatever */
    (*FsmCodes[hdr.code].action)(fp, &hdr, bp);
}

/*
 * FsmConfValue()
 *
 * Write in a configuration item with "len" bytes of information.
 * Special cases: length -2 or -4 means convert to network byte order.
 */

u_char *
FsmConfValue(u_char *cp, int type, int len, const void *data)
{
  u_char	*bytes = (u_char *) data;
  u_int16_t	sv;
  u_int32_t	lv;

  /* Special cases */
  switch (len) {
    case -2:
      len = 2;
      sv = htons(*((u_int16_t *) data));
      bytes = (u_char *) &sv;
      break;
    case -4:
      len = 4;
      lv = htonl(*((u_int32_t *) data));
      bytes = (u_char *) &lv;
      break;
    default:
      break;
  }

  /* Add header and data */
  *cp++ = type;
  *cp++ = len + 2;
  while (len-- > 0)
    *cp++ = *bytes++;

  /* Done */
  return(cp);
}

/*
 * FsmExtractOptions()
 *
 * Extract options from a config packet
 */

static int
FsmExtractOptions(Fsm fp, u_char *data, int dlen, FsmOption opts, int max)
{
  int	num;

  for (num = 0; dlen >= 2 && num < max; num++) {
    FsmOption	const opt = &opts[num];

    opt->type = data[0];
    opt->len = data[1];
    opt->data = &data[2];
    if (opt->len < 2 || opt->len > dlen)
      break;
    data += opt->len;
    dlen -= opt->len;
  }
  if (dlen != 0)
    LogDumpBuf(LG_ERR, data, dlen,
      "[%s] %s: %d extra garbage bytes in config packet", Pref(fp), Fsm(fp), dlen);
  return(num);
}

/*
 * FsmFindOption()
 */

FsmOptInfo
FsmFindOptInfo(FsmOptInfo list, u_char type)
{
  for (; list->name; list++)
    if (list->type == type)
      return(list);
  return(NULL);
}

/*
 * FsmDecodeBuffer()
 */

static void
FsmDecodeBuffer(Fsm fp, u_char *buf, int size, int mode)
{
  struct fsmoption	opts[FSM_MAX_OPTS];
  int			num;

  if (mode == MODE_REQ)
    gAckSize = gNakSize = gRejSize = 0;
  num = FsmExtractOptions(fp, buf, size, opts, FSM_MAX_OPTS);
  (*fp->type->DecodeConfig)(fp, opts, num, mode);
}

/*
 * FsmAck()
 */

void
FsmAck(Fsm fp, const struct fsmoption *opt)
{
  if (gAckSize + opt->len > sizeof(gAckBuf)) {
    Log(LG_ERR, ("[%s] %s: %s buffer full", Pref(fp), Fsm(fp), "ack"));
    return;
  }
  memcpy(&gAckBuf[gAckSize], opt, 2);
  memcpy(&gAckBuf[gAckSize + 2], opt->data, opt->len - 2);
  gAckSize += opt->len;
}

/*
 * FsmNak()
 */

void
FsmNak(Fsm fp, const struct fsmoption *opt)
{
  if (gNakSize + opt->len > sizeof(gNakBuf)) {
    Log(LG_ERR, ("[%s] %s: %s buffer full", Pref(fp), Fsm(fp), "nak"));
    return;
  }
  memcpy(&gNakBuf[gNakSize], opt, 2);
  memcpy(&gNakBuf[gNakSize + 2], opt->data, opt->len - 2);
  gNakSize += opt->len;
}

/*
 * FsmRej()
 */

void
FsmRej(Fsm fp, const struct fsmoption *opt)
{
  if (gRejSize + opt->len > sizeof(gRejBuf)) {
    Log(LG_ERR, ("[%s] %s: %s buffer full", Pref(fp), Fsm(fp), "rej"));
    return;
  }
  memcpy(&gRejBuf[gRejSize], opt, 2);
  memcpy(&gRejBuf[gRejSize + 2], opt->data, opt->len - 2);
  gRejSize += opt->len;
}

/*
 * FsmCodeName()
 */

const char *
FsmCodeName(int code)
{
  if (code >= 0 && code < NUM_FSM_CODES)
    return (FsmCodes[code].name);
  return ("UNKNOWN");
}

/*
 * FsmStateName()
 */

const char *
FsmStateName(enum fsm_state state)
{
  switch (state) {
    case ST_INITIAL:	return "Initial";
    case ST_STARTING:	return "Starting";
    case ST_CLOSED:	return "Closed";
    case ST_STOPPED:	return "Stopped";
    case ST_CLOSING:	return "Closing";
    case ST_STOPPING:	return "Stopping";
    case ST_REQSENT:	return "Req-Sent";
    case ST_ACKRCVD:	return "Ack-Rcvd";
    case ST_ACKSENT:	return "Ack-Sent";
    case ST_OPENED:	return "Opened";
  }
  return "???";
}

