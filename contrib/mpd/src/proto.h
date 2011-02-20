
/*
 * proto.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _PROTO_H_
#define _PROTO_H_

/* Network layer protocols */

  #define PROTO_IP		0x0021		/* IP */
  #define PROTO_IPV6		0x0057		/* IPv6 */
  #define PROTO_VJUNCOMP	0x002f		/* VJ Uncompressed */
  #define PROTO_VJCOMP		0x002d		/* VJ Compressed */
  #define PROTO_MP		0x003d		/* Multi-link PPP */
  #define PROTO_COMPD		0x00fd		/* Compressed datagram */
  #define PROTO_ICOMPD		0x00fb		/* Individual link compress */
  #define PROTO_CRYPT		0x0053		/* Encrypted datagram */
  #define PROTO_ICRYPT		0x0055		/* Individual link encrypted */

/* Network layer control protocols */

  #define PROTO_IPCP		0x8021
  #define PROTO_IPV6CP		0x8057
  #define PROTO_CCP		0x80fd
  #define PROTO_ICCP		0x80fb
  #define PROTO_ECP		0x8053
  #define PROTO_IECP		0x8055
  #define PROTO_ATCP		0x8029

/* Link layer control protocols */

  #define PROTO_LCP		0xc021
  #define PROTO_PAP		0xc023
  #define PROTO_LQR		0xc025
  #define PROTO_SPAP		0xc027
  #define PROTO_CHAP		0xc223
  #define PROTO_EAP		0xc227

  #define PROTO_UNKNOWN		0x0000

  #define PROT_VALID(p)		(((p) & 0x0101) == 0x0001)
  #define PROT_NETWORK_DATA(p)	(((p) & 0xC000) == 0x0000)
  #define PROT_LOW_VOLUME(p)	(((p) & 0xC000) == 0x4000)
  #define PROT_NETWORK_CTRL(p)	(((p) & 0xC000) == 0x8000)
  #define PROT_LINK_LAYER(p)	(((p) & 0xC000) == 0xC000)
  #define PROT_COMPRESSIBLE(p)	(((p) & 0xFF00) == 0x0000)

  #define PROT_CCP_COMPABLE(p)	((p) >= 0x21 && (p) < 0xfa)

/*
 * FUNCTIONS
 */

  extern const char  	*ProtoName(int proto);

#endif

