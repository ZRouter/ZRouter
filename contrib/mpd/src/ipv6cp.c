
/*
 * ipv6cp.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "ipv6cp.h"
#include "fsm.h"
#include "ip.h"
#include "iface.h"
#include "msg.h"
#include "ngfunc.h"
#include "util.h"

#include <netgraph.h>
#include <sys/mbuf.h>

/*
 * DEFINITIONS
 */

  #define IPV6CP_KNOWN_CODES	(   (1 << CODE_CONFIGREQ)	\
				  | (1 << CODE_CONFIGACK)	\
				  | (1 << CODE_CONFIGNAK)	\
				  | (1 << CODE_CONFIGREJ)	\
				  | (1 << CODE_TERMREQ)		\
				  | (1 << CODE_TERMACK)		\
				  | (1 << CODE_CODEREJ)		)

  #define TY_INTIDENT		1
  #define TY_COMPPROTO		2

  #define IPV6CP_REJECTED(p,x)	((p)->peer_reject & (1<<(x)))
  #define IPV6CP_PEER_REJ(p,x)	do{(p)->peer_reject |= (1<<(x));}while(0)

  #define IPV6CP_VJCOMP_MIN_MAXCHAN	(NG_VJC_MIN_CHANNELS - 1)
  #define IPV6CP_VJCOMP_MAX_MAXCHAN	(NG_VJC_MAX_CHANNELS - 1)
  #define IPV6CP_VJCOMP_DEFAULT_MAXCHAN	IPV6CP_VJCOMP_MAX_MAXCHAN

  /* Set menu options */
  enum {
    SET_ENABLE,
    SET_DISABLE,
    SET_ACCEPT,
    SET_DENY,
    SET_YES,
    SET_NO
  };

/*
 * INTERNAL FUNCTIONS
 */

  static void	Ipv6cpConfigure(Fsm fp);
  static void	Ipv6cpUnConfigure(Fsm fp);

  static u_char	*Ipv6cpBuildConfigReq(Fsm fp, u_char *cp);
  static void	Ipv6cpDecodeConfig(Fsm fp, FsmOption list, int num, int mode);
  static void	Ipv6cpLayerStart(Fsm fp);
  static void	Ipv6cpLayerFinish(Fsm fp);
  static void	Ipv6cpLayerUp(Fsm fp);
  static void	Ipv6cpLayerDown(Fsm fp);
  static void	Ipv6cpFailure(Fsm fp, enum fsmfail reason);

  static int	Ipv6cpSetCommand(Context ctx, int ac, char *av[], void *arg);

  void 		CreateInterfaceID(u_char *intid, int random);
/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab Ipv6cpSetCmds[] = {
    { "enable [opt ...]",		"Enable option",
	Ipv6cpSetCommand, NULL, 2, (void *) SET_ENABLE},
    { "disable [opt ...]",		"Disable option",
	Ipv6cpSetCommand, NULL, 2, (void *) SET_DISABLE},
    { "accept [opt ...]",		"Accept option",
	Ipv6cpSetCommand, NULL, 2, (void *) SET_ACCEPT},
    { "deny [opt ...]",			"Deny option",
	Ipv6cpSetCommand, NULL, 2, (void *) SET_DENY},
    { "yes [opt ...]",			"Enable and accept option",
	Ipv6cpSetCommand, NULL, 2, (void *) SET_YES},
    { "no [opt ...]",			"Disable and deny option",
	Ipv6cpSetCommand, NULL, 2, (void *) SET_NO},
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static const struct fsmoptinfo	gIpv6cpConfOpts[] = {
    { "INTIDENT",	TY_INTIDENT,		8, 8, TRUE },
    { "COMPPROTO",	TY_COMPPROTO,		4, 4, FALSE },
    { NULL }
  };

  static const struct confinfo gConfList[] = {
/*    { 1,	IPV6CP_CONF_VJCOMP,	"vjcomp"	},*/
    { 0,	0,			NULL		},
  };

  static const struct fsmtype gIpv6cpFsmType = {
    "IPV6CP",
    PROTO_IPV6CP,
    IPV6CP_KNOWN_CODES,
    FALSE,
    LG_IPV6CP, LG_IPV6CP2,
    NULL,
    Ipv6cpLayerUp,
    Ipv6cpLayerDown,
    Ipv6cpLayerStart,
    Ipv6cpLayerFinish,
    Ipv6cpBuildConfigReq,
    Ipv6cpDecodeConfig,
    Ipv6cpConfigure,
    Ipv6cpUnConfigure,
    NULL,
    NULL,
    NULL,
    NULL,
    Ipv6cpFailure,
    NULL,
    NULL,
    NULL,
  };

/*
 * Ipv6cpStat()
 */

int
Ipv6cpStat(Context ctx, int ac, char *av[], void *arg)
{
  Ipv6cpState		const ipv6cp = &ctx->bund->ipv6cp;
  Fsm			fp = &ipv6cp->fsm;

  Printf("[%s] %s [%s]\r\n", Pref(fp), Fsm(fp), FsmStateName(fp->state));
  Printf("Interface identificators:\r\n");
  Printf("\tSelf: %02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
    ipv6cp->myintid[0], ipv6cp->myintid[1], ipv6cp->myintid[2], ipv6cp->myintid[3],
    ipv6cp->myintid[4], ipv6cp->myintid[5], ipv6cp->myintid[6], ipv6cp->myintid[7]);
  Printf("\tPeer: %02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
    ipv6cp->hisintid[0], ipv6cp->hisintid[1], ipv6cp->hisintid[2], ipv6cp->hisintid[3],
    ipv6cp->hisintid[4], ipv6cp->hisintid[5], ipv6cp->hisintid[6], ipv6cp->hisintid[7]);
  Printf("IPV6CP Options:\r\n");
  OptStat(ctx, &ipv6cp->conf.options, gConfList);

  return(0);
}

/*
 * CreateInterfaceID()
 */

void
CreateInterfaceID(u_char *intid, int r)
{
    struct sockaddr_dl hwaddr;
    u_char	*ether;

    if (!r) {
	if (!GetEther(NULL, &hwaddr)) {
	    ether = (u_char *) LLADDR(&hwaddr);
	    intid[0]=ether[0] ^ 0x02; /* reverse the u/l bit*/
	    intid[1]=ether[1];
	    intid[2]=ether[2];
	    intid[3]=0xff;
	    intid[4]=0xfe;
	    intid[5]=ether[3];
	    intid[6]=ether[4];
	    intid[7]=ether[5];
	    return;
	}
    }

    srandomdev();
    ((u_int32_t*)intid)[0]=(((u_int32_t)random()) % 0xFFFFFFFF) + 1;
    ((u_int32_t*)intid)[1]=(((u_int32_t)random()) % 0xFFFFFFFF) + 1;
    intid[0] &= 0xfd;

}

/*
 * Ipv6cpInit()
 */

void
Ipv6cpInit(Bund b)
{
  Ipv6cpState	ipv6cp = &b->ipv6cp;

  /* Init state machine */
  memset(ipv6cp, 0, sizeof(*ipv6cp));
  FsmInit(&ipv6cp->fsm, &gIpv6cpFsmType, b);

  CreateInterfaceID(ipv6cp->myintid,0);

}

/*
 * Ipv6cpInst()
 */

void
Ipv6cpInst(Bund b, Bund bt)
{
  Ipv6cpState	ipv6cp = &b->ipv6cp;

  /* Init state machine */
  memcpy(ipv6cp, &bt->ipv6cp, sizeof(*ipv6cp));
  FsmInst(&ipv6cp->fsm, &bt->ipv6cp.fsm, b);
}

/*
 * Ipv6cpConfigure()
 */

static void
Ipv6cpConfigure(Fsm fp)
{
    Bund 	b = (Bund)fp->arg;
  Ipv6cpState	const ipv6cp = &b->ipv6cp;

  /* FSM stuff */
  ipv6cp->peer_reject = 0;

}

/*
 * Ipv6cpUnConfigure()
 */

static void
Ipv6cpUnConfigure(Fsm fp)
{
}

/*
 * Ipv6cpBuildConfigReq()
 */

static u_char *
Ipv6cpBuildConfigReq(Fsm fp, u_char *cp)
{
    Bund 	b = (Bund)fp->arg;
  Ipv6cpState	const ipv6cp = &b->ipv6cp;

  cp = FsmConfValue(cp, TY_INTIDENT, 8, ipv6cp->myintid);

/* Done */

  return(cp);
}

/*
 * Ipv6cpLayerStart()
 *
 * Tell the lower layer (the bundle) that we need it
 */

static void
Ipv6cpLayerStart(Fsm fp)
{
    BundNcpsStart((Bund)(fp->arg), NCP_IPV6CP);
}

/*
 * Ipv6cpLayerFinish()
 *
 * Tell the lower layer (the bundle) that we no longer need it
 */

static void
Ipv6cpLayerFinish(Fsm fp)
{
    BundNcpsFinish((Bund)(fp->arg), NCP_IPV6CP);
}

/*
 * Ipv6cpLayerUp()
 *
 * Called when IPV6CP has reached the OPEN state
 */

static void
Ipv6cpLayerUp(Fsm fp)
{
    Bund 	b = (Bund)fp->arg;
    Ipv6cpState		const ipv6cp = &b->ipv6cp;

    /* Report */
    Log(fp->log, ("[%s]   %02x%02x:%02x%02x:%02x%02x:%02x%02x -> %02x%02x:%02x%02x:%02x%02x:%02x%02x", b->name,
	ipv6cp->myintid[0], ipv6cp->myintid[1], ipv6cp->myintid[2], ipv6cp->myintid[3],
	ipv6cp->myintid[4], ipv6cp->myintid[5], ipv6cp->myintid[6], ipv6cp->myintid[7],
	ipv6cp->hisintid[0], ipv6cp->hisintid[1], ipv6cp->hisintid[2], ipv6cp->hisintid[3],
	ipv6cp->hisintid[4], ipv6cp->hisintid[5], ipv6cp->hisintid[6], ipv6cp->hisintid[7]));

    /* Enable IP packets in the PPP node */
    b->pppConfig.bund.enableIPv6 = 1;
    NgFuncSetConfig(b);

    BundNcpsJoin(b, NCP_IPV6CP);
}

/*
 * Ipv6cpLayerDown()
 *
 * Called when IPV6CP leaves the OPEN state
 */

static void
Ipv6cpLayerDown(Fsm fp)
{
    Bund 	b = (Bund)fp->arg;

    BundNcpsLeave(b, NCP_IPV6CP);

    /* Turn off IP packets */
    b->pppConfig.bund.enableIPv6 = 0;
    NgFuncSetConfig(b);
}

/*
 * Ipv6cpUp()
 */

void
Ipv6cpUp(Bund b)
{
  FsmUp(&b->ipv6cp.fsm);
}

/*
 * Ipv6cpDown()
 */

void
Ipv6cpDown(Bund b)
{
  FsmDown(&b->ipv6cp.fsm);
}

/*
 * Ipv6cpOpen()
 */

void
Ipv6cpOpen(Bund b)
{
  FsmOpen(&b->ipv6cp.fsm);
}

/*
 * Ipv6cpClose()
 */

void
Ipv6cpClose(Bund b)
{
  FsmClose(&b->ipv6cp.fsm);
}

/*
 * Ipv6cpOpenCmd()
 */

int
Ipv6cpOpenCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to open template");
    FsmOpen(&ctx->bund->ipv6cp.fsm);
    return (0);
}

/*
 * Ipv6cpCloseCmd()
 */

int
Ipv6cpCloseCmd(Context ctx)
{
    if (ctx->bund->tmpl)
	Error("impossible to close template");
    FsmClose(&ctx->bund->ipv6cp.fsm);
    return (0);
}

/*
 * Ipv6cpFailure()
 */

static void
Ipv6cpFailure(Fsm fp, enum fsmfail reason)
{
    Bund 	b = (Bund)fp->arg;
    RecordLinkUpDownReason(b, NULL, 0, STR_PROTO_ERR, STR_IPV6CP_FAILED, FsmFailureStr(reason));
}

/*
 * Ipv6cpDecodeConfig()
 */

static void
Ipv6cpDecodeConfig(Fsm fp, FsmOption list, int num, int mode)
{
    Bund 	b = (Bund)fp->arg;
  Ipv6cpState		const ipv6cp = &b->ipv6cp;
  int			k;

  /* Decode each config option */
  for (k = 0; k < num; k++) {
    FsmOption	const opt = &list[k];
    FsmOptInfo	const oi = FsmFindOptInfo(gIpv6cpConfOpts, opt->type);

    if (!oi) {
      Log(LG_IPV6CP, ("[%s]   UNKNOWN[%d] len=%d", b->name, opt->type, opt->len));
      if (mode == MODE_REQ)
	FsmRej(fp, opt);
      continue;
    }
    if (!oi->supported) {
      Log(LG_IPV6CP, ("[%s]   %s", b->name, oi->name));
      if (mode == MODE_REQ) {
	Log(LG_IPV6CP, ("[%s]     Not supported", b->name));
	FsmRej(fp, opt);
      }
      continue;
    }
    if (opt->len < oi->minLen + 2 || opt->len > oi->maxLen + 2) {
      Log(LG_IPV6CP, ("[%s]   %s", b->name, oi->name));
      if (mode == MODE_REQ) {
	Log(LG_IPV6CP, ("[%s]     bogus len=%d min=%d max=%d", b->name, opt->len, oi->minLen + 2, oi->maxLen + 2));
	FsmRej(fp, opt);
      }
      continue;
    }
    switch (opt->type) {
      case TY_INTIDENT:
	{
	  Log(LG_IPV6CP2, ("[%s]   %s %02x%02x:%02x%02x:%02x%02x:%02x%02x", b->name, oi->name,
	    opt->data[0], opt->data[1], opt->data[2], opt->data[3],
	    opt->data[4], opt->data[5], opt->data[6], opt->data[7]));
	  switch (mode) {
	    case MODE_REQ:
	      if ((((u_int32_t*)opt->data)[0]==0) && (((u_int32_t*)opt->data)[1]==0)) {
		Log(LG_IPV6CP2, ("[%s]     Empty INTIDENT, propose our.", b->name));
		CreateInterfaceID(ipv6cp->hisintid, 1);
	        memcpy(opt->data, ipv6cp->hisintid, 8);
	        FsmNak(fp, opt);
	      } else if (bcmp(opt->data, ipv6cp->myintid, 8) == 0) {
		Log(LG_IPV6CP2, ("[%s]     Duplicate INTIDENT, generate and propose other.", b->name));
		CreateInterfaceID(ipv6cp->hisintid, 1);
	        memcpy(opt->data, ipv6cp->hisintid, 8);
	        FsmNak(fp, opt);
	      } else {
		Log(LG_IPV6CP2, ("[%s]     It's OK.", b->name));
	        memcpy(ipv6cp->hisintid, opt->data, 8);
	        FsmAck(fp, opt);
	      }
	      break;
	    case MODE_NAK:
		Log(LG_IPV6CP2, ("[%s]     I agree to get this to myself.", b->name));
	        memcpy(ipv6cp->myintid, opt->data, 8);
	      break;
	    case MODE_REJ:
	      IPV6CP_PEER_REJ(ipv6cp, opt->type);
	      break;
	  }
	}
	break;

      default:
	assert(0);
    }
  }
}

/*
 * Ipv6cpInput()
 *
 * Deal with an incoming IPV6CP packet
 */

void
Ipv6cpInput(Bund b, Mbuf bp)
{
  FsmInput(&b->ipv6cp.fsm, bp);
}

/*
 * Ipv6cpSetCommand()
 */

static int
Ipv6cpSetCommand(Context ctx, int ac, char *av[], void *arg)
{
  Ipv6cpState		const ipv6cp = &ctx->bund->ipv6cp;

  if (ac == 0)
    return(-1);
  switch ((intptr_t)arg) {
    case SET_ACCEPT:
      AcceptCommand(ac, av, &ipv6cp->conf.options, gConfList);
      break;

    case SET_DENY:
      DenyCommand(ac, av, &ipv6cp->conf.options, gConfList);
      break;

    case SET_ENABLE:
      EnableCommand(ac, av, &ipv6cp->conf.options, gConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &ipv6cp->conf.options, gConfList);
      break;

    case SET_YES:
      YesCommand(ac, av, &ipv6cp->conf.options, gConfList);
      break;

    case SET_NO:
      NoCommand(ac, av, &ipv6cp->conf.options, gConfList);
      break;

    default:
      assert(0);
  }
  return(0);
}

