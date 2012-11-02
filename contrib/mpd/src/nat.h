
/*
 * nat.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _NAT_H_
#define _NAT_H_

#include <netgraph/ng_nat.h>

#ifdef NG_NAT_DESC_LENGTH
/* max. number of red-port rules */
#define NM_PORT		16
/* max. number of red-addr rules */
#define NM_ADDR		8
/* max. number of red-proto rules */
#define NM_PROTO	8
#endif

/* Configuration options */

  enum {
    NAT_CONF_LOG,
    NAT_CONF_INCOMING,
    NAT_CONF_SAME_PORTS,
    NAT_CONF_UNREG_ONLY
  };

  struct natstate {
    struct optinfo	options;		/* Configuration options */
    struct u_addr	alias_addr;		/* Alias IP address */
    struct u_addr	target_addr;		/* Target IP address */
#ifdef NG_NAT_DESC_LENGTH
    struct ng_nat_redirect_port	nrpt[NM_PORT];	/* NAT redirect port */
    int nrpt_id[NM_PORT];			/* NAT redirect port ID's */
    struct ng_nat_redirect_addr nrad[NM_ADDR];	/* NAT redirect address */
    int nrad_id[NM_ADDR];			/* NAT redirect address ID's */
    struct ng_nat_redirect_proto nrpr[NM_PROTO];/* NAT redirect proto */
    int nrpr_id[NM_PROTO];			/* NAT redirect proto ID's */
#endif
  };
  typedef struct natstate	*NatState;

/*
 * VARIABLES
 */

  extern const struct cmdtab	NatSetCmds[];

  extern void	NatInit(Bund b);
  extern int	NatStat(Context ctx, int ac, char *av[], void *arg);

#endif

