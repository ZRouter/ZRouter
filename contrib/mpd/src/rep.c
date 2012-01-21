
/*
 * rep.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "rep.h"
#include "msg.h"
#include "ngfunc.h"
#include "log.h"
#include "util.h"

#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_tee.h>
#include <netgraph.h>

/*
 * INTERNAL FUNCTIONS
 */

  static void	RepShowLinks(Context ctx, Rep r);

/*
 * RepIncoming()
 */

void
RepIncoming(Link l)
{
    Rep		r = l->rep;
    struct ngm_mkpeer       mkp;
    char	buf[64];
    
    Log(LG_REP, ("[%s] Rep: INCOMING event from %s (0)",
	r->name, l->name));
	
    if (r->csock <= 0) {
	/* Create a new netgraph node to control TCP ksocket node. */
	if (NgMkSockNode(NULL, &r->csock, NULL) < 0) {
    	    Log(LG_ERR, ("[%s] Rep: can't create control socket: %s",
    		r->name, strerror(errno)));
    	    PhysClose(l);
	    return;
	}
	(void)fcntl(r->csock, F_SETFD, 1);
    }

    strcpy(mkp.type, NG_TEE_NODE_TYPE);
    snprintf(mkp.ourhook, sizeof(mkp.ourhook), "tee");
    snprintf(mkp.peerhook, sizeof(mkp.peerhook), NG_TEE_HOOK_LEFT2RIGHT);
    if (NgSendMsg(r->csock, ".:", NGM_GENERIC_COOKIE,
        NGM_MKPEER, &mkp, sizeof(mkp)) < 0) {
    	Log(LG_ERR, ("[%s] Rep: can't attach %s %s node: %s",
    	    l->name, NG_TEE_NODE_TYPE, mkp.ourhook, strerror(errno)));
	close(r->csock);
    	PhysClose(l);
	return;
    }

    /* Get tee node ID */
    if ((r->node_id = NgGetNodeID(r->csock, ".:tee")) == 0) {
	Log(LG_ERR, ("[%s] Rep: Cannot get %s node id: %s",
	    l->name, NG_TEE_NODE_TYPE, strerror(errno)));
	close(r->csock);
    	PhysClose(l);
	return;
    };
    
    PhysGetCallingNum(r->links[0], buf, sizeof(buf));
    PhysSetCallingNum(r->links[1], buf);

    PhysGetCalledNum(r->links[0], buf, sizeof(buf));
    PhysSetCalledNum(r->links[1], buf);

    PhysOpen(r->links[1]);
}

/*
 * RepUp()
 */

void
RepUp(Link l)
{
    Rep r = l->rep;
    int n = (r->links[0] == l)?0:1;
    
    Log(LG_REP, ("[%s] Rep: UP event from %s (%d)",
	r->name, l->name, n));

    r->p_up |= (1 << n);
    
    if (n == 1)
	PhysOpen(r->links[1-n]);

    if (r->p_up == 3 && r->csock > 0 && r->node_id) {
	char path[NG_PATHSIZ];
	
	snprintf(path, sizeof(path), "[%x]:", r->node_id);
	NgFuncShutdownNode(r->csock, r->name, path);
	r->node_id = 0;
	close(r->csock);
	r->csock = -1;
    }
}

/*
 * RepDown()
 */

void
RepDown(Link l)
{
    Rep r = l->rep;
    int n = (r->links[0] == l)?0:1;

    Log(LG_REP, ("[%s] Rep: DOWN event from %s (%d)",
	r->name, l->name, n));

    r->p_up &= ~(1 << n);

    if (r->links[1-n])
	PhysClose(r->links[1-n]);

    if (r->csock > 0 && r->node_id) {
	char path[NG_PATHSIZ];
	
	snprintf(path, sizeof(path), "[%x]:", r->node_id);
	NgFuncShutdownNode(r->csock, r->name, path);
	r->node_id = 0;
	close(r->csock);
	r->csock = -1;
    }
    
    l->rep->links[n] = NULL;
    l->rep = NULL;
    if (!l->stay) {
        REF(l);
        MsgSend(&l->msgs, MSG_SHUTDOWN, l);
    }

    if (r->links[1-n] == NULL)
	RepShutdown(r);
}

/*
 * RepIsSync()
 */

int
RepIsSync(Link l) {
    Rep r = l->rep;
    int n = (r->links[0] == l)?0:1;
    
    if (r->links[1-n])
	return (PhysIsSync(r->links[1-n]));
    else
	return (0);
}

/*
 * RepSetAccm()
 */

void
RepSetAccm(Link l, u_int32_t xmit, u_int32_t recv) {
    Rep r = l->rep;
    int n = (r->links[0] == l)?0:1;
    
    Log(LG_REP, ("[%s] Rep: SetAccm(0x%08x, 0x%08x) from %s (%d)",
	r->name, xmit, recv, l->name, n));

    if (r->links[1-n])
	PhysSetAccm(r->links[1-n], xmit, recv);
}

/*
 * RepGetHook()
 */

int
RepGetHook(Link l, char *path, char *hook)
{
    Rep r = l->rep;
    int n = (r->links[0] == l)?0:1;

    if (r->node_id == 0)
	return (0);

    snprintf(path, NG_PATHSIZ, "[%lx]:", (u_long)r->node_id);
    if (n == 0)
	snprintf(hook, NG_HOOKSIZ, NG_TEE_HOOK_LEFT);
    else
	snprintf(hook, NG_HOOKSIZ, NG_TEE_HOOK_RIGHT);
    return (1);
}

/*
 * RepCommand()
 *
 * Show list of all bundles or set bundle
 */

int
RepCommand(Context ctx, int ac, char *av[], void *arg)
{
    Rep	r;
    int	k;

    if (ac > 1)
	return (-1);

    if (ac == 0) {
	Printf("Defined repeaters:\r\n");
    	for (k = 0; k < gNumReps; k++) {
	    if ((r = gReps[k]) != NULL) {
	        Printf("\t%-15s%s %s\n",
		    r->name, r->links[0]->name, r->links[1]->name);
	    }
	}
	return (0);
    }

    if ((r = RepFind(av[0])) == NULL) {
	RESETREF(ctx->rep, NULL);
	RESETREF(ctx->bund, NULL);
	RESETREF(ctx->lnk, NULL);
	Error("Repeater \"%s\" not defined.", av[0]);
    }

    /* Change repeater, and link also if needed */
    RESETREF(ctx->rep, r);
    RESETREF(ctx->bund, NULL);
    RESETREF(ctx->lnk, r->links[0]);

    return(0);
}

/*
 * RepCreate()
 *
 * Create a new repeater.
 */

int
RepCreate(Link in, const char *out)
{
    Rep		r;
    Link	l;
    int		k;

    if ((l = LinkFind(out)) == NULL) {
	Log(LG_REP, ("[%s] Can't find link \"%s\"", in->name, out));
	return (-1);
    }
    if (PhysIsBusy(l)) {
	Log(LG_REP, ("[%s] Link \"%s\" is busy", in->name, out));
	return (-1);
    }
    if (l->tmpl)
	l = LinkInst(l, NULL, 0, 0);
    if (!l) {
	Log(LG_REP, ("[%s] Can't create link \"%s\"", in->name, out));
	return (-1);
    }

    /* Create a new repeater structure */
    r = Malloc(MB_REP, sizeof(*r));
    snprintf(r->name, sizeof(r->name), "R-%s", in->name);
    r->csock = -1;

    /* Add repeater to the list of repeaters and make it the current active repeater */
    for (k = 0; k < gNumReps && gReps[k] != NULL; k++);
    if (k == gNumReps)			/* add a new repeater pointer */
        LengthenArray(&gReps, sizeof(*gReps), &gNumReps, MB_REP);
    r->id = k;
    gReps[k] = r;
    REF(r);

    /* Join all part */
    r->links[0] = in;
    r->links[1] = l;
    in->rep = r;
    l->rep = r;

    /* Done */
    return(0);
}

/*
 * RepShutdown()
 */
 
void
RepShutdown(Rep r)
{
    int k;

    gReps[r->id] = NULL;

    Log(LG_REP, ("[%s] Rep: Shutdown", r->name));
    for (k = 0; k < 2; k++) {
	Link	l;
	if ((l = r->links[k]) != NULL)
	    if (!l->stay)
		LinkShutdown(l);
    }

    if (r->csock > 0 && r->node_id) {
	char path[NG_PATHSIZ];
	
	snprintf(path, sizeof(path), "[%x]:", r->node_id);
	NgFuncShutdownNode(r->csock, r->name, path);
	r->node_id = 0;
	close(r->csock);
	r->csock = -1;
    }
    r->dead = 1;
    UNREF(r);
}

/*
 * RepStat()
 *
 * Show state of a repeater
 */

int
RepStat(Context ctx, int ac, char *av[], void *arg)
{
    Rep	r;

    /* Find repeater they're talking about */
    switch (ac) {
	case 0:
    	    r = ctx->rep;
    	    break;
	case 1:
    	    if ((r = RepFind(av[0])) == NULL)
		Error("Repeater \"%s\" not defined.", av[0]);
    	    break;
	default:
    	    return(-1);
    }

    /* Show stuff about the repeater */
    Printf("Repeater %s:\r\n", r->name);
    Printf("\tLinks           : ");
    RepShowLinks(ctx, r);

    return(0);
}

/*
 * RepShowLinks()
 */

static void
RepShowLinks(Context ctx, Rep r)
{
    int		j;

    for (j = 0; j < 2; j++) {
	if (r->links[j]) {
	    Printf("%s[%s/%s] ", r->links[j]->name, r->links[j]->type->name,
      	        gPhysStateNames[r->links[j]->state]);
	}
    }
    Printf("\r\n");
}

/*
 * RepFind()
 *
 * Find a repeater structure
 */

Rep
RepFind(char *name)
{
    int	k;

    for (k = 0;
	k < gNumReps && (!gReps[k] || strcmp(gReps[k]->name, name));
	k++);
    return((k < gNumReps) ? gReps[k] : NULL);
}

