
/*
 * log.h
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _LG_H_
#define	_LG_H_

/*
 * DEFINITIONS
 */

  enum
  {
    LG_I_ALWAYS = 0,
    LG_I_BUND,
    LG_I_BUND2,
    LG_I_LINK,
    LG_I_REP,
    LG_I_CHAT,
    LG_I_CHAT2,
    LG_I_IFACE,
    LG_I_IFACE2,
    LG_I_LCP,
    LG_I_LCP2,
    LG_I_AUTH,
    LG_I_AUTH2,
    LG_I_IPCP,
    LG_I_IPCP2,
    LG_I_IPV6CP,
    LG_I_IPV6CP2,
    LG_I_CCP,
    LG_I_CCP2,
    LG_I_ECP,
    LG_I_ECP2,
    LG_I_FSM,
    LG_I_ECHO,
    LG_I_PHYS,
    LG_I_PHYS2,
    LG_I_PHYS3,
    LG_I_FRAME,
    LG_I_RADIUS,
    LG_I_RADIUS2,
    LG_I_CONSOLE,
    LG_I_EVENTS
  };

/* Definition of log options */

  #define LG_BUND		(1 << LG_I_BUND)
  #define LG_BUND2		(1 << LG_I_BUND2)
  #define LG_LINK		(1 << LG_I_LINK)
  #define LG_REP		(1 << LG_I_REP)
  #define LG_CHAT		(1 << LG_I_CHAT)
  #define LG_CHAT2		(1 << LG_I_CHAT2)
  #define LG_IFACE		(1 << LG_I_IFACE)
  #define LG_IFACE2		(1 << LG_I_IFACE2)
  #define LG_LCP		(1 << LG_I_LCP)
  #define LG_LCP2		(1 << LG_I_LCP2)
  #define LG_AUTH		(1 << LG_I_AUTH)
  #define LG_AUTH2		(1 << LG_I_AUTH2)
  #define LG_IPCP		(1 << LG_I_IPCP)
  #define LG_IPCP2		(1 << LG_I_IPCP2)
  #define LG_IPV6CP		(1 << LG_I_IPV6CP)
  #define LG_IPV6CP2		(1 << LG_I_IPV6CP2)
  #define LG_CCP		(1 << LG_I_CCP)
  #define LG_CCP2		(1 << LG_I_CCP2)
  #define LG_ECP		(1 << LG_I_ECP)
  #define LG_ECP2		(1 << LG_I_ECP2)
  #define LG_FSM		(1 << LG_I_FSM)
  #define LG_ECHO		(1 << LG_I_ECHO)
  #define LG_PHYS		(1 << LG_I_PHYS)
  #define LG_PHYS2		(1 << LG_I_PHYS2)
  #define LG_PHYS3		(1 << LG_I_PHYS3)
  #define LG_FRAME		(1 << LG_I_FRAME)
  #define LG_RADIUS		(1 << LG_I_RADIUS)
  #define LG_RADIUS2		(1 << LG_I_RADIUS2)
  #define LG_CONSOLE		(1 << LG_I_CONSOLE)
  #define LG_ALWAYS		(1 << LG_I_ALWAYS)
  #define LG_EVENTS		(1 << LG_I_EVENTS)

  #define LG_ERR		(LG_ALWAYS)

/* Default options at startup */

  #define LG_DEFAULT_OPT	(0			\
				| LG_BUND		\
				| LG_LINK		\
				| LG_REP		\
			        | LG_IFACE		\
			        | LG_CONSOLE		\
			        | LG_CHAT		\
			        | LG_LCP		\
			        | LG_IPCP		\
			        | LG_IPV6CP		\
			        | LG_CCP		\
			        | LG_ECP		\
			        | LG_AUTH		\
			        | LG_RADIUS		\
			        | LG_FSM		\
			        | LG_PHYS		\
				)

  #define Log(lev, args)	do {				\
				  if (gLogOptions & (lev))	\
				    LogPrintf args;		\
				} while (0)

  #define Log2(lev, args)	do {				\
				  if (gLogOptions & (lev))	\
				    LogPrintf2 args;		\
				} while (0)

  #define LogDumpBuf(lev, buf, len, fmt, args...) do {		\
				  if (gLogOptions & (lev))	\
				    LogDumpBuf2(buf, len, fmt, ##args);	\
				} while (0)

  #define LogDumpBp(lev, bp, fmt, args...) do {			\
				  if (gLogOptions & (lev))	\
				    LogDumpBp2(bp, fmt, ##args);\
				} while (0)

/*
 * VARIABLES
 */

  extern int	gLogOptions;
#ifdef SYSLOG_FACILITY
  extern char	gSysLogIdent[32];
#endif

/*
 * FUNCTIONS
 */

  extern int	LogOpen(void);
  extern void	LogClose(void);
  extern void	LogPrintf(const char *fmt, ...) __printflike(1, 2);
  extern void	vLogPrintf(const char *fmt, va_list args);
  extern void	LogPrintf2(const char *fmt, ...) __printflike(1, 2);
  extern int	LogCommand(Context ctx, int ac, char *av[], void *arg);
  extern void	LogDumpBuf2(const u_char *buf, int len,
			const char *fmt, ...) __printflike(3, 4);
  extern void	LogDumpBp2(Mbuf bp, const char *fmt, ...)
			__printflike(2, 3);
  extern void	Perror(const char *fmt, ...) __printflike(1, 2);

#endif

