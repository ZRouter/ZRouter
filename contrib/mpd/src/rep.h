
/*
 * rep.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _REP_H_
#define _REP_H_

#include "defs.h"
#include "msg.h"
#include "command.h"
#include <netgraph/ng_message.h>

/*
 * DEFINITIONS
 */

  /* Total state of a repeater */
  struct rep {
    char		name[LINK_MAX_NAME];	/* Name of this repeater */
    int			id;			/* Index of this link in gReps */
    int			csock;			/* Socket node control socket */
    ng_ID_t		node_id;		/* ng_tee node ID */
    Link		links[2];		/* Links used by repeater */
    int			refs;			/* Number of references */
    u_char		p_up;			/* Up phys */
    u_char		dead;			/* Dead flag */
  };
  
/*
 * VARIABLES
 */

  extern const struct cmdtab	RepSetCmds[];

/*
 * FUNCTIONS
 */

  extern int	RepStat(Context ctx, int ac, char *av[], void *arg);
  extern int	RepCommand(Context ctx, int ac, char *av[], void *arg);
  extern int	RepCreate(Link in, const char *out);
  extern void	RepShutdown(Rep r);

  extern void	RepIncoming(Link l);
  extern int	RepIsSync(Link l); /* Is pair link is synchronous */
  extern void	RepSetAccm(Link l, u_int32_t xmit, u_int32_t recv); /* Set async accm */
  extern void	RepUp(Link l);
  extern void	RepDown(Link l);
  extern int	RepGetHook(Link l, char *path, char *hook);
  extern Rep	RepFind(char *name);

#endif

