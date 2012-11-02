
/*
 * ip.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _IP_H_
#define _IP_H_

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

/*
 * DEFINITIONS
 */

  struct u_addr {
    sa_family_t family;
    union {
	struct in_addr ip4;
	struct in6_addr ip6;
    } u;
  };

  struct u_range {
    struct u_addr	addr;
    u_char    		width;
  };

  enum {
    ALLOW_IPV4=1,
    ALLOW_IPV6,
    ALLOW_MASK
  };

/*
 * FUNCTIONS
 */

  extern int	IpShowRoutes(Context ctx, int ac, char *av[], void *arg);
  extern int	IpAddrInRange(struct u_range *range, struct u_addr *addr);

  extern int	ParseAddr(const char *s, struct u_addr *addr, u_char allow);
  extern int	ParseRange(const char *s, struct u_range *range, u_char allow);
  extern struct sockaddr_storage * ParseAddrPort(int ac, char *av[], u_char allow);

  extern sa_family_t	u_addrfamily(struct u_addr *addr);
  extern sa_family_t	u_rangefamily(struct u_range *range);

  extern char	*u_addrtoa(struct u_addr *addr, char *dst, size_t size);
  extern char	*u_rangetoa(struct u_range *range, char *dst, size_t size);

  extern struct u_addr *u_addrcopy(const struct u_addr *src, struct u_addr *dst);
  extern struct u_addr *u_rangecopy(const struct u_range *src, struct u_range *dst);

  extern struct u_addr *in_addrtou_addr(const struct in_addr *src, struct u_addr *dst);
  extern struct u_addr *in6_addrtou_addr(const struct in6_addr *src, struct u_addr *dst);

  extern struct in_addr *u_addrtoin_addr(const struct u_addr *src, struct in_addr *dst);
  extern struct in6_addr *u_addrtoin6_addr(const struct u_addr *src, struct in6_addr *dst);

  extern struct u_range *in_addrtou_range(const struct in_addr *src, u_char width, struct u_range *dst);
  extern struct u_range *in6_addrtou_range(const struct in6_addr *src, u_char width, struct u_range *dst);

  extern struct sockaddr_storage *u_rangetosockaddrs(struct u_range *range, struct sockaddr_storage *dst, struct sockaddr_storage *msk);
  extern struct sockaddr_storage *u_addrtosockaddr(struct u_addr *addr, in_port_t port, struct sockaddr_storage *dst);
  extern void 	sockaddrtou_addr(struct sockaddr_storage *src, struct u_addr *addr, in_port_t *port);

  extern struct u_addr *u_addrclear(struct u_addr *addr);
  extern struct u_range *u_rangeclear(struct u_range *range);

  extern int 	u_addrempty(struct u_addr *addr);
  extern int 	u_rangeempty(struct u_range *range);
  extern int 	u_rangehost(struct u_range *range);

  extern int 	u_addrcompare(const struct u_addr *addr1, const struct u_addr *addr2);
  extern int 	u_rangecompare(const struct u_range *range1, const struct u_range *range2);

  extern uint32_t	u_addrtoid(const struct u_addr *addr);
  
  extern u_char		in_addrtowidth(struct in_addr *mask);
  extern struct in_addr *widthtoin_addr(u_char width, struct in_addr *mask);

#endif

