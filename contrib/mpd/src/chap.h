
/*
 * chap.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _CHAP_H_
#define	_CHAP_H_

#include "mbuf.h"
#include "timer.h"

/*
 * DEFINITIONS
 */

  #define CHAP_CHALLENGE	1
  #define CHAP_RESPONSE		2
  #define CHAP_SUCCESS		3
  #define CHAP_FAILURE		4
  #define CHAP_MS_V1_CHANGE_PW	5
  #define CHAP_MS_V2_CHANGE_PW	7

  #define CHAP_MAX_NAME		64
  #define CHAP_MAX_VAL		64

  #define CHAP_ALG_MD5		0x05
  #define CHAP_ALG_MSOFT	0x80
  #define CHAP_ALG_MSOFTv2	0x81

  #define CHAP_MSOFT_CHAL_LEN	8
  #define CHAP_MSOFTv2_CHAL_LEN	16
  #define CHAP_MSOFTv2_RESP_LEN	24

  #define MSCHAP_ERROR_RESTRICTED_LOGON_HOURS	646 
  #define MSCHAP_ERROR_ACCT_DISABLED		647 
  #define MSCHAP_ERROR_PASSWD_EXPIRED		648 
  #define MSCHAP_ERROR_NO_DIALIN_PERMISSION	649 
  #define MSCHAP_ERROR_AUTHENTICATION_FAILURE	691 
  #define MSCHAP_ERROR_CHANGING_PASSWORD	709

  struct chapinfo
  {
    int			proto;				/* CHAP, EAP */
    short		next_id;			/* Packet id */
    short		retry;				/* Resend count */
    struct pppTimer	chalTimer;			/* Challenge timer */
    struct pppTimer	respTimer;			/* Reponse timer */
    u_char		*resp;				/* Response packet */
    short		resp_len;			/* Response length */
    u_char		resp_id;			/* Response ID */
  };
  typedef struct chapinfo	*ChapInfo;

  struct chapparams
  {
    u_char		chal_data[CHAP_MAX_VAL];	/* Challenge sent */
    u_char		value[CHAP_MAX_VAL];		/* Chap packet */
    int			chal_len;			/* Challenge length */
    int			value_len;			/* Packet length */
  };
  typedef struct chapparams	*ChapParams;

  struct mschapvalue {
    u_char	lmHash[24];
    u_char	ntHash[24];
    u_char	useNT;
  };

  struct mschapv2value {
    u_char	peerChal[16];
    u_char	reserved[8];
    u_char	ntHash[24];
    u_char	flags;
  };
  
  struct mschapv2value_cpw {
    u_char	encryptedPass[516];
    u_char	encryptedHash[16];
    u_char	peerChal[16];
    u_char	reserved[8];
    u_char	ntResponse[24];    
    u_char	flags[2];
  };
 
  struct authdata;
/*
 * FUNCTIONS
 */

  extern void	ChapStart(Link l, int which);
  extern void	ChapStop(ChapInfo chap);
  extern void	ChapInput(Link l, struct authdata *auth, const u_char *pkt, u_short len);
  extern void	ChapSendChallenge(Link l);
  extern void	ChapChalTimeout(void *ptr);
  extern const	char *ChapCode(int code, char *buf, size_t len);
  extern void	ChapInputFinish(Link l, struct authdata *auth);

#endif

