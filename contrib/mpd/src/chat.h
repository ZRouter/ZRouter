
/*
 * chat.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _CHAT_H_
#define _CHAT_H_

/*
 * DEFINITIONS
 */

/* Bounds */

  #define CHAT_MAX_LINE		256
  #define CHAT_MAX_LABEL	32
  #define CHAT_MAX_MATCHES	32
  #define CHAT_MAX_TIMERS	32
  #define CHAT_NUM_VARIABLES	27
  #define CHAT_READBUF_SIZE	48

/* Chat logging levels */

  #define CHAT_LG_DEBUG		1
  #define CHAT_LG_NORMAL	2
  #define CHAT_LG_ERROR		3

/* Variable names that have special meaning in chat.c */

  #define CHAT_VAR_PREFIX	'$'
  #define CHAT_VAR_MATCHED	"$matchedString"
  #define CHAT_VAR_BAUDRATE	"$Baudrate"

/* Forward decls */

  struct chatinfo;
  typedef struct chatinfo	*ChatInfo;

/* Callback function types */

  typedef int	(*chatbaudfunc_t)(void *arg, int rate);
  typedef void	(*chatresultfunc_t)(void *arg, int r, const char *msg);

/*
 * FUNCTIONS
 */

  extern ChatInfo	ChatInit(void *arg, chatbaudfunc_t setBaudrate);
  extern void		ChatPresetVar(ChatInfo c,
			  const char *var, const char *value);
  extern char		*ChatGetVar(ChatInfo c, const char *var);
  extern void		ChatStart(ChatInfo c, int fd, FILE *scriptfp,
			  const char *label, chatresultfunc_t result);
  extern int		ChatActive(ChatInfo c);
  extern void		ChatAbort(ChatInfo c);

#endif

