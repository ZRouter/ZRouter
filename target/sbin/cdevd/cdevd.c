/*-
 * Copyright (c) 2002-2003 M. Warner Losh.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * DEVD control daemon.
 */


#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <net/if.h>
#include <net/if_media.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <libutil.h>
#include <regex.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



#define SYSCTL "hw.bus.devctl_disable"
#define PATH_DEVCTL	"/dev/devctl"
#define PATH_CDEVD_CONF	"/etc/cdevd.conf"
#define PATH_CDEVD_CONF_CWD	"cdevd.conf"
#define DEVCTL_MAXBUF	1025

struct config_line {
	char * regex;
	char * handler;
	struct config_line *next;
};

struct config_line *cl_first = 0;
struct config_line *cl_current = 0;

static void event_loop(void);

static int must_exit = 0;

static int reg_match(char *regexp, char *line)
{
	char buf[1024];
	int rv;
	regex_t reg;
	regmatch_t regmatch;

	if ((rv = regcomp(&reg, regexp, REG_EXTENDED /* | REG_ICASE */)) != 0) 
		return -1;

	rv = regexec(&reg, line, 1, &regmatch, 0);
	if (rv == 0) 			return 1;
	else if (rv == REG_NOMATCH) 	return 0;
	else {
		regerror(rv, &reg, buf, sizeof(buf));
		printf("Regular expression evaluation error (%s)", buf);
		return -1;
	}
	regfree(&reg);
}


static int parse_config(void)
{
	int line = 0;
	char buf[1024];
	char *m;
	FILE * fh = fopen(PATH_CDEVD_CONF, "r");

	if (!fh) fh = fopen(PATH_CDEVD_CONF_CWD, "r");

	if (!fh) {
		printf("Error open config file\n");
		return 1;
	}

	while (fgets(buf, sizeof(buf), fh)) {
		buf[1023] = '\0';

		m = strchr(buf, '#');
		if (m) *m = '\0';
		m = strchr(buf, '\n');
		if (m) *m = '\0';
		m = strchr(buf, '\r');
		if (m) *m = '\0';

		m = buf;
		while (isspace(*m)) m++;
		if (strlen(m) == 0) continue;

		if (!cl_current) {
			cl_current = cl_first = 
			    (struct config_line *)malloc(
				sizeof(struct config_line));
		} else {
			cl_current->next = 
			    (struct config_line *)malloc(
				sizeof(struct config_line));
			cl_current = cl_current->next;
		}
		cl_current->next = 0;
		cl_current->regex = malloc(strlen(buf));
		/* First "/" */
		/* XXX between ^ and "/" mayby other chars */
		m = strchr(buf, '/');
		if (!m) {
			printf("Error in config file at line %d "
			    "Open '/' not found\n", line);
			return 1;
		}
		strcpy(cl_current->regex, m + 1);
		m = strchr(cl_current->regex, '/');
		if (!m) {
			printf("Error in config file at line %d "
			    " Closing '/' not found\n", line);
			return 1;
		}
		*m++ = '\0';
		if (!isspace(*m)) {
			printf("Error in config file at line %d "
			    "no space after closing '/'\n", line);
			return 1;
		}
		*m++ = '\0';
		while (isspace(*m)) m++;

		if (*m == '\0') {
			printf("Error in config file at line %d "
			    "no handler\n", line);
			return 1;
		}
		/* Check for handler existing, will do at event */
		cl_current->handler = m;
		if ( reg_match(cl_current->regex, "") == -1 ) {
			printf("Error in config file at line %d "
			    "regexp compile error\n", line);
			return 1;
		}
#if 0
		printf("Use\n"
			"\tRegex:\t\"%s\""
			"\tHandler:\t\"%s\"\n",
			cl_current->regex,
			cl_current->handler);
#endif
		line ++;
	}
	fclose(fh);
	return 0;
}

static int
run_handler(char *name, char *line)
{
	char *argv[2], *envp[2], env[1024];
	int pid, wpid, wstatus;

	snprintf(env, sizeof(env), "CDEVD_EVENT=%s", line);
	envp[0] = env;
	envp[1] = NULL;

	argv[0] = name;
	argv[1] = NULL;

	pid = fork();
	if (pid < 0) {
		printf("fork error\n");
		wstatus = 0;
	} else if (pid) {
		do {
			wpid = wait(&wstatus);
		} while (wpid != pid && wpid > 0);
		if (wpid < 0) {
			printf("wait error\n");
			wstatus = 0;
		}
	} else {
		execve(name, argv, envp);
		printf("error in execve (%s)\n", name);
	}

	return (wstatus & 0xff);
}

static void
gensighand(int x)
{
	must_exit++;
}


static void
process_event(char *buffer)
{
	for ( cl_current = cl_first; cl_current; cl_current = cl_current->next) {
		if ( reg_match(cl_current->regex, buffer) == 1) {
#if 0
			printf("Match"
			    "\tRegex:\t\"%s\""
			    "\tHandler:\t\"%s\"\n",
			    cl_current->regex,
			    cl_current->handler);
#endif
			run_handler(cl_current->handler, buffer);
		}
	}
}



static void
event_loop(void)
{
	int rv;
	int fd;
	char buffer[DEVCTL_MAXBUF];
	int once = 0;
	int  max_fd;
	struct timeval tv;
	fd_set fds;
	struct config_line *next;

	fd = open(PATH_DEVCTL, O_RDONLY);
	if (fd == -1)
		err(1, "Can't open devctl device %s", PATH_DEVCTL);

	if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0)
		err(1, "Can't set close-on-exec flag on devctl");

	max_fd = fd + 1;
	while (1) {
		if (must_exit) {
			for ( cl_current = cl_first; 
			    cl_current; 
			    ) {
				next = cl_current->next;
				free(cl_current->regex);
				free(cl_current);
				cl_current = next;
			}
			exit(0);
		}
		if (!once) {
			/* Check to see if we have any events pending. */
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			rv = select(fd + 1, &fds, &fds, &fds, &tv);
			/* No events -> we've processed all pending events */
			if (rv == 0) {
				daemon(0, 0);
				once++;
			}
		}
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		rv = select(max_fd, &fds, NULL, NULL, NULL);
		if (rv == -1) {
			if (errno == EINTR)
				continue;
			err(1, "select");
		}
		if (FD_ISSET(fd, &fds)) {
			rv = read(fd, buffer, sizeof(buffer) - 1);
			if (rv > 0) {
				buffer[rv] = '\0';
				while (buffer[--rv] == '\n')
					buffer[rv] = '\0';
				process_event(buffer);
			} else if (rv < 0) {
				if (errno != EINTR)
					break;
			} else {
				/* EOF */
				break;
			}
		}
	}
	close(fd);
}

static void
check_devd_enabled()
{
	int val = 0;
	size_t len;

	len = sizeof(val);
	if (sysctlbyname(SYSCTL, &val, &len, NULL, 0) != 0)
		errx(1, "devctl sysctl missing from kernel!");
	if (val) {
		warnx("Setting " SYSCTL " to 0");
		val = 0;
		sysctlbyname(SYSCTL, NULL, NULL, &val, sizeof(val));
	}
}

/*
 * main
 */
int
main(int argc, char **argv)
{
	parse_config();
	check_devd_enabled();

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, gensighand);
	signal(SIGINT, gensighand);
	signal(SIGTERM, gensighand);

	event_loop();
	return (0);
}
