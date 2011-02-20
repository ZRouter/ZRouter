/*
 * See ``COPYRIGHT.mpd''
 *
 * $Id: eap.c,v 1.34 2008/10/30 11:41:39 amotin Exp $
 *
 */

#include "ppp.h"
#include "radius.h"
#include "auth.h"
#include "ngfunc.h"

/*
 * INTERNAL FUNCTIONS
 */

  static void   EapSendRequest(Link l, u_char type);
  static void	EapSendNak(Link l, u_char id, u_char type);
  static void	EapSendIdentRequest(Link l);
  static void	EapIdentTimeout(void *ptr);
  static char	EapTypeSupported(u_char type);
  static void	EapRadiusProxy(Link l, AuthData auth, const u_char *pkt, u_short len);
  static void	EapRadiusProxyFinish(Link l, AuthData auth);
  static void	EapRadiusSendMsg(void *ptr);
  static void	EapRadiusSendMsgTimeout(void *ptr);
  static int	EapSetCommand(Context ctx, int ac, char *av[], void *arg);

  /* Set menu options */
  enum {
    SET_ACCEPT,
    SET_DENY,
    SET_ENABLE,
    SET_DISABLE,
    SET_YES,
    SET_NO
  };

/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab EapSetCmds[] = {
    { "accept [opt ...]",		"Accept option",
	EapSetCommand, NULL, 2, (void *) SET_ACCEPT },
    { "deny [opt ...]",			"Deny option",
	EapSetCommand, NULL, 2, (void *) SET_DENY },
    { "enable [opt ...]",		"Enable option",
	EapSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",		"Disable option",
	EapSetCommand, NULL, 2, (void *) SET_DISABLE },
    { "yes [opt ...]",			"Enable and accept option",
	EapSetCommand, NULL, 2, (void *) SET_YES },
    { "no [opt ...]",			"Disable and deny option",
	EapSetCommand, NULL, 2, (void *) SET_NO },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static struct confinfo	gConfList[] = {
    { 0,	EAP_CONF_RADIUS,	"radius-proxy"	},
    { 1,	EAP_CONF_MD5,		"md5"		},
    { 0,	0,			NULL		},
  };



/*
 * EapInit()
 */

void
EapInit(Link l)
{
  EapInfo	eap = &l->lcp.auth.eap;

  Disable(&eap->conf.options, EAP_CONF_MD5);
  Accept(&eap->conf.options, EAP_CONF_MD5);
}

/*
 * EapStart()
 */

void
EapStart(Link l, int which)
{
  Auth		a = &l->lcp.auth;
  EapInfo	eap = &l->lcp.auth.eap;
  int	i;

  for (i = 0; i < EAP_NUM_TYPES; i++)
    eap->peer_types[i] = eap->want_types[i] = 0;

  /* fill a list of requestable auth types */
  if (Enabled(&eap->conf.options, EAP_CONF_MD5))
    eap->want_types[0] = EAP_TYPE_MD5CHAL;

  /* fill a list of acceptable auth types */
  if (Acceptable(&eap->conf.options, EAP_CONF_MD5))
    eap->peer_types[0] = EAP_TYPE_MD5CHAL;

  if (l->originate == LINK_ORIGINATE_LOCAL)
    a->params.msoft.chap_alg = a->self_to_peer_alg;
  else
    a->params.msoft.chap_alg = a->peer_to_self_alg;

  switch (which) {
    case AUTH_PEER_TO_SELF:

      /* Initialize retry counter and timer */
      eap->next_id = 1;
      eap->retry = AUTH_RETRIES;

      TimerInit(&eap->reqTimer, "EapRadiusSendMsgTimer",
	l->conf.retry_timeout * SECONDS, EapRadiusSendMsgTimeout, (void *) l);

      TimerInit(&eap->identTimer, "EapTimer",
	l->conf.retry_timeout * SECONDS, EapIdentTimeout, (void *) l);
      TimerStart(&eap->identTimer);

      /* Send first request
       * Send the request even, if the Radius-Eap-Proxy feature is active,
       * this saves on roundtrip.
       */
      EapSendIdentRequest(l);
      break;

    case AUTH_SELF_TO_PEER:	/* Just wait for authenitcaor's request */
      break;

    default:
      assert(0);
  }
}

/*
 * EapStop()
 */

void
EapStop(EapInfo eap)
{
  TimerStop(&eap->identTimer);
  TimerStop(&eap->reqTimer);
}

/*
 * EapSendRequest()
 *
 * Send an EAP request to peer.
 */

static void
EapSendRequest(Link l, u_char type)
{
  Auth		const a = &l->lcp.auth;
  EapInfo	const eap = &a->eap;
  ChapInfo	const chap = &a->chap;
  ChapParams	const cp = &a->params.chap;
  int		i = 0;
  u_char	req_type = 0;

  if (type == 0) {
    for (i = 0; i < EAP_NUM_TYPES; i++) {
      if (eap->want_types[i] != 0) {
        req_type = eap->want_types[i];
        break;
      }
    }
  } else {
    req_type = type;
  }

  if (req_type == 0) {
    Log(LG_AUTH, ("[%s] EAP: ran out of EAP Types", l->name));
    AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
    return;
  }

  /* don't request this type again */
  eap->want_types[i] = 0;

  switch (req_type) {
    case EAP_TYPE_MD5CHAL:

      /* Invalidate any old challenge data */
      cp->chal_len = 0;
      /* Initialize retry counter and timer */
      chap->next_id = 1;
      chap->retry = AUTH_RETRIES;
      chap->proto = PROTO_EAP;
      a->peer_to_self_alg = CHAP_ALG_MD5;

      TimerInit(&chap->chalTimer, "ChalTimer",
        l->conf.retry_timeout * SECONDS, ChapChalTimeout, l);
      TimerStart(&chap->chalTimer);

      /* Send first challenge */
      ChapSendChallenge(l);
      break;

    default:
      Log(LG_AUTH, ("[%s] EAP: Type %d is currently un-implemented",
	l->name, eap->want_types[i]));
      AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
  }

  return;
}

/*
 * EapSendNak()
 *
 * Send an EAP Nak to peer.
 */

static void
EapSendNak(Link l, u_char id, u_char type)
{
  Auth		const a = &l->lcp.auth;
  EapInfo	const eap = &a->eap;
  int		i = 0;
  u_char	nak_type = 0;

  for (i = 0; i < EAP_NUM_TYPES; i++) {
    if (eap->peer_types[i] != 0) {
      nak_type = eap->peer_types[i];
      break;
    }
  }

  if (nak_type == 0) {
    Log(LG_AUTH, ("[%s] EAP: ran out of EAP Types", l->name));
    AuthFinish(l, AUTH_SELF_TO_PEER, FALSE);
    return;
  }

  /* don't nak this proto again */
  eap->peer_types[i] = 0;

  AuthOutput(l, PROTO_EAP, EAP_RESPONSE, id, &nak_type, 1, 0, EAP_TYPE_NAK);
  return;
}

/*
 * EapSendIdentRequest()
 *
 * Send an Ident Request to the peer.
 */

static void
EapSendIdentRequest(Link l)
{
  EapInfo	const eap = &l->lcp.auth.eap;

  /* Send the initial Identity request */
  AuthOutput(l, PROTO_EAP, EAP_REQUEST,  eap->next_id++, NULL, 0, 0, EAP_TYPE_IDENT);
}

/*
 * EapInput()
 *
 * Accept an incoming EAP packet
 */

void
EapInput(Link l, AuthData auth, const u_char *pkt, u_short len)
{
  Auth		const a = &l->lcp.auth;
  EapInfo	const eap = &a->eap;
  int		data_len = len - 1, i, acc_type;
  u_char	*data = NULL, type = 0;
  
  if (pkt != NULL) {
    data = data_len > 0 ? (u_char *) &pkt[1] : NULL;
    type = pkt[0];
  }
  
  if (Enabled(&eap->conf.options, EAP_CONF_RADIUS)) {
	EapRadiusProxy(l, auth, pkt, len);
	return;
  }

  switch (auth->code) {
    case EAP_REQUEST:
      switch (type) {
	case EAP_TYPE_IDENT:
	  AuthOutput(l, PROTO_EAP, EAP_RESPONSE, auth->id, (u_char *) auth->conf.authname,
	    strlen(auth->conf.authname), 0, EAP_TYPE_IDENT);
	  break;

	case EAP_TYPE_NAK:
	case EAP_TYPE_NOTIF:
	  Log(LG_AUTH, ("[%s] EAP: Type %s is invalid in Request messages",
	    l->name, EapType(type)));
	  AuthFinish(l, AUTH_SELF_TO_PEER, FALSE);
	  break;

	/* deal with Auth Types */
	default:
	  acc_type = 0;
	  if (EapTypeSupported(type)) {
	    for (i = 0; i < EAP_NUM_TYPES; i++) {
	      if (eap->peer_types[i] == type) {
		acc_type = eap->peer_types[i];
		break;
	      }
	    }

	    if (acc_type == 0) {
	      Log(LG_AUTH, ("[%s] EAP: Type %s not acceptable", l->name,
	        EapType(type)));
	      EapSendNak(l, auth->id, type);
	      break;
	    }

	    switch (type) {
	      case EAP_TYPE_MD5CHAL:
		a->self_to_peer_alg = CHAP_ALG_MD5;
		auth->code = CHAP_CHALLENGE;
		ChapInput(l, auth, &pkt[1], len - 1);
		return;

	      default:
		assert(0);
	    }
	  } else {
	    Log(LG_AUTH, ("[%s] EAP: Type %s not supported", l->name, EapType(type)));
	    EapSendNak(l, auth->id, type);
	  }
      }
      break;

    case EAP_RESPONSE:
      switch (type) {
	case EAP_TYPE_IDENT:
	  TimerStop(&eap->identTimer);
	  Log(LG_AUTH, ("[%s] EAP: Identity:%*.*s",
	    l->name, data_len, data_len, data));
	  EapSendRequest(l, 0);
	  break;

	case EAP_TYPE_NOTIF:
	  Log(LG_AUTH, ("[%s] EAP: Notify:%*.*s ", l->name,
	    data_len, data_len, data));
	  break;

	case EAP_TYPE_NAK:
	  Log(LG_AUTH, ("[%s] EAP: Nak desired Type %s ", l->name,
	    EapType(data[0])));
	  if (EapTypeSupported(data[0]))
	    EapSendRequest(l, data[0]);
	  else
	    EapSendRequest(l, 0);
	  break;

	case EAP_TYPE_MD5CHAL:
	  auth->code = CHAP_RESPONSE;
	  ChapInput(l, auth, &pkt[1], len - 1);
	  return;

	default:
	  Log(LG_AUTH, ("[%s] EAP: unknown type %d", l->name, type));
	  AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
      }
      break;

    case EAP_SUCCESS:
      AuthFinish(l, AUTH_SELF_TO_PEER, TRUE);
      break;

    case EAP_FAILURE:
      AuthFinish(l, AUTH_SELF_TO_PEER, FALSE);
      break;

    default:
      Log(LG_AUTH, ("[%s] EAP: unknown code %d", l->name, auth->code));
      AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
  }
  AuthDataDestroy(auth);
}

/*
 * EapRadiusProxy()
 *
 * Proxy EAP Requests from/to the RADIUS server
 */

static void
EapRadiusProxy(Link l, AuthData auth, const u_char *pkt, u_short len)
{
  int		data_len = len - 1;
  u_char	*data = NULL, type = 0;
  Auth		const a = &l->lcp.auth;
  EapInfo	const eap = &a->eap;
  struct fsmheader	lh;

  Log(LG_AUTH, ("[%s] EAP: Proxying packet to RADIUS", l->name));

  if (pkt != NULL) {
    data = data_len > 0 ? (u_char *) &pkt[1] : NULL;
    type = pkt[0];
  }

  if (auth->code == EAP_RESPONSE && type == EAP_TYPE_IDENT) {
    TimerStop(&eap->identTimer);
    if (data_len >= AUTH_MAX_AUTHNAME) {
      Log(LG_AUTH, ("[%s] EAP: Identity to big (%d), truncating",
	l->name, data_len));
        data_len = AUTH_MAX_AUTHNAME - 1;
    }
    memset(eap->identity, 0, sizeof(eap->identity));
    strncpy(eap->identity, (char *) data, data_len);
    Log(LG_AUTH, ("[%s] EAP: Identity: %s", l->name, eap->identity));
  }

  TimerStop(&eap->reqTimer);

  /* prepare packet */
  lh.code = auth->code;
  lh.id = auth->id;
  lh.length = htons(len + sizeof(lh));

  auth->params.eapmsg = Malloc(MB_AUTH, len + sizeof(lh));
  memcpy(auth->params.eapmsg, &lh, sizeof(lh));
  memcpy(&auth->params.eapmsg[sizeof(lh)], pkt, len);

  auth->params.eapmsg_len = len + sizeof(lh);
  strlcpy(auth->params.authname, eap->identity, sizeof(auth->params.authname));

  auth->eap_radius = TRUE;

  auth->finish = EapRadiusProxyFinish;
  AuthAsyncStart(l, auth);
  
}

/*
 * RadiusEapProxyFinish()
 *
 * Return point from the asynch RADIUS EAP Proxy Handler.
 * 
 */
 
static void
EapRadiusProxyFinish(Link l, AuthData auth)
{
  Auth		const a = &l->lcp.auth;
  EapInfo	eap = &a->eap;
  
  Log(LG_AUTH, ("[%s] EAP: RADIUS return status: %s", 
    l->name, AuthStatusText(auth->status)));

  /* this shouldn't happen normally, however be liberal */
  if (a->params.eapmsg == NULL) {
    struct fsmheader	lh;

    Log(LG_AUTH, ("[%s] EAP: Warning, rec'd empty EAP-Message", 
      l->name));
    /* prepare packet */
    lh.code = auth->status == AUTH_STATUS_SUCCESS ? EAP_SUCCESS : EAP_FAILURE;
    lh.id = auth->id;
    lh.length = htons(sizeof(lh));

    a->params.eapmsg = Mdup(MB_AUTH, &lh, sizeof(lh));
    a->params.eapmsg_len = sizeof(lh);
  }

  if (a->params.eapmsg != NULL) {
    eap->retry = AUTH_RETRIES;
    
    EapRadiusSendMsg(l);
    if (auth->status == AUTH_STATUS_UNDEF)
      TimerStart(&eap->reqTimer);
  }

  if (auth->status == AUTH_STATUS_FAIL) {
    AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
  } else if (auth->status == AUTH_STATUS_SUCCESS) {
    AuthFinish(l, AUTH_PEER_TO_SELF, TRUE);
  } 

  AuthDataDestroy(auth);  
}

/*
 * EapRadiusSendMsg()
 *
 * Send an EAP Message to the peer
 */

static void
EapRadiusSendMsg(void *ptr)
{
  Mbuf		bp;
  Link		l = (Link)ptr;
  Auth		const a = &l->lcp.auth;
  FsmHeader	const f = (FsmHeader)a->params.eapmsg;
  char		buf[32];

  if (a->params.eapmsg_len > 4) {
    Log(LG_AUTH, ("[%s] EAP: send %s #%d len: %d, type: %s",
      l->name, EapCode(f->code, buf, sizeof(buf)), f->id, htons(f->length),
      EapType(a->params.eapmsg[4])));
  } else {
    Log(LG_AUTH, ("[%s] EAP: send %s #%d len: %d",
      l->name, EapCode(f->code, buf, sizeof(buf)), f->id, htons(f->length)));
  } 

  bp = mbcopyback(NULL, 0, a->params.eapmsg, a->params.eapmsg_len);
  NgFuncWritePppFrameLink(l, PROTO_EAP, bp);
}

/*
 * EapRadiusSendMsgTimeout()
 *
 * Timer expired for reply to our request
 */

static void
EapRadiusSendMsgTimeout(void *ptr)
{
    Link	l = (Link)ptr;
    EapInfo	const eap = &l->lcp.auth.eap;

    if (--eap->retry > 0) {
	TimerStart(&eap->reqTimer);
	EapRadiusSendMsg(l);
    }
}

/*
 * EapIdentTimeout()
 *
 * Timer expired for reply to our request
 */

static void
EapIdentTimeout(void *ptr)
{
    Link	l = (Link)ptr;
    EapInfo	const eap = &l->lcp.auth.eap;

    if (--eap->retry > 0) {
	TimerStart(&eap->identTimer);
	EapSendIdentRequest(l);
    }
}

/*
 * EapStat()
 */

int
EapStat(Context ctx, int ac, char *av[], void *arg)
{
  EapInfo	const eap = &ctx->lnk->lcp.auth.eap;

  Printf("\tIdentity     : %s\r\n", eap->identity);
  Printf("EAP options\r\n");
  OptStat(ctx, &eap->conf.options, gConfList);

  return (0);
}

/*
 * EapCode()
 */

const char *
EapCode(u_char code, char *buf, size_t len)
{
  switch (code) {
    case EAP_REQUEST:
	strlcpy(buf, "REQUEST", len);
	break;
    case EAP_RESPONSE:
	strlcpy(buf, "RESPONSE", len);
	break;
    case EAP_SUCCESS:
	strlcpy(buf, "SUCCESS", len);
	break;
    case EAP_FAILURE:
	strlcpy(buf, "FAILURE", len);
	break;
    default:
	snprintf(buf, len, "code%d", code);
  }
  return(buf);
}

/*
 * EapType()
 */

const char *
EapType(u_char type)
{
  switch (type) {
    case EAP_TYPE_IDENT:
      return("Identity");
    case EAP_TYPE_NOTIF:
      return("Notification");
    case EAP_TYPE_NAK:
      return("Nak");
    case EAP_TYPE_MD5CHAL:
      return("MD5 Challenge");
    case EAP_TYPE_OTP:
      return("One Time Password");
    case EAP_TYPE_GTC:
      return("Generic Token Card");
    case EAP_TYPE_EAP_TLS:
      return("TLS");
    case EAP_TYPE_MSCHAP_V2:
      return("MS-CHAPv2");
    case EAP_TYPE_EAP_TTLS:
      return("TTLS");
    default:
      return("UNKNOWN");
  }
}

/*
 * EapTypeSupported()
 */

static char
EapTypeSupported(u_char type)
{
  switch (type) {
    case EAP_TYPE_IDENT:
    case EAP_TYPE_NOTIF:
    case EAP_TYPE_NAK:
    case EAP_TYPE_MD5CHAL:
      return 1;

    default:
      return 0;
  }
}

/*
 * EapSetCommand()
 */

static int
EapSetCommand(Context ctx, int ac, char *av[], void *arg)
{
  EapInfo	const eap = &ctx->lnk->lcp.auth.eap;

  if (ac == 0)
    return(-1);

  switch ((intptr_t)arg) {

    case SET_ACCEPT:
      AcceptCommand(ac, av, &eap->conf.options, gConfList);
      break;

    case SET_DENY:
      DenyCommand(ac, av, &eap->conf.options, gConfList);
      break;

    case SET_ENABLE:
      EnableCommand(ac, av, &eap->conf.options, gConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &eap->conf.options, gConfList);
      break;

    case SET_YES:
      YesCommand(ac, av, &eap->conf.options, gConfList);
      break;

    case SET_NO:
      NoCommand(ac, av, &eap->conf.options, gConfList);
      break;

    default:
      assert(0);
  }

  return(0);
}
