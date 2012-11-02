
/*
 * ecp_des.c
 *
 * Rewritten by Alexander Motin <mav@FreeBSD.org>
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "ecp.h"
#include "log.h"

/*
 * DEFINITIONS
 */

  #define DES_OVERHEAD		2

/*
 * INTERNAL FUNCTIONS
 */

  static int	DesInit(Bund b, int dir);
  static void	DesConfigure(Bund b);
  static int	DesSubtractBloat(Bund b, int size);
  static Mbuf	DesEncrypt(Bund b, Mbuf plain);
  static Mbuf	DesDecrypt(Bund b, Mbuf cypher);
  static void	DesCleanup(Bund b, int dir);
  static int	DesStat(Context ctx, int dir);

  static u_char	*DesBuildConfigReq(Bund b, u_char *cp);
  static void	DesDecodeConfigReq(Fsm fp, FsmOption opt, int mode);

/*
 * GLOBAL VARIABLES
 */

  const struct enctype	gDeseEncType =
  {
    "dese-old",
    ECP_TY_DESE,
    DesInit,
    DesConfigure,
    NULL,
    DesSubtractBloat,
    DesCleanup,
    DesBuildConfigReq,
    DesDecodeConfigReq,
    NULL,
    NULL,
    NULL,
    DesStat,
    DesEncrypt,
    DesDecrypt,
  };

/*
 * DesInit()
 */

static int
DesInit(Bund b, int dir)
{
  EcpState	const ecp = &b->ecp;
  DesInfo	const des = &ecp->des;

  switch (dir) {
    case ECP_DIR_XMIT:
	des->xmit_seq = 0;
      break;
    case ECP_DIR_RECV:
	des->recv_seq = 0;
      break;
    default:
      assert(0);
      return(-1);
  }
  return(0);
}

/*
 * DesConfigure()
 */

static void
DesConfigure(Bund b)
{
  EcpState	const ecp = &b->ecp;
  DesInfo	const des = &ecp->des;
  des_cblock	key;

  des_check_key = FALSE;
  des_string_to_key(ecp->key, &key);
  des_set_key(&key, des->ks);
  des->xmit_seq = 0;
  des->recv_seq = 0;
}

/*
 * DesSubtractBloat()
 */

static int
DesSubtractBloat(Bund b, int size)
{
  size -= DES_OVERHEAD;	/* reserve space for header */
  size &= ~0x7;
  return(size);
}

static int
DesStat(Context ctx, int dir) 
{
    EcpState	const ecp = &ctx->bund->ecp;
    DesInfo	const des = &ecp->des;
    
    switch (dir) {
	case ECP_DIR_XMIT:
	    Printf("\tBytes\t: %llu -> %llu (%+lld%%)\r\n",
		(unsigned long long)des->xmit_stats.OctetsIn,
		(unsigned long long)des->xmit_stats.OctetsOut,
		((des->xmit_stats.OctetsIn!=0)?
		    ((long long)(des->xmit_stats.OctetsOut - des->xmit_stats.OctetsIn)
			*100/(long long)des->xmit_stats.OctetsIn):
		    0));
	    Printf("\tFrames\t: %llu -> %llu\r\n",
		(unsigned long long)des->xmit_stats.FramesIn,
		(unsigned long long)des->xmit_stats.FramesOut);
	    Printf("\tErrors\t: %llu\r\n",
		(unsigned long long)des->xmit_stats.Errors);
	    break;
	case ECP_DIR_RECV:
	    Printf("\tBytes\t: %llu <- %llu (%+lld%%)\r\n",
		(unsigned long long)des->recv_stats.OctetsOut,
		(unsigned long long)des->recv_stats.OctetsIn,
		((des->recv_stats.OctetsOut!=0)?
		    ((long long)(des->recv_stats.OctetsIn - des->recv_stats.OctetsOut)
			*100/(long long)des->recv_stats.OctetsOut):
		    0));
	    Printf("\tFrames\t: %llu <- %llu\r\n",
		(unsigned long long)des->xmit_stats.FramesOut,
		(unsigned long long)des->xmit_stats.FramesIn);
	    Printf("\tErrors\t: %llu\r\n",
		(unsigned long long)des->recv_stats.Errors);
    	    break;
	default:
    	    assert(0);
    }
    return (0);
}

/*
 * DesEncrypt()
 */

Mbuf
DesEncrypt(Bund b, Mbuf plain)
{
  EcpState	const ecp = &b->ecp;
  DesInfo	const des = &ecp->des;
  const int	plen = MBLEN(plain);
  int		padlen = roundup2(plen, 8) - plen;
  int		clen = plen + padlen;
  Mbuf		cypher;
  int		k;

  des->xmit_stats.FramesIn++;
  des->xmit_stats.OctetsIn += plen;

/* Get mbuf for encrypted frame */

  cypher = mballoc(DES_OVERHEAD + clen);

/* Copy in sequence number */

  MBDATAU(cypher)[0] = des->xmit_seq >> 8;
  MBDATAU(cypher)[1] = des->xmit_seq & 0xff;
  des->xmit_seq++;

/* Copy in plaintext */

  mbcopy(plain, 0, MBDATA(cypher) + DES_OVERHEAD, plen);
  
  cypher->cnt = DES_OVERHEAD + clen;
  
/* Copy in plaintext and encrypt it */
  
  for (k = 0; k < clen; k += 8)
  {
    u_char	*const block = MBDATA(cypher) + DES_OVERHEAD + k;

    des_cbc_encrypt(block, block, 8, des->ks, &des->xmit_ivec, TRUE);
    memcpy(des->xmit_ivec, block, 8);
  }

  des->xmit_stats.FramesOut++;
  des->xmit_stats.OctetsOut += DES_OVERHEAD + clen;

/* Return cyphertext */

  mbfree(plain);
  return(cypher);
}

/*
 * DesDecrypt()
 */

Mbuf
DesDecrypt(Bund b, Mbuf cypher)
{
  EcpState	const ecp = &b->ecp;
  DesInfo	des = &ecp->des;
  const int	clen = MBLEN(cypher) - DES_OVERHEAD;
  u_int16_t	seq;
  Mbuf		plain;
  int		k;

  des->recv_stats.FramesIn++;
  des->recv_stats.OctetsIn += clen + DES_OVERHEAD;

/* Get mbuf for plaintext */

  if (clen < 8 || (clen & 0x7))
  {
    Log(LG_ECP, ("[%s] DESE: rec'd bogus DES cypher: len=%d",
      b->name, clen + DES_OVERHEAD));
    des->recv_stats.Errors++;
    return(NULL);
  }

/* Check sequence number */

  cypher = mbread(cypher, &seq, DES_OVERHEAD);
  seq = ntohs(seq);
  if (seq != des->recv_seq)
  {
    Mbuf	tail;

  /* Recover from dropped packet */

    Log(LG_ECP, ("[%s] DESE: rec'd wrong seq=%u, expected %u",
      b->name, seq, des->recv_seq));
    tail = mbadj(cypher, clen - 8);
    tail = mbread(tail, &des->recv_ivec, 8);
    assert(!tail);
    des->recv_seq = seq + 1;
    des->recv_stats.Errors++;
    return(NULL);
  }
  des->recv_seq++;

/* Decrypt frame */

  plain = cypher;
  for (k = 0; k < clen; k += 8)
  {
    u_char	*const block = MBDATA(plain) + k;
    des_cblock	next_ivec;

    memcpy(next_ivec, block, 8);
    des_cbc_encrypt(block, block, 8, des->ks, &des->recv_ivec, FALSE);
    memcpy(des->recv_ivec, next_ivec, 8);
  }

  des->recv_stats.FramesOut++;
  des->recv_stats.OctetsOut += clen;

/* Done */

  return(plain);
}

/*
 * DesCleanup()
 */

static void
DesCleanup(Bund b, int dir)
{
  EcpState	const ecp = &b->ecp;
  DesInfo	const des = &ecp->des;
  
  if (dir == ECP_DIR_RECV)
  {
    memset(&des->recv_stats, 0, sizeof(des->recv_stats));
  }
  if (dir == ECP_DIR_XMIT)
  {
    memset(&des->xmit_stats, 0, sizeof(des->xmit_stats));
  }
}

/*
 * DesBuildConfigReq()
 */

static u_char *
DesBuildConfigReq(Bund b, u_char *cp)
{
  EcpState	const ecp = &b->ecp;
  DesInfo	const des = &ecp->des;

  ((u_int32_t *) des->xmit_ivec)[0] = random();
  ((u_int32_t *) des->xmit_ivec)[1] = random();
  return(FsmConfValue(cp, ECP_TY_DESE, 8, des->xmit_ivec));
}

/*
 * DesDecodeConfigReq()
 */

static void
DesDecodeConfigReq(Fsm fp, FsmOption opt, int mode)
{
    Bund 	b = (Bund)fp->arg;
  DesInfo	const des = &b->ecp.des;

  if (opt->len != 10)
  {
    Log(LG_ECP, ("[%s]   bogus length %d", b->name, opt->len));
    if (mode == MODE_REQ)
      FsmRej(fp, opt);
    return;
  }
  Log(LG_ECP, ("[%s]   nonce 0x%02x%02x%02x%02x%02x%02x%02x%02x", b->name,
    opt->data[0], opt->data[1],opt->data[2],opt->data[3],
    opt->data[4], opt->data[5],opt->data[6],opt->data[7]));
  switch (mode)
  {
    case MODE_REQ:
      memcpy(des->recv_ivec, opt->data, 8);
      FsmAck(fp, opt);
      break;
    case MODE_NAK:
      break;
  }
}

