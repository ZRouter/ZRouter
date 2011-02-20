
/*
 * chat.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "chat.h"
#include "util.h"
#include <regex.h>

/*
 * DEFINITIONS
 */

/* Bounds */

  #define CHAT_MAX_ARGS		10
  #define CHAT_MAX_VARNAME	100

/* Keywords */

  #define IF			"if"
  #define SET			"set"
  #define MATCH			"match"
  #define REGEX			"regex"
  #define TIMER			"timer"
  #define CANCEL		"cancel"
  #define PRINT			"print"
  #define CALL			"call"
  #define LOG			"log"
  #define RETURN		"return"
  #define GOTO			"goto"
  #define SUCCESS		"success"
  #define FAILURE		"failure"
  #define WAIT			"wait"
  #define NOTHING		"nop"
  #define CHAT_KEYWORD_ALL	"all"

/* Special timer used when no label given */

  #define DEFAULT_LABEL		""
  #define DEFAULT_SET		""

/* States */

  enum
  {
    CHAT_IDLE = 0,
    CHAT_RUNNING,
    CHAT_WAIT
  };

/* Commands */

  enum
  {
    CMD_IF,
    CMD_SET,
    CMD_MATCH,
    CMD_REGEX,
    CMD_TIMER,
    CMD_CANCEL,
    CMD_PRINT,
    CMD_CALL,
    CMD_LOG,
    CMD_RETURN,
    CMD_GOTO,
    CMD_SUCCESS,
    CMD_FAILURE,
    CMD_WAIT,
    CMD_NOTHING
  };

/* Structures */

  struct chatcmd
  {
    const char	*name;
    short	id;
    short	min;
    short	max;
  };

  struct chatvar
  {
    char		*name;
    char		*value;
    struct chatvar	*next;
  };
  typedef struct chatvar	*ChatVar;

  struct chatmatch
  {
    char		*set;
    char		*label;
    u_char		exact:1;	/* true if this is an exact match */
    union {
      struct cm_exact {
	char	*pat;			/* exact string to match */
	u_short	*fail;			/* failure function */
	u_int	matched;		/* number of chars matched so far */
      }		exact;
      regex_t	regex;			/* regular expression to match */
    }			u;
    int			frameDepth;	/* number of frames to pop */
    struct chatmatch	*next;
  };
  typedef struct chatmatch	*ChatMatch;

  struct chattimer
  {
    char		*set;
    char		*label;
    EventRef		event;
    int			frameDepth;	/* number of frames to pop */
    struct chatinfo	*c;
    struct chattimer	*next;
  };
  typedef struct chattimer	*ChatTimer;

  struct chatframe
  {
    fpos_t		posn;
    int			lineNum;
    struct chatframe	*up;
  };
  typedef struct chatframe	*ChatFrame;

  struct chatinfo
  {
    void		*arg;		/* opaque client context */
    FILE		*fp;		/* chat script file */
    int			lineNum;	/* current script line number */
    int			state;		/* run/wait/idle state */
    int			fd;		/* device to talk to */
    EventRef		rdEvent;	/* readable event */
    EventRef		wrEvent;	/* writable event */
    char		*out;		/* output queue */
    int			outLen;		/* output queue length */
    struct chattimer	*timers;	/* pending timers */
    struct chatmatch	*matches;	/* pending matches */
    struct chatframe	*stack;		/* call/return stack */
    struct chatvar	*globals;	/* global variables */
    struct chatvar	*temps;		/* temporary variables */
    char		*lastLog;	/* last message logged */
    char		*scriptName;	/* name of script we're running */
    char		lineBuf[CHAT_MAX_LINE];		/* line buffer */
    char		readBuf[CHAT_READBUF_SIZE];	/* read buffer */
    int			readBufLen;
    chatlogfunc_t	log /*__printflike(2, 3)*/;
    chatbaudfunc_t	setBaudrate;
    chatmallocfunc_t	malloc;
    chatfreefunc_t	free;
    chatresultfunc_t	result;
  };

/*
 * INTERNAL VARIABLES
 */

  static const struct chatcmd	gCmds[] =
  {
    { IF,	CMD_IF,		4, 0 },
    { SET,	CMD_SET,	3, 3 },
    { MATCH,	CMD_MATCH,	2, 4 },
    { REGEX,	CMD_REGEX,	2, 4 },
    { TIMER,	CMD_TIMER,	2, 4 },
    { CANCEL,	CMD_CANCEL,	2, 0 },
    { PRINT,	CMD_PRINT,	2, 2 },
    { CALL,	CMD_CALL,	2, 2 },
    { LOG,	CMD_LOG,	2, 2 },
    { RETURN,	CMD_RETURN,	1, 1 },
    { GOTO,	CMD_GOTO,	2, 2 },
    { SUCCESS,	CMD_SUCCESS,	1, 1 },
    { FAILURE,	CMD_FAILURE,	1, 1 },
    { WAIT,	CMD_WAIT,	1, 2 },
    { NOTHING,	CMD_NOTHING,	1, 1 },
  };

  #define CHAT_NUM_COMMANDS	(sizeof(gCmds) / sizeof(*gCmds))

/*
 * INTERNAL FUNCTIONS
 */

  static void	ChatAddTimer(ChatInfo c, const char *set,
			u_int secs, const char *label);
  static void	ChatAddMatch(ChatInfo c, int exact,
			const char *set, char *pat, const char *label);
  static void	ChatCancel(ChatInfo c, const char *set);
  static int	ChatGoto(ChatInfo c, const char *label);
  static void	ChatCall(ChatInfo c, const char *label);
  static void	ChatLog(ChatInfo c, int code, const char *string);
  static void	ChatPrint(ChatInfo c, const char *string);
  static void	ChatReturn(ChatInfo c, int seek);
  static void	ChatRun(ChatInfo c);
  static void	ChatStop(ChatInfo c);
  static void	ChatSuccess(ChatInfo c);
  static void	ChatFailure(ChatInfo c);
  static void	ChatIf(ChatInfo c, int ac, char *av[]);

  static void	ChatRead(int type, void *cookie);
  static void	ChatWrite(int type, void *cookie);
  static void	ChatTimeout(int type, void *cookie);
  static int	ChatGetCmd(ChatInfo c, const char *token, int n_args);
  static void	ChatDoCmd(ChatInfo c, int ac, char *av[]);
  static void	ChatDumpReadBuf(ChatInfo c);

  static int	ChatVarSet(ChatInfo c, const char *name,
		  const char *val, int exp, int pre);
  static ChatVar ChatVarGet(ChatInfo c, const char *name);
  static ChatVar ChatVarFind(ChatVar *head, char *name);
  static int	ChatVarExtract(const char *string,
		  char *buf, int max, int strict);

  static void	ChatFreeMatch(ChatInfo c, ChatMatch match);
  static void	ChatFreeTimer(ChatInfo c, ChatTimer timer);
  static int	ChatMatchChar(struct cm_exact *ex, char ch);
  static int	ChatMatchRegex(ChatInfo c, regex_t *reg, const char *input);
  static void	ChatSetMatchVars(ChatInfo c, int exact, const char *input, ...);
  static void	ChatComputeFailure(ChatInfo c, struct cm_exact *ex);

  static int	ChatDecodeTime(ChatInfo c, char *string, u_int *secsp);
  static int	ChatSetBaudrate(ChatInfo c, const char *new);
  static char	*ChatExpandString(ChatInfo c, const char *string);

  static char	*ChatReadLine(ChatInfo c);
  static int	ChatParseLine(ChatInfo c, char *line, char *av[], int max);
  static int	ChatSeekToLabel(ChatInfo c, const char *label);
  static void	ChatDumpBuf(ChatInfo c, const char *buf,
		  int len, const char *fmt, ...);

/*
 * ChatInit()
 */

ChatInfo
ChatInit(void *arg, chatbaudfunc_t setBaudrate,
	chatlogfunc_t logger, chatmallocfunc_t malloc, chatfreefunc_t free)
{
  ChatInfo	c;

  c = (ChatInfo) (*malloc)(arg, sizeof(*c));
  memset(c, 0, sizeof(*c));
  c->arg = arg;
  c->setBaudrate = setBaudrate;
  c->log = logger;
  c->malloc = malloc;
  c->free = free;
  return(c);
}

/*
 * ChatActive()
 */

int
ChatActive(ChatInfo c)
{
  return(c->state != CHAT_IDLE);
}

/*
 * ChatPresetVar()
 */

void
ChatPresetVar(ChatInfo c, const char *name, const char *value)
{
  if (ChatVarSet(c, name, value, 1, 1) < 0)
  {
    (*c->log)(c->arg, CHAT_LG_ERROR, "chat: invalid variable \"%s\"", name);
    return;
  }
}

/*
 * ChatGetVar()
 *
 * Caller must free the returned value
 */

char *
ChatGetVar(ChatInfo c, const char *var)
{
  ChatVar	const cv = ChatVarGet(c, var);

  return cv ?
    strcpy((char *) (*c->malloc)(c->arg, strlen(cv->value) + 1), cv->value) :
    NULL;
}

/*
 * ChatStart()
 *
 * Start executing chat script at label, or at beginning of file
 * if no label specified. The "fd" must be set to non-blocking I/O.
 */

void
ChatStart(ChatInfo c, int fd, FILE *scriptfp,
	const char *label, chatresultfunc_t result)
{
  ChatVar	baud;
  const char	*labelName;

/* Sanity */

  assert(c->fp == NULL);
  assert(c->state == CHAT_IDLE);

  if (label)
    (*c->log)(c->arg, CHAT_LG_DEBUG, "running script at label \"%s\"", label);
  else
    (*c->log)(c->arg, CHAT_LG_DEBUG, "running script from beginning");

  c->result = result;
  c->fd = fd;
  c->fp = scriptfp;
  c->lineNum = 0;

/* Save script name */

  assert(!c->scriptName);
  labelName = label ? label : "<default>";
  c->scriptName =
    strcpy((char *) (*c->malloc)(c->arg, strlen(labelName) + 1), labelName);

/* Jump to label, if any */

  if (label && ChatGoto(c, label) < 0)
    return;

/* Make sure serial port baudrate and $Baudrate variable agree */

  if ((baud = ChatVarGet(c, CHAT_VAR_BAUDRATE)))
    ChatSetBaudrate(c, baud->value);

/* Start running script */

  ChatRun(c);
}

/*
 * ChatAbort()
 */

void
ChatAbort(ChatInfo c)
{
  if (c->state != CHAT_IDLE)
    (*c->log)(c->arg, CHAT_LG_DEBUG, "script halted");
  ChatStop(c);
}

/*
 * ChatRead()
 */

static void
ChatRead(int type, void *cookie)
{
  ChatInfo	const c = (ChatInfo) cookie;
  ChatMatch	match;
  int		nread, lineBufLen;
  char		ch;

/* Sanity */

  assert(c->state == CHAT_WAIT);

/* Process one byte at a time */

  for (lineBufLen = strlen(c->lineBuf); 1; )
  {

  /* Input next character */

    if ((nread = read(c->fd, &ch, 1)) < 0)
    {
      if (errno == EAGAIN)
	break;
      (*c->log)(c->arg, CHAT_LG_ERROR, "read: %s", strerror(errno));
      goto die;
    }
    else if (nread == 0)
    {
      (*c->log)(c->arg, CHAT_LG_NORMAL, "detected EOF from device");
die:
      ChatFailure(c);
      return;
    }

  /* Add to "bytes read" buffer for later debugging display */

    if (c->readBufLen == sizeof(c->readBuf) || ch == '\n')
      ChatDumpReadBuf(c);
    c->readBuf[c->readBufLen++] = ch;

  /* Add to current line buffer */

    if (lineBufLen < sizeof(c->lineBuf) - 1) {
      c->lineBuf[lineBufLen++] = ch;
    } else {
      (*c->log)(c->arg, CHAT_LG_NORMAL, "warning: line buffer overflow");
    }

  /* Try to match a match pattern */

    for (match = c->matches; match; match = match->next) {
      if (match->exact) {
	if (ChatMatchChar(&match->u.exact, ch)) {
	  ChatSetMatchVars(c, 1, match->u.exact.pat);
	  break;
	}
      } else {
	const int	nmatch = match->u.regex.re_nsub + 1;
	regmatch_t	*pmatch;
	int		r, flags = (REG_STARTEND | REG_NOTEOL);

      /* Check for end of line */

	pmatch = (*c->malloc)(c->arg, nmatch * sizeof(*pmatch));
	memset(pmatch, 0, nmatch * sizeof(*pmatch));
	pmatch[0].rm_so = 0;
	pmatch[0].rm_eo = lineBufLen;
	if (pmatch[0].rm_eo > 0 && c->lineBuf[pmatch[0].rm_eo - 1] == '\n') {
	  pmatch[0].rm_eo--;
	  flags &= ~REG_NOTEOL;		/* this is a complete line */
	  if (pmatch[0].rm_eo > 0 && c->lineBuf[pmatch[0].rm_eo - 1] == '\r')
	    pmatch[0].rm_eo--;		/* elide the CR byte too */
	}

      /* Do comparison */

	switch ((r = regexec(&match->u.regex,
		c->lineBuf, nmatch, pmatch, flags))) {
	  default:
	    (*c->log)(c->arg, CHAT_LG_ERROR, "regexec() returned %d?", r);
	    /* fall through */
	  case REG_NOMATCH:
	    (*c->free)(c->arg, pmatch);
	    continue;
	  case 0:
	    ChatSetMatchVars(c, 0, c->lineBuf, nmatch, pmatch);
	    (*c->free)(c->arg, pmatch);
	    break;
	}
	break;
      }
    }

    /* Reset line buffer after a newline */
    if (ch == '\n') {
      memset(&c->lineBuf, 0, sizeof(c->lineBuf));
      lineBufLen = 0;
    }

    /* Log match, pop the stack, and jump to target label */
    if (match) {
      char	label[CHAT_MAX_LABEL];
      int	numPop;

      ChatDumpReadBuf(c);
      (*c->log)(c->arg, CHAT_LG_DEBUG, "matched set \"%s\", goto label \"%s\"", 
    	    match->set, match->label);
      numPop = match->frameDepth;
      strlcpy(label, match->label, sizeof(label));
      ChatCancel(c, match->set);
      while (numPop-- > 0)
	ChatReturn(c, 0);
      if (ChatGoto(c, label) >= 0)
	ChatRun(c);
      return;
    }
  }

/* Reregister input event */

  EventRegister(&c->rdEvent, EVENT_READ, c->fd, 0, ChatRead, c);
}

/*
 * ChatTimeout()
 *
 * Handle one of our timers timing out
 */

static void
ChatTimeout(int type, void *cookie)
{
  ChatTimer	const timer = (ChatTimer) cookie;
  ChatInfo	const c = timer->c;
  ChatTimer	*tp;
  char		label[CHAT_MAX_LABEL];
  int		numPop;

/* Sanity */

  assert(c->state == CHAT_WAIT);

/* Locate timer in list */

  for (tp = &c->timers; *tp != timer; tp = &(*tp)->next);
  assert(*tp);
  (*c->log)(c->arg, CHAT_LG_DEBUG, "timer in set \"%s\" expired", timer->set);

/* Cancel set */

  strlcpy(label, timer->label, sizeof(label));
  numPop = timer->frameDepth;
  ChatCancel(c, timer->set);

/* Pop the stack as needed */

  while (numPop-- > 0)
    ChatReturn(c, 0);

/* Jump to target label */

  if (ChatGoto(c, label) >= 0)
    ChatRun(c);
}

/*
 * ChatRun()
 *
 * Run chat script
 */

static void
ChatRun(ChatInfo c)
{
  char	*line;

/* Sanity */

  assert(c->state != CHAT_RUNNING);

/* Cancel default set before running */

  ChatCancel(c, DEFAULT_SET);

/* Execute commands while running */

  for (c->state = CHAT_RUNNING;
    c->state == CHAT_RUNNING && (line = ChatReadLine(c)) != NULL; )
  {
    int		ac;
    char	*av[CHAT_MAX_ARGS];

  /* Skip labels */

    if (!isspace(*line))
    {
      (*c->free)(c->arg, line);
      continue;
    }

  /* Parse out line */

    ac = ChatParseLine(c, line, av, CHAT_MAX_ARGS);
    (*c->free)(c->arg, line);

  /* Do command */

    ChatDoCmd(c, ac, av);
    while (ac > 0)
      (*c->free)(c->arg, av[--ac]);
  }

/* What state are we in? */

  switch (c->state)
  {
    case CHAT_RUNNING:
      (*c->log)(c->arg, CHAT_LG_ERROR, "EOF while reading script");
      ChatFailure(c);
      break;
    case CHAT_WAIT:
      if (!EventIsRegistered(&c->rdEvent))
	EventRegister(&c->rdEvent, EVENT_READ, c->fd, 0, ChatRead, c);
      break;
    case CHAT_IDLE:
      break;
  }

/* Sanity */

  assert(c->state != CHAT_RUNNING);
}

/*
 * ChatDoCmd()
 */

static void
ChatDoCmd(ChatInfo c, int ac, char *av[])
{
  char	buf[200];
  u_int	secs;
  int	j, k;

/* Show command */

  for (buf[0] = k = 0; k < ac; k++) {
    for (j = 0; av[k][j] && isgraph(av[k][j]); j++);
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
      !*av[k] || av[k][j] ? " \"%s\"" : " %s", av[k]);
  }
  (*c->log)(c->arg, CHAT_LG_DEBUG, "line %d:%s", c->lineNum, buf);

/* Execute command */

  if (ac == 0)
    return;
  switch (ChatGetCmd(c, av[0], ac))
  {
    case CMD_IF:
      ChatIf(c, ac, av);
      break;

    case CMD_SET:
      if (ChatVarSet(c, av[1], av[2], 1, 0) < 0)
      {
	(*c->log)(c->arg, CHAT_LG_ERROR,
	  "line %d: %s: invalid variable \"%s\"", c->lineNum, SET, av[1]);
	ChatFailure(c);
      }
      break;

    case CMD_MATCH:
      switch (ac)
      {
	case 4:
	  ChatAddMatch(c, 1, av[1], av[2], av[3]);
	  break;
	case 3:
	  ChatAddMatch(c, 1, DEFAULT_SET, av[1], av[2]);
	  break;
	case 2:
	  ChatAddMatch(c, 1, DEFAULT_SET, av[1], DEFAULT_LABEL);
	  break;
	default:
	  assert(0);
      }
      break;

    case CMD_REGEX:
      switch (ac)
      {
	case 4:
	  ChatAddMatch(c, 0, av[1], av[2], av[3]);
	  break;
	case 3:
	  ChatAddMatch(c, 0, DEFAULT_SET, av[1], av[2]);
	  break;
	case 2:
	  ChatAddMatch(c, 0, DEFAULT_SET, av[1], DEFAULT_LABEL);
	  break;
	default:
	  assert(0);
      }
      break;

    case CMD_TIMER:
      switch (ac)
      {
	case 4:
	  if (!ChatDecodeTime(c, av[2], &secs))
	    ChatFailure(c);
	  else if (secs != 0)
	    ChatAddTimer(c, av[1], secs, av[3]);
	  break;
	case 3:
	  if (!ChatDecodeTime(c, av[1], &secs))
	    ChatFailure(c);
	  else if (secs != 0)
	    ChatAddTimer(c, DEFAULT_SET, secs, av[2]);
	  break;
	case 2:
	  if (!ChatDecodeTime(c, av[1], &secs))
	    ChatFailure(c);
	  else if (secs != 0)
	    ChatAddTimer(c, DEFAULT_SET, secs, DEFAULT_LABEL);
	  break;
	default:
	  assert(0);
      }
      break;

    case CMD_WAIT:
      if (ac == 2)
      {
	if (!ChatDecodeTime(c, av[1], &secs))
	  ChatFailure(c);
	else if (secs > 0)
	{
	  ChatAddTimer(c, DEFAULT_SET, secs, DEFAULT_LABEL);
	  c->state = CHAT_WAIT;
	}
	else
	  ChatCancel(c, DEFAULT_SET);  /* Simulate immediate expiration */
      }
      else
      {
	if (!c->matches && !c->timers)
	{
	  (*c->log)(c->arg, CHAT_LG_ERROR,
	    "line %d: %s with no events pending", c->lineNum, WAIT);
	  ChatFailure(c);
	}
	else
	  c->state = CHAT_WAIT;
      }
      break;

    case CMD_CANCEL:
      for (k = 1; k < ac; k++)
	ChatCancel(c, av[k]);
      break;

    case CMD_PRINT:
      ChatPrint(c, av[1]);
      break;

    case CMD_CALL:
      ChatCall(c, av[1]);
      break;

    case CMD_NOTHING:
      break;

    case CMD_LOG:
      ChatLog(c, 0, av[1]);
      break;

    case CMD_RETURN:
      ChatReturn(c, 1);
      break;

    case CMD_GOTO:
      ChatGoto(c, av[1]);
      break;

    case CMD_SUCCESS:
      ChatSuccess(c);
      break;

    case -1:
    case CMD_FAILURE:
      ChatFailure(c);
      break;

    default:
      assert(0);
  }
}

/*
 * ChatIf()
 */

static void
ChatIf(ChatInfo c, int ac, char *av[])
{
  char	*const arg1 = ChatExpandString(c, av[1]);
  char	*const arg2 = ChatExpandString(c, av[3]);
  int	proceed = 0, invert = 0;

/* Check operator */

  if (!strcmp(av[2], "==") || !strcmp(av[2], "!=")) {
    proceed = !strcmp(arg1, arg2);
    invert = (*av[2] == '!');
  } else if (!strncmp(av[2], MATCH, strlen(MATCH))
	|| !strncmp(av[2], "!" MATCH, strlen("!" MATCH))) {
    regex_t	regex;
    char	errbuf[100];
    int		errcode;

    if ((errcode = regcomp(&regex, arg2, REG_EXTENDED)) != 0) {
      regerror(errcode, &regex, errbuf, sizeof(errbuf));
      (*c->log)(c->arg, CHAT_LG_ERROR,
	"line %d: invalid regular expression \"%s\": %s",
	c->lineNum, arg2, errbuf);
    } else {
      proceed = ChatMatchRegex(c, &regex, arg1);
      regfree(&regex);
      invert = (*av[2] == '!');
    }
  } else {
    (*c->log)(c->arg, CHAT_LG_ERROR,
      "line %d: invalid operator \"%s\"", c->lineNum, av[2]);
  }
  (*c->free)(c->arg, arg1);
  (*c->free)(c->arg, arg2);

/* Do command */

  if (proceed ^ invert)
    ChatDoCmd(c, ac - 4, av + 4);
}

/*
 * ChatSuccess()
 */

static void
ChatSuccess(ChatInfo c)
{
  ChatStop(c);
  (*c->result)(c->arg, 1, NULL);
}

/*
 * ChatFailure()
 */

static void
ChatFailure(ChatInfo c)
{
  char	*reason;

  reason = c->lastLog;
  c->lastLog = NULL;
  ChatStop(c);
  (*c->result)(c->arg, 0, reason);
  (*c->free)(c->arg, reason);
}

/*
 * ChatStop()
 */

static void
ChatStop(ChatInfo c)
{
  ChatVar	var, next;

/* Free temporary variables */

  for (var = c->temps; var; var = next)
  {
    next = var->next;
    (*c->free)(c->arg, var->name);
    (*c->free)(c->arg, var->value);
    (*c->free)(c->arg, var);
  }
  c->temps = NULL;

/* Close script file */

  if (c->fp)
  {
    fclose(c->fp);
    c->fp = NULL;
  }

/* Forget active script name */

  (*c->free)(c->arg, c->scriptName);
  c->scriptName = NULL;

/* Cancel all sets */

  ChatCancel(c, CHAT_KEYWORD_ALL);

/* Cancel all input and output */

  EventUnRegister(&c->rdEvent);
  EventUnRegister(&c->wrEvent);
  if (c->out != NULL) {
    (*c->free)(c->arg, c->out);
    c->out = NULL;
    c->outLen = 0;
  }

/* Pop the stack */

  while (c->stack)
    ChatReturn(c, 0);

/* Empty read buffer and last log message buffer */

  c->readBufLen = 0;
  if (c->lastLog)
  {
    (*c->free)(c->arg, c->lastLog);
    c->lastLog = NULL;
  }

/* Done */

  c->state = CHAT_IDLE;
}

/*
 * ChatAddMatch()
 */

static void
ChatAddMatch(ChatInfo c, int exact, const char *set,
	char *pat, const char *label)
{
  ChatMatch	match, *mp;

  /* Expand pattern */
  pat = ChatExpandString(c, pat);

  /* Create new match */
  match = (ChatMatch) (*c->malloc)(c->arg, sizeof(*match));
  memset(match, 0, sizeof(*match));
  match->exact = !!exact;
  if (exact) {
    match->u.exact.pat = pat;
    match->u.exact.matched = 0;
    ChatComputeFailure(c, &match->u.exact);
  } else {
    int		errcode;
    char	errbuf[100];

    /* Convert pattern into compiled regular expression */
    errcode = regcomp(&match->u.regex, pat, REG_EXTENDED);
    (*c->free)(c->arg, pat);

    /* Check for error */
    if (errcode != 0) {
      regerror(errcode, &match->u.regex, errbuf, sizeof(errbuf));
      (*c->log)(c->arg, CHAT_LG_ERROR,
	"line %d: invalid regular expression \"%s\": %s",
	c->lineNum, pat, errbuf);
      ChatFailure(c);
      (*c->free)(c->arg, match);
      return;
    }
  }
  match->set = ChatExpandString(c, set);
  match->label = ChatExpandString(c, label);

/* Add at the tail of the list, so if there's a tie, first match defined wins */

  for (mp = &c->matches; *mp; mp = &(*mp)->next);
  *mp = match;
  match->next = NULL;
}

/*
 * ChatAddTimer()
 */

static void
ChatAddTimer(ChatInfo c, const char *set, u_int secs, const char *label)
{
  ChatTimer	timer;

/* Add new timer */

  timer = (ChatTimer) (*c->malloc)(c->arg, sizeof(*timer));
  memset(timer, 0, sizeof(*timer));
  timer->c = c;
  timer->set = ChatExpandString(c, set);
  timer->label = ChatExpandString(c, label);
  timer->next = c->timers;
  c->timers = timer;

/* Start it */

  EventRegister(&timer->event, EVENT_TIMEOUT,
    secs * 1000, 0, ChatTimeout, timer);
}

/*
 * ChatCancel()
 */

static void
ChatCancel(ChatInfo c, const char *set0)
{
  int		all;
  char		*set;
  ChatMatch	match, *mp;
  ChatTimer	timer, *tp;

/* Expand set name; check for special "all" keyword */

  set = ChatExpandString(c, set0);
  all = !strcasecmp(set, CHAT_KEYWORD_ALL);

/* Nuke matches */

  for (mp = &c->matches; (match = *mp) != NULL; ) {
    if (all || !strcmp(match->set, set)) {
      *mp = match->next;
      ChatFreeMatch(c, match);
    } else
      mp = &match->next;
  }

/* Nuke timers */

  for (tp = &c->timers; (timer = *tp) != NULL; ) {
    if (all || !strcmp(timer->set, set)) {
      *tp = timer->next;
      ChatFreeTimer(c, timer);
    } else
      tp = &timer->next;
  }

/* Done */

  (*c->free)(c->arg, set);
}

/*
 * ChatGoto()
 */

static int
ChatGoto(ChatInfo c, const char *label0)
{
  const int	lineNum = c->lineNum;
  char		*label;
  int		rtn;

/* Expand label */

  label = ChatExpandString(c, label0);

/* Default label means "here" */

  if (!strcmp(label, DEFAULT_LABEL))
  {
    (*c->free)(c->arg, label);
    return(0);
  }

/* Search script file */

  if ((rtn = ChatSeekToLabel(c, label)) < 0)
  {
    (*c->log)(c->arg, CHAT_LG_ERROR,
      "line %d: label \"%s\" not found", lineNum, label);
    ChatFailure(c);
  }
  (*c->free)(c->arg, label);
  return(rtn);
}

/*
 * ChatCall()
 */

static void
ChatCall(ChatInfo c, const char *label0)
{
  ChatFrame	frame;
  ChatMatch	match;
  ChatTimer	timer;
  char		*label;

/* Expand label */

  label = ChatExpandString(c, label0);

/* Adjust stack */

  frame = (ChatFrame) (*c->malloc)(c->arg, sizeof(*frame));
  memset(frame, 0, sizeof(*frame));
  fgetpos(c->fp, &frame->posn);
  frame->lineNum = c->lineNum;
  frame->up = c->stack;
  c->stack = frame;

/* Find label */

  if (ChatSeekToLabel(c, label) < 0)
  {
    (*c->log)(c->arg, CHAT_LG_ERROR,
      "line %d: %s: label \"%s\" not found", frame->lineNum, CALL, label);
    ChatFailure(c);
  }
  (*c->free)(c->arg, label);

/* Increment call depth for timer and match events */

  for (match = c->matches; match; match = match->next)
    match->frameDepth++;
  for (timer = c->timers; timer; timer = timer->next)
    timer->frameDepth++;
}

/*
 * ChatReturn()
 */

static void
ChatReturn(ChatInfo c, int seek)
{
  ChatFrame	frame;
  ChatMatch	*mp, match;
  ChatTimer	*tp, timer;

  if (c->stack == NULL)
  {
    (*c->log)(c->arg, CHAT_LG_ERROR,
      "line %d: %s without corresponding %s", c->lineNum, RETURN, CALL);
    ChatFailure(c);
    return;
  }

/* Pop frame */

  frame = c->stack;
  if (seek) {
    fsetpos(c->fp, &frame->posn);
    c->lineNum = frame->lineNum;
  }
  c->stack = frame->up;
  (*c->free)(c->arg, frame);

/* Decrement call depth for timer and match events */

  for (mp = &c->matches; (match = *mp) != NULL; ) {
    if (--match->frameDepth < 0) {
      *mp = match->next;
      ChatFreeMatch(c, match);
    } else
      mp = &match->next;
  }
  for (tp = &c->timers; (timer = *tp) != NULL; ) {
    if (--timer->frameDepth < 0) {
      *tp = timer->next;
      ChatFreeTimer(c, timer);
    } else
      tp = &timer->next;
  }
}

/*
 * ChatLog()
 */

static void
ChatLog(ChatInfo c, int code, const char *string)
{
  char	*exp_string;

  exp_string = ChatExpandString(c, string);
  (*c->log)(c->arg, CHAT_LG_NORMAL, "%s", exp_string);
  if (c->lastLog)
    (*c->free)(c->arg, c->lastLog);
  c->lastLog = exp_string;
}

/*
 * ChatPrint()
 */

static void
ChatPrint(ChatInfo c, const char *string)
{
  char	*exp_string, *buf;
  int	exp_len;

/* Add variable-expanded string to output queue */

  exp_len = strlen(exp_string = ChatExpandString(c, string));
  buf = (*c->malloc)(c->arg, c->outLen + exp_len);
  if (c->out != NULL) {
    memcpy(buf, c->out, c->outLen);
    (*c->free)(c->arg, c->out);
  } else
    assert(c->outLen == 0);
  memcpy(buf + c->outLen, exp_string, exp_len);
  c->out = buf;
  c->outLen += exp_len;

/* Debugging dump */

  ChatDumpBuf(c, exp_string, exp_len, "sending");
  (*c->free)(c->arg, exp_string);

/* Simulate a writable event to get things going */

  ChatWrite(EVENT_WRITE, c);
}

/*
 * ChatWrite()
 */

static void
ChatWrite(int type, void *cookie)
{
  ChatInfo	const c = (ChatInfo) cookie;
  int		nw;

/* Write as much as we can */

  assert(c->out != NULL && c->outLen > 0);
  if ((nw = write(c->fd, c->out, c->outLen)) < 0) {
    if (errno == EAGAIN)
      return;
    (*c->log)(c->arg, CHAT_LG_ERROR, "write: %s", strerror(errno));
    ChatFailure(c);
    return;
  }

/* Update output queue */

  c->outLen -= nw;
  if (c->outLen <= 0) {
    (*c->free)(c->arg, c->out);
    c->out = NULL;
    c->outLen = 0;
  } else {
    memmove(c->out, c->out + nw, c->outLen);
    EventRegister(&c->wrEvent, EVENT_WRITE, c->fd, 0, ChatWrite, c);
  }
}

/*
 * ChatSetBaudrate()
 */

static int
ChatSetBaudrate(ChatInfo c, const char *new)
{
  char	*endptr;
  int	rate = strtoul(new, &endptr, 0);

  if (!*new || *endptr || (*c->setBaudrate)(c->arg, rate) < 0)
  {
    (*c->log)(c->arg, CHAT_LG_NORMAL,
      "line %d: invalid baudrate \"%s\"", c->lineNum, new);
    ChatFailure(c);
    return(-1);
  }
  return(0);
}

/*
 * ChatVarSet()
 */

static int
ChatVarSet(ChatInfo c, const char *rname,
	const char *value, int expand, int pre)
{
  ChatVar	var, *head;
  char		*new, name[CHAT_MAX_VARNAME];

/* Check & extract variable name */

  assert(rname && value);
  if (!*rname || ChatVarExtract(rname + 1, name, sizeof(name), 1) < 0)
    return(-1);
  head = isupper(*name) ? &c->globals : &c->temps;

/* Create new value */

  if (expand)
    new = ChatExpandString(c, value);
  else
  {
    new = (*c->malloc)(c->arg, strlen(value) + 1);
    strcpy(new, value);
  }

/* Check for special variable names */

  if (!strcmp(rname, CHAT_VAR_BAUDRATE))
  {
    if (!pre && ChatSetBaudrate(c, new) < 0)
    {
      (*c->free)(c->arg, new);
      return(0);
    }
  }

/* Find variable if it exists, otherwise create and add to list */

  if ((var = ChatVarFind(head, name)))
  {
    char	*ovalue;

  /* Replace value; free it after expanding (might be used in expansion) */

    ovalue = var->value;
    var->value = new;
    (*c->free)(c->arg, ovalue);
  }
  else
  {

  /* Create new struct and add to list */

    var = (*c->malloc)(c->arg, sizeof(*var));
    memset(var, 0, sizeof(*var));
    var->name = (*c->malloc)(c->arg, strlen(name) + 1);
    strcpy(var->name, name);
    var->value = new;
    var->next = *head;
    *head = var;
  }
  return(0);
}

/*
 * ChatVarGet()
 */

static ChatVar
ChatVarGet(ChatInfo c, const char *rname)
{
  char	name[CHAT_MAX_VARNAME];

/* Check & extract variable name */

  if (!*rname || ChatVarExtract(rname + 1, name, sizeof(name), 1) < 0)
    return(NULL);
  return(ChatVarFind(isupper(*name) ? &c->globals : &c->temps, name));
}

/*
 * ChatVarFind()
 */

static ChatVar
ChatVarFind(ChatVar *head, char *name)
{
  ChatVar	var, *vp;

  assert(head && name);
  for (vp = head;
    (var = *vp) && strcmp(var->name, name);
    vp = &var->next);
  if (var)
  {
    *vp = var->next;		/* caching */
    var->next = *head;
    *head = var;
  }
  return(var);
}

/*
 * ChatVarExtract()
 */

static int
ChatVarExtract(const char *string, char *buf, int max, int strict)
{
  int	k, len, brace;

/* Get variable name, optionally surrounded by braces */

  k = brace = (*string == '{');
  if (!isalpha(string[k]))
    return(-1);
  for (len = 0, k = brace;
      (isalnum(string[k]) || string[k] == '_') && len < max - 1;
      k++)
    buf[len++] = string[k];
  buf[len] = 0;
  if (len == 0 || (brace && string[k] != '}') || (strict && string[k + brace]))
    return(-1);
  return(len + brace * 2);
}

/*
 * ChatFreeMatch()
 */

static void
ChatFreeMatch(ChatInfo c, ChatMatch match)
{
  (*c->free)(c->arg, match->set);
  (*c->free)(c->arg, match->label);
  if (match->exact) {
    (*c->free)(c->arg, match->u.exact.pat);
    (*c->free)(c->arg, match->u.exact.fail);
  } else {
    regfree(&match->u.regex);
  }
  (*c->free)(c->arg, match);
}

/*
 * ChatFreeTimer()
 */

static void
ChatFreeTimer(ChatInfo c, ChatTimer timer)
{
  EventUnRegister(&timer->event);
  (*c->free)(c->arg, timer->set);
  (*c->free)(c->arg, timer->label);
  (*c->free)(c->arg, timer);
}

/*
 * ChatDumpReadBuf()
 *
 * Display accumulated input bytes for debugging purposes. Reset buffer.
 */

static void
ChatDumpReadBuf(ChatInfo c)
{
  if (c->readBufLen)
  {
    ChatDumpBuf(c, c->readBuf, c->readBufLen, "read");
    c->readBufLen = 0;
  }
}

/*
 * ChatGetCmd()
 */

static int
ChatGetCmd(ChatInfo c, const char *token, int n_args)
{
  int	k;

  for (k = 0; k < CHAT_NUM_COMMANDS; k++)
    if (!strcasecmp(gCmds[k].name, token))
    {
      if ((gCmds[k].min && n_args < gCmds[k].min)
	|| (gCmds[k].max && n_args > gCmds[k].max))
      {
	(*c->log)(c->arg, CHAT_LG_ERROR,
	  "line %d: %s: bad argument count", c->lineNum, token);
	return(-1);
      }
      else
	return(gCmds[k].id);
    }
  (*c->log)(c->arg, CHAT_LG_ERROR,
    "line %d: unknown command \"%s\"", c->lineNum, token);
  return(-1);
}

/*
 * ChatExpandString()
 *
 * Expand variables in string. Return result in malloc'd buffer.
 */

static char *
ChatExpandString(ChatInfo c, const char *string)
{
  ChatVar	var;
  int		k, j, new_len, nlen, doit = 0;
  char		*new = NULL, nbuf[CHAT_MAX_VARNAME];

/* Compute expanded length and then rescan and expand */

  new_len = 0;
rescan:
  for (j = k = 0; string[k]; k++)
    switch (string[k])
    {
      case CHAT_VAR_PREFIX:
	*nbuf = string[k];
	if (string[k + 1] == CHAT_VAR_PREFIX)	/* $$ -> $ */
	{
	  k++;
	  goto normal;
	}
	if ((nlen = ChatVarExtract(string + k + 1,
	    nbuf + 1, sizeof(nbuf) - 1, 0)) < 0)
	  goto normal;
	k += nlen;
	if ((var = ChatVarGet(c, nbuf)))
	{
	  if (doit)
	  {
	    memcpy(new + j, var->value, strlen(var->value));
	    j += strlen(var->value);
	  }
	  else
	    new_len += strlen(var->value);
	}
	break;
      default: normal:
	if (doit)
	  new[j++] = string[k];
	else
	  new_len++;
	break;
    }

/* Allocate and rescan */

  if (!doit)
  {
    new = (*c->malloc)(c->arg, new_len + 1);
    doit = 1;
    goto rescan;
  }

/* Done */

  new[j] = 0;
  assert(j == new_len);
  return(new);
}

/*
 * ChatSetMatchVars()
 *
 * Set the various special match string variables after a successful
 * match (of either type, exact or regular expression).
 */

static void
ChatSetMatchVars(ChatInfo c, int exact, const char *input, ...)
{
  const int	preflen = strlen(CHAT_VAR_MATCHED);
  ChatVar	var, *vp;
  va_list	args;

  /* Unset old match variables */
  for (vp = &c->temps; (var = *vp) != NULL; ) {
    char	*eptr;

    if (strncmp(var->name, CHAT_VAR_MATCHED, preflen)) {
      vp = &var->next;
      continue;
    }
    (void) strtoul(var->name + preflen, &eptr, 10);
    if (*eptr) {
      vp = &var->next;
      continue;
    }
    *vp = var->next;
    (*c->free)(c->arg, var->name);
    (*c->free)(c->arg, var->value);
    (*c->free)(c->arg, var);
  }

  /* Set new match variables */
  va_start(args, input);
  if (exact) {
    ChatVarSet(c, CHAT_VAR_MATCHED, input, 0, 0);
  } else {
    const int	nmatch = va_arg(args, int);
    regmatch_t	*const pmatch = va_arg(args, regmatch_t *);
    char	*const value = (*c->malloc)(c->arg, strlen(input) + 1);
    int		k;

    for (k = 0; k < nmatch; k++) {
      const int	len = pmatch[k].rm_eo - pmatch[k].rm_so;
      char	name[sizeof(CHAT_VAR_MATCHED) + 16];

      memcpy(value, input + pmatch[k].rm_so, len);
      value[len] = 0;
      snprintf(name, sizeof(name), "%s%d", CHAT_VAR_MATCHED, k);
      if (k == 0)
	ChatVarSet(c, CHAT_VAR_MATCHED, value, 0, 0);
      ChatVarSet(c, name, value, 0, 0);
    }
    (*c->free)(c->arg, value);
  }
  va_end(args);
}

/*
 * ChatMatchChar()
 *
 * Update "ex" given that "ch" is the next character.
 * Returns 1 if target has become fully matched.
 */

static int
ChatMatchChar(struct cm_exact *ex, char ch)
{
  const int	len = strlen(ex->pat);

  /* Account for zero length pattern string -- match on next input char */
  if (len == 0)
    return 1;

  /* Update longest-matched-prefix-length based on next char */
  assert(ex->matched < len);
  while (1) {
    if (ch == ex->pat[ex->matched])
      return ++ex->matched == len;
    if (ex->matched == 0)
      return 0;
    ex->matched = ex->fail[ex->matched];
  }
}

/*
 * ChatMatchRegex()
 *
 * See if a line matches a regular expression. If so, return 1
 * and set the corresponding match variables.
 */

static int
ChatMatchRegex(ChatInfo c, regex_t *reg, const char *input)
{
  const int	nmatch = reg->re_nsub + 1;
  regmatch_t	*pmatch = (*c->malloc)(c->arg, nmatch * sizeof(*pmatch));
  int		rtn, match;

  memset(pmatch, 0, nmatch * sizeof(*pmatch));
  switch ((rtn = regexec(reg, input, nmatch, pmatch, 0))) {
    default:
      (*c->log)(c->arg, CHAT_LG_ERROR, "regexec() returned %d?", rtn);
      /* fall through */
    case REG_NOMATCH:
      match = 0;
      break;
    case 0:
      ChatSetMatchVars(c, 0, input, nmatch, pmatch);
      match = 1;
      break;
  }
  (*c->free)(c->arg, pmatch);
  return match;
}

/*
 * ChatComputeFailure()
 *
 * Compute the failure function for the exact match pattern string.
 * That is, for each character position i=0..n-1, compute fail(i) =
 * 0 if i==0, else fail(i) = the greatest j < i such that characters
 * 0..j match characters (i - j)..(i - 1).
 *
 * There are linear time ways to compute this, but we're lazy.
 */

static void
ChatComputeFailure(ChatInfo c, struct cm_exact *ex)
{
  const int	len = strlen(ex->pat);
  int		i, j, k;

  ex->fail = (u_short *) (*c->malloc)(c->arg, len * sizeof(*ex->fail));
  memset(ex->fail, 0, len * sizeof(*ex->fail));
  for (i = 1; i < len; i++) {
    for (j = i - 1; j > 0; j--) {
      for (k = 0; k < j && ex->pat[k] == ex->pat[i - j + k]; k++);
      if (k == j) {
	ex->fail[i] = j;
	break;
      }
    }
  }
}

/*
 * ChatDecodeTime()
 */

static int
ChatDecodeTime(ChatInfo c, char *string, u_int *secsp)
{
  u_long	secs;
  char		*secstr, *mark;

  secstr = ChatExpandString(c, string);
  secs = strtoul(secstr, &mark, 0);
  (*c->free)(c->arg, secstr);
  if (mark == secstr) {
    (*c->log)(c->arg, CHAT_LG_ERROR,
      "line %d: illegal value \"%s\"", c->lineNum, string);
    return(0);
  }
  *secsp = (u_int) secs;
  return(1);
}

/*
 * ChatReadLine()
 */

static char *
ChatReadLine(ChatInfo c)
{
  return ReadFullLine(c->fp, &c->lineNum, NULL, 0);
}

/*
 * ChatParseLine()
 */

static int
ChatParseLine(ChatInfo c, char *line, char *av[], int max)
{
  return ParseLine(line, av, max, 1);
}

/*
 * ChatSeekToLabel()
 */

static int
ChatSeekToLabel(ChatInfo c, const char *label)
{
  return SeekToLabel(c->fp, label, &c->lineNum, NULL);
}

/*
 * ChatDumpBuf()
 */

#define DUMP_BYTES_PER_LINE   16

static void
ChatDumpBuf(ChatInfo c, const char *buf, int len, const char *fmt, ...)
{
  va_list	args;
  char		cbuf[128];
  int		k, pos;

  /* Do label */
  va_start(args, fmt);
  vsnprintf(cbuf, sizeof(cbuf), fmt, args);
  va_end(args);
  (*c->log)(c->arg, CHAT_LG_DEBUG, "%s", cbuf);

  /* Do bytes */
  for (pos = 0; pos < len; pos += DUMP_BYTES_PER_LINE) {
    *cbuf = '\0';
    for (k = 0; k < DUMP_BYTES_PER_LINE; k++) {
	if (pos + k < len) {
	    snprintf(cbuf + strlen(cbuf), sizeof(cbuf) - strlen(cbuf),
		" %02x", buf[pos + k]);
	} else {
	    snprintf(cbuf + strlen(cbuf), sizeof(cbuf) - strlen(cbuf),
		"   ");
	}
    }
    snprintf(cbuf + strlen(cbuf), sizeof(cbuf) - strlen(cbuf), "  ");
    for (k = 0; k < DUMP_BYTES_PER_LINE; k++) {
	if (pos + k < len) {
	    snprintf(cbuf + strlen(cbuf), sizeof(cbuf) - strlen(cbuf),
		"%c", isprint(buf[pos + k]) ? buf[pos + k] : '.');
	} else {
	    snprintf(cbuf + strlen(cbuf), sizeof(cbuf) - strlen(cbuf),
		" ");
	}
    }
    (*c->log)(c->arg, CHAT_LG_DEBUG, "%s", cbuf);
  }
}


