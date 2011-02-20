
/*
 * ecp_des.h
 *
 * Rewritten by Alexander Motin <mav@FreeBSD.org>
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _ECP_DES_H_
#define _ECP_DES_H_

#include "defs.h"
#include "mbuf.h"
#include <openssl/des.h>

/*
 * DEFINITIONS
 */

  struct dese_stats {
	uint64_t	FramesIn;
	uint64_t	FramesOut;
	uint64_t	OctetsIn;
	uint64_t	OctetsOut;
	uint64_t	Errors;
  };
  typedef struct dese_stats	*DeseStats;
  
  struct desinfo
  {
    des_cblock		xmit_ivec;	/* Xmit initialization vector */
    des_cblock		recv_ivec;	/* Recv initialization vector */
    u_int16_t		xmit_seq;	/* Transmit sequence number */
    u_int16_t		recv_seq;	/* Receive sequence number */
    des_key_schedule	ks;		/* Key schedule */
    struct dese_stats	recv_stats;	
    struct dese_stats	xmit_stats;	
  };
  typedef struct desinfo	*DesInfo;

/*
 * VARIABLES
 */

  extern const struct enctype	gDeseEncType;

#endif

