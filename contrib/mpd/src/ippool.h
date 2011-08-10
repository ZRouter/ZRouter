
/*
 * ippool.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _IPPOOL_H_
#define _IPPOOL_H_

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

/*
 * DEFINITIONS
 */

/*
 * VARIABLES
 */

  extern const struct cmdtab IPPoolSetCmds[];

/*
 * FUNCTIONS
 */

  extern int	IPPoolGet(char *pool, struct u_addr *ip);
  extern void	IPPoolFree(char *pool, struct u_addr *ip);
  
  extern void	IPPoolInit(void);
  extern int	IPPoolStat(Context ctx, int ac, char *av[], void *arg);

#endif

