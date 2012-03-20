
/*
 * ppp.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _PPP_H_
#define _PPP_H_

/* Keep source files simple */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <netdb.h>
#include <fcntl.h>
#include <machine/endian.h>
#include <net/ppp_defs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#ifdef NOLIBPDEL
#include "contrib/libpdel/structs/structs.h"
#include "contrib/libpdel/structs/type/array.h"
#include "contrib/libpdel/util/typed_mem.h"
#include "contrib/libpdel/util/pevent.h"
#include "contrib/libpdel/util/paction.h"
#include "contrib/libpdel/util/ghash.h"
#else
#include <pdel/structs/structs.h>
#include <pdel/structs/type/array.h>
#include <pdel/util/typed_mem.h>
#include <pdel/util/pevent.h>
#include <pdel/util/paction.h>
#include <pdel/util/ghash.h>
#endif

#include <netgraph/ng_message.h>
#include <netgraph/ng_ppp.h>

#include "defs.h"

/*
 * DEFINITIONS
 */

  /* Do our own version of assert() so it shows up in the logs */
  #define assert(e)	((e) ? (void)0 : DoAssert(__FILE__, __LINE__, #e))

  /* Giant Mutex handling */
  #define GIANT_MUTEX_LOCK()	assert(pthread_mutex_lock(&gGiantMutex) == 0)
  #define GIANT_MUTEX_UNLOCK()	assert(pthread_mutex_unlock(&gGiantMutex) == 0)

  #define MUTEX_LOCK(m)		assert(pthread_mutex_lock(&m) == 0)
  #define MUTEX_UNLOCK(m)	assert(pthread_mutex_unlock(&m) == 0)

  #define RWLOCK_RDLOCK(m)	assert(pthread_rwlock_rdlock(&m) == 0)
  #define RWLOCK_WRLOCK(m)	assert(pthread_rwlock_wrlock(&m) == 0)
  #define RWLOCK_UNLOCK(m)	assert(pthread_rwlock_unlock(&m) == 0)
  
  #define SETOVERLOAD(q)	do {					\
				    int t = (q);			\
				    if (t > 60) {			\
					gOverload = 100;		\
				    } else if (t > 10) {		\
					gOverload = (t - 10) * 2;	\
				    } else {				\
					gOverload = 0;			\
				    }					\
				} while (0)

  #define OVERLOAD()		(gOverload > (random() % 100))
  
  #define REF(p)		do {					\
				    (p)->refs++;			\
				} while (0)

  #define UNREF(p)		do {					\
				    if ((--(p)->refs) == 0)		\
					Freee(p);			\
				} while (0)

  #define RESETREF(v, p)	do {					\
				    if (v) UNREF(v);			\
				    (v) = (p);				\
				    if (v) REF(v);			\
				} while (0)

  #define ADLG_WAN_AUTHORIZATION_FAILURE	0
  #define ADLG_WAN_CONNECTED			1
  #define ADLG_WAN_CONNECTING			2
  #define ADLG_WAN_CONNECT_FAILURE		3
  #define ADLG_WAN_DISABLED			4
  #define ADLG_WAN_MESSAGE			5
  #define ADLG_WAN_NEGOTIATION_FAILURE		6
  #define ADLG_WAN_WAIT_FOR_DEMAND		7

#ifndef NG_PPP_STATS64
  /* internal 64 bit counters as workaround for the 32 bit 
   * limitation for ng_ppp_link_stat
   */
  struct ng_ppp_link_stat64 {
	u_int64_t 	xmitFrames;	/* xmit frames on link */
	u_int64_t 	xmitOctets;	/* xmit octets on link */
	u_int64_t 	recvFrames;	/* recv frames on link */
	u_int64_t	recvOctets;	/* recv octets on link */
	u_int64_t 	badProtos;	/* frames rec'd with bogus protocol */
	u_int64_t 	runts;		/* Too short MP fragments */
	u_int64_t 	dupFragments;	/* MP frames with duplicate seq # */
	u_int64_t	dropFragments;	/* MP fragments we had to drop */
  };
#endif

#if defined(USE_NG_BPF) || defined(USE_IPFW)
  /* max. length of acl rule */
  #define ACL_LEN	256
  #define ACL_NAME_LEN	16
  /* max. number of acl_filters */
  #define ACL_FILTERS	16
  /* There are two directions for acl_limits */
  #define ACL_DIRS	2
#endif

#ifdef USE_NG_BPF
  struct svcssrc {
    int				type;
  #define SSSS_IN	1
  #define SSSS_MATCH	2
  #define SSSS_NOMATCH	3
  #define SSSS_OUT	4
    char			hook[NG_HOOKSIZ];
    SLIST_ENTRY(svcssrc)	next;
  };

  struct svcs {
    char 			name[ACL_NAME_LEN]; 	/* Name of ACL */
    SLIST_HEAD(, svcssrc)	src;
    SLIST_ENTRY(svcs)		next;
  };

  struct svcstatrec {
    char			name[ACL_NAME_LEN]; 	/* Name of ACL */
    u_int64_t			Packets;
    u_int64_t			Octets;
    SLIST_ENTRY(svcstatrec)	next;
  };
  
  struct svcstat {
    SLIST_HEAD(, svcstatrec)	stat[ACL_DIRS];
  };
#endif /* USE_NG_BPF */

#include "bund.h"
#include "link.h"
#include "rep.h"
#include "phys.h"
#include "msgdef.h"

/*
 * VARIABLES
 */

  extern Rep		*gReps;			/* Repeaters */
  extern Link		*gLinks;		/* Links */
  extern Bund		*gBundles;		/* Bundles */

  extern int		gNumReps;		/* Total number of repeaters */
  extern int		gNumLinks;		/* Total number of links */
  extern int		gNumBundles;		/* Total number of bundles */
  extern struct console	gConsole;
  extern struct web	gWeb;
  extern struct radsrv	gRadsrv;
  extern int		gBackground;
  extern int		gShutdownInProgress;
  extern int		gOverload;
  extern pid_t		gPid;
  extern int		gRouteSeq;

#ifdef PHYSTYPE_PPTP
  extern int		gPPTPto;
  extern int		gPPTPtunlimit;
#endif
#ifdef PHYSTYPE_L2TP
  extern int		gL2TPto;
  extern int		gL2TPtunlimit;
#endif
  extern int		gChildren;
  extern int		gMaxChildren;

  extern struct globalconf	gGlobalConf;	/* Global config settings */

#ifdef USE_NG_BPF
  extern struct acl		*acl_filters[ACL_FILTERS]; /* mpd's internal bpf filters */
#endif

  extern struct pevent_ctx	*gPeventCtx;
  extern pthread_mutex_t	gGiantMutex;	/* Giant Mutex */

  extern const char	*gVersion;		/* Program version string */
  extern const char	*gConfigFile;		/* Main config file */
  extern const char	*gConfDirectory;	/* Where the files are */

/*
 * FUNCTIONS
 */

  extern void		Greetings(void);
  extern void		SendSignal(int sig);
  extern void		DoExit(int code) __dead2;
  extern void		DoAssert(const char *file, int line, const char *x) __dead2;
  extern void		CheckOneShot(void);

#endif

