
/*
 * pap.c
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
#include "auth.h"
#include "util.h"

/*
 * INTERNAL FUNCTIONS
 */

  static void	PapSendRequest(Link l);
  static void	PapTimeout(void *ptr);

/*
 * PapStart()
 */

void
PapStart(Link l, int which)
{
  PapInfo pap = &l->lcp.auth.pap;

  switch (which) {
    case AUTH_PEER_TO_SELF:	/* Just wait for peer's request */
      break;

    case AUTH_SELF_TO_PEER:

      /* Initialize retry counter and timer */
      pap->next_id = 1;
      pap->retry = AUTH_RETRIES;

      TimerInit(&pap->timer, "PapTimer",
	l->conf.retry_timeout * SECONDS, PapTimeout, l);
      TimerStart(&pap->timer);

      /* Send first request */
      PapSendRequest(l);
      break;

    default:
      assert(0);
  }
}

/*
 * PapStop()
 */

void
PapStop(PapInfo pap)
{
  TimerStop(&pap->timer);
}

/*
 * PapSendRequest()
 *
 * Send a PAP packet to peer.
 */

static void
PapSendRequest(Link l)
{
    PapInfo		pap = &l->lcp.auth.pap;
    char		password[AUTH_MAX_PASSWORD];
    int			name_len, pass_len;
    u_char		*pkt;

    /* Get password corresponding to my authname */
    Log(LG_AUTH, ("[%s] PAP: using authname \"%s\"", 
	l->name, l->lcp.auth.conf.authname));
    if (l->lcp.auth.conf.password[0] != 0) {
	strlcpy(password, l->lcp.auth.conf.password, sizeof(password));
    } else if (AuthGetData(l->lcp.auth.conf.authname, password, 
	    sizeof(password), NULL, NULL) < 0) {
	Log(LG_AUTH, ("[%s] PAP: Warning: no secret for \"%s\" found", 
	    l->name, l->lcp.auth.conf.authname));
    }

    /* Build response packet */
    name_len = strlen(l->lcp.auth.conf.authname);
    pass_len = strlen(password);

    pkt = Malloc(MB_AUTH, 1 + name_len + 1 + pass_len);
    pkt[0] = name_len;
    memcpy(pkt + 1, l->lcp.auth.conf.authname, name_len);
    pkt[1 + name_len] = pass_len;
    memcpy(pkt + 1 + name_len + 1, password, pass_len);

    /* Send it off */
    AuthOutput(l, PROTO_PAP, PAP_REQUEST, pap->next_id++, pkt,
	1 + name_len + 1 + pass_len, 0, 0);
    Freee(pkt);
}

/*
 * PapInput()
 *
 * Accept an incoming PAP packet
 */

void
PapInput(Link l, AuthData auth, const u_char *pkt, u_short len)
{
    Auth		const a = &l->lcp.auth;
    PapInfo		const pap = &a->pap;
    PapParams		const pp = &auth->params.pap;
    char		failMesg[64];
    char		buf[16];

  switch (auth->code) {
    case PAP_REQUEST:
      {
	char		*name_ptr, name[256];
	char		*pass_ptr, pass[256];
	int		name_len, pass_len;

	/* Is this appropriate? */
	if (a->peer_to_self != PROTO_PAP) {
	    if (l->lcp.want_auth == PROTO_PAP && a->peer_to_self == 0) {
		Log(LG_AUTH, ("[%s] PAP: retransmitting ACK",
		    l->name));
		AuthOutput(l, PROTO_PAP, PAP_ACK, auth->id,
		    (u_char *) AUTH_MSG_WELCOME, strlen(AUTH_MSG_WELCOME), 1, 0);
		break;
	    }
	    Log(LG_AUTH, ("[%s] PAP: %s not expected",
		l->name, PapCode(auth->code, buf, sizeof(buf))));
	    auth->why_fail = AUTH_FAIL_NOT_EXPECTED;
	    PapInputFinish(l, auth);
	    return;
	}

	/* Sanity check packet and extract fields */
	if (len < 1)
	    goto error;

	name_len = pkt[0];
	name_ptr = (char *)pkt + 1;

	if (1 + name_len >= len)
	    goto error;

	pass_len = pkt[1 + name_len];
	pass_ptr = (char *)pkt + 1 + name_len + 1;

	if (name_len + 1 + pass_len + 1 > len)
	    goto error;

	memcpy(name, name_ptr, name_len);
	name[name_len] = 0;
	memcpy(pass, pass_ptr, pass_len);
	pass[pass_len] = 0;

	strlcpy(pp->peer_name, name, sizeof(pp->peer_name));
	strlcpy(pp->peer_pass, pass, sizeof(pp->peer_pass));
	strlcpy(auth->params.authname, name, sizeof(auth->params.authname));
	auth->params.password[0] = 0;

	auth->finish = PapInputFinish;
	AuthAsyncStart(l, auth);
	return;
      }
      break;

    case PAP_ACK:
    case PAP_NAK:
      {
	/* Is this appropriate? */
	if (a->self_to_peer != PROTO_PAP) {
	    Log(LG_AUTH, ("[%s] PAP: %s not expected",
		l->name, PapCode(auth->code, buf, sizeof(buf))));
	    break;
	}

	/* Stop resend timer */
	TimerStop(&pap->timer);

	/* Show reply message */
	if (len > 0) {
	    int		msg_len = pkt[0];
	    char	*msg = (char *) &pkt[1];
	    if (msg_len < len - 1)
		msg_len = len - 1;
	    ShowMesg(LG_AUTH, l->name, msg, msg_len);
	}

	/* Done with my auth to peer */
	AuthFinish(l, AUTH_SELF_TO_PEER, auth->code == PAP_ACK);	
      }
      break;

    default:
      Log(LG_AUTH, ("[%s] PAP: unknown code", l->name));
      break;
  }
    AuthDataDestroy(auth);
    return;

error:
    Log(LG_AUTH, ("[%s] PAP: Bad PAP packet", l->name));
    auth->why_fail = AUTH_FAIL_INVALID_PACKET;
    AuthFailMsg(auth, failMesg, sizeof(failMesg));
    AuthOutput(l, PROTO_PAP, PAP_NAK, auth->id, (u_char *)failMesg, strlen(failMesg), 1, 0);
    AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
    AuthDataDestroy(auth);
}

/*
 * PapInputFinish()
 *
 * Possible return point from the asynch auth handler.
 * 
 */
 
void PapInputFinish(Link l, AuthData auth)
{
  Auth		const a = &l->lcp.auth;
  PapParams	pap = &a->params.pap;
  const char	*Mesg;
  
    Log(LG_AUTH, ("[%s] PAP: Auth return status: %s", 
	l->name, AuthStatusText(auth->status)));

    if (auth->status == AUTH_STATUS_BUSY) {
	AuthDataDestroy(auth);  
	return;
    } else if (auth->status == AUTH_STATUS_FAIL)
	goto badRequest;
    else if (auth->status == AUTH_STATUS_SUCCESS)
	goto goodRequest;
  
    /* Do name & password match? */
    if (strcmp(a->params.authname, pap->peer_name) ||
        strcmp(a->params.password, pap->peer_pass)) {
	Log(LG_AUTH, ("[%s] PAP: Invalid response", l->name));
	auth->why_fail = AUTH_FAIL_INVALID_LOGIN;
	goto badRequest;
    }

goodRequest:
    /* Login accepted */
    Log(LG_AUTH, ("[%s] PAP: Response is valid", l->name));
    if (auth->reply_message) {
	Mesg = auth->reply_message;
    } else {
	Mesg = AUTH_MSG_WELCOME;
    }
    Log(LG_AUTH, ("[%s] PAP: Reply message: %s", l->name, Mesg));
    AuthOutput(l, PROTO_PAP, PAP_ACK, auth->id, (u_char *) Mesg, strlen(Mesg), 1, 0);
    AuthFinish(l, AUTH_PEER_TO_SELF, TRUE);  
    AuthDataDestroy(auth);
    return;

badRequest:
  {
    char        failMesg[64];

    Mesg = AuthFailMsg(auth, failMesg, sizeof(failMesg));
    Log(LG_AUTH, ("[%s] PAP: Reply message: %s", l->name, Mesg));
    AuthOutput(l, PROTO_PAP, PAP_NAK, auth->id, (u_char *) Mesg, strlen(Mesg), 1, 0);
    AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
    AuthDataDestroy(auth);  
  }
}

/*
 * PapTimeout()
 *
 * Timer expired for reply to our request
 */

static void
PapTimeout(void *ptr)
{
  Link		const l = (Link) ptr;
  PapInfo	const pap = &l->lcp.auth.pap;

  if (--pap->retry > 0) {
    TimerStart(&pap->timer);
    PapSendRequest(l);
  }
}

/*
 * PapCode()
 */

const char *
PapCode(int code, char *buf, size_t len)
{
  switch (code) {
    case PAP_REQUEST:
      strlcpy(buf, "REQUEST", len);
      break;
    case PAP_ACK:
      strlcpy(buf, "ACK", len);
      break;
    case PAP_NAK:
      strlcpy(buf, "NAK", len);
      break;
    default:
      snprintf(buf, len, "code%d", code);
  }
  return(buf);
}

