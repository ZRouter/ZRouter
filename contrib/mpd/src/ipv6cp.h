
/*
 * ipv6cp.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _IPV6CP_H_
#define _IPV6CP_H_

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include "command.h"

/*
 * DEFINITONS
 */
 
   /* Configuration options */
/*  enum {
  };*/

  struct ipv6cpconf {
    struct optinfo	options;	/* Configuraion options */
  };
  typedef struct ipv6cpconf	*Ipv6cpConf;

  struct ipv6cpstate {
    struct ipv6cpconf	conf;		/* Configuration */

    u_char 		myintid[8];
    u_char 		hisintid[8];

    uint32_t		peer_reject;	/* Request codes rejected by peer */

    struct fsm		fsm;
  };
  typedef struct ipv6cpstate	*Ipv6cpState;

/*
 * VARIABLES
 */

  extern const struct cmdtab	Ipv6cpSetCmds[];

/*
 * FUNCTIONS
 */

  extern void	Ipv6cpInit(Bund b);
  extern void	Ipv6cpInst(Bund b, Bund bt);
  extern void	Ipv6cpUp(Bund b);
  extern void	Ipv6cpDown(Bund b);
  extern void	Ipv6cpOpen(Bund b);
  extern void	Ipv6cpClose(Bund b);
  extern int	Ipv6cpOpenCmd(Context ctx);
  extern int	Ipv6cpCloseCmd(Context ctx);
  extern void	Ipv6cpInput(Bund b, Mbuf bp);
  extern void	Ipv6cpDefAddress(void);
  extern int	Ipv6cpStat(Context ctx, int ac, char *av[], void *arg);

#endif


