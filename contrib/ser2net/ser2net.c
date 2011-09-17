/*
 *  ser2net - A program for allowing telnet connection to serial ports
 *  Copyright (C) 2001  Corey Minyard <minyard@acm.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This is the entry point for the ser2net program.  It reads
   parameters, initializes everything, then starts the select loop. */

/* TODO
 *
 * Add some type of security
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include "readconfig.h"
#include "controller.h"
#include "utils.h"
#include "selector.h"
#include "dataxfer.h"

static char *config_file = "/usr/local/etc/ser2net.conf";
static char *config_port = NULL;
static char *pid_file = NULL;
static int detach = 1;
static int debug = 0;
#ifdef USE_UUCP_LOCKING
int uucp_locking_enabled = 1;
#endif
int cisco_ios_baud_rates = 0;

selector_t *ser2net_sel;

static char *help_string =
"%s: Valid parameters are:\n"
"  -c <config file> - use a config file besides /usr/local/etc/ser2net.conf\n"
"  -C <config line> - Handle a single configuration line.  This may be\n"
"     specified multiple times for multiple lines.  This is just like a\n"
"     line in the config file.  This disables the default config file,\n"
"     you must specify a -c after the last -C to have it read a config\n"
"     file, too.\n"
"  -p <controller port> - Start a controller session on the given TCP port\n"
"  -P <file> - set location of pid file\n"
"  -n - Don't detach from the controlling terminal\n"
"  -d - Don't detach and send debug I/O to standard output\n"
#ifdef USE_UUCP_LOCKING
"  -u - Disable UUCP locking\n"
#endif
"  -b - Do CISCO IOS baud-rate negotiation, instead of RFC2217\n"
"  -v - print the program's version and exit\n";

void
reread_config(void)
{
    if (config_file) {
	syslog(LOG_INFO, "Got SIGHUP, re-reading configuration");
	readconfig(config_file);
    }
}

void
arg_error(char *name)
{
    fprintf(stderr, help_string, name);
    exit(1);
}

void
make_pidfile(char *pidfile)
{
    FILE *fpidfile;
    if (!pidfile)
	return;
    fpidfile = fopen(pidfile, "w");
    if (!fpidfile) {
	syslog(LOG_WARNING,
	       "Error opening pidfile '%s': %m, pidfile not created",
	       pidfile);
	return;
    }
    fprintf(fpidfile, "%d\n", getpid());
    fclose(fpidfile);
}

int
main(int argc, char *argv[])
{
    int i;
    int err;

    err = sel_alloc_selector(&ser2net_sel);
    if (err) {
	fprintf(stderr,
		"Could not initialize ser2net selector: '%s'\n",
		strerror(err));
	return -1;
    }

    for (i=1; i<argc; i++) {
	if ((argv[i][0] != '-') || (strlen(argv[i]) != 2)) {
	    fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
	    arg_error(argv[0]);
	}

	switch (argv[i][1]) {
	case 'n':
	    detach = 0;
	    break;

	case 'd':
	    detach = 0;
	    debug = 1;
	    break;

	case 'b':
	    cisco_ios_baud_rates = 1;
	    break;

	case 'C':
	    /* Get a config line. */
	    i++;
	    if (i == argc) {
		fprintf(stderr, "No config line specified with -C\n");
		arg_error(argv[0]);
	    }
	    handle_config_line(argv[i]);
	    config_file = NULL;
	    break;

	case 'c':
	    /* Get a config file. */
	    i++;
	    if (i == argc) {
		fprintf(stderr, "No config file specified with -c\n");
		arg_error(argv[0]);
	    }
	    config_file = argv[i];
	    break;

	case 'p':
	    /* Get the control port. */
	    i++;
	    if (i == argc) {
		fprintf(stderr, "No control port specified with -p\n");
		arg_error(argv[0]);
	    }
	    config_port = argv[i];
	    break;
	
	case 'P':
	    i++;
	    if (i == argc) {
		fprintf(stderr, "No pid file specified with -P\n");
		arg_error(argv[0]);
	    }
	    pid_file = argv[i];
	    break;

#ifdef USE_UUCP_LOCKING
	case 'u':
	    uucp_locking_enabled = 0;
	    break;
#endif

	case 'v':
	    printf("%s version %s\n", argv[0], VERSION);
	    exit(0);

	default:
	    fprintf(stderr, "Invalid option: '%s'\n", argv[i]);
	    arg_error(argv[0]);
	}
    }

    setup_sighup();
    if (config_port != NULL) {
	if (controller_init(config_port) == -1) {
	    fprintf(stderr, "Invalid control port specified with -p\n");
	    arg_error(argv[0]);
	}
    }

    if (config_file) {
	if (readconfig(config_file) == -1) {
	    return 1;
	}
    }

    if (detach) {
	int pid;

	/* Detach from the calling terminal. */
	openlog("ser2net", LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_NOTICE, "ser2net startup");
	if ((pid = fork()) > 0) {
	    exit(0);
	} else if (pid < 0) {
	    syslog(LOG_ERR, "Error forking first fork");
	    exit(1);
	} else {
	    /* setsid() is necessary if we really want to demonize */
	    setsid();
	    /* Second fork to really deamonize me. */
	    if ((pid = fork()) > 0) {
		exit(0);
	    } else if (pid < 0) {
		syslog(LOG_ERR, "Error forking second fork");
		exit(1);
	    }
	}

	/* Close all my standard I/O. */
	chdir("/");
	close(0);
	close(1);
	close(2);
    } else if (debug) {
	openlog("ser2net", LOG_PID | LOG_CONS | LOG_PERROR, LOG_DAEMON);
    }

    /* write pid file */
    make_pidfile(pid_file);

    /* Ignore SIGPIPEs so they don't kill us. */
    signal(SIGPIPE, SIG_IGN);

    set_sighup_handler(reread_config);

    sel_select_loop(ser2net_sel);

    return 0;
}

