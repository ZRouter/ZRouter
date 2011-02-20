
/*
 * ccp_pred1.c
 *
 * Rewritten by Alexander Motin <mav@FreeBSD.org>
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

/*
 * pred1.c
 *
 * Test program for Dave Rand's rendition of the predictor algorithm
 *
 * Updated by: archie@freebsd.org (Archie Cobbs)
 * Updated by: iand@labtam.labtam.oz.au (Ian Donaldson)
 * Updated by: Carsten Bormann <cabo@cs.tu-berlin.de>
 * Original  : Dave Rand <dlr@bungi.com>/<dave_rand@novell.com>
 */

#include "ppp.h"
#include "ccp.h"
#include "util.h"
#include "ngfunc.h"

#ifdef USE_NG_PRED1
#include <netgraph/ng_message.h>
#include <netgraph.h>
#endif

/*
 * DEFINITIONS
 */

  #define PRED1_DECOMP_BUF_SIZE	4096

  #define PRED1_MAX_BLOWUP(n)	((n) * 9 / 8 + 24)

/*
 * The following hash code is the heart of the algorithm:
 * It builds a sliding hash sum of the previous 3-and-a-bit characters
 * which will be used to index the guess table.
 * A better hash function would result in additional compression,
 * at the expense of time.
 */

  #define IHASH(x) p->iHash = (p->iHash << 4) ^ (x)
  #define OHASH(x) p->oHash = (p->oHash << 4) ^ (x)

/*
 * INTERNAL FUNCTIONS
 */

  static int	Pred1Init(Bund b, int direction);
  static void	Pred1Cleanup(Bund b, int direction);
#ifndef USE_NG_PRED1
  static Mbuf	Pred1Compress(Bund b, Mbuf plain);
  static Mbuf	Pred1Decompress(Bund b, Mbuf comp);
#endif

  static u_char	*Pred1BuildConfigReq(Bund b, u_char *cp, int *ok);
  static void   Pred1DecodeConfigReq(Fsm fp, FsmOption opt, int mode);
  static Mbuf	Pred1RecvResetReq(Bund b, int id, Mbuf bp, int *noAck);
  static Mbuf	Pred1SendResetReq(Bund b);
  static void	Pred1RecvResetAck(Bund b, int id, Mbuf bp);
  static int    Pred1Negotiated(Bund b, int xmit);
  static int    Pred1SubtractBloat(Bund b, int size);
  static int    Pred1Stat(Context ctx, int dir);

#ifndef USE_NG_PRED1
  static int	Compress(Bund b, u_char *source, u_char *dest, int len);
  static int	Decompress(Bund b, u_char *source, u_char *dest, int slen, int dlen);
  static void	SyncTable(Bund b, u_char *source, u_char *dest, int len);
#endif

/*
 * GLOBAL VARIABLES
 */

  const struct comptype	gCompPred1Info =
  {
    "pred1",
    CCP_TY_PRED1,
    1,
    Pred1Init,
    NULL,
    NULL,
    NULL,
    Pred1SubtractBloat,
    Pred1Cleanup,
    Pred1BuildConfigReq,
    Pred1DecodeConfigReq,
    Pred1SendResetReq,
    Pred1RecvResetReq,
    Pred1RecvResetAck,
    Pred1Negotiated,
    Pred1Stat,
#ifndef USE_NG_PRED1
    Pred1Compress,
    Pred1Decompress,
#else
    NULL,
    NULL,
#endif
  };

/*
 * Pred1Init()
 */

static int
Pred1Init(Bund b, int dir)
{
#ifndef USE_NG_PRED1
    Pred1Info	p = &b->ccp.pred1;

    if (dir == COMP_DIR_XMIT) {
	p->oHash = 0;
	p->OutputGuessTable = Malloc(MB_COMP, PRED1_TABLE_SIZE);
    } else {
	p->iHash = 0;
	p->InputGuessTable = Malloc(MB_COMP, PRED1_TABLE_SIZE);
    }
#else
    struct ngm_mkpeer	mp;
    struct ng_pred1_config conf;
    const char		*pred1hook, *ppphook;
    char		path[NG_PATHSIZ];
    ng_ID_t             id;

    memset(&conf, 0, sizeof(conf));
    conf.enable = 1;
    if (dir == COMP_DIR_XMIT) {
        ppphook = NG_PPP_HOOK_COMPRESS;
        pred1hook = NG_PRED1_HOOK_COMP;
    } else {
        ppphook = NG_PPP_HOOK_DECOMPRESS;
        pred1hook = NG_PRED1_HOOK_DECOMP;
    }

    /* Attach a new PRED1 node to the PPP node */
    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    strcpy(mp.type, NG_PRED1_NODE_TYPE);
    strcpy(mp.ourhook, ppphook);
    strcpy(mp.peerhook, pred1hook);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Log(LG_ERR, ("[%s] can't create %s node: %s",
    	    b->name, mp.type, strerror(errno)));
	return(-1);
    }

    strlcat(path, ppphook, sizeof(path));

    id = NgGetNodeID(-1, path);
    if (dir == COMP_DIR_XMIT) {
	b->ccp.comp_node_id = id;
    } else {
	b->ccp.decomp_node_id = id;
    }

    /* Configure PRED1 node */
    snprintf(path, sizeof(path), "[%x]:", id);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_PRED1_COOKIE, NGM_PRED1_CONFIG, &conf, sizeof(conf)) < 0) {
	Log(LG_ERR, ("[%s] can't config %s node at %s: %s",
    	    b->name, NG_PRED1_NODE_TYPE, path, strerror(errno)));
	NgFuncShutdownNode(gCcpCsock, b->name, path);
	return(-1);
    }
#endif
    return 0;
}

/*
 * Pred1Cleanup()
 */

void
Pred1Cleanup(Bund b, int dir)
{
#ifndef USE_NG_PRED1
    Pred1Info	p = &b->ccp.pred1;

    if (dir == COMP_DIR_XMIT) {
	assert(p->OutputGuessTable);
	Freee(p->OutputGuessTable);
	p->OutputGuessTable = NULL;
	memset(&p->xmit_stats, 0, sizeof(p->xmit_stats));
    } else {
	assert(p->InputGuessTable);
	Freee(p->InputGuessTable);
	p->InputGuessTable = NULL;
	memset(&p->recv_stats, 0, sizeof(p->recv_stats));
    }
#else
    char		path[NG_PATHSIZ];

    /* Remove node */
    if (dir == COMP_DIR_XMIT) {
	snprintf(path, sizeof(path), "[%x]:", b->ccp.comp_node_id);
	b->ccp.comp_node_id = 0;
    } else {
	snprintf(path, sizeof(path), "[%x]:", b->ccp.decomp_node_id);
	b->ccp.decomp_node_id = 0;
    }
    NgFuncShutdownNode(gCcpCsock, b->name, path);
#endif
}

#ifndef USE_NG_PRED1
/*
 * Pred1Compress()
 *
 * Compress a packet and return a compressed version.
 * The original is untouched.
 */

Mbuf
Pred1Compress(Bund b, Mbuf plain)
{
  u_char	*wp, *uncomp, *comp;
  u_int16_t	fcs;
  int		len;
  Mbuf		res;
  int		orglen;
  Pred1Info	p = &b->ccp.pred1;
  
  orglen = MBLEN(plain);
  uncomp = MBDATA(plain);
  
  p->xmit_stats.InOctets += orglen;
  p->xmit_stats.FramesPlain++;
  
  res = mballoc(PRED1_MAX_BLOWUP(orglen + 2));
  comp = MBDATA(res);

  wp = comp;

  *wp++ = (orglen >> 8) & 0x7F;
  *wp++ = orglen & 0xFF;

/* Compute FCS */

  fcs = Crc16(PPP_INITFCS, comp, 2);
  fcs = Crc16(fcs, uncomp, orglen);
  fcs = ~fcs;

/* Compress data */

  len = Compress(b, uncomp, wp, orglen);

/* What happened? */

  if (len < orglen)
  {
    *comp |= 0x80;
    wp += len;
    p->xmit_stats.FramesComp++;
  }
  else
  {
    memcpy(wp, uncomp, orglen);
    wp += orglen;
    p->xmit_stats.FramesUncomp++;
  }

/* Add FCS */

  *wp++ = fcs & 0xFF;
  *wp++ = fcs >> 8;

  res->cnt = (wp - comp);
  
  mbfree(plain);
  Log(LG_CCP2, ("[%s] Pred1: orig (%d) --> comp (%d)", b->name, orglen, res->cnt));

  p->xmit_stats.OutOctets += res->cnt;

  return res;
}

/*
 * Pred1Decompress()
 *
 * Decompress a packet and return a compressed version.
 * The original is untouched.
 */

Mbuf
Pred1Decompress(Bund b, Mbuf mbcomp)
{
  u_char	*uncomp, *comp;
  u_char	*cp;
  u_int16_t	len, len1, cf, lenn;
  u_int16_t	fcs;
  int           orglen;
  Mbuf		mbuncomp;
  Pred1Info	p = &b->ccp.pred1;

  orglen = MBLEN(mbcomp);
  comp = MBDATA(mbcomp);
  cp = comp;
  
  p->recv_stats.InOctets += orglen;
  
  mbuncomp = mballoc(PRED1_DECOMP_BUF_SIZE);
  uncomp = MBDATA(mbuncomp);

/* Get initial length value */
  len = *cp++ << 8;
  len += *cp++;
  
  cf = (len & 0x8000);
  len &= 0x7fff;
  
/* Is data compressed or not really? */
  if (cf)
  {
    p->recv_stats.FramesComp++;
    len1 = Decompress(b, cp, uncomp, orglen - 4, PRED1_DECOMP_BUF_SIZE);
    if (len != len1)	/* Error is detected. Send reset request */
    {
      Log(LG_CCP2, ("[%s] Length error (%d) --> len (%d)", b->name, len, len1));
      p->recv_stats.Errors++;
      mbfree(mbcomp);
      mbfree(mbuncomp);
      CcpSendResetReq(b);
      return NULL;
    }
    cp += orglen - 4;
  }
  else
  {
    p->recv_stats.FramesUncomp++;
    SyncTable(b, cp, uncomp, len);
    cp += len;
  }

  mbuncomp->cnt = len;

  /* Check CRC */
  lenn = htons(len);
  fcs = Crc16(PPP_INITFCS, (u_char *)&lenn, 2);
  fcs = Crc16(fcs, uncomp, len);
  fcs = Crc16(fcs, cp, 2);

#ifdef DEBUG
    if (fcs != PPP_GOODFCS)
      Log(LG_CCP2, ("fcs = %04x (%s), len = %x, olen = %x",
	   fcs, (fcs == PPP_GOODFCS)? "good" : "bad", len, orglen));
#endif

  if (fcs != PPP_GOODFCS)
  {
    Log(LG_CCP2, ("[%s] Pred1: Bad CRC-16", b->name));
    p->recv_stats.Errors++;
    mbfree(mbcomp);
    mbfree(mbuncomp);
    CcpSendResetReq(b);
    return NULL;
  }

  Log(LG_CCP2, ("[%s] Pred1: orig (%d) <-- comp (%d)", b->name, mbuncomp->cnt, orglen));
  mbfree(mbcomp);
  
  p->recv_stats.FramesPlain++;
  p->recv_stats.OutOctets += mbuncomp->cnt;
    
  return mbuncomp;
}
#endif

/*
 * Pred1RecvResetReq()
 */

static Mbuf
Pred1RecvResetReq(Bund b, int id, Mbuf bp, int *noAck)
{
#ifndef USE_NG_PRED1
  Pred1Info     p = &b->ccp.pred1;
  Pred1Init(b, COMP_DIR_XMIT);
  p->xmit_stats.Errors++;
#else
    char		path[NG_PATHSIZ];
    /* Forward ResetReq to the Predictor1 compression node */
    snprintf(path, sizeof(path), "[%x]:", b->ccp.comp_node_id);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_PRED1_COOKIE, NGM_PRED1_RESETREQ, NULL, 0) < 0) {
	Log(LG_ERR, ("[%s] reset to %s node: %s",
    	    b->name, NG_PRED1_NODE_TYPE, strerror(errno)));
    }
#endif
return(NULL);
}

/*
 * Pred1SendResetReq()
 */

static Mbuf
Pred1SendResetReq(Bund b)
{
#ifndef USE_NG_PRED1
    Pred1Init(b, COMP_DIR_RECV);
#endif
    return(NULL);
}

/*
 * Pred1RecvResetAck()
 */

static void
Pred1RecvResetAck(Bund b, int id, Mbuf bp)
{
#ifndef USE_NG_PRED1
    Pred1Init(b, COMP_DIR_RECV);
#else
    char		path[NG_PATHSIZ];
    /* Forward ResetReq to the Predictor1 decompression node */
    snprintf(path, sizeof(path), "[%x]:", b->ccp.decomp_node_id);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_PRED1_COOKIE, NGM_PRED1_RESETREQ, NULL, 0) < 0) {
	Log(LG_ERR, ("[%s] reset to %s node: %s",
    	    b->name, NG_PRED1_NODE_TYPE, strerror(errno)));
    }
#endif
}

/*
 * Pred1BuildConfigReq()
 */

static u_char *
Pred1BuildConfigReq(Bund b, u_char *cp, int *ok)
{
  cp = FsmConfValue(cp, CCP_TY_PRED1, 0, NULL);
  *ok = 1;
  return (cp);
}

/*
 * Pred1DecodeConfigReq()
 */

static void
Pred1DecodeConfigReq(Fsm fp, FsmOption opt, int mode)
{
  /* Deal with it */
  switch (mode) {
    case MODE_REQ:
	FsmAck(fp, opt);
      break;

    case MODE_NAK:
      break;
  }
}

/*
 * Pred1Negotiated()
 */

static int
Pred1Negotiated(Bund b, int dir)
{
  return 1;
}

/*
 * Pred1SubtractBloat()
 */

static int
Pred1SubtractBloat(Bund b, int size)
{
  return(size - 4);
}

static int
Pred1Stat(Context ctx, int dir) 
{
#ifndef USE_NG_PRED1
    Pred1Info	p = &ctx->bund->ccp.pred1;
    
    switch (dir) {
	case COMP_DIR_XMIT:
	    Printf("\tBytes\t: %llu -> %llu (%+lld%%)\r\n",
		(unsigned long long)p->xmit_stats.InOctets,
		(unsigned long long)p->xmit_stats.OutOctets,
		((p->xmit_stats.InOctets!=0)?
		    ((long long)(p->xmit_stats.OutOctets - p->xmit_stats.InOctets)
			*100/(long long)p->xmit_stats.InOctets):
		    0));
	    Printf("\tFrames\t: %llu -> %lluc + %lluu\r\n",
		(unsigned long long)p->xmit_stats.FramesPlain,
		(unsigned long long)p->xmit_stats.FramesComp,
		(unsigned long long)p->xmit_stats.FramesUncomp);
	    Printf("\tErrors\t: %llu\r\n",
		(unsigned long long)p->recv_stats.Errors);
	    break;
	case COMP_DIR_RECV:
	    Printf("\tBytes\t: %llu <- %llu (%+lld%%)\r\n",
		(unsigned long long)p->recv_stats.OutOctets,
		(unsigned long long)p->recv_stats.InOctets,
		((p->recv_stats.OutOctets!=0)?
		    ((long long)(p->recv_stats.InOctets - p->recv_stats.OutOctets)
			*100/(long long)p->recv_stats.OutOctets):
		    0));
	    Printf("\tFrames\t: %llu <- %lluc + %lluu\r\n",
		(unsigned long long)p->xmit_stats.FramesPlain,
		(unsigned long long)p->xmit_stats.FramesComp,
		(unsigned long long)p->xmit_stats.FramesUncomp);
	    Printf("\tErrors\t: %llu\r\n",
		(unsigned long long)p->recv_stats.Errors);
    	    break;
	default:
    	    assert(0);
    }
    return (0);
#else
    Bund			b = ctx->bund;
    char			path[NG_PATHSIZ];
    struct ng_pred1_stats	stats;
    union {
	u_char			buf[sizeof(struct ng_mesg) + sizeof(stats)];
	struct ng_mesg		reply;
    }				u;

    switch (dir) {
	case COMP_DIR_XMIT:
	    snprintf(path, sizeof(path), "mpd%d-%s:%s", gPid, b->name,
		NG_PPP_HOOK_COMPRESS);
	    break;
	case COMP_DIR_RECV:
	    snprintf(path, sizeof(path), "mpd%d-%s:%s", gPid, b->name,
		NG_PPP_HOOK_DECOMPRESS);
	    break;
	default:
	    assert(0);
    }
    if (NgFuncSendQuery(path, NGM_PRED1_COOKIE, NGM_PRED1_GET_STATS, NULL, 0, 
	&u.reply, sizeof(u), NULL) < 0) {
	    Log(LG_ERR, ("[%s] can't get %s stats: %s",
		b->name, NG_PRED1_NODE_TYPE, strerror(errno)));
	    return(0);
    }
    memcpy(&stats, u.reply.data, sizeof(stats));
    switch (dir) {
	case COMP_DIR_XMIT:
	    Printf("\tBytes\t: %llu -> %llu (%+lld%%)\r\n",
		stats.InOctets,
		stats.OutOctets,
		((stats.InOctets!=0)?
		    ((int64_t)(stats.OutOctets - stats.InOctets)*100/(int64_t)stats.InOctets):
		    0));
	    Printf("\tFrames\t: %llu -> %lluc + %lluu\r\n",
		stats.FramesPlain,
		stats.FramesComp,
		stats.FramesUncomp);
	    Printf("\tErrors\t: %llu\r\n",
		stats.Errors);
	    break;
	case COMP_DIR_RECV:
	    Printf("\tBytes\t: %llu <- %llu (%+lld%%)\r\n",
		stats.OutOctets,
		stats.InOctets,
		((stats.OutOctets!=0)?
		    ((int64_t)(stats.InOctets - stats.OutOctets)*100/(int64_t)stats.OutOctets):
		    0));
	    Printf("\tFrames\t: %llu <- %lluc + %lluu\r\n",
		stats.FramesPlain,
		stats.FramesComp,
		stats.FramesUncomp);
	    Printf("\tErrors\t: %llu\r\n",
		stats.Errors);
    	    break;
	default:
    	    assert(0);
    }
    return (0);
#endif
}

#ifndef USE_NG_PRED1
/*
 * Compress()
 */

static int
Compress(Bund b, u_char *source, u_char *dest, int len)
{
  Pred1Info	p = &b->ccp.pred1;
  int		i, bitmask;
  u_char	flags;
  u_char	*flagdest, *orgdest;

  orgdest = dest;
  while (len)
  {
    flagdest = dest++; flags = 0;   /* All guess wrong initially */
    for (bitmask=1, i=0; i < 8 && len; i++, bitmask <<= 1) {
      if (p->OutputGuessTable[p->oHash] == *source)
	flags |= bitmask;       /* Guess was right - don't output */
      else
      {
	p->OutputGuessTable[p->oHash] = *source;
	*dest++ = *source;      /* Guess wrong, output char */
      }
      OHASH(*source++);
      len--;
    }
    *flagdest = flags;
  }
  return(dest - orgdest);
}

/*
 * Decompress()
 *
 * Returns decompressed size, or -1 if we ran out of space
 */

static int
Decompress(Bund b, u_char *source, u_char *dest, int slen, int dlen)
{
  Pred1Info	p = &b->ccp.pred1;
  int		i, bitmask;
  u_char	flags, *orgdest;

  orgdest = dest;
  while (slen)
  {
    flags = *source++;
    slen--;
    for (i=0, bitmask = 1; i < 8; i++, bitmask <<= 1)
    {
      if (dlen <= 0)
	return(-1);
      if (flags & bitmask)
	*dest = p->InputGuessTable[p->iHash];		/* Guess correct */
      else
      {
	if (!slen)
	  break;			/* we seem to be really done -- cabo */
	p->InputGuessTable[p->iHash] = *source;		/* Guess wrong */
	*dest = *source++;				/* Read from source */
	slen--;
      }
      IHASH(*dest++);
      dlen--;
    }
  }
  return(dest - orgdest);
}

/*
 * SyncTable()
 */

static void
SyncTable(Bund b, u_char *source, u_char *dest, int len)
{
  Pred1Info	p = &b->ccp.pred1;

  while (len--)
  {
    if (p->InputGuessTable[p->iHash] != *source)
      p->InputGuessTable[p->iHash] = *source;
    IHASH(*dest++ = *source++);
  }
}
#endif
