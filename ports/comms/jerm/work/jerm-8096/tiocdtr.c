/*
 * tiocdtr.c - ioctrl(TIOCSDTR), ioctl(TIOCCDTR)
 * $Id: tiocdtr.c,v 1.7 2007/08/09 07:58:14 candy Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

static char *myname;
enum cmd_t {
	C_GET,
	C_CLEAR,
	C_SET,
};

static int
xwrite(int fd, const void *buf, size_t size)
{
	int err = write(fd, buf, size);
	if (err < 0)
		perror("write");
	return err;
}/* xwrite */

#define IOCTL(X,Y,Z) xioctl(#Y,X,Y,Z)
static int
xioctl(const char *msg, int fd, int cmd, void *arg)
{
	int err = ioctl(fd, cmd, arg);
	if (err < 0)
		perror(msg);
	return err;
}/* xioctl */

static int
print_status(int state)
{
#ifdef TIOCM_LE
	printf(" LE=%d", !!(state & TIOCM_LE));
#endif
	printf(" DTR=%d", !!(state & TIOCM_DTR));
	printf(" RTS=%d", !!(state & TIOCM_RTS));
#ifdef TIOCM_ST
	printf(" ST=%d", !!(state & TIOCM_ST));
#endif
#ifdef TIOCM_SR
	printf(" SR=%d", !!(state & TIOCM_SR));
#endif
	printf(" CTS=%d", !!(state & TIOCM_CTS));
	printf(" CAR=%d", !!(state & TIOCM_CAR));
	printf(" CD=%d", !!(state & TIOCM_CD));
	printf(" RNG=%d", !!(state & TIOCM_RNG));
	printf(" RI=%d", !!(state & TIOCM_RI));
	printf(" DSR=%d", !!(state & TIOCM_DSR));
	printf("\n");
	return 0;
}/* print_status */

static int
nain(int fd, char *cmds[])
{
	int err = 0;
	while (*cmds != NULL) {
		int state;
		if (strcmp(*cmds, "get") == 0) {
			err = IOCTL(fd, TIOCMGET, &state);
			if (err == 0)
				print_status(state);
		}
#if defined(TIOCCDTR) && defined(TIOCSDTR)
		else if (strcmp(*cmds, "clear") == 0) {
			err = IOCTL(fd, TIOCCDTR, NULL);
		}
		else if (strcmp(*cmds, "set") == 0) {
			err = IOCTL(fd, TIOCSDTR, NULL);
		}
#endif
		else if (strcmp(*cmds, "write") == 0) {
			if (cmds[1] != NULL) {
				cmds++;
				err = xwrite(fd, *cmds, strlen(*cmds));
			}
			else
				fprintf(stderr, "write message\n");
		}
		else if (strcmp(*cmds, "pause") == 0) {
			if (cmds[1] != NULL) {
				int x = strtol(*++cmds, NULL, 0);
				usleep(x * 1000);
			}
			else
				fprintf(stderr, "pause N [msec]\n");
		}
		cmds++;
	}/* while */
	return 0;
}/* nain */

static char *usage_msg =
	"usage: %s [-f /dev/cuaa?] clear | set | get | write msg | pause N[msec]\n"
	"function: clear/set ER(DTR)\n"
	;

int
main(int argc, char *argv[])
{
	int ex = 1, ch, show_usage = 0;
	char *dev = NULL;
	myname = argv[0];
	while ((ch = getopt(argc, argv, "f:V")) != EOF) {
		switch (ch) {
		default:
		case 'V':
			show_usage = 1;
			break;
		case 'f':
			dev = optarg;
			break;
		}/* switch */
	}/* while */
	if (argc - optind < 1)
		show_usage = 1;
	if (show_usage)
		fprintf(stderr, usage_msg, myname);
	else {
		int fd = 0;
		if (dev != NULL) {
			fd = open(dev, O_RDWR);
			if (fd < 0)
				perror(dev);
		}
		if (fd >= 0) {
			int err = nain(fd, &argv[optind]);
			if (err == 0)
				ex = 0;
			if (fd != 0) {
				err = close(fd);
				if (err < 0)
					perror("close");
			}
		}
	}
	return ex;
}/* main */

