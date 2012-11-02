
/*
 * mp.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _MP_H_
#define _MP_H_

#include <sys/types.h>
#include "fsm.h"
#include "mbuf.h"

/*
 * DEFINITIONS
 */

/* Discriminators */

  #define MAX_DISCRIM		50

  #define DISCRIM_CLASS_NULL	0
  #define DISCRIM_CLASS_LOCAL	1
  #define DISCRIM_CLASS_IPADDR	2
  #define DISCRIM_CLASS_802_1	3
  #define DISCRIM_CLASS_MAGIC	4
  #define DISCRIM_CLASS_PSN	5

  struct discrim {
    u_char	len;
    u_char	class;
    u_char	bytes[MAX_DISCRIM];
  };
  typedef struct discrim	*Discrim;

/* Bounds on things */

  #define MP_MIN_MRRU		LCP_MIN_MRU		/* Per RFC 1990 */
  #define MP_MAX_MRRU		4096
  #define MP_DEFAULT_MRRU	2048

/* LCP codes acceptable to transmit over the virtual link */

  #define MP_LCP_CODE_OK(c)	((c) >= CODE_CODEREJ && (c) <= CODE_ECHOREP)

/*
 * FUNCTIONS
 */

  extern void	MpSetDiscrim(void);
  extern int	MpDiscrimEqual(Discrim dis1, Discrim dis2);
  extern char *	MpDiscrimText(Discrim dis, char *buf, size_t len);

#endif

