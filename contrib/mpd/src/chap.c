
/*
 * chap.c
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
#include "msoft.h"
#include "util.h"
#include <openssl/md5.h>

/*
 * INTERNAL FUNCTIONS
 */

  static int	ChapHash(Link l, int alg, u_char *hash_value, u_char id,
		  const char *username, const char *secret,
		  const u_char *challenge, int clen, int local);
  static int	ChapHashAgree(int alg, const u_char *self, int slen,
		  const u_char *peer, int plen);
  static int	ChapParsePkt(const char *pref, const u_char *pkt, const int pkt_len,
		  char *peer_name, u_char *chap_value,
		  int *chap_value_size);
  static char	*ChapGetSecret(Link l, int alg, char *password);
  static void	ChapGenRandom(Link l, u_char *buf, int len);

/*
 * INTERNAL VARIABLES
 */

  static const u_char	gIdBytes[] = { 0x3b, 0x1e, 0x68 };

/*
 * ChapStart()
 */

void
ChapStart(Link l, int which)
{
  Auth 		a = &l->lcp.auth;
  ChapInfo 	chap = &l->lcp.auth.chap;
  ChapParams	cp = &l->lcp.auth.params.chap;
  
  chap->proto = PROTO_CHAP;

  if (l->originate == LINK_ORIGINATE_LOCAL)
    a->params.msoft.chap_alg = a->self_to_peer_alg;
  else
    a->params.msoft.chap_alg = a->peer_to_self_alg;
  
  switch (which)
  {
    case AUTH_SELF_TO_PEER:	/* Just wait for peer's challenge */
      break;

    case AUTH_PEER_TO_SELF:

      /* Invalidate any old challenge data */
      cp->chal_len = 0;

      /* Initialize retry counter and timer */
      chap->next_id = 1;
      chap->retry = AUTH_RETRIES;

      TimerInit(&chap->chalTimer, "ChalTimer",
	l->conf.retry_timeout * SECONDS, ChapChalTimeout, l);
      TimerStart(&chap->chalTimer);

      /* Send first challenge */
      ChapSendChallenge(l);
      break;

    default:
      assert(0);
  }
}

/*
 * ChapStop()
 */

void
ChapStop(ChapInfo chap)
{
  TimerStop(&chap->chalTimer);
  TimerStop(&chap->respTimer);
  if (chap->resp) {
    Freee(chap->resp);
    chap->resp = NULL;
  }
}

/*
 * ChapSendChallenge()
 */

void
ChapSendChallenge(Link l)
{
  Auth          const a = &l->lcp.auth;
  ChapInfo	chap = &a->chap;
  ChapParams	cp = &a->params.chap;
  u_char	*pkt;

  /* don't generate new challenges on re-transmit */
  if (cp->chal_len)
    goto send_pkt;

  /* Put random challenge data in buffer (only once for Microsoft CHAP) */
  switch (a->peer_to_self_alg) {
    case CHAP_ALG_MSOFT: {
	cp->chal_len = CHAP_MSOFT_CHAL_LEN;
	ChapGenRandom(l, cp->chal_data, cp->chal_len);
	if (l->originate == LINK_ORIGINATE_REMOTE) {
	  memcpy(a->params.msoft.msChal, cp->chal_data, cp->chal_len);
	}
      }
      break;
    case CHAP_ALG_MSOFTv2:
      cp->chal_len = CHAP_MSOFTv2_CHAL_LEN;
      ChapGenRandom(l, cp->chal_data, cp->chal_len);
      if (l->originate == LINK_ORIGINATE_REMOTE) {
	memcpy(a->params.msoft.msChal, cp->chal_data, cp->chal_len);
      }
      break;
    case CHAP_ALG_MD5:
      cp->chal_len = random() % 32 + 16;
      ChapGenRandom(l, cp->chal_data, cp->chal_len);
      break;
    default:
      assert(0);
  }
  assert(cp->chal_len <= sizeof(cp->chal_data));

send_pkt:
  /* Build a challenge packet */
  pkt = Malloc(MB_AUTH, 1 + cp->chal_len + strlen(a->conf.authname) + 1);
  pkt[0] = cp->chal_len;
  memcpy(pkt + 1, cp->chal_data, cp->chal_len);
  memcpy(pkt + 1 + cp->chal_len,
    a->conf.authname, strlen(a->conf.authname));

  /* Send it off */
  AuthOutput(l, chap->proto,
    chap->proto == PROTO_CHAP ? CHAP_CHALLENGE : EAP_REQUEST,
    chap->next_id++, pkt,
    1 + cp->chal_len + strlen(a->conf.authname), 0,
    EAP_TYPE_MD5CHAL);
  Freee(pkt);
}

/*
 * ChapSendResponse()
 */

static void
ChapSendResponse(Link l)
{
  ChapInfo	chap = &l->lcp.auth.chap;

  /* Stop response timer */
  TimerStop(&chap->respTimer);

  /* Send response (possibly again) */
  assert(chap->resp);
  AuthOutput(l, chap->proto,
    chap->proto == PROTO_CHAP ? CHAP_RESPONSE : EAP_RESPONSE,
    chap->resp_id, chap->resp, chap->resp_len, 0, EAP_TYPE_MD5CHAL);

  /* Start re-send timer (only during authenticate phase where the
     authentication timer is still running) */
  if (l->lcp.phase == PHASE_AUTHENTICATE) {
    TimerInit(&chap->respTimer, "RespTimer",
      l->conf.retry_timeout * SECONDS,
      (void (*)(void *)) ChapSendResponse, (void *) l);
    TimerStart(&chap->respTimer);
  }
}

/*
 * ChapParsePkt()
 *
 */

static int
ChapParsePkt(const char *pref, const u_char *pkt, const int pkt_len,
  char *peer_name, u_char *chap_value, int *chap_value_size)
{
  int		val_len, name_len;

  /* Compute and check lengths */
  if (pkt == NULL
      || pkt_len < 1
      || (val_len = pkt[0]) < 1
      || val_len > CHAP_MAX_VAL
      || (name_len = (pkt_len - val_len - 1)) < 0
      || name_len > CHAP_MAX_NAME) {
    Log(LG_AUTH, ("[%s]   Bogus packet", pref));
    return(-1);
  }

  /* Extract stuff */
  memcpy(peer_name, pkt + 1 + val_len, name_len);
  peer_name[name_len] = 0;
  memcpy(chap_value, pkt + 1, val_len);
  *chap_value_size = val_len;
  Log(LG_AUTH, ("[%s]   Name: \"%s\"", pref, peer_name));
#if 0
  Log(LG_AUTH, ("[%s]   Value: %d bytes", pref, *chap_value_size));
#endif
  return(0);
}

/*
 * ChapChalTimeout()
 *
 * Timer expired for reply to challenge packet
 */

void
ChapChalTimeout(void *ptr)
{
  Link		const l = (Link) ptr;
  ChapInfo	const chap = &l->lcp.auth.chap;

  if (--chap->retry > 0) {
    TimerStart(&chap->chalTimer);
    ChapSendChallenge(l);
  }
}

/*
 * ChapInput()
 */

void
ChapInput(Link l, AuthData auth, const u_char *pkt, u_short len)
{
  Auth		const a = &l->lcp.auth;
  ChapInfo	const chap = &a->chap;
  char		peer_name[CHAP_MAX_NAME + 1];
  char		password[AUTH_MAX_PASSWORD];
  u_char	hash_value[CHAP_MAX_VAL];
  int		hash_value_size;
  
  chap->proto = auth->proto;
  
  switch (auth->code) {
    case CHAP_CHALLENGE:
      {
	u_char	value[CHAP_MAX_VAL];		/* Chap packet */
	int	value_len;			/* Packet length */
	char	*name, *secret;
	int	name_len, idFail;

	auth->alg = a->self_to_peer_alg;

	/* Check packet */
	if ((a->self_to_peer != PROTO_CHAP && a->self_to_peer != PROTO_EAP)
	    || l->lcp.phase != PHASE_AUTHENTICATE)
	  Log(LG_AUTH, ("[%s] CHAP: Not expected, but that's OK", l->name));
	if (ChapParsePkt(l->name, pkt, len, peer_name, value, &value_len) < 0)
	  break;

	/* Never respond to our own outstanding challenge */
	if (value_len == auth->params.chap.chal_len
	    && !memcmp(value, auth->params.chap.chal_data, auth->params.chap.chal_len)) {
	  Log(LG_AUTH, ("[%s] CHAP: SECURITY: peer sent same challenge! Ignoring.", l->name));
	  break;
	}

	/* Don't respond to a challenge that looks like it came from
	   us and has the wrong origination value embedded in it. This
	   avoids a security hole associated with using the same CHAP
	   password to authenticate in both directions on a link. */
	idFail = 0;
	do {
	  char	buf[sizeof(gIdBytes)];
	  int	chalOrig;

	  /* Check challenge length */
	  if (value_len < sizeof(buf))
	    break;

	  /* Copy challenge bits and extract origination value */
	  memcpy(buf, value, sizeof(buf));
	  chalOrig = (buf[0] >> 6) & 0x03;
	  buf[0] &= 0x3f;

	  /* Check for same ID bytes in the challenge */
	  if (memcmp(buf, gIdBytes, sizeof(gIdBytes)) != 0)
	    break;

	  /* ID bytes match; origination value must be opposite. Note this
	     assumes that if we can tell the origination direction of a link,
	     then so can the peer. */
	  switch (l->originate) {
	    case LINK_ORIGINATE_LOCAL:
	      idFail = (chalOrig != LINK_ORIGINATE_REMOTE);
	      break;
	    case LINK_ORIGINATE_REMOTE:
	      idFail = (chalOrig != LINK_ORIGINATE_LOCAL);
	      break;
	    case LINK_ORIGINATE_UNKNOWN:
	    default:
	      idFail = 0;	/* XXX assumes leased line, etc is secure */
	      break;
	  }

	  /* Log failure */
	  if (idFail) {
	    Log(LG_AUTH,
	      ("[%s] CHAP: SECURITY: origination value check failed (%s,%s). Ignoring.",
	        l->name,
		LINK_ORIGINATION(l->originate),
		LINK_ORIGINATION(chalOrig)));
	    break;
	  }
	} while (0);
	if (idFail)
	  break;

	/*
	 * Name we use to authenticate ourselves:
	 *
	 * 1. The manually configured authname ("set authname ...")
	 * 2. The peer's supplied name
	 */
	if (*auth->conf.authname)
	  name = auth->conf.authname;
	else
	  name = peer_name;
	name_len = strlen(name);
	Log(LG_AUTH, ("[%s] CHAP: Using authname \"%s\"", l->name, name));

	strlcpy(auth->params.authname, name, sizeof(auth->params.authname));

	/* Get the corresponding secret */
	if ((strcmp(auth->conf.authname, auth->params.authname) == 0) && 
	    auth->conf.password[0] != 0) {
		strlcpy(password, auth->conf.password, sizeof(password));
	} else if (AuthGetData(auth->params.authname, password, 
	    sizeof(password), NULL, NULL) < 0) {
		Log(LG_AUTH, ("[%s] CHAP: Warning: no secret for \"%s\" found",
		    l->name, 
		    auth->params.authname));
		break;
	}

	secret = ChapGetSecret(l, a->self_to_peer_alg, password);

	/* Get hash value */
	if ((hash_value_size = ChapHash(l, a->self_to_peer_alg, hash_value,
	    auth->id, name, secret, value, value_len, 1)) < 0) {
	  Log(LG_AUTH, ("[%s] CHAP: Hash failure", l->name));
	  break;
	}

	/* Need to remember MS-CHAP stuff for use with MPPE encryption */
	switch (a->self_to_peer_alg) {
	case CHAP_ALG_MSOFT:
  	  if (l->originate == LINK_ORIGINATE_LOCAL)
		memcpy(a->params.msoft.msChal, value, CHAP_MSOFT_CHAL_LEN);
	  break;
	case CHAP_ALG_MSOFTv2:
  	  if (l->originate == LINK_ORIGINATE_LOCAL) {
		memcpy(a->params.msoft.msChal, value, CHAP_MSOFTv2_CHAL_LEN);
		memcpy(a->params.msoft.ntResp,
	    	    hash_value + offsetof(struct mschapv2value, ntHash),
		    CHAP_MSOFTv2_RESP_LEN);
	  }
	  break;
	}

	/* Build response packet */
	if (chap->resp)
	  Freee(chap->resp);
	chap->resp = Malloc(MB_AUTH, 1 + hash_value_size + name_len);
	chap->resp[0] = hash_value_size;
	memcpy(&chap->resp[1], hash_value, hash_value_size);
	memcpy(&chap->resp[1 + hash_value_size], name, name_len);
	chap->resp_len = 1 + hash_value_size + name_len;
	chap->resp_id = auth->id;

	/* Send response to peer */
	ChapSendResponse(l);
      }
      break;

    case CHAP_RESPONSE:
	auth->alg = a->peer_to_self_alg;

	/* Stop challenge timer */
	TimerStop(&chap->chalTimer);

	/* Check response */
	if ((a->peer_to_self != PROTO_CHAP && a->peer_to_self != PROTO_EAP)
	    || l->lcp.phase != PHASE_AUTHENTICATE)
	  Log(LG_AUTH, ("[%s] CHAP: Not expected, but that's OK", l->name));
	if (ChapParsePkt(l->name, pkt, len,
	    peer_name, auth->params.chap.value, &auth->params.chap.value_len) < 0) {
	  auth->why_fail = AUTH_FAIL_INVALID_PACKET;
	  ChapInputFinish(l, auth);
	  return;
	}

	/* Strip MS domain if any */
	if (!Enabled(&l->conf.options, LINK_CONF_MSDOMAIN))
	  if (a->peer_to_self_alg == CHAP_ALG_MSOFT
	      || a->peer_to_self_alg == CHAP_ALG_MSOFTv2) {
	    char	*s;

	    if ((s = strrchr(peer_name, '\\')))
	      memmove(peer_name, s + 1, strlen(s) + 1);
	  }

	strlcpy(auth->params.authname, peer_name, sizeof(auth->params.authname));
	
	auth->finish = ChapInputFinish;
	AuthAsyncStart(l, auth);
	return;

    case CHAP_SUCCESS:
    case CHAP_FAILURE:

	auth->alg = a->self_to_peer_alg;

      /* Stop response timer */
      TimerStop(&chap->respTimer);
      if (chap->resp) {
	Freee(chap->resp);
	chap->resp = NULL;
      }

      /* Appropriate? */
      if (a->self_to_peer != PROTO_CHAP
	  || l->lcp.phase != PHASE_AUTHENTICATE) {
	Log(LG_AUTH, ("[%s] CHAP: Not expected, but that's OK", l->name));
	break;
      }

      /* Log message */
      ShowMesg(LG_AUTH, l->name, (char *) pkt, len);
      AuthFinish(l, AUTH_SELF_TO_PEER, auth->code == CHAP_SUCCESS);
      break;
      
    case CHAP_MS_V1_CHANGE_PW:
      Log(LG_AUTH, ("[%s] CHAP: Sorry changing passwords using MS-CHAPv1 is not yet implemented",
	l->name));
      goto badResponse;

    case CHAP_MS_V2_CHANGE_PW:
      Log(LG_AUTH, ("[%s] CHAP: Sorry changing passwords using MS-CHAPv2 is not yet implemented",
	l->name));
      goto badResponse;

    default:
      Log(LG_AUTH, ("[%s] CHAP: unknown code %d", l->name, auth->code));
      break;
  }
    AuthDataDestroy(auth);
    return;
  
badResponse:
  {
    char	failMesg[64];

    auth->why_fail = AUTH_FAIL_NOT_EXPECTED;
    AuthFailMsg(auth, failMesg, sizeof(failMesg));
    AuthOutput(l, auth->proto, auth->proto == PROTO_CHAP ? CHAP_FAILURE : EAP_FAILURE,
	auth->id, (u_char *)failMesg, strlen(failMesg), 0, EAP_TYPE_MD5CHAL);
    AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
    AuthDataDestroy(auth);
  }
}

/*
 * ChapInputFinish()
 *
 * Possible return point from the asynch auth handler.
 * 
 */
 
void 
ChapInputFinish(Link l, AuthData auth)
{
    Auth	a = &l->lcp.auth;
    ChapInfo	chap = &a->chap;
    u_char	hash_value[CHAP_MAX_VAL];
    int		hash_value_size;
    char	ackMesg[128], *secret;
   
    Log(LG_AUTH, ("[%s] CHAP: Auth return status: %s", 
	l->name, AuthStatusText(auth->status)));
    
    if (auth->status == AUTH_STATUS_BUSY) {
	AuthDataDestroy(auth);  
	return;
    }
    
    if (a->peer_to_self_alg == CHAP_ALG_MSOFTv2 && 
      auth->mschapv2resp != NULL) {
	strlcpy(ackMesg, auth->mschapv2resp, sizeof(ackMesg));
    } else if (auth->reply_message != NULL) {
	strlcpy(ackMesg, auth->reply_message, sizeof(ackMesg));
    } else {
	strlcpy(ackMesg, AUTH_MSG_WELCOME, sizeof(ackMesg));
    }

    if (auth->status == AUTH_STATUS_FAIL)
	goto badResponse;
    else if (auth->status == AUTH_STATUS_SUCCESS)
	goto goodResponse;
  
    /* Copy in peer challenge for MS-CHAPv2 */
    if (a->peer_to_self_alg == CHAP_ALG_MSOFTv2)
	memcpy(hash_value, a->params.chap.value, 16);
    
    secret = ChapGetSecret(l, a->peer_to_self_alg, a->params.password);

    /* Get expected hash value */
    if ((hash_value_size = ChapHash(l, a->peer_to_self_alg, hash_value, auth->id,
      a->params.authname, secret, a->params.chap.chal_data, a->params.chap.chal_len,
      0)) < 0) {
	Log(LG_AUTH, ("[%s] CHAP: Hash failure", l->name));
	auth->why_fail = AUTH_FAIL_INVALID_PACKET;
	goto badResponse;
    }

    /* Compare with peer's response */
    if (a->params.chap.chal_len == 0
      || !ChapHashAgree(a->peer_to_self_alg, hash_value, hash_value_size,
		a->params.chap.value, a->params.chap.value_len)) {
	Log(LG_AUTH, ("[%s] CHAP: Invalid response", l->name));
	auth->why_fail = AUTH_FAIL_INVALID_LOGIN;
	goto badResponse;
    }
  
    /* Response is good */
    Log(LG_AUTH, ("[%s] CHAP: Response is valid", l->name));

    if (a->peer_to_self_alg == CHAP_ALG_MSOFTv2) {
	struct mschapv2value *const pv = (struct mschapv2value *)a->params.chap.value;
	char hex[41];
	u_char authresp[20];
	int i;

	/* Generate MS-CHAPv2 'authenticator response' */
	GenerateAuthenticatorResponse(a->params.msoft.nt_hash, pv->ntHash,
    	    pv->peerChal, a->params.chap.chal_data, a->params.authname, authresp);
	for (i = 0; i < 20; i++)
    	    sprintf(hex + (i * 2), "%02X", authresp[i]);
    	/* If we have reply message, send it. */
	if (auth->reply_message != NULL) {
	    snprintf(ackMesg, sizeof(ackMesg), "S=%s M=%s",
		hex, auth->reply_message);
	} else
	    snprintf(ackMesg, sizeof(ackMesg), "S=%s", hex);
    }
  
goodResponse:
#ifdef USE_OPIE
    /* make a dummy verify to force an update of the opiekeys database */
    if (a->params.authentic == AUTH_CONF_OPIE)
	opieverify(&auth->opie.data, a->params.password);
#endif

    /* Need to remember MS-CHAP stuff for use with MPPE encryption */
    if (l->originate == LINK_ORIGINATE_REMOTE &&
      a->peer_to_self_alg == CHAP_ALG_MSOFTv2 &&
      !memcmp(a->params.msoft.ntResp, gMsoftZeros, CHAP_MSOFTv2_RESP_LEN)) {
	memcpy(a->params.msoft.ntResp,
    	    a->params.chap.value + offsetof(struct mschapv2value, ntHash),
    	    CHAP_MSOFTv2_RESP_LEN);
    }
  
    Log(LG_AUTH, ("[%s] CHAP: Reply message: %s", l->name, ackMesg));
    AuthOutput(l, chap->proto, chap->proto == PROTO_CHAP ? CHAP_SUCCESS : EAP_SUCCESS,
	auth->id, (u_char *)ackMesg, strlen(ackMesg), 0, EAP_TYPE_MD5CHAL);
    AuthFinish(l, AUTH_PEER_TO_SELF, TRUE);
    AuthDataDestroy(auth);
    return;  

badResponse:
  {
    char        failMesg[64];

    AuthFailMsg(auth, failMesg, sizeof(failMesg));
    Log(LG_AUTH, ("[%s] CHAP: Reply message: %s", l->name, failMesg));
    AuthOutput(l, chap->proto, chap->proto == PROTO_CHAP ? CHAP_FAILURE : EAP_FAILURE,
	auth->id, (u_char *)failMesg, strlen(failMesg), 0, EAP_TYPE_MD5CHAL);
    AuthFinish(l, AUTH_PEER_TO_SELF, FALSE);
    AuthDataDestroy(auth);  
  }
}

/*
 * ChapGetSecret()
 * 
 * returns either the plaintext pass for CHAP-MD5
 * or the NT-Hash for MS-CHAP. Set's credentials for
 * MPPE-Key derivation
 */

static char *
ChapGetSecret(Link l, int alg, char *password)
{
  Auth		a = &l->lcp.auth;
  char		*pw;
  
  if (alg == CHAP_ALG_MD5)
    pw = password;
  else {
    if (!a->params.msoft.has_nt_hash)
    {
      NTPasswordHash(password, a->params.msoft.nt_hash);
      NTPasswordHashHash(a->params.msoft.nt_hash, a->params.msoft.nt_hash_hash);
      LMPasswordHash(password, a->params.msoft.lm_hash);
      a->params.msoft.has_nt_hash = TRUE;
      a->params.msoft.has_lm_hash = TRUE;
    }

    pw = (char *) a->params.msoft.nt_hash;
  }

  return pw;
}

/*
 * ChapGenRandom()
 */

static void
ChapGenRandom(Link l, u_char *buf, int len)
{
  int	k;

  /* Prefix with our unique ID plus origination value */
  for (k = 0; k < sizeof(gIdBytes) && k < len; k++)
    buf[k] = gIdBytes[k];
  buf[0] |= (l->originate & 0x03) << 6;

  /* Fill the rest with semi-random bytes */
  for (; k < len; k++)
    buf[k] = random() & 0xff;
}

/*
 * ChapHash()
 */

static int
ChapHash(Link l, int alg, u_char *hash_value, u_char id, const char *username,
	const char *secret, const u_char *challenge, int clen, int local)
{
  int	hash_size;

  switch (alg) {
    case CHAP_ALG_MD5:
      {
	MD5_CTX	md5ctx;

	MD5_Init(&md5ctx);
	MD5_Update(&md5ctx, &id, 1);
	MD5_Update(&md5ctx, secret, strlen(secret));
	MD5_Update(&md5ctx, challenge, clen);
	MD5_Final(hash_value, &md5ctx);
	hash_size = 16;
      }
      break;

    case CHAP_ALG_MSOFT:
      {
	struct mschapvalue	*const val = (struct mschapvalue *) hash_value;

	/* We don't generate the LANManager hash because it's too insecure */
	memset(val->lmHash, 0, sizeof(val->lmHash));
	NTChallengeResponse(challenge, secret, val->ntHash);
	val->useNT = 1;
	hash_size = 49;
      }
      break;
    case CHAP_ALG_MSOFTv2:
      {
	struct mschapv2value *const val = (struct mschapv2value *) hash_value;
	const char *s;

	if ((s = strrchr(username, '\\')) != NULL)
	  username = s + 1;
	if (local) {			/* generate reverse 'peer challenge' */
	  ChapGenRandom(l, val->peerChal, sizeof(val->peerChal));
	  memset(val->reserved, 0, sizeof(val->reserved));
	}
	GenerateNTResponse(challenge,
	  val->peerChal, username, secret, val->ntHash);

	hash_size = 49;
      }
      break;

    default:
      return(-1);
  }

  /* Done */
  return(hash_size);
}

/*
 * ChapHashAgree()
 */

static int
ChapHashAgree(int alg, const u_char *self, int slen,
	const u_char *peer, int plen)
{
  switch (alg)
  {
    case CHAP_ALG_MD5:
      return(slen == plen && !memcmp(self, peer, slen));

    case CHAP_ALG_MSOFT:
      {
	struct mschapvalue	*const sv = (struct mschapvalue *) self;
	struct mschapvalue	*const pv = (struct mschapvalue *) peer;

	if (slen != 49 || plen != 49)
	  return(0);
	if (sv->useNT != 1 || pv->useNT != 1)
	  return(0);
	return(!memcmp(&sv->ntHash, &pv->ntHash, sizeof(sv->ntHash)));
      }
    case CHAP_ALG_MSOFTv2:
      {
	struct mschapv2value	*const sv =(struct mschapv2value *) self;
	struct mschapv2value	*const pv =(struct mschapv2value *) peer;

	if (slen != 49 || plen != 49)
	  return(0);
	return(!memcmp(&sv->ntHash, &pv->ntHash, sizeof(sv->ntHash)));
      }

    default:
      return(0);
  }
}

/*
 * ChapCode()
 */

const char *
ChapCode(int code, char *buf, size_t len)
{
  switch (code) {
    case CHAP_CHALLENGE:
	strlcpy(buf, "CHALLENGE", len);
	break;
    case CHAP_RESPONSE:
	strlcpy(buf, "RESPONSE", len);
	break;
    case CHAP_SUCCESS:
	strlcpy(buf, "SUCCESS", len);
	break;
    case CHAP_FAILURE:
	strlcpy(buf, "FAILURE", len);
	break;
    default:
	snprintf(buf, len, "code%d", code);
  }
  return(buf);
}
