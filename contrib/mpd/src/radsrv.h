
/*
 * radsrv.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _RADSRV_H_
#define	_RADSRV_H_

#include "defs.h"
#include <radlib.h>

/*
 * DEFINITIONS
 */

  #define RADSRV_MAX_SERVERS	10

  /* Configuration options */
  enum {
    RADSRV_DISCONNECT,	/* enable Disconnect-Request */
    RADSRV_COA		/* enable CoA-Request */
  };

  /* Configuration for a radius server */
  struct radiusclient_conf {
    char	*hostname;
    char	*sharedsecret;
    struct	radiusclient_conf *next;
  };

  struct radsrv {
    struct optinfo	options;
    struct u_addr 	addr;
    in_port_t		port;
    int			fd;
    struct rad_handle	*handle;
    struct radiusclient_conf *clients;
    EventRef		event;		/* connect-event */
  };

  typedef struct radsrv *Radsrv;

/*
 * VARIABLES
 */

  extern const struct cmdtab RadsrvSetCmds[];

/*
 * FUNCTIONS
 */

  extern int	RadsrvInit(Radsrv c);
  extern int	RadsrvOpen(Radsrv c);
  extern int	RadsrvClose(Radsrv c);
  extern int	RadsrvStat(Context ctx, int ac, char *av[], void *arg);

#endif

