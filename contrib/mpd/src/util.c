
/*
 * util.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "util.h"
#include <termios.h>

#include <netdb.h>
#include <tcpd.h>
#include <sys/wait.h>
#include <sys/sysctl.h>
#include <net/route.h>
#include <netinet/if_ether.h>

/*
 * DEFINITIONS
 */

  #define MAX_FILENAME		1000
  #define MAX_LINE_ARGS		50
  #define BIG_LINE_SIZE		1000
  #define MAX_OPEN_DELAY	2
  #define MAX_LOCK_ATTEMPTS	30

/*
 * INTERNAL VARIABLES
 */

static const u_int16_t Crc16Table[256] = {
/* 00 */    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
/* 08 */    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
/* 10 */    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
/* 18 */    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
/* 20 */    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
/* 28 */    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
/* 30 */    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
/* 38 */    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
/* 40 */    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
/* 48 */    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
/* 50 */    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
/* 58 */    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
/* 60 */    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
/* 68 */    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
/* 70 */    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
/* 78 */    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
/* 80 */    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
/* 88 */    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
/* 90 */    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
/* 98 */    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
/* a0 */    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
/* a8 */    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
/* b0 */    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
/* b8 */    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
/* c0 */    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
/* c8 */    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
/* d0 */    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
/* d8 */    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
/* e0 */    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
/* e8 */    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
/* f0 */    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
/* f8 */    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

  static FILE			*lockFp = NULL;

/*
 * INTERNAL FUNCTIONS
 */

  static int		UuLock(const char *devname);
  static int		UuUnlock(const char *devname);

  static void		Escape(char *line);
  static char		*ReadLine(FILE *fp, int *lineNum, char *result, int resultsize);

  static char		HexVal(char c);

  static void           IndexConfFile(FILE *fp, struct configfile **cf);
  
  struct configfiles	*ConfigFilesIndex=NULL;

#undef isspace
#define isspace(c) (((c)==' '||(c)=='\t'||(c)=='\n'||(c)=='\r')?1:0)

/*
 * LengthenArray()
 */

void
LengthenArray(void *array, size_t esize, int *alenp, const char *type)
{
  void **const arrayp = (void **)array;
  void *newa;

  newa = Malloc(type, (*alenp + 1) * esize);
  if (*arrayp != NULL) {
    memcpy(newa, *arrayp, *alenp * esize);
    Freee(*arrayp);
  }
  *arrayp = newa;
  (*alenp)++;
}

/*
 * ExecCmd()
 */

int
ExecCmd(int log, const char *label, const char *fmt, ...)
{
  int		rtn;
  char		cmd[BIG_LINE_SIZE];
  char		cmdn[BIG_LINE_SIZE];
  va_list	ap;

  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  strcpy(cmdn, cmd);

/* Log command on the console */

  Log(log, ("[%s] system: %s", label, cmd));

/* Hide any stdout output of command */

  snprintf(cmdn + strlen(cmdn), sizeof(cmdn) - strlen(cmdn), " >/dev/null 2>&1");

/* Do command */

  if ((rtn = system(cmdn)))
    Log(log|LG_ERR, ("[%s] system: command \"%s\" returned %d", label, cmd, rtn));

/* Return command's return value */

  return(rtn);
}

/*
 * ExecCmdNosh()
 */

int
ExecCmdNosh(int log, const char *label, const char *fmt, ...)
{
    int		rtn;
    char	cmd[BIG_LINE_SIZE];
    char	*cmdp = &(cmd[0]);
    char	*argv[256];
    char 	**arg;
    va_list	ap;

    pid_t pid, savedpid;
    int pstat;
    struct sigaction ign, intact, quitact;
    sigset_t newsigblock, oldsigblock;

    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);
  
    /* Log command on the console */
    Log(log, ("[%s] exec: %s", label, cmd));

    /* Parce args */
    for (arg = &argv[0]; (*arg = strsep(&cmdp, " \t")) != NULL;) {
	if (**arg != '\0') {
	    if (++arg >= &argv[255])
		break;
	}
    }
    *arg = NULL;

    /* Do command */

	/*
	 * Ignore SIGINT and SIGQUIT, block SIGCHLD. Remember to save
	 * existing signal dispositions.
	 */
	ign.sa_handler = SIG_IGN;
	(void)sigemptyset(&ign.sa_mask);
	ign.sa_flags = 0;
	(void)sigaction(SIGINT, &ign, &intact);
	(void)sigaction(SIGQUIT, &ign, &quitact);
	(void)sigemptyset(&newsigblock);
	(void)sigaddset(&newsigblock, SIGCHLD);
	(void)sigprocmask(SIG_BLOCK, &newsigblock, &oldsigblock);
	switch(pid = fork()) {
	case -1:			/* error */
		break;
	case 0:				/* child */
		/*
		 * Restore original signal dispositions and exec the command.
		 */
		(void)sigaction(SIGINT, &intact, NULL);
		(void)sigaction(SIGQUIT,  &quitact, NULL);
		(void)sigprocmask(SIG_SETMASK, &oldsigblock, NULL);
		close(1);
		open("/dev/null", O_WRONLY);
		close(2);
		open("/dev/null", O_WRONLY);
		execv(argv[0], argv);
		exit(127);
	default:			/* parent */
		savedpid = pid;
		do {
			pid = wait4(savedpid, &pstat, 0, (struct rusage *)0);
		} while (pid == -1 && errno == EINTR);
		break;
	}
	(void)sigaction(SIGINT, &intact, NULL);
	(void)sigaction(SIGQUIT,  &quitact, NULL);
	(void)sigprocmask(SIG_SETMASK, &oldsigblock, NULL);

	rtn = (pid == -1 ? -1 : pstat);

    if (rtn)
	Log(log|LG_ERR, ("[%s] system: command \"%s\" returned %d", label, cmd, rtn));

    /* Return command's return value */
    return(rtn);
}

/*
 * ParseLine()
 *
 * Parse arguments, respecting double quotes and backslash escapes.
 * Returns number of arguments, at most "max_args". This destroys
 * the original line. The arguments returned are Malloc()'d strings
 * which must be freed by the caller using FreeArgs().
 */

int
ParseLine(char *line, char *av[], int max_args, int copy)
{
  int	ac;
  char	*s, *arg;

/* Get args one at a time */

  for (ac = 0; ac < max_args; ac++)
  {

  /* Skip white space */

    while (*line && isspace(*line))
      line++;

  /* Done? */

    if (*line == 0)
      break;

  /* Get normal or quoted arg */

    if (*line == '"')
    {

    /* Stop only upon matching quote or NUL */

      for (arg = ++line; *line; line++)
	if (*line == '"')
	{
	  *line++ = 0;
	  break;
	}
	else if (*line == '\\' && line[1] != 0)
	{
	  strcpy(line, line + 1);
	  Escape(line);
	}
    }
    else
    {

    /* NUL terminate this argument at first white space */

      for (arg = line; *line && !isspace(*line); line++);
      if (*line)
	*line++ = 0;

    /* Convert characters */

      for (s = arg; *s; s++)
	if (*s == '\\')
	{
	  strcpy(s, s + 1);
	  Escape(s);
	}
    }

  /* Make a copy of this arg */

    if (copy) {
	strcpy(av[ac] = Malloc(MB_CMD, strlen(arg) + 1), arg);
    }
    else
	av[ac] = arg;
  }

#if 0
  {
    int	k;

    printf("ParseLine: %d args:\n", ac);
    for (k = 0; k < ac; k++)
      printf("  [%2d] \"%s\"\n", k, av[k]);
  }
#endif

  return(ac);
}

/*
 * FreeArgs()
 */

void
FreeArgs(int ac, char *av[])
{
  while (ac > 0)
    Freee(av[--ac]);
}

/*
 * Escape()
 *
 * Give a string, interpret the beginning characters as an escape
 * code and return with that code converted.
 */

static void
Escape(char *line)
{
  int	x, k;
  char	*s = line;

  switch (*line)
  {
    case 't': *s = '\t'; return;
    case 'n': *s = '\n'; return;
    case 'r': *s = '\r'; return;
    case 's': *s =  ' '; return;
    case '"': *s =  '"'; return;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      for (x = k = 0; k < 3 && *s >= '0' && *s <= '7'; s++)
	x = (x << 3) + (*s - '0');
      *--s = x;
      break;
    case 'x':
      for (s++, x = k = 0; k < 2 && isxdigit(*s); s++)
	x = (x << 4) + (isdigit(*s) ? (*s - '0') : (tolower(*s) - 'a' + 10));
      *--s = x;
      break;
    default:
      return;
  }
  strcpy(line, s);
}

/*
 * ReadFile()
 *
 * Read the commands specified for the target in the specified
 * file, which can be found in the PATH_CONF_DIR directory.
 * Returns negative if the file or target was not found.
 */

int
ReadFile(const char *filename, const char *target,
	int (*func)(Context ctx, int ac, char *av[], const char *file, int line), Context ctx)
{
  FILE	*fp;
  int	ac;
  char	*av[MAX_LINE_ARGS];
  char	*line;
  char  buf[BIG_LINE_SIZE];
  struct configfile *cf;
  int   lineNum;

/* Open file */

  if ((fp = OpenConfFile(filename, &cf)) == NULL)
    return(-1);

/* Find label */

  if (SeekToLabel(fp, target, &lineNum, cf) < 0) {
    fclose(fp);
    return(-1);
  }

/* Execute command list */

  while ((line = ReadFullLine(fp, &lineNum, buf, sizeof(buf))) != NULL)
  {
    if (!isspace(*line))
    {
      break;
    }
    ac = ParseLine(line, av, sizeof(av) / sizeof(*av), 0);
    (*func)(ctx, ac, av, filename, lineNum);
  }

/* Done */

  fclose(fp);
  return(0);
}

/*
 * IndexConfFile()
 *
 * Scan config file for labels
 */

static void
IndexConfFile(FILE *fp, struct configfile **cf)
{
  char	*s, *line;
  char  buf[BIG_LINE_SIZE];
  struct configfile **tmp;
  int   lineNum;

/* Start at beginning */

  rewind(fp);
  lineNum = 0;

  tmp=cf;

/* Find label */

  while ((line = ReadFullLine(fp, &lineNum, buf, sizeof(buf))) != NULL)
  {
    if (isspace(*line))
      continue;
    if ((s = strtok(line, " \t\f:"))) {
	(*tmp)=Malloc(MB_CMDL, sizeof(struct configfile));
	(*tmp)->label=strcpy(Malloc(MB_CMDL, strlen(s)+1),s);
	(*tmp)->linenum=lineNum;
	(*tmp)->seek=ftello(fp);
	tmp=&((*tmp)->next);
    }
  }
}

/*
 * SeekToLabel()
 *
 * Find a label in file and position file pointer just after it
 */

int
SeekToLabel(FILE *fp, const char *label, int *lineNum, struct configfile *cf)
{
  char	*s, *line;
  char  buf[BIG_LINE_SIZE];
  struct configfile *tmp;

  if (cf) { /* Trying to use index */
    tmp=cf;
    while (tmp && strcmp(tmp->label,label)) {
	tmp=tmp->next;
    }
    if (tmp) {
	fseeko(fp,tmp->seek, SEEK_SET);
	if (lineNum)
	    *lineNum=tmp->linenum;
	return(0);
    }
  } else { /* There are no index */
  
/* Start at beginning */
    rewind(fp);
    if (lineNum)
      *lineNum = 0;

/* Find label */

    while ((line = ReadFullLine(fp, lineNum, buf, sizeof(buf))) != NULL)
    {
      if (isspace(*line))
        continue;
      if ((s = strtok(line, " \t\f:")) && !strcmp(s, label))
	return(0);
    }
  }

/* Not found */
  Log(LG_ERR, ("Label '%s' not found", label));
  return(-1);
}

/*
 * OpenConfFile()
 *
 * Open a configuration file
 */

FILE *
OpenConfFile(const char *name, struct configfile **cf)
{
  char	pathname[MAX_FILENAME];
  FILE	*fp;
  struct configfiles **tmp;

/* Build full pathname */
    if (name[0] == '/')
	snprintf(pathname, sizeof(pathname), "%s", name);
    else
	snprintf(pathname, sizeof(pathname), "%s/%s", gConfDirectory, name);

/* Open file */

  if ((fp = fopen(pathname, "r")) == NULL)
  {
    Perror("%s: Can't open file '%s'", __FUNCTION__, pathname);
    return(NULL);
  }
  (void) fcntl(fileno(fp), F_SETFD, 1);
  
  if (cf) {
    tmp=&ConfigFilesIndex;
    while ((*tmp) && strcmp((*tmp)->filename,name)) {
	tmp=&((*tmp)->next);
    }
    if (!(*tmp)) {
	(*tmp) = Malloc(MB_CMD, sizeof(struct configfiles));
	(*tmp)->filename = strcpy(Malloc(MB_CMD, strlen(name)+1),name);
	(*tmp)->sections = NULL;
	(*tmp)->next = NULL;
	IndexConfFile(fp, &((*tmp)->sections));
    }
    *cf=(*tmp)->sections;
  }
  
  return(fp);
}

/*
 * ReadFullLine()
 *
 * Read a full line, respecting backslash continuations.
 * Returns pointer to Malloc'd storage, which must be Freee'd
 */

char *
ReadFullLine(FILE *fp, int *lineNum, char *result, int resultsize)
{
  int		len, linelen, resultlinesize, continuation;
  char		line[BIG_LINE_SIZE];
  char		real_line[BIG_LINE_SIZE];
  char		*resultline;

  if (result!=NULL && resultsize>0) {
    resultline=result;
    resultlinesize=resultsize;
  } else {
    resultline=line;
    resultlinesize=sizeof(line);
  }

  resultline[0] = 0;
  linelen = 0;
  continuation = TRUE;
  
  while ( continuation )
  {

  /* Get next real line */

    if (ReadLine(fp, lineNum, real_line, sizeof(real_line)) == NULL) {
      if (*resultline)
	break;
      else
	return(NULL);
    }

  /* Strip trailing white space, detect backslash */

    for (len = strlen(real_line);
	len > 0 && isspace(real_line[len - 1]);
	len--) {};
    real_line[len] = 0;
    
    if ((continuation = (len && real_line[len - 1] == '\\')))
	real_line[len - 1] = ' ';

  /* Append real line to what we've got so far */

    strlcpy(resultline + linelen, real_line, resultlinesize - linelen);
    linelen += len;
    if (linelen > sizeof(line) - 1)
	linelen = sizeof(line) - 1;
  }

/* Report any overflow */

  if (linelen >= sizeof(line) - 1)
    Log(LG_ERR, ("warning: line too long, truncated"));

/* Copy line and return */

  if (result!=NULL && resultsize>0)
     return resultline;
  else 
     return strcpy(Malloc(MB_CMD, linelen + 1), resultline);
}

/*
 * ReadLine()
 *
 * Read a line, skipping blank lines & comments. A comment
 * is a line whose first non-white-space character is a hash.
 */

static char *
ReadLine(FILE *fp, int *lineNum, char *result, int resultsize)
{
  int		empty;
  char		*s;
  int		ch;

    if ((!result) || (resultsize <= 0))
	return (NULL);

    /* Get first non-empty, non-commented line */
    empty = TRUE;
    while ( empty ) {

	/* Read next line from file */
	if ((fgets(result, resultsize, fp)) == NULL)
	    return(NULL);
	if (lineNum)
    	    (*lineNum)++;

	/* Truncate long lines */
	if (strlen(result) > (resultsize - 2)) {
    	    Log(LG_ERR, ("warning: line too long, truncated"));
    	    while ((ch = getc(fp)) != EOF && ch != '\n');
	}

	/* Ignore comments */
	s = result + strspn(result, " \t");
	if (*s == '#') {
    	    *s = 0;
	} else {
	    /* Is this line empty? */
    	    for ( ; *s; s++) {
    		if (!isspace(*s)) {
        	    empty = FALSE;
        	    break;
    		}
	    }
	}
    }

    return(result);
}

/*
 * OpenSerialDevice()
 *
 * Open and configure a serial device. Call ExclusiveCloseDevice()
 * to close a file descriptor returned by this function.
 */

int
OpenSerialDevice(const char *label, const char *path, int baudrate)
{
  struct termios	attr;
  int			fd;

/* Open & lock serial port */

  if ((fd = ExclusiveOpenDevice(label, path)) < 0)
    return(-1);

/* Set non-blocking I/O */

  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
  {
    Log(LG_ERR, ("[%s] can't set \"%s\" to non-blocking: %s",
      label, path, strerror(errno)));
    goto failed;
  }

/* Set serial port raw mode, baud rate, hardware flow control, etc. */

  if (tcgetattr(fd, &attr) < 0)
  {
    Log(LG_ERR, ("[%s] can't tcgetattr \"%s\": %s",
      label, path, strerror(errno)));
    goto failed;
  }

  cfmakeraw(&attr);

  attr.c_cflag &= ~(CSIZE|PARENB|PARODD);
  attr.c_cflag |= (CS8|CREAD|CLOCAL|HUPCL|CCTS_OFLOW|CRTS_IFLOW);
  attr.c_iflag &= ~(IXANY|IMAXBEL|ISTRIP|IXON|IXOFF|BRKINT|ICRNL|INLCR);
  attr.c_iflag |= (IGNBRK|IGNPAR);
  attr.c_oflag &= ~OPOST;
  attr.c_lflag = 0;

  cfsetspeed(&attr, (speed_t) baudrate);

  if (tcsetattr(fd, TCSANOW, &attr) < 0)
  {
    Log(LG_ERR, ("[%s] can't tcsetattr \"%s\": %s",
      label, path, strerror(errno)));
failed:
    ExclusiveCloseDevice(label, fd, path);
    return(-1);
  }

/* OK */

  return(fd);
}

/*
 * ExclusiveOpenDevice()
 */

int
ExclusiveOpenDevice(const char *label, const char *pathname)
{
  int		fd, locked = FALSE;
  const char	*ttyname = NULL;
  time_t	startTime;

/* Lock device UUCP style, if it resides in /dev */

  if (!strncmp(pathname, "/dev/", 5))
  {
    ttyname = pathname + 5;
    if (UuLock(ttyname) < 0)
    {
      Log(LG_ERR, ("[%s] can't lock device %s", label, ttyname));
      return(-1);
    }
    locked = TRUE;
  }

/* Open it, but give up after so many interruptions */

  for (startTime = time(NULL);
      (fd = open(pathname, O_RDWR, 0)) < 0
      && time(NULL) < startTime + MAX_OPEN_DELAY; )
    if (errno != EINTR)
    {
      Log(LG_ERR, ("[%s] can't open %s: %s",
	label, pathname, strerror(errno)));
      if (locked)
	UuUnlock(ttyname);
      return(-1);
    }

/* Did we succeed? */

  if (fd < 0)
  {
    Log(LG_ERR, ("[%s] can't open %s after %d secs",
      label, pathname, MAX_OPEN_DELAY));
    if (locked)
      UuUnlock(ttyname);
    return(-1);
  }
  (void) fcntl(fd, F_SETFD, 1);

/* Done */

  return(fd);
}

/*
 * ExclusiveCloseDevice()
 */

void
ExclusiveCloseDevice(const char *label, int fd, const char *pathname)
{
  int		rtn = -1;
  const char	*ttyname;
  time_t	startTime;

/* Close file(s) */

  for (startTime = time(NULL);
      time(NULL) < startTime + MAX_OPEN_DELAY && (rtn = close(fd)) < 0; )
    if (errno != EINTR)
    {
      Log(LG_ERR, ("[%s] can't close %s: %s",
	label, pathname, strerror(errno)));
      break;
    }

/* Did we succeed? */

  if ((rtn < 0) && (errno == EINTR))
  {
    Log(LG_ERR, ("[%s] can't close %s after %d secs",
      label, pathname, MAX_OPEN_DELAY));
    DoExit(EX_ERRDEAD);
  }

/* Remove lock */

  if (!strncmp(pathname, "/dev/", 5))
  {
    ttyname = pathname + 5;
    if (UuUnlock(ttyname) < 0)
      Log(LG_ERR, ("[%s] can't unlock %s: %s",
	label, ttyname, strerror(errno)));
  }
}

/*
 * UuLock()
 *
 * Try to atomically create lockfile. Returns negative if failed.
 */

static int
UuLock(const char *ttyname)
{
  int	fd, pid;
  char	tbuf[sizeof(PATH_LOCKFILENAME) + MAX_FILENAME];
  char	pid_buf[64];

  snprintf(tbuf, sizeof(tbuf), PATH_LOCKFILENAME, ttyname);
  if ((fd = open(tbuf, O_RDWR|O_CREAT|O_EXCL, 0664)) < 0)
  {

  /* File is already locked; Check to see if the process
   * holding the lock still exists */

    if ((fd = open(tbuf, O_RDWR, 0)) < 0)
    {
      Perror("%s: open(%s)", __FUNCTION__, tbuf);
      return(-1);
    }

    if (read(fd, pid_buf, sizeof(pid_buf)) <= 0)
    {
      (void)close(fd);
      Perror("%s: read", __FUNCTION__);
      return(-1);
    }

    pid = atoi(pid_buf);

    if (kill(pid, 0) == 0 || errno != ESRCH)
    {
      (void)close(fd);  /* process is still running */
      return(-1);
    }

  /* The process that locked the file isn't running, so we'll lock it */

    if (lseek(fd, (off_t) 0, L_SET) < 0)
    {
      (void)close(fd);
      Perror("%s: lseek", __FUNCTION__);
      return(-1);
    }
  }

/* Finish the locking process */

  sprintf(pid_buf, "%10u\n", (int) gPid);
  if (write(fd, pid_buf, strlen(pid_buf)) != strlen(pid_buf))
  {
    (void)close(fd);
    (void)unlink(tbuf);
    Perror("%s: write", __FUNCTION__);
    return(-1);
  }
  (void)close(fd);
  return(0);
}

/*
 * UuUnlock()
 */

static int
UuUnlock(const char *ttyname)
{
  char	tbuf[sizeof(PATH_LOCKFILENAME) + MAX_FILENAME];

  (void) sprintf(tbuf, PATH_LOCKFILENAME, ttyname);
  return(unlink(tbuf));
}

/*
 * GenerateMagic()
 *
 * Generate random number which will be used as magic number.
 * This could be made a little more "random"...
 */

u_long
GenerateMagic(void)
{
  time_t		now;
  struct timeval	tval;

  time(&now);
  gettimeofday(&tval, NULL);
  now += (tval.tv_sec ^ tval.tv_usec) + getppid();
  now *= gPid;
  return(now);
}

/*
 * PIDCheck()
 *
 * See if process is already running and deal with PID file.
 */

int
PIDCheck(const char *filename, int killem)
{
  int	fd = -1, n_tries;

/* Sanity */

  assert(!lockFp);

/* Atomically open and lock file */

  for (n_tries = 0;
    n_tries < MAX_LOCK_ATTEMPTS
      && (fd = open(filename, O_RDWR|O_CREAT|O_EXLOCK|O_NONBLOCK, 0644)) < 0;
    n_tries++)
  {
    int		nscan, old_pid;
    FILE	*fp;

  /* Abort on any unexpected errors */

    if (errno != EAGAIN)
    {
      Perror("%s: open(%s)", __FUNCTION__, filename);
      return(-1);
    }

  /* We're already running ... see who it is */

    if ((fp = fopen(filename, "r")) == NULL)
    {
      Perror("%s: fopen(%s)", __FUNCTION__, filename);
      return(-1);
    }

  /* If there's a PID in there, sniff it out */

    nscan = fscanf(fp, "%d", &old_pid);
    fclose(fp);
    if (nscan != 1)
    {
      Log(LG_ERR, ("%s: contents mangled", filename));
      return(-1);
    }

  /* Maybe kill the other guy */

    if (!killem)
    {
      Log(LG_ERR, ("already running as process %d", old_pid));
      return(-1);
    }
    if (kill(old_pid, SIGTERM) < 0)
      switch (errno)
      {
	case ESRCH:
	  Log(LG_ERR, ("process %d no longer exists", old_pid));
	  break;
	default:
	  Perror("%s: kill(%d)", __FUNCTION__, old_pid);
	  return(-1);
      }

  /* Wait and try again */

    Log(LG_ERR, ("waiting for process %d to die...", old_pid));
    sleep(1);
  }
  if (n_tries == MAX_LOCK_ATTEMPTS)
  {
    Log(LG_ERR, ("can't lock %s after %d attempts", filename, n_tries));
    return(-1);
  }

/* Close on exec */

  (void) fcntl(fd, F_SETFD, 1);

/* Create a stream on top of file descriptor */

  if ((lockFp = fdopen(fd, "r+")) == NULL)
  {
    Perror("%s: fdopen", __FUNCTION__);
    return(-1);
  }
  setbuf(lockFp, NULL);

/* Write my PID in there */

  rewind(lockFp);
  fprintf(lockFp, "%u\n", (u_int) gPid);
  fflush(lockFp);
  (void) ftruncate(fileno(lockFp), ftell(lockFp));
  return(0);
}

/*
 * GetInetSocket()
 *
 * Get a TCP socket and bind it to an address. Set SO_REUSEADDR on the socket.
 */

int
GetInetSocket(int type, struct u_addr *addr, in_port_t port, int block, char *ebuf, size_t len)
{
  int			sock;
  static int		one = 1;
  struct sockaddr_storage sa;

  u_addrtosockaddr(addr,port,&sa);

/* Get and bind non-blocking socket */

  if ((sock = socket(sa.ss_family, type, type == SOCK_STREAM ? IPPROTO_TCP : 0)) < 0)
  {
    snprintf(ebuf, len, "socket: %s", strerror(errno));
    return(-1);
  }
  (void) fcntl(sock, F_SETFD, 1);
  if (!block) 
  {
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
    {
      snprintf(ebuf, len, "can't set socket non-blocking: %s", strerror(errno));
      close(sock);
      return(-1);
    }
  }
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)))
  {
    snprintf(ebuf, len, "setsockopt: %s", strerror(errno));
    close(sock);
    return(-1);
  }
  
  if (bind(sock, (struct sockaddr *) &sa, sa.ss_len) < 0)
  {
    snprintf(ebuf, len, "bind: %s", strerror(errno));
    close(sock);
    return(-1);
  }
  
  return(sock);
}


/*
 * TcpGetListenPort()
 *
 * Get port for incoming telnet connections
 */

int
TcpGetListenPort(struct u_addr *addr, in_port_t port, int block)
{
  char	ebuf[100];
  int	sock;
  int	saverrno;

/* Get socket */

  if ((sock = GetInetSocket(SOCK_STREAM, addr, port, block, ebuf, sizeof(ebuf))) < 0)
  {
    saverrno = errno;
    Log(LG_ERR, ("%s", ebuf));
    errno = saverrno;
    return(-1);
  }

/* Make socket available for connections  */

  if (listen(sock, 2) < 0)
  {
    Perror("%s: listen", __FUNCTION__);
    (void) close(sock);
    return(-1);
  }

/* Done */

  return(sock);
}


/*
 * TcpAcceptConnection()
 *
 * Accept next connection on port
 */

int
TcpAcceptConnection(int sock, struct sockaddr_storage *addr, int block)
{
  int	new_sock;
  socklen_t size=sizeof(struct sockaddr_storage);

/* Accept incoming connection */

  memset(addr, 0, sizeof(*addr));
  if ((new_sock = accept(sock, (struct sockaddr *) addr, &size)) < 0) {
    Perror("%s: accept", __FUNCTION__);
    return(-1);
  }
  
#ifdef USE_WRAP
    if (Enabled(&gGlobalConf.options, GLOBAL_CONF_TCPWRAPPER)) {
      struct request_info req;
      request_init(&req, RQ_DAEMON, "mpd", RQ_FILE, new_sock, NULL);
      fromhost(&req);
      if (!hosts_access(&req)) {
	Log(LG_ERR, ("refused connection (tcp-wrapper) from %s", 
	  eval_client(&req)));
	close(new_sock);
	return(-1);
      }
    }
#endif
  
  if (!block) 
  {
    (void) fcntl(new_sock, F_SETFD, 1);
    if (fcntl(new_sock, F_SETFL, O_NONBLOCK) < 0) {
      Perror("%s: fcntl", __FUNCTION__);
      return(-1);
    }
  }

/* Done */

  return(new_sock);
}


/*
 * ShowMesg()
 */

void
ShowMesg(int log, const char *pref, const char *buf, int len)
{
  char	*s, mesg[256];

  if (len > 0)
  {
    if (len > sizeof(mesg) - 1)
      len = sizeof(mesg) - 1;
    memcpy(mesg, buf, len);
    mesg[len] = 0;
    for (s = strtok(mesg, "\r\n"); s; s = strtok(NULL, "\r\n"))
      Log(log, ("[%s]   MESG: %s", pref, s));
  }
}

/*
 * Bin2Hex()
 */

char *
Bin2Hex(const unsigned char *bin, size_t len)
{
  static char	hexconvtab[] = "0123456789abcdef";
  size_t	i, j;
  char		*buf;
  
  buf = Malloc(MB_UTIL, len * 2 + 1);
  for (i = j = 0; i < len; i++) {
    buf[j++] = hexconvtab[bin[i] >> 4];
    buf[j++] = hexconvtab[bin[i] & 15];
  }
  buf[j] = 0;
  return buf;
}

/*
 * Hex2Bin()
 */

u_char *
Hex2Bin(char *hexstr)
{
  int		i;
  u_char	*binval;

  binval = Malloc(MB_UTIL, strlen(hexstr) / 2);

  for (i = 0; i < strlen(hexstr) / 2; i++) {
    binval[i] = 16 * HexVal(hexstr[2*i]) + HexVal(hexstr[2*i+1]);
  }

  return binval;
}
 
static char
HexVal(char c)
{
  if (c >= '0' && c <= '9') {
    return (c - '0');
  } else if (c >= 'a' && c <= 'z') {
    return (c - 'a' + 10);
  } else if (c >= 'A' && c <= 'Z') {
    return (c - 'A' + 10);
  } else {
    return (-1);
  }
}

/*
 * Crc16()
 *
 * Compute the 16 bit frame check value, per RFC 1171 Appendix B,
 * on an array of bytes.
 */

u_short
Crc16(u_short crc, u_char *cp, int len)
{
  while (len--)
    crc = (crc >> 8) ^ Crc16Table[(crc ^ *cp++) & 0xff];
  return(crc);
}

/*
 * GetAnyIpAddress()
 *
 * Get any non-loopback IP address owned by this machine
 * Prefer addresses from non-point-to-point interfaces.
 */

int
GetAnyIpAddress(struct u_addr *ipaddr, const char *ifname)
{
  int			s, p2p = 0;
  struct in_addr	ipa = { 0 };
  static struct in_addr	nipa = { 0 };
  static int		have_nipa = 0;
  struct ifreq		*ifr, *ifend;
  struct ifreq		ifreq;
  struct ifconf		ifc;
  unsigned int		buffsize = IFCONF_BUFFSIZE;

    /* use cached IP to reduce number of syscalls */
    if (ifname == NULL && have_nipa) {
	in_addrtou_addr(&nipa, ipaddr);
	return(0);
    }

    /* Get socket */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	Perror("%s: Socket creation error", __FUNCTION__);
	return(-1);
    }

    /* Try simple call for the first IP on interface */
    if (ifname != NULL) {
	strlcpy(ifreq.ifr_name, ifname, sizeof(ifreq.ifr_name));
        if (ioctl(s, SIOCGIFADDR, &ifreq) < 0) {
	    if (errno != ENXIO)
    		Perror("%s: ioctl(SIOCGIFADDR)", __FUNCTION__);
    	    close(s);
    	    return(-1);
        }
	ipa = ((struct sockaddr_in *)&ifreq.ifr_ifru.ifru_addr)->sin_addr;
	if ((ntohl(ipa.s_addr)>>24) == 127)
	    ipa.s_addr = 0; 	/* We don't like 127.0.0.1 */
    }

    /* If simple is not enouth try complex call */
    if (ipa.s_addr == 0) {
      struct ifreq *ifs;
      while (1) {
        ifc.ifc_len = buffsize;
        ifc.ifc_req = ifs = Malloc(MB_UTIL, ifc.ifc_len);
        if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
          Freee(ifs);
          if (errno != ENXIO)
    	     Perror("%s: ioctl(SIOCGIFCONF)", __FUNCTION__);
          close(s);
          return(-1);
        }
        
        /* if used size is too close to allocated size retry with a larger buffer */
        if (ifc.ifc_len + 128 < buffsize)
          break;
        
         Freee(ifs);
        if (buffsize >= IFCONF_BUFFMAXSIZE) {
       	  Log(LG_ERR, ("%s: Max buffer size reached", __FUNCTION__));
          close(s);
          return(-1);
        }
        buffsize *= 2;
      }

      for (ifend = (struct ifreq *)(void *)(ifc.ifc_buf + ifc.ifc_len),
    	    ifr = ifc.ifc_req;
          ifr < ifend;
          ifr = (struct ifreq *)(void *)((char *) &ifr->ifr_addr
	    + MAX(ifr->ifr_addr.sa_len, sizeof(ifr->ifr_addr)))) {
        if (ifr->ifr_addr.sa_family == AF_INET) {

          if (ifname!=NULL && strcmp(ifname,ifr->ifr_name))
	    continue;

          /* Check that the interface is up; prefer non-p2p and non-loopback */
          strlcpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
          if (ioctl(s, SIOCGIFFLAGS, &ifreq) < 0)
    	    continue;
          if ((ifreq.ifr_flags & IFF_UP) != IFF_UP)
	    continue;
	  if ((ifreq.ifr_flags & (IFF_POINTOPOINT|IFF_LOOPBACK)) && ipa.s_addr)
	    continue;
	  if ((ntohl(((struct sockaddr_in *)(void *)&ifr->ifr_addr)->sin_addr.s_addr)>>24)==127)
	    continue;

          /* Save IP address and interface name */
          ipa = ((struct sockaddr_in *)(void *)&ifr->ifr_addr)->sin_addr;
          p2p = (ifreq.ifr_flags & (IFF_POINTOPOINT|IFF_LOOPBACK)) != 0;
      
          if (!p2p) break;
        }
      }
      Freee(ifs);
    }
    close(s);

    /* Found? */
    if (ipa.s_addr == 0)
	return(-1);
    if (ifname == NULL) {
	nipa = ipa;
	have_nipa = 1;
    }
    in_addrtou_addr(&ipa, ipaddr);
    return(0);
}

/*
 * GetEther()
 *
 * Get the hardware address of an interface on the the same subnet as addr.
 * If addr == NULL, finds the address of any local ethernet interface.
 */

int
GetEther(struct u_addr *addr, struct sockaddr_dl *hwaddr)
{
  int			s;
  struct ifreq		*ifr, *bifr, *ifend, *ifp;
  u_int32_t		ina, mask, bmask;
  struct ifreq		ifreq;
  struct ifconf		ifc;
  struct ifreq 		*ifs;
  unsigned int buffsize = IFCONF_BUFFSIZE;
  
  static struct sockaddr_dl nhwaddr;
  static int		have_nhwaddr = 0;

    /* cache value to reduce number of syscalls */
    if (addr == NULL && have_nhwaddr) {
        memcpy(hwaddr, &nhwaddr,
	    sizeof(*hwaddr));
	return(0);
    }

    /* Get interface list */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	Perror("%s: Socket creation error", __FUNCTION__);
	return(-1);
    }

    while (1) {
	ifc.ifc_len = buffsize;
	ifc.ifc_req = ifs = Malloc(MB_UTIL, ifc.ifc_len);
	if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
	    Freee(ifs);
	    Perror("%s: ioctl(SIOCGIFCONF)", __FUNCTION__);
    	    close(s);
	    return(-1);
	}
	 
	/* if used size is too close to allocated size retry with a larger buffer */
	if (ifc.ifc_len + 128 < buffsize)
	    break;
	 
	Freee(ifs);
	if (buffsize >= IFCONF_BUFFMAXSIZE) {
	    Log(LG_ERR, ("%s: Max buffer size reached", __FUNCTION__));
    	    close(s);
	    return(-1);
	}
	buffsize *= 2;
    }

  /*
   * Scan through looking for an interface with an IP
   * address on same subnet as `addr'.
   */
  bifr = NULL;
  bmask = 0;
  for (ifend = (struct ifreq *)(void *)(ifc.ifc_buf + ifc.ifc_len),
	ifr = ifc.ifc_req;
      ifr < ifend;
      ifr = (struct ifreq *)(void *)((char *) &ifr->ifr_addr
	+ MAX(ifr->ifr_addr.sa_len, sizeof(ifr->ifr_addr)))) {
    if (ifr->ifr_addr.sa_family == AF_INET) {

      /* Save IP address and interface name */
      ina = ((struct sockaddr_in *)(void *)&ifr->ifr_addr)->sin_addr.s_addr;
      strlcpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
      ifreq.ifr_addr = ifr->ifr_addr;

      /* Check that the interface is up, and not point-to-point or loopback */
      if (ioctl(s, SIOCGIFFLAGS, &ifreq) < 0)
	continue;
      if ((ifreq.ifr_flags &
	  (IFF_UP|IFF_BROADCAST|IFF_POINTOPOINT|IFF_LOOPBACK|IFF_NOARP))
	  != (IFF_UP|IFF_BROADCAST))
	continue;

      if (addr) {
        /* Get its netmask and check that it's on the right subnet */
        if (ioctl(s, SIOCGIFNETMASK, &ifreq) < 0)
	    continue;
        mask = ((struct sockaddr_in *)(void *)&ifreq.ifr_addr)->sin_addr.s_addr;
        if ((addr->u.ip4.s_addr & mask) != (ina & mask))
	    continue;
	/* Is this the best match? */
	if (mask >= bmask) {
	    bmask = mask;
	    bifr = ifr;
	}
	continue;
      }

      /* OK */
      bifr = ifr;
      break;
    }
  }
  close(s);

  /* Found? */
  if (bifr == NULL) {
    Freee(ifs);
    return(-1);
  }

  /* Now scan again looking for a link-level address for this interface */
  for (ifp = bifr, ifr = ifc.ifc_req; ifr < ifend; ) {
    if (strcmp(ifp->ifr_name, ifr->ifr_name) == 0
	&& ifr->ifr_addr.sa_family == AF_LINK) {
      if (addr == NULL) {
        memcpy(&nhwaddr, (struct sockaddr_dl *)(void *)&ifr->ifr_addr,
	    sizeof(*hwaddr));
	have_nhwaddr = 1;
      }
      memcpy(hwaddr, (struct sockaddr_dl *)(void *)&ifr->ifr_addr,
	sizeof(*hwaddr));
      Freee(ifs);
      return(0);
    }
    ifr = (struct ifreq *)(void *)((char *)&ifr->ifr_addr
      + MAX(ifr->ifr_addr.sa_len, sizeof(ifr->ifr_addr)));
  }

  /* Not found! */
  Freee(ifs);
  return(-1);
}

int
GetPeerEther(struct u_addr *addr, struct sockaddr_dl *hwaddr)
{
	int mib[6];
	size_t needed;
	char *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_inarp *sin2;
	struct sockaddr_dl *sdl;
	int st, found_entry = 0;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = addr->family;
	mib[4] = NET_RT_FLAGS;
#ifdef RTF_LLINFO
	mib[5] = RTF_LLINFO;
#else
	mib[5] = 0;
#endif
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
		Log(LG_ERR, ("route-sysctl-estimate"));
		return (0);
	}
	if (needed == 0)	/* empty table */
		return 0;
	buf = NULL;
	for (;;) {
		if (buf)
		    Freee(buf);
		buf = Malloc(MB_UTIL, needed);
		st = sysctl(mib, 6, buf, &needed, NULL, 0);
		if (st == 0 || errno != ENOMEM)
			break;
		needed += needed / 8;
	}
	if (st == -1) {
		Log(LG_ERR, ("actual retrieval of routing table"));
		Freee(buf);
		return (0);
	}
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin2 = (struct sockaddr_inarp *)(rtm + 1);
		if (addr->u.ip4.s_addr == sin2->sin_addr.s_addr) {
			sdl = (struct sockaddr_dl *)((char *)sin2 + SA_SIZE(sin2));
			memcpy(hwaddr, sdl, sdl->sdl_len);
			found_entry = 1;
			break;
		}
	}
	Freee(buf);
	return (found_entry);
}

/*
 * Decode ASCII message
 */
void
ppp_util_ascify(char *buf, size_t bsiz, const char *data, size_t len)
{
	char *bp;
	int i;

	for (bp = buf, i = 0; i < len; i++) {
		const char ch = (char)data[i];

		if (bsiz < 3)
			break;
		switch (ch) {
		case '\t':
			*bp++ = '\\';
			*bp++ = 't';
			bsiz -= 2;
			break;
		case '\n':
			*bp++ = '\\';
			*bp++ = 'n';
			bsiz -= 2;
			break;
		case '\r':
			*bp++ = '\\';
			*bp++ = 'r';
			bsiz -= 2;
			break;
		default:
			if (isprint(ch & 0x7f)) {
				*bp++ = ch;
				bsiz--;
			} else {
				*bp++ = '^';
				*bp++ = '@' + (ch & 0x1f);
				bsiz -= 2;
			}
			break;
		}
	}
	*bp = '\0';
}

