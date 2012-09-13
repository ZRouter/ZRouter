
/*
 * ccp_deflate.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "defs.h"

#ifdef CCP_DEFLATE

#include "ppp.h"
#include "ccp.h"
#include "util.h"
#include "ngfunc.h"

#include <netgraph/ng_message.h>
#include <netgraph.h>

/*
 * INTERNAL FUNCTIONS
 */

  static int	DeflateInit(Bund b, int direction);
  static int	DeflateConfigure(Bund b);
  static char   *DeflateDescribe(Bund b, int xmit, char *buf, size_t len);
  static void	DeflateCleanup(Bund b, int direction);

  static u_char	*DeflateBuildConfigReq(Bund b, u_char *cp, int *ok);
  static void   DeflateDecodeConfigReq(Fsm fp, FsmOption opt, int mode);
  static Mbuf	DeflateRecvResetReq(Bund b, int id, Mbuf bp, int *noAck);
  static Mbuf	DeflateSendResetReq(Bund b);
  static void	DeflateRecvResetAck(Bund b, int id, Mbuf bp);
  static int    DeflateNegotiated(Bund b, int xmit);
  static int    DeflateSubtractBloat(Bund b, int size);
  static int	DeflateStat(Context ctx, int dir);

/*
 * GLOBAL VARIABLES
 */

  const struct comptype	gCompDeflateInfo =
  {
    "deflate",
    CCP_TY_DEFLATE,
    2,
    DeflateInit,
    DeflateConfigure,
    NULL,
    DeflateDescribe,
    DeflateSubtractBloat,
    DeflateCleanup,
    DeflateBuildConfigReq,
    DeflateDecodeConfigReq,
    DeflateSendResetReq,
    DeflateRecvResetReq,
    DeflateRecvResetAck,
    DeflateNegotiated,
    DeflateStat,
    NULL,
    NULL,
  };

/*
 * DeflateInit()
 */

static int
DeflateInit(Bund b, int dir)
{
    DeflateInfo		const deflate = &b->ccp.deflate;
    struct ng_deflate_config	conf;
    struct ngm_mkpeer	mp;
    char		path[NG_PATHSIZ];
    const char		*deflatehook, *ppphook;
    ng_ID_t		id;

    /* Initialize configuration structure */
    memset(&conf, 0, sizeof(conf));
    conf.enable = 1;
    if (dir == COMP_DIR_XMIT) {
	ppphook = NG_PPP_HOOK_COMPRESS;
    	deflatehook = NG_DEFLATE_HOOK_COMP;
    	conf.windowBits = deflate->xmit_windowBits;
    } else {
        ppphook = NG_PPP_HOOK_DECOMPRESS;
        deflatehook = NG_DEFLATE_HOOK_DECOMP;
        conf.windowBits = deflate->recv_windowBits;
    }

    /* Attach a new DEFLATE node to the PPP node */
    snprintf(path, sizeof(path), "[%x]:", b->nodeID);
    strcpy(mp.type, NG_DEFLATE_NODE_TYPE);
    strcpy(mp.ourhook, ppphook);
    strcpy(mp.peerhook, deflatehook);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_GENERIC_COOKIE, NGM_MKPEER, &mp, sizeof(mp)) < 0) {
	Perror("[%s] can't create %s node", b->name, mp.type);
	return(-1);
    }

    strlcat(path, ppphook, sizeof(path));

    id = NgGetNodeID(-1, path);
    if (dir == COMP_DIR_XMIT) {
	b->ccp.comp_node_id = id;
    } else {
	b->ccp.decomp_node_id = id;
    }

    /* Configure DEFLATE node */
    snprintf(path, sizeof(path), "[%x]:", id);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_DEFLATE_COOKIE, NGM_DEFLATE_CONFIG, &conf, sizeof(conf)) < 0) {
	Perror("[%s] can't config %s node at %s",
    	    b->name, NG_DEFLATE_NODE_TYPE, path);
	NgFuncShutdownNode(gCcpCsock, b->name, path);
	return(-1);
    }

    return 0;
}

/*
 * DeflateConfigure()
 */

static int
DeflateConfigure(Bund b)
{
    CcpState	const ccp = &b->ccp;
    DeflateInfo	const deflate = &ccp->deflate;
  
    deflate->xmit_windowBits=15;
    deflate->recv_windowBits=0;
  
    return(0);
}

/*
 * DeflateCleanup()
 */

static char *
DeflateDescribe(Bund b, int dir, char *buf, size_t len)
{
    CcpState	const ccp = &b->ccp;
    DeflateInfo	const deflate = &ccp->deflate;

    switch (dir) {
	case COMP_DIR_XMIT:
	    snprintf(buf, len, "win %d", deflate->xmit_windowBits);
	    break;
	case COMP_DIR_RECV:
	    snprintf(buf, len, "win %d", deflate->recv_windowBits);
	    break;
	default:
    	    assert(0);
    	    return(NULL);
    }
    return (buf);
}

/*
 * DeflateCleanup()
 */

void
DeflateCleanup(Bund b, int dir)
{
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
}

/*
 * DeflateRecvResetReq()
 */

static Mbuf
DeflateRecvResetReq(Bund b, int id, Mbuf bp, int *noAck)
{
    char		path[NG_PATHSIZ];
    /* Forward ResetReq to the DEFLATE compression node */
    snprintf(path, sizeof(path), "[%x]:", b->ccp.comp_node_id);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_DEFLATE_COOKIE, NGM_DEFLATE_RESETREQ, NULL, 0) < 0) {
	Perror("[%s] reset-req to %s node", b->name, NG_DEFLATE_NODE_TYPE);
    }
    return(NULL);
}

/*
 * DeflateSendResetReq()
 */

static Mbuf
DeflateSendResetReq(Bund b)
{
  return(NULL);
}

/*
 * DeflateRecvResetAck()
 */

static void
DeflateRecvResetAck(Bund b, int id, Mbuf bp)
{
    char		path[NG_PATHSIZ];
    /* Forward ResetReq to the DEFLATE compression node */
    snprintf(path, sizeof(path), "[%x]:", b->ccp.decomp_node_id);
    if (NgSendMsg(gCcpCsock, path,
    	    NGM_DEFLATE_COOKIE, NGM_DEFLATE_RESETREQ, NULL, 0) < 0) {
	Perror("[%s] reset-ack to %s node", b->name, NG_DEFLATE_NODE_TYPE);
    }
}

/*
 * DeflateBuildConfigReq()
 */

static u_char *
DeflateBuildConfigReq(Bund b, u_char *cp, int *ok)
{
  CcpState	const ccp = &b->ccp;
  DeflateInfo	const deflate = &ccp->deflate;
  u_int16_t	opt;
  
  if (deflate->xmit_windowBits > 0) {
    opt = ((deflate->xmit_windowBits-8)<<12) + (8<<8) + (0<<2) + 0;
  
    cp = FsmConfValue(cp, CCP_TY_DEFLATE, -2, &opt);
    *ok = 1;
  }
  return (cp);
}

/*
 * DeflateDecodeConfigReq()
 */

static void
DeflateDecodeConfigReq(Fsm fp, FsmOption opt, int mode)
{
    Bund 	b = (Bund)fp->arg;
  CcpState	const ccp = &b->ccp;
  DeflateInfo	const deflate = &ccp->deflate;
  u_int16_t     o;
  u_char	window, method, chk;

  /* Sanity check */
  if (opt->len != 4) {
    Log(LG_CCP, ("[%s]     bogus length %d", b->name, opt->len));
    if (mode == MODE_REQ)
      FsmRej(fp, opt);
    return;
  }

  /* Get bits */
  memcpy(&o, opt->data, 2);
  o = ntohs(o);
  window = (o>>12)&0x000F;
  method = (o>>8)&0x000F;
  chk = o&0x0003;

  /* Display it */
  Log(LG_CCP, ("[%s]     0x%04x: w:%d, m:%d, c:%d", b->name, o, window, method, chk));

  /* Deal with it */
  switch (mode) {
    case MODE_REQ:
	if ((window > 0) && (window<=7) && (method == 8) && (chk == 0)) {
	    deflate->recv_windowBits = window + 8;
	    FsmAck(fp, opt);
	} else {
	    o = htons((7<<12) + (8<<8) + (0<<2) + 0);
	    memcpy(opt->data, &o, 2);
	    FsmNak(fp, opt);
	}
      break;

    case MODE_NAK:
	if ((window > 0) && (window<=7) && (method == 8) && (chk == 0))
	    deflate->xmit_windowBits = window + 8;
	else {
	    deflate->xmit_windowBits = 0;
	}
      break;
  }
}

/*
 * DeflateNegotiated()
 */

static int
DeflateNegotiated(Bund b, int dir)
{
  return 1;
}

/*
 * DeflateSubtractBloat()
 */

static int
DeflateSubtractBloat(Bund b, int size)
{
  return(size + CCP_OVERHEAD);  /* Compression compensate header size */
}

static int
DeflateStat(Context ctx, int dir) 
{
    Bund			b = ctx->bund;
    char			path[NG_PATHSIZ];
    struct ng_deflate_stats	stats;
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
    if (NgFuncSendQuery(path, NGM_DEFLATE_COOKIE, NGM_DEFLATE_GET_STATS, NULL, 0, 
	&u.reply, sizeof(u), NULL) < 0) {
	    Perror("[%s] can't get %s stats", b->name, NG_DEFLATE_NODE_TYPE);
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
}

#endif /* CCP_DEFLATE */
