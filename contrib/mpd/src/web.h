
/*
 * web.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _WEB_H_
#define	_WEB_H_

#include "defs.h"
#include <openssl/ssl.h>
#ifdef NOLIBPDEL
#include "contrib/libpdel/http/http_defs.h"
#include "contrib/libpdel/http/http_server.h"
#include "contrib/libpdel/http/http_servlet.h"
#include "contrib/libpdel/http/servlet/basicauth.h"
#else
#include <pdel/http/http_defs.h>
#include <pdel/http/http_server.h>
#include <pdel/http/http_servlet.h>
#include <pdel/http/servlet/basicauth.h>
#endif

/*
 * DEFINITIONS
 */

  /* Configuration options */
  enum {
    WEB_AUTH	/* enable auth */
  };

  struct web {
    struct optinfo	options;
    struct u_addr 	addr;
    in_port_t		port;
    struct http_server *srv;
    struct http_servlet srvlet;
    EventRef		event;		/* connect-event */
  };

  typedef struct web *Web;

/*
 * VARIABLES
 */

  extern const struct cmdtab WebSetCmds[];


/*
 * FUNCTIONS
 */

  extern int	WebInit(Web c);
  extern int	WebOpen(Web c);
  extern int	WebClose(Web c);
  extern int	WebStat(Context ctx, int ac, char *av[], void *arg);


#endif

