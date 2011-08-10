
/*
 * pap.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _PAP_H_
#define	_PAP_H_

#include "mbuf.h"
#include "timer.h"

/*
 * DEFINITIONS
 */

  #define PAP_REQUEST		1
  #define PAP_ACK		2
  #define PAP_NAK		3

  struct papinfo {
    short		next_id;			/* Packet id */
    short		retry;				/* Resend count */
    struct pppTimer	timer;				/* Resend timer */
  };
  typedef struct papinfo	*PapInfo;

  struct papparams {
    char		peer_pass[AUTH_MAX_PASSWORD];
    char		peer_name[AUTH_MAX_AUTHNAME];    
  };
  typedef struct papparams	*PapParams;

  struct authdata;
  
/*
 * FUNCTIONS
 */

  extern void	PapStart(Link l, int which);
  extern void	PapStop(PapInfo pap);
  extern void	PapInput(Link l, struct authdata *auth, const u_char *pkt, u_short len);
  extern void	PapInputFinish(Link l, struct authdata *auth);
  extern const	char *PapCode(int code, char *buf, size_t len);

#endif

