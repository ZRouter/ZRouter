
/*
 * ippool.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "ip.h"
#include "ippool.h"
#include "util.h"

enum {
    SET_ADD
};

struct ippool_rec {
    struct in_addr	ip;
    int			used;
};

struct ippool {
    char		name[LINK_MAX_NAME];
    struct ippool_rec	*pool;
    int			size;
    SLIST_ENTRY(ippool)	next;
};

typedef	struct ippool	*IPPool;

SLIST_HEAD(, ippool)	gIPPools;
pthread_mutex_t		gIPPoolMutex;

static void	IPPoolAdd(char *pool, struct in_addr begin, struct in_addr end);
static int	IPPoolSetCommand(Context ctx, int ac, char *av[], void *arg);

  const struct cmdtab IPPoolSetCmds[] = {
    { "add {pool} {start} {end}",	"Add IP range to the pool",
	IPPoolSetCommand, NULL, 2, (void *) SET_ADD },
    { NULL },
  };

void
IPPoolInit(void)
{
    int ret = pthread_mutex_init (&gIPPoolMutex, NULL);
    if (ret != 0) {
	Log(LG_ERR, ("Could not create IP pool mutex: %d", ret));
	exit(EX_UNAVAILABLE);
    }
    SLIST_INIT(&gIPPools);
}

int IPPoolGet(char *pool, struct u_addr *ip)
{
    IPPool	p;
    int		i;

    MUTEX_LOCK(gIPPoolMutex);
    SLIST_FOREACH(p, &gIPPools, next) {
	if (strcmp(p->name, pool) == 0)
	    break;
    }
    if (!p) {
	MUTEX_UNLOCK(gIPPoolMutex);
	return (-1);
    }
    i = 0;
    while (i < p->size) {
	if (!p->pool[i].used) {
	    p->pool[i].used = 1;
	    in_addrtou_addr(&p->pool[i].ip, ip);
	    MUTEX_UNLOCK(gIPPoolMutex);
	    return (0);
	}
	i++;
    }
    MUTEX_UNLOCK(gIPPoolMutex);
    return (-1);
}

void IPPoolFree(char *pool, struct u_addr *ip) {
    IPPool	p;
    int		i;

    MUTEX_LOCK(gIPPoolMutex);
    SLIST_FOREACH(p, &gIPPools, next) {
	if (strcmp(p->name, pool) == 0)
	    break;
    }
    if (!p) {
	MUTEX_UNLOCK(gIPPoolMutex);
	return;
    }
    i = 0;
    while (i < p->size) {
        if (p->pool[i].ip.s_addr == ip->u.ip4.s_addr) {
    	    p->pool[i].used = 0;
	    MUTEX_UNLOCK(gIPPoolMutex);
	    return;
	}
	i++;
    }
    MUTEX_UNLOCK(gIPPoolMutex);
}

static void
IPPoolAdd(char *pool, struct in_addr begin, struct in_addr end)
{

    IPPool 		p;
    struct ippool_rec	*r;
    int			i, j, k;
    int			total;
    u_int		c = ntohl(end.s_addr) - ntohl(begin.s_addr) + 1;
    
    if (c > 65536) {
	Log(LG_ERR, ("Too big IP range: %d", c));
	return;
    }
    
    MUTEX_LOCK(gIPPoolMutex);
    SLIST_FOREACH(p, &gIPPools, next) {
	if (strcmp(p->name, pool) == 0)
	    break;
    }

    if (!p) {
	p = Malloc(MB_IPPOOL, sizeof(struct ippool));
	strlcpy(p->name, pool, sizeof(p->name));
	SLIST_INSERT_HEAD(&gIPPools, p, next);
    }
    total = 0;
    for (i = 0; i < p->size; i++) {
	if (p->pool[i].ip.s_addr)
	    total++;
    }
    r = Malloc(MB_IPPOOL, (total + c) * sizeof(struct ippool_rec));
    if (p->pool != NULL) {
	memcpy(r, p->pool, p->size * sizeof(struct ippool_rec));
	Freee(p->pool);
    }
    p->pool = r;
    k = p->size;
    for (i = 0; i < c; i++) {
	struct in_addr ip;
	ip.s_addr = htonl(ntohl(begin.s_addr) + i);
	for (j = 0; j < p->size; j++) {
	    if (p->pool[j].ip.s_addr == ip.s_addr)
		break;
	}
	if (j != p->size)
	    continue;
	p->pool[k++].ip = ip;
    }
    p->size = k;
    MUTEX_UNLOCK(gIPPoolMutex);
}

/*
 * IPPoolStat()
 */

int
IPPoolStat(Context ctx, int ac, char *av[], void *arg)
{
    IPPool 	p;

    Printf("Available IP pools:\r\n");
    MUTEX_LOCK(gIPPoolMutex);
    SLIST_FOREACH(p, &gIPPools, next) {
	int	i;
	int	total = 0;
	int	used = 0;
	for (i = 0; i < p->size; i++) {
	    if (p->pool[i].ip.s_addr) {
		total++;
		if (p->pool[i].used)
		    used++;
	    }
	}
	Printf("\t%s:\tused %4d of %4d\r\n", p->name, used, total);
    }
    MUTEX_UNLOCK(gIPPoolMutex);
    return(0);
}

/*
 * IPPoolSetCommand()
 */

static int
IPPoolSetCommand(Context ctx, int ac, char *av[], void *arg)
{
    switch ((intptr_t)arg) {
    case SET_ADD:
      {
	struct u_addr	begin;
	struct u_addr	end;

	/* Parse args */
	if (ac != 3
	    || !ParseAddr(av[1], &begin, ALLOW_IPV4)
	    || !ParseAddr(av[2], &end, ALLOW_IPV4))
	  return(-1);

	IPPoolAdd(av[0], begin.u.ip4, end.u.ip4);
      }
      break;
    default:
      assert(0);
  }
  return(0);
}

