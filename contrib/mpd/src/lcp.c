
/*
 * lcp.c
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
#include "lcp.h"
#include "fsm.h"
#include "mp.h"
#include "phys.h"
#include "link.h"
#include "msg.h"
#include "util.h"

/*
 * DEFINITIONS
 */

  #define LCP_ECHO_INTERVAL	5	/* Enable keep alive by default */
  #define LCP_ECHO_TIMEOUT	40

  #define LCP_KNOWN_CODES	(   (1 << CODE_CONFIGREQ)	\
				  | (1 << CODE_CONFIGACK)	\
				  | (1 << CODE_CONFIGNAK)	\
				  | (1 << CODE_CONFIGREJ)	\
				  | (1 << CODE_TERMREQ)		\
				  | (1 << CODE_TERMACK)		\
				  | (1 << CODE_CODEREJ)		\
				  | (1 << CODE_PROTOREJ)	\
				  | (1 << CODE_ECHOREQ)		\
				  | (1 << CODE_ECHOREP)		\
				  | (1 << CODE_DISCREQ)		\
				  | (1 << CODE_IDENT)		\
				  | (1 << CODE_TIMEREM)		)

  #define LCP_PEER_REJECTED(p,x)	((p)->peer_reject & (1<<x))
  #define LCP_PEER_REJ(p,x)	do{(p)->peer_reject |= (1<<(x));}while(0)
  #define LCP_PEER_UNREJ(p,x)	do{(p)->peer_reject &= ~(1<<(x));}while(0)

/*
 * INTERNAL FUNCTIONS
 */

  static void	LcpConfigure(Fsm fp);
  static void	LcpNewState(Fsm fp, enum fsm_state old, enum fsm_state new);
  static void	LcpNewPhase(Link l, enum lcp_phase new);

  static u_char	*LcpBuildConfigReq(Fsm fp, u_char *cp);
  static void	LcpDecodeConfig(Fsm fp, FsmOption list, int num, int mode);
  static void	LcpLayerDown(Fsm fp);
  static void	LcpLayerStart(Fsm fp);
  static void	LcpLayerFinish(Fsm fp);
  static int	LcpRecvProtoRej(Fsm fp, int proto, Mbuf bp);
  static void	LcpFailure(Fsm fp, enum fsmfail reason);
  static const struct fsmoption	*LcpAuthProtoNak(ushort proto, u_char alg);
  static short 	LcpFindAuthProto(ushort proto, u_char alg);
  static void	LcpRecvIdent(Fsm fp, Mbuf bp);
  static void	LcpStopActivity(Link l);

/*
 * INTERNAL VARIABLES
 */

  static const struct fsmoptinfo	gLcpConfOpts[] = {
    { "VENDOR", TY_VENDOR, 4, 255, TRUE },
    { "MRU", TY_MRU, 2, 2, TRUE },
    { "ACCMAP", TY_ACCMAP, 4, 4, TRUE },
    { "AUTHPROTO", TY_AUTHPROTO, 2, 255, TRUE },
    { "QUALPROTO", TY_QUALPROTO, 0, 0, FALSE },
    { "MAGICNUM", TY_MAGICNUM, 4, 4, TRUE },
    { "RESERVED", TY_RESERVED, 0, 0, FALSE }, /* DEPRECATED */
    { "PROTOCOMP", TY_PROTOCOMP, 0, 0, TRUE },
    { "ACFCOMP", TY_ACFCOMP, 0, 0, TRUE },
    { "FCSALT", TY_FCSALT, 0, 0, FALSE },
    { "SDP", TY_SDP, 0, 0, FALSE },
    { "NUMMODE", TY_NUMMODE, 0, 0, FALSE },
    { "MULTILINK", TY_MULTILINK, 0, 0, FALSE }, /* DEPRECATED */
    { "CALLBACK", TY_CALLBACK, 1, 255, TRUE },
    { "CONNECTTIME", TY_CONNECTTIME, 0, 0, FALSE }, /* DEPRECATED */
    { "COMPFRAME", TY_COMPFRAME, 0, 0, FALSE }, /* DEPRECATED */
    { "NDS", TY_NDS, 0, 0, FALSE }, /* DEPRECATED */
    { "MP MRRU", TY_MRRU, 2, 2, TRUE },
    { "MP SHORTSEQ", TY_SHORTSEQNUM, 0, 0, TRUE },
    { "ENDPOINTDISC", TY_ENDPOINTDISC, 1, 255, TRUE },
    { "PROPRIETARY", TY_PROPRIETARY, 0, 0, FALSE },
    { "DCEIDENTIFIER", TY_DCEIDENTIFIER, 0, 0, FALSE },
    { "MULTILINKPLUS", TY_MULTILINKPLUS, 0, 0, FALSE },
    { "BACP", TY_BACP, 0, 0, FALSE },
    { "LCPAUTHOPT", TY_LCPAUTHOPT, 0, 0, FALSE },
    { "COBS", TY_COBS, 0, 0, FALSE },
    { "PREFIXELISION", TY_PREFIXELISION, 0, 0, FALSE },
    { "MULTILINKHEADERFMT", TY_MULTILINKHEADERFMT, 0, 0, FALSE },
    { "INTERNAT", TY_INTERNAT, 0, 0, FALSE },
    { "SDATALINKSONET", TY_SDATALINKSONET, 0, 0, FALSE },
    { NULL }
  };

  static struct fsmtype gLcpFsmType = {
    "LCP",			/* Name of protocol */
    PROTO_LCP,			/* Protocol Number */
    LCP_KNOWN_CODES,
    TRUE,
    LG_LCP, LG_LCP2,
    LcpNewState,
    NULL,
    LcpLayerDown,
    LcpLayerStart,
    LcpLayerFinish,
    LcpBuildConfigReq,
    LcpDecodeConfig,
    LcpConfigure,
    NULL,
    NULL,
    NULL,
    NULL,
    LcpRecvProtoRej,
    LcpFailure,
    NULL,
    NULL,
    LcpRecvIdent,
  };

  /* List of possible Authentication Protocols */
  static struct lcpauthproto gLcpAuthProtos[] = {
    {
      PROTO_PAP,
      0,
      LINK_CONF_PAP,
    },
    {
      PROTO_CHAP,
      CHAP_ALG_MD5,
      LINK_CONF_CHAPMD5
    },
    {
      PROTO_CHAP,
      CHAP_ALG_MSOFT,
      LINK_CONF_CHAPMSv1
    },
    {
      PROTO_CHAP,
      CHAP_ALG_MSOFTv2,
      LINK_CONF_CHAPMSv2
    },
    {
      PROTO_EAP,
      0,
      LINK_CONF_EAP
    }

  };

  static const char *PhaseNames[] = {
    "DEAD",
    "ESTABLISH",
    "AUTHENTICATE",
    "NETWORK",
    "TERMINATE",
  };

/*
 * LcpInit()
 */

void
LcpInit(Link l)
{
  LcpState	const lcp = &l->lcp;

  memset(lcp, 0, sizeof(*lcp));
  FsmInit(&lcp->fsm, &gLcpFsmType, l);
  lcp->fsm.conf.echo_int = LCP_ECHO_INTERVAL;
  lcp->fsm.conf.echo_max = LCP_ECHO_TIMEOUT;
  lcp->phase = PHASE_DEAD;
  
  AuthInit(l);
}

/*
 * LcpInst()
 */

void
LcpInst(Link l, Link lt)
{
  LcpState	const lcp = &l->lcp;

  memcpy(lcp, &lt->lcp, sizeof(*lcp));
  FsmInst(&lcp->fsm, &lt->lcp.fsm, l);
  AuthInst(&lcp->auth, &lt->lcp.auth);
}

/*
 * LcpShutdown()
 */

void
LcpShutdown(Link l)
{
    AuthShutdown(l);
}

/*
 * LcpConfigure()
 */

static void
LcpConfigure(Fsm fp)
{
    Link	l = (Link)fp->arg;
    LcpState	const lcp = &l->lcp;
    short	i;

    /* FSM stuff */
    lcp->fsm.conf.passive = Enabled(&l->conf.options, LINK_CONF_PASSIVE);
    lcp->fsm.conf.check_magic =
	Enabled(&l->conf.options, LINK_CONF_CHECK_MAGIC);
    lcp->peer_reject = 0;

    /* Initialize normal LCP stuff */
    lcp->peer_mru = l->conf.mtu;
    lcp->want_mru = l->conf.mru;
    if (l->type && (lcp->want_mru > l->type->mru))
	lcp->want_mru = l->type->mru;
    lcp->peer_accmap = 0xffffffff;
    lcp->want_accmap = l->conf.accmap;
    lcp->peer_acfcomp = FALSE;
    lcp->want_acfcomp = Enabled(&l->conf.options, LINK_CONF_ACFCOMP);
    lcp->peer_protocomp = FALSE;
    lcp->want_protocomp = Enabled(&l->conf.options, LINK_CONF_PROTOCOMP);
    lcp->peer_magic = 0;
    lcp->want_magic = Enabled(&l->conf.options,
	LINK_CONF_MAGICNUM) ? GenerateMagic() : 0;
    if (l->originate == LINK_ORIGINATE_LOCAL)
	lcp->want_callback = Enabled(&l->conf.options, LINK_CONF_CALLBACK);
    else
	lcp->want_callback = FALSE;

    /* Authentication stuff */
    lcp->peer_auth = 0;
    lcp->want_auth = 0;
    lcp->peer_alg = 0;
    lcp->want_alg = 0;
    lcp->peer_ident[0] = 0;

    memset(lcp->want_protos, 0, sizeof(lcp->want_protos));
    /* fill my list of possible auth-protos, most to least secure */
    /* prefer MS-CHAP to others to get encryption keys */
    lcp->want_protos[0] = &gLcpAuthProtos[LINK_CONF_CHAPMSv2];
    lcp->want_protos[1] = &gLcpAuthProtos[LINK_CONF_CHAPMSv1];
    lcp->want_protos[2] = &gLcpAuthProtos[LINK_CONF_CHAPMD5];
    lcp->want_protos[3] = &gLcpAuthProtos[LINK_CONF_PAP];
    lcp->want_protos[4] = &gLcpAuthProtos[LINK_CONF_EAP];

    /* Use the same list for the MODE_REQ */
    memcpy(lcp->peer_protos, lcp->want_protos, sizeof(lcp->peer_protos));

    for (i = 0; i < LCP_NUM_AUTH_PROTOS; i++) {
	if (Enabled(&l->conf.options, lcp->want_protos[i]->conf) && lcp->want_auth == 0) {
	    lcp->want_auth = lcp->want_protos[i]->proto;
	    lcp->want_alg = lcp->want_protos[i]->alg;
    	    /* avoid re-requesting this proto, if it was nak'd by the peer */
    	    lcp->want_protos[i] = NULL;
	} else if (!Enabled(&l->conf.options, lcp->want_protos[i]->conf)) {
    	    /* don't request disabled Protos */
    	    lcp->want_protos[i] = NULL;
	}

	/* remove all denied protos */
	if (!Acceptable(&l->conf.options, lcp->peer_protos[i]->conf))
    	    lcp->peer_protos[i] = NULL;
    }

    /* Multi-link stuff */
    lcp->peer_mrru = 0;
    lcp->peer_shortseq = FALSE;
    if (Enabled(&l->conf.options, LINK_CONF_MULTILINK)) {
	lcp->want_mrru = l->conf.mrru;
	lcp->want_shortseq = Enabled(&l->conf.options, LINK_CONF_SHORTSEQ);
    } else {
	lcp->want_mrru = 0;
	lcp->want_shortseq = FALSE;
    }

    /* Peer discriminator */
    lcp->peer_discrim.class = DISCRIM_CLASS_NULL;
    lcp->peer_discrim.len = 0;
}

/*
 * LcpNewState()
 *
 * Keep track of phase shifts
 */

static void
LcpNewState(Fsm fp, enum fsm_state old, enum fsm_state new)
{
    Link	l = (Link)fp->arg;

  switch (old) {
    case ST_INITIAL:			/* DEAD */
    case ST_STARTING:
      switch (new) {
	case ST_INITIAL:
	  /* fall through */
	case ST_STARTING:
	  break;
	default:
	  LcpNewPhase(l, PHASE_ESTABLISH);
	  break;
      }
      break;

    case ST_CLOSED:			/* ESTABLISH */
    case ST_STOPPED:
      switch (new) {
	case ST_INITIAL:
	case ST_STARTING:
	  LcpNewPhase(l, PHASE_DEAD);
	  break;
	default:
	  break;
      }
      break;

    case ST_CLOSING:			/* TERMINATE */
    case ST_STOPPING:
      switch (new) {
	case ST_INITIAL:
	case ST_STARTING:
	  LcpNewPhase(l, PHASE_DEAD);
	  break;
	case ST_CLOSED:
	case ST_STOPPED:
	  LcpNewPhase(l, PHASE_ESTABLISH);
	  break;
	default:
	  break;
      }
      break;

    case ST_REQSENT:			/* ESTABLISH */
    case ST_ACKRCVD:
    case ST_ACKSENT:
      switch (new) {
	case ST_INITIAL:
	case ST_STARTING:
	  LcpNewPhase(l, PHASE_DEAD);
	  break;
	case ST_CLOSING:
	case ST_STOPPING:
	  LcpNewPhase(l, PHASE_TERMINATE);
	  break;
	case ST_OPENED:
	  LcpNewPhase(l, PHASE_AUTHENTICATE);
	  break;
	default:
	  break;
      }
      break;

    case ST_OPENED:			/* AUTHENTICATE, NETWORK */
      switch (new) {
	case ST_STARTING:
	  LcpNewPhase(l, PHASE_DEAD);
	  break;
	case ST_REQSENT:
	case ST_ACKSENT:
	  LcpNewPhase(l, PHASE_ESTABLISH);
	  break;
	case ST_CLOSING:
	case ST_STOPPING:
	  LcpNewPhase(l, PHASE_TERMINATE);
	  break;
	default:
	  assert(0);
      }
      break;

    default:
      assert(0);
  }

    LinkShutdownCheck(l, new);
}

/*
 * LcpNewPhase()
 */

static void
LcpNewPhase(Link l, enum lcp_phase new)
{
    LcpState	const lcp = &l->lcp;
    enum lcp_phase	old = lcp->phase;

    /* Logit */
    Log(LG_LCP2, ("[%s] %s: phase shift %s --> %s",
	Pref(&lcp->fsm), Fsm(&lcp->fsm), PhaseNames[old], PhaseNames[new]));

    /* Sanity check transition (The picture on RFC 1661 p. 6 is incomplete) */
    switch (old) {
	case PHASE_DEAD:
    	    assert(new == PHASE_ESTABLISH);
    	    break;
    case PHASE_ESTABLISH:
        assert(new == PHASE_DEAD
	  || new == PHASE_TERMINATE
	  || new == PHASE_AUTHENTICATE);
        break;
    case PHASE_AUTHENTICATE:
        assert(new == PHASE_TERMINATE
	  || new == PHASE_ESTABLISH
	  || new == PHASE_NETWORK
	  || new == PHASE_DEAD);
        break;
    case PHASE_NETWORK:
        assert(new == PHASE_TERMINATE
	  || new == PHASE_ESTABLISH
	  || new == PHASE_DEAD);
        break;
    case PHASE_TERMINATE:
        assert(new == PHASE_ESTABLISH
	  || new == PHASE_DEAD);
        break;
    default:
        assert(0);
    }

    /* Change phase now */
    lcp->phase = new;

    /* Do whatever for leaving old phase */
    switch (old) {
    case PHASE_AUTHENTICATE:
      if (new != PHASE_NETWORK)
	AuthCleanup(l);
      break;

    case PHASE_NETWORK:
      if (l->joined_bund)
	BundLeave(l);
      AuthCleanup(l);
      break;

    default:
      break;
    }

    /* Do whatever for entering new phase */
    switch (new) {
    case PHASE_ESTABLISH:
      break;

    case PHASE_AUTHENTICATE:
      if (!PhysIsSync(l))
        PhysSetAccm(l, lcp->peer_accmap, lcp->want_accmap);
      AuthStart(l);
      break;

    case PHASE_NETWORK:
      /* Send ident string, if configured */
      if (l->conf.ident != NULL)
	FsmSendIdent(&lcp->fsm, l->conf.ident);

      /* Send Time-Remaining if known */
      if (Enabled(&l->conf.options, LINK_CONF_TIMEREMAIN) &&
    	    lcp->auth.params.session_timeout != 0)
	FsmSendTimeRemaining(&lcp->fsm, lcp->auth.params.session_timeout);

      /* Join my bundle */
      if (!BundJoin(l)) {
	  Log(LG_LINK|LG_BUND,
	    ("[%s] link did not validate in bundle",
	    l->name));
	  RecordLinkUpDownReason(NULL, l,
	    0, STR_PROTO_ERR, "%s", STR_MULTI_FAIL);
	  FsmFailure(&l->lcp.fsm, FAIL_NEGOT_FAILURE);
      } else {
    	    /* If link connection complete, reset redial counter */
	    l->num_redial = 0;
      }
      break;

    case PHASE_TERMINATE:
      break;

    case PHASE_DEAD:
        break;

    default:
      assert(0);
    }
}

/*
 * LcpAuthResult()
 */

void
LcpAuthResult(Link l, int success)
{
  Log(LG_AUTH|LG_LCP, ("[%s] %s: authorization %s",
    Pref(&l->lcp.fsm), Fsm(&l->lcp.fsm), success ? "successful" : "failed"));
  if (success) {
    if (l->lcp.phase != PHASE_NETWORK)
      LcpNewPhase(l, PHASE_NETWORK);
  } else {
    RecordLinkUpDownReason(NULL, l, 0, STR_LOGIN_FAIL,
      "%s", STR_PPP_AUTH_FAILURE);
    FsmFailure(&l->lcp.fsm, FAIL_NEGOT_FAILURE);
  }
}

/*
 * LcpStat()
 */

int
LcpStat(Context ctx, int ac, char *av[], void *arg)
{
    Link	const l = ctx->lnk;
    LcpState	const lcp = &l->lcp;
    char	buf[64];

    Printf("%s [%s]\r\n", lcp->fsm.type->name, FsmStateName(lcp->fsm.state));

    Printf("Self:\r\n");
    Printf(	"\tMRU      : %d bytes\r\n"
		"\tMAGIC    : 0x%08x\r\n"
		"\tACCMAP   : 0x%08x\r\n"
		"\tACFCOMP  : %s\r\n"
		"\tPROTOCOMP: %s\r\n"
		"\tAUTHTYPE : %s\r\n",
	(int) lcp->want_mru,
	(int) lcp->want_magic,
	(int) lcp->want_accmap,
	lcp->want_acfcomp ? "Yes" : "No",
	lcp->want_protocomp ? "Yes" : "No",
	(lcp->want_auth)?ProtoName(lcp->want_auth):"none");

    if (lcp->want_mrru) {
	Printf(	"\tMRRU     : %d bytes\r\n", (int) lcp->want_mrru);
	Printf(	"\tSHORTSEQ : %s\r\n", lcp->want_shortseq ? "Yes" : "No");
	Printf(	"\tENDPOINTDISC: %s\r\n", MpDiscrimText(&self_discrim, buf, sizeof(buf)));
    }
    if (l->conf.ident)
	Printf(	"\tIDENT    : %s\r\n", l->conf.ident);

    Printf("Peer:\r\n");
    Printf(	"\tMRU      : %d bytes\r\n"
		"\tMAGIC    : 0x%08x\r\n"
		"\tACCMAP   : 0x%08x\r\n"
		"\tACFCOMP  : %s\r\n"
		"\tPROTOCOMP: %s\r\n"
		"\tAUTHTYPE : %s\r\n",
	(int) lcp->peer_mru,
        (int) lcp->peer_magic,
	(int) lcp->peer_accmap,
        lcp->peer_acfcomp ? "Yes" : "No",
        lcp->peer_protocomp ? "Yes" : "No",
        (lcp->peer_auth)?ProtoName(lcp->peer_auth):"none");

    if (lcp->peer_mrru) {
	Printf(	"\tMRRU     : %d bytes\r\n", (int) lcp->peer_mrru);
	Printf(	"\tSHORTSEQ : %s\r\n", lcp->peer_shortseq ? "Yes" : "No");
	Printf(	"\tENDPOINTDISC: %s\r\n", MpDiscrimText(&lcp->peer_discrim, buf, sizeof(buf)));
    }
    if (lcp->peer_ident[0])
	Printf(	"\tIDENT    : %s\r\n", lcp->peer_ident);

    return(0);
}

/*
 * LcpBuildConfigReq()
 */

static u_char *
LcpBuildConfigReq(Fsm fp, u_char *cp)
{
    Link	l = (Link)fp->arg;
    LcpState	const lcp = &l->lcp;

    /* Standard stuff */
    if (lcp->want_acfcomp && !LCP_PEER_REJECTED(lcp, TY_ACFCOMP))
	cp = FsmConfValue(cp, TY_ACFCOMP, 0, NULL);
    if (lcp->want_protocomp && !LCP_PEER_REJECTED(lcp, TY_PROTOCOMP))
	cp = FsmConfValue(cp, TY_PROTOCOMP, 0, NULL);
    if ((!PhysIsSync(l)) && (!LCP_PEER_REJECTED(lcp, TY_ACCMAP)))
	cp = FsmConfValue(cp, TY_ACCMAP, -4, &lcp->want_accmap);
    if (!LCP_PEER_REJECTED(lcp, TY_MRU))
	cp = FsmConfValue(cp, TY_MRU, -2, &lcp->want_mru);
    if (lcp->want_magic && !LCP_PEER_REJECTED(lcp, TY_MAGICNUM))
	cp = FsmConfValue(cp, TY_MAGICNUM, -4, &lcp->want_magic);
    if (lcp->want_callback && !LCP_PEER_REJECTED(lcp, TY_CALLBACK)) {
	struct {
    	    u_char	op;
    	    u_char	data[0];
	} s_callback;

	s_callback.op = 0;
	cp = FsmConfValue(cp, TY_CALLBACK, 1, &s_callback);
    }

    /* Authorization stuff */
    switch (lcp->want_auth) {
	case PROTO_PAP:
	case PROTO_EAP:
    	    cp = FsmConfValue(cp, TY_AUTHPROTO, -2, &lcp->want_auth);
    	    break;
	case PROTO_CHAP: {
	    struct {
		u_short	want_auth;
		u_char	alg;
	    } s_mdx;

	    s_mdx.want_auth = htons(PROTO_CHAP);
	    s_mdx.alg = lcp->want_alg;
	    cp = FsmConfValue(cp, TY_AUTHPROTO, 3, &s_mdx);
        }
        break;
    }

    /* Multi-link stuff */
    if (Enabled(&l->conf.options, LINK_CONF_MULTILINK)
    	    && !LCP_PEER_REJECTED(lcp, TY_MRRU)) {
	cp = FsmConfValue(cp, TY_MRRU, -2, &lcp->want_mrru);
	if (lcp->want_shortseq && !LCP_PEER_REJECTED(lcp, TY_SHORTSEQNUM))
    	    cp = FsmConfValue(cp, TY_SHORTSEQNUM, 0, NULL);
	if (!LCP_PEER_REJECTED(lcp, TY_ENDPOINTDISC))
    	    cp = FsmConfValue(cp, TY_ENDPOINTDISC, 1 + self_discrim.len, &self_discrim.class);
    }

    /* Done */
    return(cp);
}

static void
LcpLayerStart(Fsm fp)
{
    Link	l = (Link)fp->arg;
  
    LinkNgInit(l);
    if (!TimerStarted(&l->openTimer))
	PhysOpen(l);
}

static void
LcpStopActivity(Link l)
{
    AuthStop(l);
}

static void
LcpLayerFinish(Fsm fp)
{
    Link	l = (Link)fp->arg;

    LcpStopActivity(l);
    if (!l->rep) {
	PhysClose(l);
	LinkNgShutdown(l);
    }
}

/*
 * LcpLayerDown()
 */

static void
LcpLayerDown(Fsm fp)
{
    Link	l = (Link)fp->arg;
    LcpStopActivity(l);
}

void LcpOpen(Link l)
{
  FsmOpen(&l->lcp.fsm);
}

void LcpClose(Link l)
{
  FsmClose(&l->lcp.fsm);
}

void LcpUp(Link l)
{
  FsmUp(&l->lcp.fsm);
}

void LcpDown(Link l)
{
  FsmDown(&l->lcp.fsm);
}

/*
 * LcpRecvProtoRej()
 */

static int
LcpRecvProtoRej(Fsm fp, int proto, Mbuf bp)
{
    Link	l = (Link)fp->arg;
  int	fatal = FALSE;
  Fsm	rej = NULL;

  /* Which protocol? */
  switch (proto) {
    case PROTO_CCP:
    case PROTO_COMPD:
      rej = l->bund ? &l->bund->ccp.fsm : NULL;
      break;
    case PROTO_ECP:
    case PROTO_CRYPT:
      rej = l->bund ? &l->bund->ecp.fsm : NULL;
      break;
    case PROTO_IPCP:
      rej = l->bund ? &l->bund->ipcp.fsm : NULL;
      break;
    case PROTO_IPV6CP:
      rej = l->bund ? &l->bund->ipv6cp.fsm : NULL;
      break;
    default:
      break;
  }

  /* Turn off whatever protocol got rejected */
  if (rej)
    FsmFailure(rej, FAIL_WAS_PROTREJ);
  return(fatal);
}

/*
 * LcpRecvIdent()
 */

static void
LcpRecvIdent(Fsm fp, Mbuf bp)
{
    Link	l = (Link)fp->arg;
    int		len, clen;

    if (bp == NULL)
	return;

    if (l->lcp.peer_ident[0] != 0)
	strlcat(l->lcp.peer_ident, " ", sizeof(l->lcp.peer_ident));
  
    len = strlen(l->lcp.peer_ident);
    clen = sizeof(l->lcp.peer_ident) - len - 1;
    if (clen > MBLEN(bp))
	clen = MBLEN(bp);
    memcpy(l->lcp.peer_ident + len, (char *) MBDATAU(bp), clen);
    l->lcp.peer_ident[len + clen] = 0;
}

/*
 * LcpFailure()
 */

static void
LcpFailure(Fsm fp, enum fsmfail reason)
{
    Link	l = (Link)fp->arg;
  char	buf[100];

  snprintf(buf, sizeof(buf), STR_LCP_FAILED, FsmFailureStr(reason));
  RecordLinkUpDownReason(NULL, l, 0, reason == FAIL_ECHO_TIMEOUT ?
    STR_ECHO_TIMEOUT : STR_PROTO_ERR, "%s", buf);
}

/*
 * LcpDecodeConfig()
 */

static void
LcpDecodeConfig(Fsm fp, FsmOption list, int num, int mode)
{
    Link	l = (Link)fp->arg;
    LcpState	const lcp = &l->lcp;
    int		k;

    /* If we have got request, forget the previous values */
    if (mode == MODE_REQ) {
	lcp->peer_mru = l->conf.mtu;
	lcp->peer_accmap = 0xffffffff;
	lcp->peer_acfcomp = FALSE;
	lcp->peer_protocomp = FALSE;
	lcp->peer_magic = 0;
	lcp->peer_auth = 0;
	lcp->peer_alg = 0;
	lcp->peer_mrru = 0;
	lcp->peer_shortseq = FALSE;
    }

  /* Decode each config option */
  for (k = 0; k < num; k++) {
    FsmOption	const opt = &list[k];
    FsmOptInfo	const oi = FsmFindOptInfo(gLcpConfOpts, opt->type);

    /* Check option */
    if (!oi) {
      Log(LG_LCP, ("[%s]   UNKNOWN[%d] len=%d", l->name, opt->type, opt->len));
      if (mode == MODE_REQ)
	FsmRej(fp, opt);
      continue;
    }
    if (!oi->supported) {
      Log(LG_LCP, ("[%s]   %s", l->name, oi->name));
      if (mode == MODE_REQ) {
	Log(LG_LCP, ("[%s]     Not supported", l->name));
	FsmRej(fp, opt);
      }
      continue;
    }
    if (opt->len < oi->minLen + 2 || opt->len > oi->maxLen + 2) {
      Log(LG_LCP, ("[%s]   %s", l->name, oi->name));
      if (mode == MODE_REQ) {
	Log(LG_LCP, ("[%s]     Bogus length=%d", l->name, opt->len));
	FsmRej(fp, opt);
      }
      continue;
    }

    /* Do whatever */
    switch (opt->type) {
      case TY_MRU:		/* link MRU */
	{
	  u_int16_t	mru;

	  memcpy(&mru, opt->data, 2);
	  mru = ntohs(mru);
	  Log(LG_LCP, ("[%s]   %s %d", l->name, oi->name, mru));
	  switch (mode) {
	    case MODE_REQ:
	      if (mru < LCP_MIN_MRU) {
		mru = htons(LCP_MIN_MRU);
		memcpy(opt->data, &mru, 2);
		FsmNak(fp, opt);
		break;
	      }
	      if (mru < lcp->peer_mru)
		lcp->peer_mru = mru;
	      FsmAck(fp, opt);
	      break;
	    case MODE_NAK:
	      /* Windows 2000 PPPoE bug workaround */
	      if (mru == lcp->want_mru) {
	        LCP_PEER_REJ(lcp, opt->type);
		break;
	      }
	      if (mru >= LCP_MIN_MRU
		  && (mru <= l->type->mru || mru < lcp->want_mru))
		lcp->want_mru = mru;
	      break;
	    case MODE_REJ:
	      LCP_PEER_REJ(lcp, opt->type);
	      break;
	  }
	}
	break;

      case TY_ACCMAP:		/* async control character escape map */
	{
	  u_int32_t	accm;

	  memcpy(&accm, opt->data, 4);
	  accm = ntohl(accm);
	  Log(LG_LCP, ("[%s]   %s 0x%08x", l->name, oi->name, accm));
	  switch (mode) {
	    case MODE_REQ:
	      lcp->peer_accmap = accm;
	      FsmAck(fp, opt);
	      break;
	    case MODE_NAK:
	      lcp->want_accmap = accm;
	      break;
	    case MODE_REJ:
	      LCP_PEER_REJ(lcp, opt->type);
	      break;
	  }
	}
	break;

      case TY_AUTHPROTO:		/* authentication protocol */
	{
	  u_int16_t		proto;
	  int			bogus = 0, i, protoPos = -1;
	  LcpAuthProto 		authProto = NULL;

	  memcpy(&proto, opt->data, 2);
	  proto = ntohs(proto);

	  /* Display it */
	  switch (proto) {
	    case PROTO_CHAP:
	      if (opt->len >= 5) {
		char		buf[20];
		const char	*ts;

		switch (opt->data[2]) {
		  case CHAP_ALG_MD5:
		    ts = "MD5";
		    break;
		  case CHAP_ALG_MSOFT:
		    ts = "MSOFT";
		    break;
		  case CHAP_ALG_MSOFTv2:
		    ts = "MSOFTv2";
		    break;
		  default:
		    snprintf(buf, sizeof(buf), "0x%02x", opt->data[2]);
		    ts = buf;
		    break;
		}
		Log(LG_LCP, ("[%s]   %s %s %s", l->name, oi->name, ProtoName(proto), ts));
		break;
	      }
	      break;
	    default:
	      Log(LG_LCP, ("[%s]   %s %s", l->name, oi->name, ProtoName(proto)));
	      break;
	  }

	  /* Sanity check */
	  switch (proto) {
	    case PROTO_PAP:
	      if (opt->len != 4) {
		Log(LG_LCP, ("[%s]     Bad len=%d", l->name, opt->len));
		bogus = 1;
	      }
	      break;
	    case PROTO_CHAP:
	      if (opt->len != 5) {
		Log(LG_LCP, ("[%s]     Bad len=%d", l->name, opt->len));
		bogus = 1;
	      }
	      break;
	  }
	  if (!bogus) {
	    protoPos = LcpFindAuthProto(proto, proto == PROTO_CHAP ? opt->data[2] : 0);
	    authProto = (protoPos == -1) ? NULL : &gLcpAuthProtos[protoPos];
	  }

	  /* Deal with it */
	  switch (mode) {
	    case MODE_REQ:

	      /* let us check, whether the requested auth-proto is acceptable */
	      if ((authProto != NULL) && Acceptable(&l->conf.options, authProto->conf)) {
		lcp->peer_auth = proto;
	        if (proto == PROTO_CHAP)
		  lcp->peer_alg = opt->data[2];
		FsmAck(fp, opt);
		break;
	      }

	      /* search an acceptable proto */
	      for(i = 0; i < LCP_NUM_AUTH_PROTOS; i++) {
		if (lcp->peer_protos[i] != NULL) {
		  FsmNak(fp, LcpAuthProtoNak(lcp->peer_protos[i]->proto, lcp->peer_protos[i]->alg));
		  break;
		}
	      }

	      /* no other acceptable auth-proto found */
	      if (i == LCP_NUM_AUTH_PROTOS)
		FsmRej(fp, opt);
	      break;

	    case MODE_NAK:
	      /* this should never happen */
	      if (authProto == NULL)
		break;

	      /* let us check, whether the requested auth-proto is enabled */
	      if (Enabled(&l->conf.options, authProto->conf)) {
	        lcp->want_auth = proto;
	        if (proto == PROTO_CHAP)
		  lcp->want_alg = opt->data[2];
		break;
	      }

	      /* Remove the disabled proto from my list */
	      lcp->want_protos[protoPos] = NULL;

	      /* Search the next enabled proto */
	      for(i = 0; i < LCP_NUM_AUTH_PROTOS; i++) {
		if (lcp->want_protos[i] != NULL) {
		  lcp->want_auth = lcp->want_protos[i]->proto;
		  lcp->want_alg = lcp->want_protos[i]->alg;
		  break;
		}
	      }
	      break;

	    case MODE_REJ:
	      LCP_PEER_REJ(lcp, opt->type);
	      if (l->originate == LINK_ORIGINATE_LOCAL
		  && Enabled(&l->conf.options, LINK_CONF_NO_ORIG_AUTH)) {
		lcp->want_auth = 0;
	      }
	      break;
	  }
	}
	break;

      case TY_MRRU:			/* multi-link MRRU */
	{
	  u_int16_t	mrru;

	  memcpy(&mrru, opt->data, 2);
	  mrru = ntohs(mrru);
	  Log(LG_LCP, ("[%s]   %s %d", l->name, oi->name, mrru));
	  switch (mode) {
	    case MODE_REQ:
	      if (!Enabled(&l->conf.options, LINK_CONF_MULTILINK)) {
		FsmRej(fp, opt);
		break;
	      }
	      if (mrru < MP_MIN_MRRU) {
	        mrru = htons(MP_MIN_MRRU);
		memcpy(opt->data, &mrru, 2);
		FsmNak(fp, opt);
		break;
	      }
	      lcp->peer_mrru = mrru;
	      FsmAck(fp, opt);
	      break;
	    case MODE_NAK:
	      {
	        /* Let the peer to change it's mind. */
		if (LCP_PEER_REJECTED(lcp, opt->type)) {
		    LCP_PEER_UNREJ(lcp, opt->type);
		    if (Enabled(&l->conf.options, LINK_CONF_MULTILINK))
	    		lcp->want_mrru = l->conf.mrru;
		}
		/* Make sure we don't violate any rules by changing MRRU now */
		if (mrru > lcp->want_mrru)		/* too big */
		  break;
		if (mrru < MP_MIN_MRRU)			/* too small; clip */
		  mrru = MP_MIN_MRRU;

		/* Update our links */
		lcp->want_mrru = mrru;
	      }
	      break;
	    case MODE_REJ:
	      lcp->want_mrru = 0;
	      LCP_PEER_REJ(lcp, opt->type);
	      break;
	  }
	}
	break;

      case TY_SHORTSEQNUM:		/* multi-link short sequence numbers */
	Log(LG_LCP, ("[%s]   %s", l->name, oi->name));
	switch (mode) {
	  case MODE_REQ:
	    if (!Enabled(&l->conf.options, LINK_CONF_MULTILINK) ||
		!Acceptable(&l->conf.options, LINK_CONF_SHORTSEQ)) {
	      FsmRej(fp, opt);
	      break;
	    }
	    lcp->peer_shortseq = TRUE;
	    FsmAck(fp, opt);
	    break;
	  case MODE_NAK:
	        /* Let the peer to change it's mind. */
		if (LCP_PEER_REJECTED(lcp, opt->type)) {
		    LCP_PEER_UNREJ(lcp, opt->type);
		    if (Enabled(&l->conf.options, LINK_CONF_MULTILINK))
			lcp->want_shortseq = Enabled(&l->conf.options, LINK_CONF_SHORTSEQ);
		}
		break;
	  case MODE_REJ:
	      lcp->want_shortseq = FALSE;
	      LCP_PEER_REJ(lcp, opt->type);
	    break;
	}
	break;

      case TY_ENDPOINTDISC:		/* multi-link endpoint discriminator */
	{
	  struct discrim	dis;
	  char			buf[64];

	  if (opt->len < 3 || opt->len > sizeof(dis.bytes)) {
	    Log(LG_LCP, ("[%s]   %s bad len=%d", l->name, oi->name, opt->len));
	    if (mode == MODE_REQ)
	      FsmRej(fp, opt);
	    break;
	  }
	  memcpy(&dis.class, opt->data, opt->len - 2);
	  dis.len = opt->len - 3;
	  Log(LG_LCP, ("[%s]   %s %s", l->name, oi->name, MpDiscrimText(&dis, buf, sizeof(buf))));
	  switch (mode) {
	    case MODE_REQ:
	      lcp->peer_discrim = dis;
	      FsmAck(fp, opt);
	      break;
	    case MODE_NAK:
	        /* Let the peer to change it's mind. */
		LCP_PEER_UNREJ(lcp, opt->type);
		break;
	    case MODE_REJ:
	      LCP_PEER_REJ(lcp, opt->type);
	      break;
	  }
	}
	break;

      case TY_MAGICNUM:			/* magic number */
	{
	  u_int32_t	magic;

	  memcpy(&magic, opt->data, 4);
	  magic = ntohl(magic);
	  Log(LG_LCP, ("[%s]   %s %08x", l->name, oi->name, magic));
	  switch (mode) {
	    case MODE_REQ:
	      if (lcp->want_magic) {
		if (magic == lcp->want_magic) {
		  Log(LG_LCP, ("[%s]     Same magic! Detected loopback condition", l->name));
		  magic = htonl(~magic);
		  memcpy(opt->data, &magic, 4);
		  FsmNak(fp, opt);
		  break;
		}
		lcp->peer_magic = magic;
		FsmAck(fp, opt);
		break;
	      }
	      FsmRej(fp, opt);
	      break;
	    case MODE_NAK:
	      lcp->want_magic = GenerateMagic();
	      break;
	    case MODE_REJ:
	      lcp->want_magic = 0;
	      LCP_PEER_REJ(lcp, opt->type);
	      break;
	  }
	}
	break;

      case TY_PROTOCOMP:		/* Protocol field compression */
	Log(LG_LCP, ("[%s]   %s", l->name, oi->name));
	switch (mode) {
	  case MODE_REQ:
	    if (Acceptable(&l->conf.options, LINK_CONF_PROTOCOMP)) {
	      lcp->peer_protocomp = TRUE;
	      FsmAck(fp, opt);
	      break;
	    }
	    FsmRej(fp, opt);
	    break;
	  case MODE_NAK:	/* a NAK here doesn't make sense */
	  case MODE_REJ:
	    lcp->want_protocomp = FALSE;
	    LCP_PEER_REJ(lcp, opt->type);
	    break;
	}
	break;

      case TY_ACFCOMP:			/* Address field compression */
	Log(LG_LCP, ("[%s]   %s", l->name, oi->name));
	switch (mode) {
	  case MODE_REQ:
	    if (Acceptable(&l->conf.options, LINK_CONF_ACFCOMP)) {
	      lcp->peer_acfcomp = TRUE;
	      FsmAck(fp, opt);
	      break;
	    }
	    FsmRej(fp, opt);
	    break;
	  case MODE_NAK:	/* a NAK here doesn't make sense */
	  case MODE_REJ:
	    lcp->want_acfcomp = FALSE;
	    LCP_PEER_REJ(lcp, opt->type);
	    break;
	}
	break;

      case TY_CALLBACK:			/* Callback */
	Log(LG_LCP, ("[%s]   %s %d", l->name, oi->name, opt->data[0]));
	switch (mode) {
	  case MODE_REQ:	/* we only support peer calling us back */
	    FsmRej(fp, opt);
	    break;
	  case MODE_NAK:	/* we only know one way to do it */
	    /* fall through */
	  case MODE_REJ:
	    lcp->want_callback = FALSE;
	    LCP_PEER_REJ(lcp, opt->type);
	    break;
	}
	break;

      case TY_VENDOR:
	{
	  Log(LG_LCP, ("[%s]   %s %02x%02x%02x:%d", l->name, oi->name,
	    opt->data[0], opt->data[1], opt->data[2], opt->data[3]));
	  switch (mode) {
	    case MODE_REQ:
	      FsmRej(fp, opt);
	      break;
	    case MODE_NAK:
	      /* fall through */
	    case MODE_REJ:
	      LCP_PEER_REJ(lcp, opt->type);
	      break;
	  }
	  break;
	}
	break;

      default:
	assert(0);
    }
  }
}

/*
 * LcpInput()
 */

void
LcpInput(Link l, Mbuf bp)
{
  FsmInput(&l->lcp.fsm, bp);
}

static const struct fsmoption *
LcpAuthProtoNak(ushort proto, u_char alg)
{
  static const u_char	chapmd5cf[] =
    { PROTO_CHAP >> 8, PROTO_CHAP & 0xff, CHAP_ALG_MD5 };
  static const struct	fsmoption chapmd5Nak =
    { TY_AUTHPROTO, 2 + sizeof(chapmd5cf), (u_char *) chapmd5cf };

  static const u_char	chapmsv1cf[] =
    { PROTO_CHAP >> 8, PROTO_CHAP & 0xff, CHAP_ALG_MSOFT };
  static const struct	fsmoption chapmsv1Nak =
    { TY_AUTHPROTO, 2 + sizeof(chapmsv1cf), (u_char *) chapmsv1cf };

  static const u_char	chapmsv2cf[] =
    { PROTO_CHAP >> 8, PROTO_CHAP & 0xff, CHAP_ALG_MSOFTv2 };
  static const struct	fsmoption chapmsv2Nak =
    { TY_AUTHPROTO, 2 + sizeof(chapmsv2cf), (u_char *) chapmsv2cf };

  static const u_char	papcf[] =
    { PROTO_PAP >> 8, PROTO_PAP & 0xff };
  static const struct	fsmoption papNak =
    { TY_AUTHPROTO, 2 + sizeof(papcf), (u_char *) papcf };

  static const u_char	eapcf[] =
    { PROTO_EAP >> 8, PROTO_EAP & 0xff };
  static const struct	fsmoption eapNak =
    { TY_AUTHPROTO, 2 + sizeof(eapcf), (u_char *) eapcf };

  if (proto == PROTO_PAP) {
    return &papNak;
  } else if (proto == PROTO_EAP) {
    return &eapNak;
  } else {
    switch (alg) {
      case CHAP_ALG_MSOFTv2:
        return &chapmsv2Nak;

      case CHAP_ALG_MSOFT:
        return &chapmsv1Nak;

      case CHAP_ALG_MD5:
        return &chapmd5Nak;

      default:
        return NULL;
    }
  }

}

/*
 * LcpFindAuthProto()
 *
 */
static short
LcpFindAuthProto(ushort proto, u_char alg)
{
  int i;

  for(i = 0; i < LCP_NUM_AUTH_PROTOS; i++) {
    if (gLcpAuthProtos[i].proto == proto && gLcpAuthProtos[i].alg == alg) {
      return i;
    }
  }

  return -1;

}
