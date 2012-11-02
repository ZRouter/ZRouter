
/*
 * ip.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "ip.h"
#include "util.h"

/*
 * IpShowRoutes()
 *
 * Show routing tables
 */

int
IpShowRoutes(Context ctx, int ac, char *av[], void *arg)
{
  FILE	*fp;
  char	buf[256];
  char	*c;

  if ((fp = popen(PATH_NETSTAT " -nr -f inet", "r")) == NULL)
  {
    Perror("popen");
    return(CMD_ERR_OTHER);
  }
  while (fgets(buf,sizeof(buf)-1,fp)) {
    if ((c=strrchr(buf,'\n')))
	*c='\r';
    Printf("%s\n",buf);
  }
  pclose(fp);
  return(0);
}

/*
 * IpAddrInRange()
 *
 * Is the IP address within the range?
 */

int
IpAddrInRange(struct u_range *range, struct u_addr *addr)
{
  u_int32_t mask;
  u_int8_t bmask;
  int	i;

  if (u_rangeempty(range))
    return(1);	// For compatibility we think that empty range includes any ip.

  if (range->addr.family!=addr->family)
    return(0);

  switch (range->addr.family) {
    case AF_INET:
	    mask = range->width ? htonl(~0 << (32 - range->width)) : 0;
	    return((addr->u.ip4.s_addr & mask) == (range->addr.u.ip4.s_addr & mask));
	break;
    case AF_INET6:
	    for (i=0; i<((range->width+7)/8); i++) {
		if ((i*8+7)<range->width) {
		    if (addr->u.ip6.s6_addr[i]!=range->addr.u.ip6.s6_addr[i])
			return 0;
		} else {
		    bmask=(range->width==i*8)? 0 : (~0 << (8 - range->width));
		    if ((addr->u.ip6.s6_addr[i] & bmask) != (range->addr.u.ip6.s6_addr[i] & bmask))
			return 0;
		}
	    }
	    return 1;
	break;
  }
  return 0;
}

/*
 * ParseAddr()
 *
 * Parse an IP address & mask of the form X.Y.Z.W/M.
 * If no slash, then M is assumed to be 32.
 * Returns TRUE if successful
 */

int
ParseAddr(const char *s, struct u_addr *addr, u_char allow)
{
  if (strchr(s, '/'))
    return (FALSE);

/* Get IP address part; if that fails, try looking up hostname */

  if ((allow&ALLOW_IPV4) && inet_pton(AF_INET, s, &addr->u.ip4))
  {
    addr->family=AF_INET;
  } 
  else if ((allow&ALLOW_IPV6) && inet_pton(AF_INET6, s, &addr->u.ip6))
  {
    addr->family=AF_INET6;
  } 
  else if ((allow&ALLOW_IPV4) && !GetAnyIpAddress(addr, s)) 
  {
    addr->family=AF_INET;
  }
  else if (allow&ALLOW_IPV4)
  {
  	struct hostent	*hptr;
	int		k;

	if ((hptr = gethostbyname(s)) == NULL) {
    	    return (FALSE);
	}
	for (k = 0; hptr->h_addr_list[k]; k++);
	if (k == 0)
	    k = 1;
	memcpy(&addr->u.ip4, hptr->h_addr_list[random() % k], sizeof(addr->u.ip4));
	addr->family=AF_INET;
  } 
  else 
  {
    return (FALSE);
  }

  return(TRUE);
}

/*
 * ParseRange()
 *
 * Parse an IP address & mask of the form X.Y.Z.W/M.
 * If no slash, then M is assumed to be 32.
 * Returns TRUE if successful
 */

int
ParseRange(const char *s, struct u_range *range, u_char allow)
{
  int			n_bits;
  char			*widp, buf[64];

  strlcpy(buf, s, sizeof(buf));
  if ((widp = strchr(buf, '/')) != NULL)
    *widp++ = '\0';
  else
    widp = buf + strlen(buf);

/* Get IP address part; if that fails, try looking up hostname */

  if (!ParseAddr(buf, &range->addr, allow)) 
    return(FALSE);

/* Get mask width */

  if (*widp)
  {
    if ((n_bits = atoi(widp)) < 0 || (range->addr.family==AF_INET && n_bits > 32) || (range->addr.family==AF_INET6 && n_bits > 128))
    {
      return(FALSE);
    }
  }
  else if (range->addr.family==AF_INET) {
    n_bits = 32;
  } else {
    n_bits = 128;
  }
  range->width = n_bits;

  return(TRUE);
}

/*
 * ParseAddrPort()
 *
 * Parse an IP address & port of the form X.Y.Z.W P.
 * Returns pointer to sockaddr_in. Not thread safe!
 */

struct sockaddr_storage *
ParseAddrPort(int ac, char *av[], u_char allow)
{
  static struct sockaddr_storage ss;
  struct u_addr addr;
  int		port = 0;

  if (ac < 1 || ac > 2)
    return (NULL);

  if (!ParseAddr(av[0], &addr, allow))
    return(NULL);

  if (ac > 1) {
    if ((port = atoi(av[1])) < 1 || port > 65535) {
      Log(LG_ERR, ("Bad port \"%s\"", av[1]));
      return (NULL);
    }
  }

  memset(&ss, 0, sizeof(ss));
  ss.ss_family = addr.family;
  switch (addr.family) {
    case AF_INET:
	ss.ss_len = sizeof(struct sockaddr_in);
	((struct sockaddr_in*)&ss)->sin_addr = addr.u.ip4;
	((struct sockaddr_in*)&ss)->sin_port = htons(port);
	break;
    case AF_INET6:
	ss.ss_len = sizeof(struct sockaddr_in6);
	((struct sockaddr_in6*)&ss)->sin6_addr = addr.u.ip6;
	((struct sockaddr_in6*)&ss)->sin6_port = htons(port);
	break;
    default:
	ss.ss_len = sizeof(struct sockaddr_storage);
  }

  return (&ss);
}

sa_family_t	u_addrfamily(struct u_addr *addr)
{
    return addr->family;
}

sa_family_t	u_rangefamily(struct u_range *range)
{
    return range->addr.family;
}

char   *u_addrtoa(struct u_addr *addr, char *dst, size_t size)
{
    dst[0]=0;
    
    if (addr->family==AF_INET) {
	inet_ntop(addr->family, &addr->u.ip4, dst, size);
    } 
    else if (addr->family==AF_INET6) 
    {
        inet_ntop(addr->family, &addr->u.ip6, dst, size);
    }
    else if (addr->family==AF_UNSPEC) 
    {
        snprintf(dst,size,"UNSPEC");
    }

    return dst;
}

char   *u_rangetoa(struct u_range *range, char *dst, size_t size)
{
    if (!u_addrtoa(&range->addr, dst, size))
	return NULL;

    if (range->addr.family!=AF_UNSPEC)
	snprintf(dst+strlen(dst), size-strlen(dst), "/%d", range->width);

    return dst;
}

struct u_addr *u_addrcopy(const struct u_addr *src, struct u_addr *dst)
{
    return memcpy(dst,src,sizeof(struct u_addr));
}

struct u_addr *u_rangecopy(const struct u_range *src, struct u_range *dst)
{
    return memcpy(dst,src,sizeof(struct u_range));
}

struct u_addr *u_addrclear(struct u_addr *addr)
{
    memset(addr,0,sizeof(struct u_addr));
    return addr;
}

struct u_range *u_rangeclear(struct u_range *range)
{
    memset(range,0,sizeof(struct u_range));
    return range;
}

struct u_addr *in_addrtou_addr(const struct in_addr *src, struct u_addr *dst)
{
    u_addrclear(dst);
    dst->family=AF_INET;
    dst->u.ip4=*src;
    return dst;
}

struct u_addr *in6_addrtou_addr(const struct in6_addr *src, struct u_addr *dst)
{
    u_addrclear(dst);
    dst->family=AF_INET6;
    dst->u.ip6=*src;
    return dst;
}

struct in_addr *u_addrtoin_addr(const struct u_addr *src, struct in_addr *dst)
{
    *dst=src->u.ip4;
    return dst;
}

struct in6_addr *u_addrtoin6_addr(const struct u_addr *src, struct in6_addr *dst)
{
    *dst=src->u.ip6;
    return dst;
}

struct u_range *in_addrtou_range(const struct in_addr *src, u_char width, struct u_range *dst)
{
    u_rangeclear(dst);
    in_addrtou_addr(src, &dst->addr);
    dst->width = width;
    return dst;
}

struct u_range *in6_addrtou_range(const struct in6_addr *src, u_char width, struct u_range *dst)
{
    u_rangeclear(dst);
    in6_addrtou_addr(src, &dst->addr);
    dst->width = width;
    return dst;
}

int u_addrempty(struct u_addr *addr)
{
    int i;
    switch (addr->family) {
	case AF_INET:
		return (addr->u.ip4.s_addr==0); 
	    break;
	case AF_INET6:
		for (i=0;i<16;i++) {
		    if (addr->u.ip6.s6_addr[i]!=0)
			return 0;
		}
		return 1;
	    break;
    }
    return 1;
}

int u_rangeempty(struct u_range *range)
{
    return u_addrempty(&range->addr);
}

int u_rangehost(struct u_range *range)
{
    switch (range->addr.family) {
	case AF_INET:
		return (range->width==32); 
	    break;
	case AF_INET6:
		return (range->width==128); 
	    break;
    }
    return 0;
}

static struct in6_addr
bits2mask6(int bits)
{
  struct in6_addr result;
  u_int32_t bit = 0x80;
  u_char *c = result.s6_addr;

  memset(&result, '\0', sizeof(result));

  while (bits) {
    if (bit == 0) {
      bit = 0x80;
      c++;
    }
    *c |= bit;
    bit >>= 1;
    bits--;
  }

  return result;
}

struct sockaddr_storage *u_rangetosockaddrs(struct u_range *range, struct sockaddr_storage *dst, struct sockaddr_storage *msk)
{
    memset(dst,0,sizeof(struct sockaddr_storage));
    memset(msk,0,sizeof(struct sockaddr_storage));
    dst->ss_family = msk->ss_family = range->addr.family;
    switch (range->addr.family) {
	case AF_INET:
		((struct sockaddr_in*)dst)->sin_len=sizeof(struct sockaddr_in);
		((struct sockaddr_in*)dst)->sin_addr=range->addr.u.ip4;
		((struct sockaddr_in*)msk)->sin_len=sizeof(struct sockaddr_in);
		((struct sockaddr_in*)msk)->sin_addr.s_addr=htonl(0xFFFFFFFFLL<<(32 - range->width));
	    break;
	case AF_INET6:
		((struct sockaddr_in6*)dst)->sin6_len=sizeof(struct sockaddr_in6);
		((struct sockaddr_in6*)dst)->sin6_addr=range->addr.u.ip6;
		((struct sockaddr_in6*)msk)->sin6_len=sizeof(struct sockaddr_in6);
		((struct sockaddr_in6*)msk)->sin6_addr=bits2mask6(range->width);
	    break;
	default:
		dst->ss_len=sizeof(struct sockaddr_storage);
	    break;
    }
    return dst;
}

struct sockaddr_storage *u_addrtosockaddr(struct u_addr *addr, in_port_t port, struct sockaddr_storage *dst)
{
    memset(dst,0,sizeof(struct sockaddr_storage));
    dst->ss_family=addr->family;
    switch (addr->family) {
	case AF_INET:
		((struct sockaddr_in*)dst)->sin_len=sizeof(struct sockaddr_in);
		((struct sockaddr_in*)dst)->sin_addr=addr->u.ip4;
		((struct sockaddr_in*)dst)->sin_port=htons(port);
	    break;
	case AF_INET6:
		((struct sockaddr_in6*)dst)->sin6_len=sizeof(struct sockaddr_in6);
		((struct sockaddr_in6*)dst)->sin6_addr=addr->u.ip6;
		((struct sockaddr_in6*)dst)->sin6_port=htons(port);
	    break;
	default:
		dst->ss_len=sizeof(struct sockaddr_storage);
	    break;
    }
    return dst;
}

void sockaddrtou_addr(struct sockaddr_storage *src, struct u_addr *addr, in_port_t *port)
{
    addr->family=src->ss_family;
    switch (addr->family) {
	case AF_INET:
		addr->u.ip4=((struct sockaddr_in*)src)->sin_addr;
		*port=ntohs(((struct sockaddr_in*)src)->sin_port);
	    break;
	case AF_INET6:
		addr->u.ip6=((struct sockaddr_in6*)src)->sin6_addr;
		*port=ntohs(((struct sockaddr_in6*)src)->sin6_port);
	    break;
	default:
	    memset(addr,0,sizeof(struct u_addr));
	    *port=0;
	    break;
    }
}

int u_addrcompare(const struct u_addr *addr1, const struct u_addr *addr2)
{
  int	i;

  if (addr1->family<addr2->family)
    return(-1);
  else if (addr1->family>addr2->family)
    return(1);
  
  switch (addr1->family) {
    case AF_INET:
	    if (addr1->u.ip4.s_addr < addr2->u.ip4.s_addr)
		return (-1);
	    else if (addr1->u.ip4.s_addr == addr2->u.ip4.s_addr)
		return (0);
	    else 
		return (1);
	break;
    case AF_INET6:
	    for (i=0; i<16; i++) {
		if (addr1->u.ip6.s6_addr[i] < addr2->u.ip6.s6_addr[i])
		    return (-1);
		else if (addr1->u.ip6.s6_addr[i] > addr2->u.ip6.s6_addr[i])
		    return (1);
	    }
	    return 0;
	break;
  }
  return 0;
}

int u_rangecompare(const struct u_range *range1, const struct u_range *range2)
{
  if (range1->width<range2->width)
    return(-1);
  else if (range1->width>range2->width)
    return(1);

  return u_addrcompare(&range1->addr,&range2->addr);
}

uint32_t
u_addrtoid(const struct u_addr *addr)
{
    uint32_t id;

    if (addr->family==AF_INET) {
	id = ntohl(addr->u.ip4.s_addr);
    } else if (addr->family==AF_INET6) {
	uint32_t *a32 = (uint32_t *)(&addr->u.ip6.s6_addr[0]);
        id = a32[0] + a32[1] + a32[2] + a32[3];
    } else {
	id = 0;
    }

    return (id);
}

u_char
in_addrtowidth(struct in_addr *mask)
{
	u_char width = 0;
	uint32_t m = ntohl(mask->s_addr);

	while ((m & 0x80000000) != 0) {
		m <<= 1;
		width++;
	}
	return (width);
}

struct in_addr *
widthtoin_addr(u_char width, struct in_addr *mask)
{
	mask->s_addr = htonl(0xFFFFFFFF << (32 - width));
	return (mask);
}
