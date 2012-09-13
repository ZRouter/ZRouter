
/*
 * defs.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _DEFS_H_
#define _DEFS_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sysexits.h>
#include "config.h"

/*
 * DEFINITIONS
 */

  /* Compile time configuring. */
#ifndef HAVE_NG_CAR
  #undef USE_NG_CAR
#endif
#if !defined(HAVE_NG_DEFLATE) || !defined(USE_NG_DEFLATE) || !defined(CCP_DEFLATE)
  #undef CCP_DEFLATE
  #undef USE_NG_DEFLATE
#endif
#ifndef HAVE_NG_IPACCT
  #undef USE_NG_IPACCT
#endif
#if !defined(HAVE_NG_MPPC) || !defined(USE_NG_MPPC) || !defined(CCP_MPPC)
  #undef CCP_MPPC
  #undef USE_NG_MPPC
#endif
#ifndef HAVE_NG_NAT
  #undef USE_NG_NAT
#endif
#ifndef HAVE_NG_NETFLOW
  #undef USE_NG_NETFLOW
#endif
#if !defined(HAVE_NG_PRED1) || !defined(CCP_PRED1)
  #undef USE_NG_PRED1
#endif
#ifndef HAVE_NG_TCPMSS
  #undef USE_NG_TCPMSS
#endif
#ifndef HAVE_NG_VJC
  #undef USE_NG_VJC
#endif
#ifndef HAVE_NG_BPF
  #undef USE_NG_BPF
#endif
#ifndef HAVE_IPFW
  #undef USE_IPFW
#endif

  /* Boolean */
#ifndef TRUE
  #define TRUE 			1
#endif
#ifndef FALSE
  #define FALSE 		0
#endif

#ifndef MPD_VENDOR
  #define MPD_VENDOR		"FreeBSD MPD"
#endif

  /* Exit codes */
  #define EX_NORMAL		EX_OK
  #define EX_ERRDEAD		EX_SOFTWARE
  #define EX_TERMINATE		99	/* pseudo-code */

  /* Pathnames */
  #define CONF_FILE 		"mpd.conf"
  #define SECRET_FILE		"mpd.secret"
  #define SCRIPT_FILE		"mpd.script"

#ifndef PATH_CONF_DIR
  #define PATH_CONF_DIR		"/etc/ppp"
#endif

  #define LG_FILE		"/var/log/mpd"
  #define PID_FILE		"/var/run/mpd.pid"
  #define PATH_LOCKFILENAME	"/var/spool/lock/LCK..%s"

  #define PATH_IFCONFIG		"/sbin/ifconfig"
  #define PATH_ARP		"/usr/sbin/arp"
#ifdef USE_IPFW
  #define PATH_IPFW		"/sbin/ipfw"
#endif
  #define PATH_NETSTAT		"/usr/bin/netstat"

  #define AUTH_MAX_AUTHNAME	64
  #define AUTH_MAX_PASSWORD	64
  #define AUTH_MAX_SESSIONID	32

  #define LINK_MAX_NAME		16

  #define DEFAULT_CONSOLE_PORT	5005
  #define DEFAULT_CONSOLE_IP	"127.0.0.1"

  #define DEFAULT_WEB_PORT	5006
  #define DEFAULT_WEB_IP	"127.0.0.1"

  #define DEFAULT_RADSRV_PORT	3799
  #define DEFAULT_RADSRV_IP	"0.0.0.0"

  /* Characters, leave for interface number. For example: ppp9999 */
  #define IFNUMLEN		(sizeof("9999") - 1)

  /* Forward decl's */
  struct linkst;
  typedef struct linkst *Link;
  struct bundle;
  typedef struct bundle *Bund;
  struct rep;
  typedef struct rep *Rep;

  struct context;
  typedef struct context *Context;

#endif

