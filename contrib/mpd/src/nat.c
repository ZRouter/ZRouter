
/*
 * nat.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "nat.h"
#include "iface.h"
#include "netgraph.h"
#include "util.h"

/*
 * DEFINITIONS
 */

/* Set menu options */

  enum {
    SET_ADDR,
    SET_TARGET,
    SET_ENABLE,
    SET_DISABLE,
    SET_REDIRECT_PORT,
    SET_REDIRECT_ADDR,
    SET_REDIRECT_PROTO
  };

static int	NatSetCommand(Context ctx, int ac, char *av[], void *arg);
  
/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab NatSetCmds[] = {
    { "address {addr}",		"Set alias address",
	NatSetCommand, AdmitBund, 2, (void *) SET_ADDR },
    { "target {addr}",		"Set target address",
	NatSetCommand, AdmitBund, 2, (void *) SET_TARGET },
#ifdef NG_NAT_DESC_LENGTH
    { "red-port {proto} {alias_addr} {alias_port} {local_addr} {local_port} [{remote_addr} {remote_port}]",	"Redirect port",
	NatSetCommand, AdmitBund, 2, (void *) SET_REDIRECT_PORT },
    { "red-addr {alias_addr} {local_addr}",	"Redirect address",
	NatSetCommand, AdmitBund, 2, (void *) SET_REDIRECT_ADDR },
    { "red-proto {proto} {alias-addr} {local_addr} [{remote-addr}]",	"Redirect protocol",
	NatSetCommand, AdmitBund, 2, (void *) SET_REDIRECT_PROTO },
#endif
    { "enable [opt ...]",		"Enable option",
	NatSetCommand, AdmitBund, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",		"Disable option",
	NatSetCommand, AdmitBund, 2, (void *) SET_DISABLE },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static const struct confinfo	gConfList[] = {
    { 0,	NAT_CONF_LOG,			"log"		},
    { 0,	NAT_CONF_INCOMING,		"incoming"	},
    { 0,	NAT_CONF_SAME_PORTS,		"same-ports"	},
    { 0,	NAT_CONF_UNREG_ONLY,		"unreg-only"	},
    { 0,	0,				NULL		},
  };

/*
 * NatInit()
 */

void
NatInit(Bund b)
{
  NatState	const nat = &b->iface.nat;

  /* Default configuration */
  u_addrclear(&nat->alias_addr);
  u_addrclear(&nat->target_addr);
  Disable(&nat->options, NAT_CONF_LOG);
  Enable(&nat->options, NAT_CONF_INCOMING);
  Enable(&nat->options, NAT_CONF_SAME_PORTS);
  Disable(&nat->options, NAT_CONF_UNREG_ONLY);
#ifdef NG_NAT_DESC_LENGTH
  bzero(nat->nrpt, sizeof(nat->nrpt));
  bzero(nat->nrpt_id, sizeof(nat->nrpt_id));
  bzero(nat->nrad, sizeof(nat->nrad));
  bzero(nat->nrad_id, sizeof(nat->nrad_id));
  bzero(nat->nrpr, sizeof(nat->nrpr));
  bzero(nat->nrpr_id, sizeof(nat->nrpr_id));
#endif
}


/*
 * NatSetCommand()
 */

static int
NatSetCommand(Context ctx, int ac, char *av[], void *arg)
{
  NatState	const nat = &ctx->bund->iface.nat;

  if (ac == 0)
    return(-1);
  switch ((intptr_t)arg) {
    case SET_TARGET:
#ifndef NG_NAT_LOG
	Error("Target address setting is unsupported by current kernel");
#endif
    /* FALL */
    case SET_ADDR:
      {
	struct u_addr	addr;

	/* Parse */
	if (ac != 1)
	  return(-1);
	if (!ParseAddr(av[0], &addr, ALLOW_IPV4))
	  Error("bad IP address \"%s\"", av[0]);

	/* OK */
	if ((intptr_t)arg == SET_ADDR) {
	    nat->alias_addr = addr;
	} else {
	    nat->target_addr = addr;
	}
      }
      break;

#ifdef NG_NAT_DESC_LENGTH
    case SET_REDIRECT_PORT:
      {
	struct protoent	*proto;
	struct in_addr	l_addr, a_addr, r_addr;
	int lp, ap, rp = 0, k;

	/* Parse */
	if (ac != 5 && ac != 7)
	  return(-1);
	if ((proto = getprotobyname(av[0])) == 0)
	  Error("bad PROTO name \"%s\"", av[0]);
	if (!inet_aton (av[1], &a_addr))
	  Error("bad alias IP address \"%s\"", av[1]);
	ap = atoi(av[2]);
	if (ap <= 0 || ap > 65535)
	  Error("Incorrect alias port number \"%s\"", av[2]);
	if (!inet_aton (av[3], &l_addr))
	  Error("bad local IP address \"%s\"", av[3]);
	lp = atoi(av[4]);
	if (lp <= 0 || lp > 65535)
	  Error("Incorrect local port number \"%s\"", av[4]);
	if (ac == 7) {
	  if (!inet_aton (av[5], &r_addr))
	    Error("bad remote IP address \"%s\"", av[5]);
	  rp = atoi(av[6]);
	  if (rp <= 0 || rp > 65535)
	    Error("Incorrect remote port number \"%s\"", av[6]);
	}
	/* OK */
	for (k=0;k<NM_PORT;k++) {
	  if (nat->nrpt_id[k] == 0) {
	    memcpy(&nat->nrpt[k].local_addr, &l_addr, sizeof(struct in_addr));
	    memcpy(&nat->nrpt[k].alias_addr, &a_addr, sizeof(struct in_addr));
	    nat->nrpt[k].local_port = lp;
	    nat->nrpt[k].alias_port = ap;
	    if (ac == 7) {
	      memcpy(&nat->nrpt[k].remote_addr, &r_addr, sizeof(struct in_addr));
	      nat->nrpt[k].remote_port = rp;
	    }
	    nat->nrpt[k].proto = (uint8_t)proto->p_proto;
	    snprintf(nat->nrpt[k].description, NG_NAT_DESC_LENGTH, "nat-port-%d", k);
	    nat->nrpt_id[k] = 1;
	    break;
	  }
	}
	if (k == NM_PORT)
	  Error("max number of redirect-port \"%d\" reached", NM_PORT);
      }
      break;

    case SET_REDIRECT_ADDR:
      {
	struct in_addr	l_addr, a_addr;
	int k;

	/* Parse */
	if (ac != 2)
	  return(-1);
	if (!inet_aton (av[0], &a_addr))
	  Error("bad alias IP address \"%s\"", av[0]);
	if (!inet_aton (av[1], &l_addr))
	  Error("bad local IP address \"%s\"", av[1]);

	/* OK */
	for (k=0;k<NM_ADDR;k++) {
	  if (nat->nrad_id[k] == 0) {
	    memcpy(&nat->nrad[k].local_addr, &l_addr, sizeof(struct in_addr));
	    memcpy(&nat->nrad[k].alias_addr, &a_addr, sizeof(struct in_addr));
	    snprintf(nat->nrad[k].description, NG_NAT_DESC_LENGTH, "nat-addr-%d", k);
	    nat->nrad_id[k] = 1;
	    break;
	  }
	}
	if (k == NM_ADDR)
	  Error("max number of redirect-addr \"%d\" reached", NM_ADDR);
      }
      break;

    case SET_REDIRECT_PROTO:
      {
	struct protoent	*proto;
	struct in_addr	l_addr, a_addr, r_addr;
	int k;

	/* Parse */
	if (ac != 3 && ac != 4)
	  return(-1);
	if ((proto = getprotobyname(av[0])) == 0)
	  Error("bad PROTO name \"%s\"", av[0]);
	if (!inet_aton (av[1], &a_addr))
	  Error("bad alias IP address \"%s\"", av[1]);
	if (!inet_aton (av[2], &l_addr))
	  Error("bad local IP address \"%s\"", av[2]);
	if (ac == 4) {
	  if (!inet_aton (av[3], &r_addr))
	    Error("bad remote IP address \"%s\"", av[3]);
	}

	/* OK */
	for (k=0;k<NM_PROTO;k++) {
	  if (nat->nrpr_id[k] == 0) {
	    memcpy(&nat->nrpr[k].local_addr, &l_addr, sizeof(struct in_addr));
	    memcpy(&nat->nrpr[k].alias_addr, &a_addr, sizeof(struct in_addr));
	    if (ac == 4)
	      memcpy(&nat->nrpr[k].remote_addr, &r_addr, sizeof(struct in_addr));
	    nat->nrpr[k].proto = (uint8_t)proto->p_proto;
	    snprintf(nat->nrpr[k].description, NG_NAT_DESC_LENGTH, "nat-proto-%d", k);
	    nat->nrpr_id[k] = 1;
	    break;
	  }
	}
	if (k == NM_PROTO)
	  Error("max number of redirect-proto \"%d\" reached", NM_PROTO);
      }
      break;
#endif

    case SET_ENABLE:
      EnableCommand(ac, av, &nat->options, gConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &nat->options, gConfList);
      break;

    default:
      assert(0);
  }
  return(0);
}

/*
 * NatStat()
 */

int
NatStat(Context ctx, int ac, char *av[], void *arg)
{
    NatState	const nat = &ctx->bund->iface.nat;
    char	buf[48];
    int k;

    Printf("NAT configuration:\r\n");
    Printf("\tAlias addresses : %s\r\n", 
	u_addrtoa(&nat->alias_addr,buf,sizeof(buf)));
    Printf("\tTarget addresses: %s\r\n", 
	u_addrtoa(&nat->target_addr,buf,sizeof(buf)));
#ifdef NG_NAT_DESC_LENGTH
    Printf("Redirect ports:\r\n");
    for (k=0;k<NM_PORT;k++) {
      if (nat->nrpt_id[k]) {
	struct protoent	*proto;
	char	li[15], ai[15], ri[15];
	inet_ntop(AF_INET, &nat->nrpt[k].local_addr, li, sizeof(li));
	inet_ntop(AF_INET, &nat->nrpt[k].alias_addr, ai, sizeof(ai));
	inet_ntop(AF_INET, &nat->nrpt[k].remote_addr, ri, sizeof(ri));
	proto = getprotobynumber(nat->nrpt[k].proto);
	Printf("\t%s %s:%d %s:%d %s:%d\r\n", proto->p_name,
	    ai, nat->nrpt[k].alias_port, li, nat->nrpt[k].local_port,
	    ri, nat->nrpt[k].remote_port);
      }
    }
    Printf("Redirect address:\r\n");
    for (k=0;k<NM_ADDR;k++) {
      if (nat->nrad_id[k]) {
	char	li[15], ai[15];
	inet_ntop(AF_INET, &nat->nrad[k].local_addr, li, sizeof(li));
	inet_ntop(AF_INET, &nat->nrad[k].alias_addr, ai, sizeof(ai));
	Printf("\t%s %s\r\n", ai, li);
      }
    }
    Printf("Redirect proto:\r\n");
    for (k=0;k<NM_PROTO;k++) {
      if (nat->nrpr_id[k]) {
	struct protoent	*proto;
	char	li[15], ai[15], ri[15];
	proto = getprotobynumber(nat->nrpr[k].proto);
	inet_ntop(AF_INET, &nat->nrpr[k].local_addr, li, sizeof(li));
	inet_ntop(AF_INET, &nat->nrpr[k].alias_addr, ai, sizeof(ai));
	inet_ntop(AF_INET, &nat->nrpr[k].remote_addr, ri, sizeof(ri));
	Printf("\t%s %s %s %s\r\n", proto->p_name, ai, li, ri);
      }
    }
#endif
    Printf("NAT options:\r\n");
    OptStat(ctx, &nat->options, gConfList);
    return(0);
}
