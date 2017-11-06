/*
 * jerm.c - terminal emulator
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 candy
 * $Id: jerm.c,v 1.25 2007/08/09 09:04:43 candy Exp candy $
 */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>

#ifdef __CYGWIN__
#include "getaddrinfo.c"
#endif

#ifdef USE_LIBWRAP
#include <tcpd.h>
#include <syslog.h>
int allow_severity = LOG_INFO; /* for connection logging */
int deny_severity = LOG_WARNING; /* for connection logging */

static int use_libwrap = 0; /* use libwrap or not. 0 = no, nonzero = yes */
#endif

#define PORT 8086

#define PI 3.1415926535897932

static char *myname;

enum parity_t {
	P_NONE,
	P_EVEN,
	P_ODD,
};

enum flow_t {
	F_NONE,
	F_HARD,
	F_X,
};

static int speed = B9600; /* B38400 - B75 */
static enum parity_t parity = P_NONE;
static int data_bit = CS8;
static int stop_bit = 1;
static enum flow_t flow_control = F_NONE;
static int hex_mode; /* hex dump ¥â¡¼¥É */
static int pipe_mode; /* ¥·¥ê¥¢¥ë¤Ë¿âÎ®¤· */
static int is_server = 0; /* server ¥â¡¼¥É */
static int quit_flag;

/* ¹ÔËöÊÑ´¹¤Î¥×¥ê¥ß¥Æ¥£¥Ö */
enum rn_conv_t {
	RN_DROP, /* ¼Î¤Æ¤ë */
	RN_MAP_CR, /* CR ¤ËÊÑ´¹ */
	RN_MAP_LF, /* LF ¤ËÊÑ´¹ */
	RN_MAP_CRLF, /* CR LF ¤ËÊÑ´¹ */
	RN_CONV_SIZE
};

/* rn_conv_t ¤ËÂÐ±þ¤¹¤ëÊÑ´¹ÃÍ */
static char *RN_OUTPUT[RN_CONV_SIZE] = {
	"",
	"\r",
	"\n",
	"\r\n",
};

/* rn_conv_t ¤ËÂÐ±þ¤¹¤ë¥ª¥×¥·¥ç¥óÊ¸»ú */
static char RN_STR[RN_CONV_SIZE + 1] = "xrnt";

/* ¹ÔËöÊÑ´¹¥Æ¡¼¥Ö¥ë rn_map[] ¤Î¥¤¥ó¥Ç¥¯¥¹ */
enum rn_index_t {
	RN_RECV_CR, /* ¥ê¥â¡¼¥È¤«¤é CR ¼õ¿®¤·¤¿ */
	RN_RECV_LF, /* ¥ê¥â¡¼¥È¤«¤é LF ¼õ¿®¤·¤¿ */
	RN_SEND_CR, /* ¥í¡¼¥«¥ë¤«¤é CR ¼õ¿®¤·¤¿ */
	RN_SEND_LF, /* ¥í¡¼¥«¥ë¤«¤é LF ¼õ¿®¤·¤¿ */
	RN_INDEX_SIZE
};

static enum rn_conv_t rn_map[RN_INDEX_SIZE] = {
	RN_MAP_CR,
	RN_MAP_LF,
	RN_MAP_CR,
	RN_MAP_LF,
};


/*
 * select on 2 file descriptors.
 */
static int
sel2(fd_set *m, int f1, int f2)
{
	int maxfd = f1 > f2 ? f1 : f2;
	FD_ZERO(m);
	FD_SET(f1, m);
	FD_SET(f2, m);
	errno = 0;
	return select(maxfd + 1, m, NULL, NULL, NULL);
}/* sel2 */

/*
 * host:port ¤Ë TCP ÀÜÂ³¡£
 * ¥½¥±¥Ã¥È¤òÊÖ¤¹¡£
 */
static int
conn(const char *host, int port, int family)
{
	int so = -1;
	int err;
	char servname[16];
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	sprintf(servname, "%d", port);
	err = getaddrinfo(host, servname, &hints, &res);
	if (err == 0) {
		if (res != NULL) {
			so = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (so >= 0) {
				int err = connect(so, res->ai_addr, res->ai_addrlen);
				if (err < 0) {
					perror("connect");
					close(so);
					so = -1;
				}
			}
			else
				perror("socket");
			freeaddrinfo(res);
		}
		else
			fprintf(stderr, "getaddrinfo: no address\n");
	}
		
	else
		perror("gethostbyname");
	return so;
}/* conn */

/*
 * ¥½¥±¥Ã¥È¤Ë size_ ¥Ð¥¤¥ÈÁ÷¿®¡£
 */
static int
so_write(int so, const void *buf, size_t size_)
{
	int err = 0;
	int size = size_;
	const char *mv = buf;
	while (err == 0 && size >= 1) {
		int wrote = write(so, mv, size);
		if (wrote >= size) {
			size -= wrote;
			mv += wrote;
		}
		else
			err = -1;
	}/* while */
	if (err == 0)
		err = size_;
	return err;
}/* so_write */

/*
 *
 */
static struct termios tio_save;
static int ti_fd;

/*
 * »à¤ÌÁ°¤Ë¥¿¡¼¥ß¥Ê¥ë¤Î¾õÂÖ¤òÌá¤¹¡£
 */
static void
restore_local_terminal(void)
{
	tcsetattr(ti_fd, TCSANOW, &tio_save);
}/* restore_local_terminal */

/*
 * ¥¿¡¼¥ß¥Ê¥ë¤Î¾õÂÖ¤òÊÝÂ¸¡£
 */
static int
save_local_terminal(int fd)
{
	int err = tcgetattr(fd, &tio_save);
	if (err < 0) {
		perror("tcgetattr");
	}
	else {
		ti_fd = fd;
		atexit(restore_local_terminal);
	}
	return err;
}/* save_local_terminal */


/*
 * cfmakeraw() by blogger323
 * <URL:http://blogger323.blog83.fc2.com/blog-entry-202.html>
 */
#ifdef __CYGWIN__ /* [ */
/* Workaround for Cygwin, which is missing cfmakeraw */
/* Pasted from man page; added in serial.c arbitrarily */
void
cfmakeraw(struct termios *termios_p)
{
	termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
	|INLCR|IGNCR|ICRNL|IXON);
	termios_p->c_oflag &= ~OPOST;
	termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	termios_p->c_cflag &= ~(CSIZE|PARENB);
	termios_p->c_cflag |= CS8;
}/* cfmakeraw */

#endif /* ] */

/*
 * ¥¿¡¼¥ß¥Ê¥ë¤ò raw ¥â¡¼¥É¤Ë¤¹¤ë¡£
 */
static int
make_local_raw(int fd)
{
	int err = 0;
	if (isatty(fd)) {
		err = save_local_terminal(fd);
		if (err == 0) {
			struct termios tio;
			if ((err = tcgetattr(fd, &tio)) < 0)
				perror("tcgetattr");
			else {
				cfmakeraw(&tio);
				if ((err = tcsetattr(fd, TCSANOW, &tio)) < 0)
					perror("tcsetattr");
			}
		}
	}
	return err;
}/* make_local_raw */

/*
 * ²þ¹Ô
 */
static char *NL = "\r\n";

/*
 * -r rnRN
 * ¹ÔËö¤ÎÊÑ´¹¥ª¥×¥·¥ç¥ó
 *    r: ¥ê¥â¡¼¥È¤«¤é¼õ¿®¤·¤¿ CR ¤ò¥í¡¼¥«¥ë¤ËÉ½¼¨¤¹¤ë»þ²¿¤ËÊÑ´¹¤¹¤ë¤«
 *    n: ¥ê¥â¡¼¥È¤«¤é¼õ¿®¤·¤¿ NL ¤ò¥í¡¼¥«¥ë¤ËÉ½¼¨¤¹¤ë»þ²¿¤ËÊÑ´¹¤¹¤ë¤«
 *    R: ¥í¡¼¥«¥ë¤«¤é¼õ¿®¤·¤¿ CR ¤ò¥ê¥â¡¼¥È¤ËÁ÷¿®¤¹¤ë»þ²¿¤ËÊÑ´¹¤¹¤ë¤«
 *    N: ¥í¡¼¥«¥ë¤«¤é¼õ¿®¤·¤¿ NL ¤ò¥ê¥â¡¼¥È¤ËÁ÷¿®¤¹¤ë»þ²¿¤ËÊÑ´¹¤¹¤ë¤«
 * ¤½¤ì¤¾¤ì°ì·å¤ÎÊ¸»ú¤ÇÉ½¤¹¡£
 *    x = ¼õ¿®¤·¤¿Ê¸»ú¤ò¼Î¤Æ¤ë¡£
 *    r = CR ¤ËÊÑ´¹¤¹¤ë¡£
 *    n = LF ¤ËÊÑ´¹¤¹¤ë¡£
 *    t = CR LF ¤ËÊÑ´¹¤¹¤ë¡£
 *    - = ÊÑ´¹Ìµ¤·¡£
 */
static int
parse_rn_char(int c, int dflt)
{
	int ret = dflt;
	switch (c) {
	case 'x': ret = RN_DROP; break;
	case 'r': ret = RN_MAP_CR; break;
	case 'n': ret = RN_MAP_LF; break;
	case 't': ret = RN_MAP_CRLF; break;
	}/* switch */
	return ret;
}/* parse_rn_char */

static int
set_rn_opt(const char *rnopt)
{
	int err = -1;
	int len = strlen(rnopt);
	int rn[RN_INDEX_SIZE];
	if (len == RN_INDEX_SIZE) {
		if ((rn[RN_RECV_CR] = parse_rn_char(rnopt[RN_RECV_CR], -1)) != -1 &&
		    (rn[RN_RECV_LF] = parse_rn_char(rnopt[RN_RECV_LF], -1)) != -1 &&
		    (rn[RN_SEND_CR] = parse_rn_char(rnopt[RN_SEND_CR], -1)) != -1 &&
		    (rn[RN_SEND_LF] = parse_rn_char(rnopt[RN_SEND_LF], -1)) != -1) {
			memcpy(rn_map, rn, sizeof(rn_map));
			err = 0;
		}
	}
	return err;
}/* set_rn_opt */

static char *
print_rn_opt(char *buf, int size)
{
	char *ret = NULL;
	if (size >= RN_INDEX_SIZE + 1) {
		buf[RN_RECV_CR] = RN_STR[rn_map[RN_RECV_CR]];
		buf[RN_RECV_LF] = RN_STR[rn_map[RN_RECV_LF]];
		buf[RN_SEND_CR] = RN_STR[rn_map[RN_SEND_CR]];
		buf[RN_SEND_LF] = RN_STR[rn_map[RN_SEND_LF]];
		buf[RN_INDEX_SIZE] = '\0';
		ret = buf;
	}
	return ret;
}/* print_rn_opt */

/* $BB.EY?tCM(B $B$H(B Bxxx$BDj5ACM(B $B$NBP1~I=(B */
static struct speed_map {
	int num;
	speed_t speed;
} speed_map[] = {
	{ 0, B0 },
	{ 50, B50 },
	{ 75, B75 }, 
	{ 110, B110 },
	{ 134, B134 },
	{ 150, B150 },
	{ 200, B200 },
	{ 300, B300 },
	{ 600, B600 },
	{ 1200, B1200 },
	{ 1800, B1800 },
	{ 2400, B2400 },
	{ 4800, B4800 },
	{ 9600, B9600 },
	{ 19200, B19200 },
	{ 38400, B38400 },
	{ 57600, B57600 },
	{ 115200, B115200 },
};

/*
 * $BB.EY?tCM(B $B$+$i(B Bxxx$BDj5ACM$KJQ49(B
 * $B$@$a$J$i$=$N$^$^(B
 */
static int
speed_to_num(speed_t speed)
{
	int i = sizeof(speed_map)/sizeof(speed_map[0]);
	while(i--)
		if( speed_map[i].speed == speed)
			return speed_map[i].num;
	return speed;
}/* speed_to_num */

/*
 * Bxxx$BDj5ACM(B $B$+$i(B $BB.EY?tCM(B $BDj5ACM$KJQ49(B
 * $B$@$a$J$i$=$N$^$^(B
 */
static speed_t
num_to_speed(int num)
{
	int i = sizeof(speed_map)/sizeof(speed_map[0]);
	while(i--)
		if( speed_map[i].num == num)
			return speed_map[i].speed;
	return num;
}/* num_to_speed */


/*
 * ¥¿¡¼¥ß¥Ê¥ë¤Î¾õÂÖ¤òÉ½¼¨¡£
 * stty -a ¤ß¤¿¤¤¤Ê¤ä¤Ä¡£
 */
static void
print_status(const struct termios *tio)
{
	int cs;
	char *csstr = "?";
	speed_t ispeed = cfgetispeed(tio);
	speed_t ospeed = cfgetospeed(tio);
	fprintf(stderr, " ispeed %d", speed_to_num(ispeed));
	fprintf(stderr, " ospeed %d", speed_to_num(ospeed));
	fprintf(stderr, "%s", NL);
	fprintf(stderr, " %cIGNBRK", (tio->c_iflag & IGNBRK) ? '+' : '-');
	fprintf(stderr, " %cBRKINT", (tio->c_iflag & BRKINT) ? '+' : '-');
	fprintf(stderr, " %cIGNPAR", (tio->c_iflag & IGNPAR) ? '+' : '-');
	fprintf(stderr, " %cPARMRK", (tio->c_iflag & PARMRK) ? '+' : '-');
	fprintf(stderr, " %cINPCK", (tio->c_iflag & INPCK) ? '+' : '-');
	fprintf(stderr, " %cISTRIP", (tio->c_iflag & ISTRIP) ? '+' : '-');
	fprintf(stderr, " %cINLCR", (tio->c_iflag & INLCR) ? '+' : '-');
	fprintf(stderr, " %cIGNCR", (tio->c_iflag & IGNCR) ? '+' : '-');
	fprintf(stderr, " %cICRNL", (tio->c_iflag & ICRNL) ? '+' : '-');
	fprintf(stderr, " %cIXON", (tio->c_iflag & IXON) ? '+' : '-');
	fprintf(stderr, " %cIXOFF", (tio->c_iflag & IXOFF) ? '+' : '-');
	fprintf(stderr, " %cIXANY", (tio->c_iflag & IXANY) ? '+' : '-');
	fprintf(stderr, " %cIMAXBEL", (tio->c_iflag & IMAXBEL) ? '+' : '-');
	fprintf(stderr, "%s", NL);
	fprintf(stderr, " %cOPOST", (tio->c_oflag & OPOST) ? '+' : '-');
	fprintf(stderr, " %cONLCR", (tio->c_oflag & ONLCR) ? '+' : '-');
#ifdef OXTABS
	fprintf(stderr, " %cOXTABS", (tio->c_oflag & OXTABS) ? '+' : '-');
#endif
#if defined(TABDLY) && defined(XTABS) /* linux */
	fprintf(stderr, " %cTABDLY", (tio->c_oflag & TABDLY) == XTABS ? '+' : '-');
#endif
#ifdef ONOEOT
	fprintf(stderr, " %cONOEOT", (tio->c_oflag & ONOEOT) ? '+' : '-');
#endif
	fprintf(stderr, "%s", NL);
	cs = tio->c_cflag & CSIZE;
	switch (cs) {
	case CS5: csstr = "5"; break;
	case CS6: csstr = "6"; break;
	case CS7: csstr = "7"; break;
	case CS8: csstr = "8"; break;
	default: csstr = "?"; break;
	}/* switch */
	fprintf(stderr, " cs%s", csstr);
	fprintf(stderr, " %cCSTOPB", (tio->c_cflag & CSTOPB) ? '+' : '-');
	fprintf(stderr, " %cCREAD", (tio->c_cflag & CREAD) ? '+' : '-');
	fprintf(stderr, " %cPARENB", (tio->c_cflag & PARENB) ? '+' : '-');
	fprintf(stderr, " %cPARODD", (tio->c_cflag & PARODD) ? '+' : '-');
	fprintf(stderr, " %cHUPCL", (tio->c_cflag & HUPCL) ? '+' : '-');
	fprintf(stderr, " %cCLOCAL", (tio->c_cflag & CLOCAL) ? '+' : '-');
#ifdef CCTS_OFLOW
	fprintf(stderr, " %cCCTS_OFLOW", (tio->c_cflag & CCTS_OFLOW) ? '+' : '-');
#endif
	fprintf(stderr, " %cCRTSCTS", (tio->c_cflag & CRTSCTS) ? '+' : '-');
#ifdef CRTS_IFLOW
	fprintf(stderr, " %cCRTS_IFLOW", (tio->c_cflag & CRTS_IFLOW) ? '+' : '-');
#endif
#ifdef MDMBUF
	fprintf(stderr, " %cMDMBUF", (tio->c_cflag & MDMBUF) ? '+' : '-');
#endif
	fprintf(stderr, " %cECHOKE", (tio->c_lflag & ECHOKE) ? '+' : '-');
	fprintf(stderr, " %cECHOE", (tio->c_lflag & ECHOE) ? '+' : '-');
	fprintf(stderr, " %cECHO", (tio->c_lflag & ECHO) ? '+' : '-');
	fprintf(stderr, " %cECHONL", (tio->c_lflag & ECHONL) ? '+' : '-');
#ifdef ECHOPRT
	fprintf(stderr, " %cECHOPRT", (tio->c_lflag & ECHOPRT) ? '+' : '-');
#endif
	fprintf(stderr, " %cECHOCTL", (tio->c_lflag & ECHOCTL) ? '+' : '-');
	fprintf(stderr, " %cISIG", (tio->c_lflag & ISIG) ? '+' : '-');
	fprintf(stderr, " %cICANON", (tio->c_lflag & ICANON) ? '+' : '-');
#ifdef ALTWERASE
	fprintf(stderr, " %cALTWERASE", (tio->c_lflag & ALTWERASE) ? '+' : '-');
#endif
	fprintf(stderr, " %cIEXTEN", (tio->c_lflag & IEXTEN) ? '+' : '-');
	fprintf(stderr, "%s", NL);
#ifdef EXTPROC
	fprintf(stderr, " %cEXTPROC", (tio->c_lflag & EXTPROC) ? '+' : '-');
#endif
	fprintf(stderr, " %cTOSTOP", (tio->c_lflag & TOSTOP) ? '+' : '-');
	fprintf(stderr, " %cFLUSHO", (tio->c_lflag & FLUSHO) ? '+' : '-');
#ifdef NOKERNINFO
	fprintf(stderr, " %cNOKERNINFO", (tio->c_lflag & NOKERNINFO) ? '+' : '-');
#endif
#ifdef PENDIN
	fprintf(stderr, " %cPENDIN", (tio->c_lflag & PENDIN) ? '+' : '-');
#endif
	fprintf(stderr, " %cNOFLSH", (tio->c_lflag & NOFLSH) ? '+' : '-');
	fprintf(stderr, "%s", NL);
	fflush(stderr);
}/* print_status */

/*
 * ¥°¥í¡¼¥Ð¥ëÊÑ¿ô¤ò»²¾È¤·¤Æ¡¢¥¿¡¼¥ß¥Ê¥ë¤Î¾õÂÖ
 * (speed, parity, data bits Åù)¤òÀßÄê¤¹¤ë¡£
 */
static void
set_status(struct termios *tio)
{
	cfsetispeed(tio, speed);
	cfsetospeed(tio, speed);
	tio->c_cflag = (tio->c_cflag & ~CSIZE) | data_bit;
	if (stop_bit == 1)
		tio->c_cflag &= ~CSTOPB;
	else if (stop_bit == 2)
		tio->c_cflag |= CSTOPB;
	switch (parity) {
	case P_NONE:
		tio->c_cflag &= ~PARENB;
		break;
	case P_EVEN:
		tio->c_cflag = (tio->c_cflag | PARENB) & ~PARODD;
		break;
	case P_ODD:
		tio->c_cflag = tio->c_cflag | PARENB | PARODD;
		break;
	}/* switch */
	switch (flow_control) {
	case F_NONE:
		tio->c_cflag &= ~CRTSCTS;
		tio->c_iflag &= ~(IXON | IXOFF);
		break;
	case F_HARD:
		tio->c_cflag |= CRTSCTS;
		tio->c_iflag &= ~(IXON | IXOFF);
		break;
	case F_X:
		tio->c_cflag &= ~CRTSCTS;
		tio->c_iflag |= (IXON | IXOFF);
		break;
	}/* switch */
}/* set_status */

/*
 * »ØÄê¤µ¤ì¤¿¥Õ¥¡¥¤¥ë¥Ï¥ó¥É¥ë¤Î¥¿¡¼¥ß¥Ê¥ë¾õÂÖ¤òÉ½¼¨¡£
 */
static int
print_fdstatus(int fd)
{
	int err = -1;
	struct termios tio;
	if (tcgetattr(fd, &tio) < 0)
		perror("tcgetattr");
	else {
		print_status(&tio);
		err = 0;
	}
	return err;
}/* print_fdstatus */

/*
 * »ØÄê¤µ¤ì¤¿¥Õ¥¡¥¤¥ë¥Ï¥ó¥É¥ë¤Î¥¿¡¼¥ß¥Ê¥ë¤Î¾õÂÖ¤ò½é´ü²½¤¹¤ë¡£
 * raw ¥â¡¼¥É¤Ë¤·¤Æ¡¢CLOCAL ¤òÀßÄê¤¹¤ë¡£
 */
static int
fd_init(int fd)
{
	struct termios tio;
	int err = -1;
	if (tcgetattr(fd, &tio) < 0)
		perror("tcgetattr");
	else {
		cfmakeraw(&tio);
		tio.c_cflag |= CLOCAL; /* do not send SIGHUP to me! */
		if (tcsetattr(fd, TCSANOW, &tio) < 0)
			perror("tcsetattr");
		else
			err = 0;
	}
	return err;
}/* fd_init */

/*
 * »ØÄê¤µ¤ì¤¿¥Õ¥¡¥¤¥ë¥Ï¥ó¥É¥ë¤Ë¤Ä¤¤¤Æ¡¢
 * ¥°¥í¡¼¥Ð¥ëÊÑ¿ô¤ò»²¾È¤·¤Æ¡¢¥¿¡¼¥ß¥Ê¥ë¤Î¾õÂÖ
 * (speed, parity, data bits Åù)¤òÀßÄê¤¹¤ë¡£
 */
static int
set_fdstatus(int fd)
{
	struct termios tio;
	int err = -1;
	if (tcgetattr(fd, &tio) < 0)
		perror("tcgetattr");
	else {
		set_status(&tio);
		if (tcsetattr(fd, TCSANOW, &tio) < 0)
			perror("tcsetattr");
		else
			err = 0;
	}
	return err;
}/* set_fdstatus */

/*
 *
 */

static int
chardump(const void *buf_, size_t size)
{
	const unsigned char *buf = buf_;
	int i;
	printf(" |");
	for (i = 0; i < size; i++) {
		if (buf[i] >= ' ' && buf[i] < 0x7f)
			printf("%c", buf[i]);
		else
			printf(".");
	}/* for */
	printf("%s", NL);
	fflush(stdout);
	return 0;
}/* chardump */

static unsigned char hex_buf[16];
static int hex_count;

static int
hexdump_start(void)
{
	fprintf(stderr, "[HEX MODE]%s", NL);
	hex_count = 0;
	return 0;
}/* hexdump_start */

static int
hexdump_stop(void)
{
	if (hex_count > 0)
		chardump(hex_buf, hex_count);
	fprintf(stderr, "[NORMAL MODE]%s", NL);
	return 0;
}/* hexdump_stop */

/*
 *
 */
static int
hexdump(const void *buf_, size_t size)
{
	const unsigned char *buf = buf_;
	int i;
	for (i = 0; i < size; i++) {
		printf("%02x ", buf[i]);
		hex_buf[hex_count++] = buf[i];
		if (hex_count == sizeof(hex_buf)) {
			chardump(hex_buf, hex_count);
			hex_count = 0;
		}
	}/* for */
	return 0;
}/* hexdump */



#ifdef JUPITER /* [ */
/*
 * JUPITER GPS (rockwell binary mode) decoder
 */

static int rockwell_mode;
static int jdebug;

struct jm1000_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned short sseq;
	unsigned short navval;
	unsigned short navtype;
	unsigned short nmeas;
	unsigned short polar;
	unsigned short gweek;
	unsigned short sweek[2];
	unsigned short nsweek[2];
	unsigned short utcday;
	unsigned short utcmon;
	unsigned short utcyear;
	unsigned short utchour;
	unsigned short utcmin;
	unsigned short utcsec;
	unsigned short utcnsec[2];
	unsigned short latitude[2];
	unsigned short longitude[2];
	unsigned short height[2];
	unsigned short gsep;
	unsigned short speed[2];
	unsigned short course;
	unsigned short mvar;
	unsigned short climb;
	unsigned short map;
	unsigned short herr[2];
	unsigned short verr[2];
	unsigned short terr[2];
	unsigned short hverr;
	unsigned short bias[2];
	unsigned short biassd[2];
	unsigned short drift[2];
	unsigned short driftsd[2];
	unsigned short dsum;
};

struct jm1002_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned short sseq;
	unsigned short gweek;
	unsigned short sweek[2];
	unsigned short nsweek[2];
	struct {
		unsigned short flag;
		unsigned short sate_prn;
		unsigned short c_no;
	} ch_summary[12];
	unsigned short dsum;
};

struct jm1003_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned short gdop;
	unsigned short pdop;
	unsigned short hdop;
	unsigned short vdop;
	unsigned short tdop;
	unsigned short num_vis;
	struct {
		unsigned short sate_prn;
		unsigned short sate_azimuth;
		unsigned short sate_elevation;
	} vis_sate[12];
	unsigned short dsum;
};

struct jm1011_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned char num_ch[10];
	unsigned char sw_ver[10];
	unsigned char sw_date[10];
	unsigned char opt_list[10];
	unsigned char reserve[10];
	unsigned short dsum;
};

struct jm1012_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned short flags;
	unsigned short cold_timeout;
	unsigned short dgps_timeout;
	short ele_mask;
	unsigned short candi1;
	unsigned short candi2;
	unsigned short solval;
	unsigned short nsate;
	unsigned short min_hor[2];
	unsigned short min_ver[2];
	unsigned short platform;
	unsigned short dsum;
};

struct jm1108_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned short reserved[5];
	unsigned short sweek[2];
	short offs;
	unsigned short offns[2];
	unsigned short flags;
	unsigned short dsum;
};

struct jm1135_t {
	unsigned short stime[2];
	unsigned short seq;
	unsigned short id_prn;
	unsigned short dsum;
};

static time_t
get_gpsweek(int utc_of_week)
{
	time_t ret = 0;
	int err;
	struct timeval tv;
	err = gettimeofday(&tv, NULL);
	if (err == 0) {
		time_t t0 = tv.tv_sec;
		struct tm tm = *gmtime(&t0);
		int sec_of_week = tm.tm_wday * 86400 + tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
		/*int diff = utc_of_week - sec_of_week;*/
		ret = tv.tv_sec - sec_of_week;
	}
	else
		perror("gettimeofday");
	return ret;
}/* get_gpsweek */


static int
get_now(int utc_of_week)
{
	time_t gps0 = get_gpsweek(utc_of_week);
	time_t t = gps0 + utc_of_week;
	return t;
}/* get_now */

#define UDI(v_) (((unsigned long)(v_)[1] << 16) | (v_)[0])
#define DI(v_) (((long)(v_)[1] << 16) | (v_)[0])

static int
print_C(const void *buf, int size, FILE *fp)
{
	const unsigned char *p = buf;
	int i;
	for (i = 0; i < size; i++) {
		if (p[i] >= ' ' && p[i] < 0x7f)
			fprintf(fp, "%c", p[i]);
		else
			fprintf(fp, "\\x%02x", p[i]);
	}
	return 0;
}/* print_C */

static void
rad_to_deg(double rad, int *deg, int *min, int *sec, int *usec)
{
	double d = rad * 180.0 / PI;
	*deg = floor(d);
	d = (d - floor(d)) * 60;
	*min = floor(d);
	d = (d - floor(d)) * 60;
	*sec = floor(d);
	*usec = (d - floor(d)) * 1000000;
}/* rad_to_deg */


static void
gm_to_local(int *Y, int *m, int *d, int *H, int *M, int *S)
{
	struct tm tm;
	time_t t;
	tm.tm_year = *Y - 1900;
	tm.tm_mon = *m - 1;
	tm.tm_mday = *d;
	tm.tm_hour = *H;
	tm.tm_min = *M;
	tm.tm_sec = *S;
	tm.tm_isdst = 0;
	t = timegm(&tm);
	tm = *localtime(&t);
	*Y = tm.tm_year + 1900;
	*m = tm.tm_mon + 1;
	*d = tm.tm_mday;
	*H = tm.tm_hour;
	*M = tm.tm_min;
	*S = tm.tm_sec;
}

#define A 6378137.0 /* ÀÖÆ»È¾·Â m */
#define F 298.257222101 /* Ù¨Ê¿Î¨¤ÎµÕ¿ô=1/((A-B)/A) */
#define B 6356752.31414035584785210687 /* ¶ËÈ¾·Â=A-A/F */

static void
xymove(double nt, double et, double *delta_n, double *delta_e)
{
	static double nt0, et0;
	double cos_nt, sin_nt, r1, r2;
	double rdn, rde;
	if (nt0 == 0) {
		nt0 = nt;
		et0 = et;
	}
	cos_nt = cos(nt);
	sin_nt = sin(nt);
	r1 = sqrt(A * A * cos_nt * cos_nt + B * B * sin_nt * sin_nt); /* ËÌ°Þ nt ¤ÎÃÏµåÈ¾·Â */
	r2 = A * cos_nt; /* ËÌ°Þ nt ¤Î±ß¤ÎÈ¾·Â */
	rdn = (nt - nt0); /* ËÌ°ÞÊÑ°Ì [rad] */
	rde = (et - et0); /* Åì·ÐÊÑ°Ì [rad] */
	*delta_n = r1 * rdn; /* ËÌÊÑ°Ì[m] */
	*delta_e = r2 * rde; /* ÅìÊÑ°Ì[m] */
}

static int
decode(unsigned short *dt, int count)
{
	int id = dt[1];
	/*int dlen = dt[2];*/
	struct jm1000_t *jm1000;
	struct jm1002_t *jm1002;
	struct jm1003_t *jm1003;
	struct jm1011_t *jm1011;
	struct jm1012_t *jm1012;
	struct jm1108_t *jm1108;
	struct jm1135_t *jm1135;
	int latdeg, latmin, latsec, latusec;
	int longdeg, longmin, longsec, longusec;
	int Y, m, d, H, M, S;
	double dx, dy;
	switch (id) {
	case 1000:
		jm1000 = (struct jm1000_t *)&dt[5];
		rad_to_deg(DI(jm1000->latitude) * 1e-8, &latdeg, &latmin, &latsec, &latusec);
		rad_to_deg(DI(jm1000->longitude) * 1e-8, &longdeg, &longmin, &longsec, &longusec);
		Y = jm1000->utcyear;
		m = jm1000->utcmon;
		d = jm1000->utcday;
		H = jm1000->utchour;
		M = jm1000->utcmin;
		S = jm1000->utcsec;
		gm_to_local(&Y, &m, &d, &H, &M, &S);
		xymove(DI(jm1000->latitude) * 1e-8, DI(jm1000->longitude) * 1e-8, &dx, &dy);
		printf("%04d/%02d/%02d %02d:%02d:%02d %d%02d%02d.%06d %d%02d%02d.%06d %.2f %d %c %5.1f %7.3f %7.3f\r\n",
			Y, m, d, H, M, S,
			latdeg, latmin, latsec, latusec,
			longdeg, longmin, longsec, longusec,
			DI(jm1000->height) / 100.0,
			jm1000->map,
			((jm1000->navval & 0x4) ? 'V' : 'A'),
			UDI(jm1000->speed) * 1e-2 * 3.6,
			dx * 1e-3, dy * 1e-3
			);
		break;
	case 1002:
		jm1002 = (struct jm1002_t *)&dt[5];
		printf("%d", jm1002->seq);
		{
			int i;
			for (i = 0; i < 12; i++) {
				printf(" %d %d", jm1002->ch_summary[i].sate_prn, jm1002->ch_summary[i].c_no); 
			}
			printf("\r\n");
		}
		break;
	case 1003:
		jm1003 = (struct jm1003_t *)&dt[5];
		printf("%d", jm1003->seq);
		printf(" G%d P%d H%d V%d T%d N%d",
			jm1003->gdop,
			jm1003->pdop,
			jm1003->hdop,
			jm1003->vdop,
			jm1003->tdop,
			jm1003->num_vis);
		{
			int i;
			for (i = 0; i < 12; i++) {
				printf(" %d", jm1003->vis_sate[i].sate_prn);
			}
			printf("\r\n");
		}
		break;
	case 1011:
		jm1011 = (struct jm1011_t *)&dt[5];
		printf("num_ch=[");
		print_C(jm1011->num_ch, 10, stdout);
		printf("] sw_ver=[");
		print_C(jm1011->sw_ver, 10, stdout);
		printf("] sw_date=[");
		print_C(jm1011->sw_date, 10, stdout);
		printf("] opt_list=[");
		print_C(jm1011->opt_list, 10, stdout);
		printf("]\r\n");
		break;
	case 1012:
		jm1012 = (struct jm1012_t *)&dt[5];
		printf("flag=%04x ele_mask=%.3f nsate=%d platform=%d\r\n",
			jm1012->flags,
			jm1012->ele_mask * 1e-3,
			jm1012->nsate,
			jm1012->platform);
		break;
	case 1108:
		jm1108 = (struct jm1108_t *)&dt[5];
		{
			time_t t = get_now(UDI(jm1108->sweek));
			struct tm tm = *localtime(&t);
			printf("%d wsec %lu GtoU %d.%09ld A/V=%d GPS/UTC=%d %04d/%02d/%02d %02d:%02d:%02d\r\n",
				jm1108->seq,
				UDI(jm1108->sweek),
				jm1108->offs,
				UDI(jm1108->offns),
				jm1108->flags & 1,
				jm1108->flags & 2,
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec
				);
		}
		break;
	case 1135:
		jm1135 = (struct jm1135_t *)&dt[5];
		printf("%d %d %d\r\n", jm1135->seq, jm1135->id_prn & 0xff, jm1135->id_prn >> 8);
		break;
	default:
		printf("ndata=%d\r\n", dt[2]);
		{
			int i;
			for (i = 0; i < dt[2]; i++) {
				printf(" %d=%04x", 6 + i, dt[5 + i]);
			}
			printf("\r\n");
		}
		break;
	}/* switch */
	return 0;
}/* decode */

#define WORD_MAX 300
static unsigned short bdata[WORD_MAX];



static int
rockwell2(int ch)
{
	static int stat;
	static int soh_del;
	static int message_id;
	static int data_word_count;
	static int dcl0_qran;
	static int header_checksum;
	static int wc;
	static int sum;
	int err = 0;
	switch (stat) {
	case 0:
		bdata[0] = ch;
		if (ch == 0x81ff) {
			soh_del = ch;
			if (jdebug) fprintf(stderr, "soh_del = %04x\r\n", ch);
			stat++;
			printf(":");
			fflush(stdout);
		}
		else {
			if (jdebug) fprintf(stderr, "not soh_del : %04x\r\n", ch);
			err = -1;
		}
		break;
	case 1:
		bdata[1] = ch;
		message_id = ch;
		if (jdebug) fprintf(stderr, "message_id = %04x (%d)\r\n", ch, ch);
		printf("%d ", ch);
		fflush(stdout);
		stat++;
		break;
	case 2:
		bdata[2] = ch;
		data_word_count = ch;
		if (jdebug) fprintf(stderr, "data_word_count = %04x\r\n", ch);
		if (data_word_count >= WORD_MAX) {
			fprintf(stderr, "data_word_count %d >= %d\r\n", data_word_count, WORD_MAX);
			err = -1;
		}
		stat++;
		break;
	case 3:	
		bdata[3] = ch;
		dcl0_qran = ch;
		if (jdebug) fprintf(stderr, "dcl0_qran = %04x\r\n", ch);
		stat++;
		break;
	case 4:
		bdata[4] = ch;
		header_checksum = ch;
		wc = 0;
		stat++;
		sum = (-(0x81ff + message_id + data_word_count + dcl0_qran)) & 0xffff;
		if (jdebug) fprintf(stderr, "header_checksum = %04x calc = %04x\r\n", ch, sum);
		if (sum != header_checksum) {
			fprintf(stderr, "checksum error: header = %04x, calc = %04x\r\n", header_checksum, sum);
			err = -1;
		}
		sum = 0;
		break;
	case 5:
		if (5 + wc < WORD_MAX)
			bdata[5 + wc] = ch;
		if (wc < data_word_count) {
			if (jdebug) fprintf(stderr, " %d=%04x", wc + 6, ch);
			sum = (sum + ch) & 0xffff;
		}
		else {
			sum = -sum & 0xffff;
			if (jdebug) fprintf(stderr, "\r\n");
			if (jdebug) fprintf(stderr, " data sum = %04x, calc = %04x\r\n", ch, sum);
			if (sum != ch) {
				fprintf(stderr, "checksum error: data sum = %04x, calc = %04x\r\n", ch, sum);
				err = -1;
			}
			else {
				if (5 + wc < WORD_MAX) {
					decode(bdata, 5 + wc + 1);
				}
			}
			stat = 0;
		}
		wc++;
		break;
	}/* switch */
	if (err < 0)	
		stat = 0;
	return err;
}/* rockwell2 */

static int
rockwell(const void *buf_, size_t size)
{
	const unsigned char *buf = buf_;
	static int stat, lo_ch = EOF;
	int i = 0, ch;
	int err;
	for (i = 0; i < size; i++) {
	/* fprintf(stderr, " %02x\r\n", buf[i]); */
		switch (stat) {
		case 0:
			if (buf[i] == 0xff)
				stat = 1;
			break;
		case 1:
			if (buf[i] == 0x81) {
				stat = 2;
				lo_ch = EOF;
				err = rockwell2(0x81ff);
			}
			else if (buf[i] == 0xff)
				stat = 1;
			else
				stat = 0;
			break;
		case 2:
			if (lo_ch == EOF)
				lo_ch = buf[i];
			else {
				ch = lo_ch | (buf[i] << 8);
				lo_ch = EOF;
				err = rockwell2(ch);
				if (err < 0)
					stat = 0;
			}
			break;
		}
	}
	return 0;
}/* rockwell */

#endif /* ] */


/*
 * ¥ê¥â¡¼¥È¤«¤é¥Ç¡¼¥¿¤¬Á÷¤é¤ì¤ÆÍè¤¿½èÍý¡£
 */
static int
read_remote(int localfd, int remotefd, FILE *log)
{
	unsigned char lbuf[80];
	int err = read(remotefd, lbuf, sizeof(lbuf));
	if (err < 0)
		perror("remote read");
	else if (err == 0) {
		fprintf(stderr, "remote closed");
		err = -1;
	}
	else {
		if (log != NULL) {
			fwrite(lbuf, 1, err, log);
			fflush(log);
		}
		if (is_server) {
			if (so_write(localfd, lbuf, err) == err)
				err = 0;
			else
				perror("so_write");
		}
		else if (pipe_mode) {
			if (fwrite(lbuf, 1, err, stdout) == err)
				err = 0;
			else
				perror("so_write");
		}
		else if (hex_mode)
			err = hexdump(lbuf, err);
#ifdef JUPITER
		else if (rockwell_mode)
			err = rockwell(lbuf, err);
#endif
		else {
			int i = 0, n = err;
			err = 0;
			while (err == 0 && i < n) {
				switch (lbuf[i]) {
				case '\n':	
					if (fputs(RN_OUTPUT[rn_map[RN_RECV_LF]], stdout) == EOF)
						err = -1;
					break;
				case '\r':
					if (fputs(RN_OUTPUT[rn_map[RN_RECV_CR]], stdout) == EOF)
						err = -1;
					break;
				default:
					if (fputc(lbuf[i], stdout) == EOF)
						err = -1;
				}/* switch */
				i++;
			}/* while */
		}
		fflush(stdout);
	}
	return err;
}/* read_remote */

/*
 *
 */
static char *
read_line(char *buf, int size, int fd)
{
	int idx = 0, err = 0, done = 0, ch = EOF;
	char *ret = NULL, lbuf[1];
	while (!done && (err = read(fd, lbuf, 1) > 0)) {
		int i;
		ch = (unsigned char)lbuf[0];
		switch (ch) {
		case '[' - '@':
			fprintf(stderr, "%s", NL);
			done = 1;
			break;
		case 'M' - '@':
		case 'J' - '@':
			if (idx < size)
				buf[idx] = '\0';
			fprintf(stderr, "%s", NL);
			done = 1;
			break;
		case 'H' - '@':
			if (idx >= 1) {
				fprintf(stderr, "\b \b");
				idx--;
			}
			break;
		case 'U' - '@':
			for (i = 0; i < idx; i++)
				fprintf(stderr, "\b \b");
			idx = 0;
			break;
		default:
			if (iscntrl(ch))
				fprintf(stderr, "\a");
			else {
				if (idx + 1 < size) { /* keep 1 for '\0' */
					buf[idx++] = ch;
					fprintf(stderr, "%c", ch);
				}
				else
					fprintf(stderr, "\a");
			}
			break;
		}/* switch */
	}/* while */
	if (err > 0) {
		if (ch != '[' - '@')
			ret = buf;
	}
	return ret;
}/* read_line */

/*
 *
 */
static int
put_file(const char *name, int fd)
{
	int err = -1;
	FILE *fp = fopen(name, "r");
	if (fp == NULL)
		fprintf(stderr, "%s: %s%s", name, strerror(errno), NL);
	else {
		int xx;
		char lbuf[256];
		err = 0;
		while (err == 0 && (xx = fread(lbuf, 1, sizeof(lbuf), fp)) > 0) {
			if (write(fd, lbuf, xx) != xx) {
				fprintf(stderr, "%s: %s%s", name, strerror(errno), NL);
				err = -1;
			}
		}/* while */
		fclose(fp);
	}
	return err;
}/* put_file */

/*
 * ¥¨¥¹¥±¡¼¥× (²þ¹Ô¡¦¥Á¥ë¥À) ¤Î¸å¤Ë cmd ¤¬ÆþÎÏ¤µ¤ì¤¿½èÍý¡£
 * return
 *	0: ok (¥¨¥¹¥±¡¼¥×Â³¤¯)
 *	1: ok (¥¨¥¹¥±¡¼¥×½ª¤ë)
 *	-1: error (¥¨¥¹¥±¡¼¥×½ª¤ë)
 */
static int
do_escape(int cmd, int localfd, int remotefd)
{
	int err = 0;
	char obuf[2];
	switch (cmd) {
	case '?':
		fprintf(stderr, "[COMMANDS]%s", NL);
		fprintf(stderr, "[~?]\thelp%s", NL);
		fprintf(stderr, "[~.]\tquit%s", NL);
		fprintf(stderr, "[~~]\tsend '~'%s", NL);
		fprintf(stderr, "[~#]\tsend break%s", NL);
		fprintf(stderr, "[~x]\ttoggle hex mode%s", NL);
		fprintf(stderr, "[~r rnRN]\tset crlf map mode%s", NL);
		fprintf(stderr, "[~>flie]\tput file%s", NL);
		break;
	case '.':
		quit_flag = 1; /* ¥×¥í¥°¥é¥à½ªÎ» */
		break;
	case '>':
		{
			char lbuf[256];
			fprintf(stderr, "put file>");
			if (read_line(lbuf, sizeof(lbuf), localfd) != NULL) {
				put_file(lbuf, remotefd);
			}
		}
		break;	
	case '~':
		err = 1;
		if (so_write(remotefd, "~", 1) != 1) {
			perror("write remote");
			err = -1;
		}
		break;
	case '#':
		if (tcsendbreak(remotefd, 0) < 0) {
			perror("tcsendbreak");
			err = -1;
		}
		break;
	case 'x':
		hex_mode = !hex_mode;
		if (hex_mode)
			hexdump_start();
		else
			hexdump_stop();
		break;
	case 'r':
		{
			char lbuf[8], map[8];
			print_rn_opt(map, sizeof(map));
			fprintf(stderr, "crlf map: x=drop, r=CR, n=LF, t=CRLF%s", NL);
			fprintf(stderr, "crlf map: option order = {recv CR, recv NL, send CR, send LF}%s", NL);
			fprintf(stderr, "crlf map: current option=%s, input new option=", map);
			if (read_line(lbuf, sizeof(lbuf), localfd) != NULL) {
				if (set_rn_opt(lbuf) < 0)
					fprintf(stderr, "%s: wrong format. ignored%s", lbuf, NL);
			}
		}
		break;
	default:
		err = 1;
		obuf[0] = '~';
		obuf[1] = cmd;
		if (so_write(remotefd, obuf, 2) != 2) {
			perror("write remote");
			err = -1;
		}
		break;
	}/* switch */
	return err;
}/* do_escape */

/*
 * ¥­¡¼¥Ü¡¼¥É¤«¤éÆþÎÏ¤¬¤¢¤Ã¤¿½èÍý¡£
 */
static int
read_local(int localfd, int remotefd, FILE *log)
{
	static int lastch = '\n';
	unsigned char lbuf[1];
	int err = read(localfd, lbuf, sizeof(lbuf));
	if (err < 0)
		perror("local read");
	else if (err == 0) {
		fprintf(stderr, "local closed\n");
		err = -1;
	}
	else {
		if (is_server || pipe_mode) {
			if (so_write(remotefd, lbuf, err) == err)
				err = 0;
			else
				perror("so_write");
		}
		else if ((lastch == '\n' || lastch == '\r') && lbuf[0] == '~') {
			err = read(localfd, lbuf, sizeof(lbuf));
			if (err < 0)
				perror("local read");
			else if (err == 0) {
				fprintf(stderr, "local closed\n");
				err = -1;
			}
			else {
				err = do_escape(lbuf[0], localfd, remotefd);
				if (err != 0)
					lastch = lbuf[0];
				if (err == 1)
					err = 0;
			}
		}
		else {
			char *obuf = lbuf;
			int n = err;
			switch (lbuf[0]) {
			case '\r':
				obuf = RN_OUTPUT[rn_map[RN_SEND_CR]];
				n = strlen(obuf);
				break;
			case '\n':
				obuf = RN_OUTPUT[rn_map[RN_SEND_LF]];
				n = strlen(obuf);
				break;
			}/* switch */
			err = so_write(remotefd, obuf, n);
			if (err < 0) {
				perror("write remote");
			}
			else
				err = 0;
			lastch = lbuf[0];
		}
	}
	return err;
}/* read_local */

/*
 * ¥á¥¤¥ó¥ë¡¼¥×
 */
static int
fnain(int localfd, int remotefd, FILE *log)
{
	int err = 0;
	if (pipe_mode)
		set_fdstatus(localfd);
	quit_flag = 0;
	while (quit_flag == 0 && err == 0) {
		fd_set fds;
		err = sel2(&fds, localfd, remotefd);
		if (err == -1)
			perror("select");
		else if (err == 0) {
			/* timeout */
		}
		else {
			if (FD_ISSET(remotefd, &fds)) {
				err = read_remote(localfd, remotefd, log);
			}
			if (FD_ISSET(localfd, &fds)) {
				err = read_local(localfd, remotefd, log);
			}
		}
	}/* while */
	return err;
}/* fnain */

/*
 * ¥·¥ê¥¢¥ë¥Ý¡¼¥È¤¸¤ã¤Ê¤¯¤Æ¡¢
 * host:port ¤Ë TCP ¤ÇÀÜÂ³¤·¤ÆÄÌ¿®¤¹¤ë¡£
 * telnet(1) ¤ß¤¿¤¤¤ÊÅÛ¡£
 */
static int
oain(const char *host, int port, int family, FILE *log)
{
	int err = -1;
	int so = conn(host, port, family);
	if (so >= 0) {
		if (make_local_raw(0) == 0)
			err = fnain(0, so, log);
		close(so);
	}
	return err;
}/* oain */

/*
 * ¥Õ¥¡¥¤¥ëÌ¾ dev ¤òÄÌ¤·¤Æ¥·¥ê¥¢¥ëÄÌ¿®¤ò¹Ô¤¦¡£
 */
static int
nain(int localfd, const char *dev, FILE *log)
{
	int err = -1;
	int remotefd = open(dev, O_RDWR);
	if (remotefd < 0)
		perror(dev);
	else {
		if (make_local_raw(localfd) == 0 && fd_init(remotefd) == 0 && set_fdstatus(remotefd) == 0) {
			if (!is_server)
				print_fdstatus(remotefd);
			err = fnain(localfd, remotefd, log);
		}
		close(remotefd);
	}
	return err;
}/* nain */


static int
server(const char *dev, int port, int family, FILE *log)
{
	int err;
	char servname[16];
	struct addrinfo hints, *res;
	is_server = 1;
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	sprintf(servname, "%d", port);
	err = getaddrinfo(NULL, servname, &hints, &res);
	if (err == 0) {
		if (res != NULL) {
			int so = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (so >= 0) {
				int on = 1;
				setsockopt(so, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
				if (bind(so, res->ai_addr, res->ai_addrlen) == 0) {
					if (listen(so, 5) == 0) {
						while (1) {
							int len = res->ai_addrlen;
							int newso = accept(so, res->ai_addr, &len);
							err = 0;
#ifdef USE_LIBWRAP /*[*/
							if (use_libwrap) {
								char hostname[NI_MAXHOST];
								struct request_info accessreq;
								memset(hostname, 0x00, sizeof(hostname));
								err = getnameinfo(res->ai_addr, len, hostname, sizeof(hostname), NULL, 0, NI_NUMERICHOST);
								if (err == 0) {
									request_init(&accessreq, RQ_DAEMON, "jerm", RQ_CLIENT_ADDR, hostname, NULL);
									if (!hosts_access(&accessreq))
										err = -1;
								}
								else {
									fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
									if (err == EAI_SYSTEM)
										fprintf(stderr, "getnameinfo: %s\n", strerror(errno));
								}
							}
#endif /*]*/
							if (err == 0)
								err = nain(newso, dev, log);
							close(newso);
						}/* while */
					}
					else
						fprintf(stderr, "listen: %s\n", strerror(errno));
				}
				else
					fprintf(stderr, "bind: %d: %s\n", port, strerror(errno));
			}
			else 
				fprintf(stderr, "socket: %s\n", strerror(errno));
			freeaddrinfo(res);
		}
		else
			fprintf(stderr, "getaddrinfo: no address\n");
	}
	else {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		if (err == EAI_SYSTEM)
			fprintf(stderr, "getaddrinfo: %s\n", strerror(errno));
	}
	return err;
}/* server */

static char *copyright = "Jerminal v0.8096  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2007 candy";

/*
 *
 */
static char *usage_msg =
	"usage1: %s [optinos] device\n"
	"\t-b speed\tspeed\n"
	"\t-p [none|even|odd]\tparity\n"
	"\t-d [7|8]\tdata bits\n"
	"\t-s [1|2|1.5]\tstop bit\n"
	"\t-f [none|x|hard]\tflow control\n"
	"\t-F\tSet FUCK MODE for TA-100KR/RA SYSTEMS CORP.\n"
	"usage2: %s [-46i][-P port] [options] host\n"
	"\tclient mode.\n"
	"\t-i\tinit stdin/out by device options (pipe mode)\n"
	"common optinos:\n"
	"\t-l file\tlog file\n"
	"\t-z\ttruncate log file\n"
	"\t-x\tstart in hex dump mode\n"
	"\t-r cccc\tset CR LF mapping. (default = rnrn)\n"
	"\t\tc = {x=drop, r=CR, n=LF, t=CRLF}\n"
	"\t\toption order = {recv CR, recv NL, send CR, send LF}\n"
#ifdef JUPITER
	"\t-j\trockwell binary data mode\n"
#endif
	"usage3: %s -D [-46T][-P port][-z][-l file] device\n"
	"\tserver mode.\n"
#ifdef USE_LIBWRAP
	"\t-T\tuse libwrap(3)\n"
#endif
	;

/*
 *
 */
int
main(int argc, char *argv[])
{
	int ex = 1, ch, show_usage = 0;
	int port = PORT, truncate_log = 0;
	int family = AF_INET; /* AF_UNSPEC; */
	char *logfile = NULL;
	int daemon = 0;
	myname = argv[0];
	while ((ch = getopt(argc, argv, "46b:d:Df:Fijl:p:P:r:s:TVxz")) != EOF) {
		switch (ch) {
			int x;
		default:
		case 'V':
			show_usage = 1;
			break;
		case '4':
			family = AF_INET;
			break;
		case '6':
#ifdef AF_INET6
			family = AF_INET6;
#else
			fprintf(stderr, "%s: WARNING: IPv6 is not supported, ignored.\n", myname);
#endif
			break;
		case 'b':
			x = strtol(optarg, NULL, 10);
			speed = num_to_speed(x);
			break;
		case 'd':
			x = strtol(optarg, NULL, 10);
			switch (x) {
			case 8: data_bit = CS8; break;
			case 7: data_bit = CS7; break;
			default: show_usage = 1; break;
			}/* switch */
			break;
		case 'D':
			daemon++;
			break;
#ifdef USE_LIBWRAP
		case 'T':
			use_libwrap = 1;
			break;
#endif
		case 'f':
			switch (optarg[0]) {
			case 'n': flow_control = F_NONE; break;
			case 'x': flow_control = F_X; break;
			case 'h': flow_control = F_HARD; break;
			default: show_usage = 1; break;
			}/* switch */
			break;
		case 'F':
			/* TA-100 ÍÑ */
			speed = B9600;
			data_bit = CS8;
			stop_bit = 1;
			parity = P_EVEN;
			flow_control = F_HARD;
			rn_map[RN_SEND_CR] = RN_MAP_CRLF;
			break;
		case 'i':
			pipe_mode = 1;
			break;
		case 'j':
#ifdef JUPITER
			rockwell_mode = 1;
#else
			fprintf(stderr, "Error: -j not available. compile with '-DJUPITER'\n");
#endif
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'p':
			switch (optarg[0]) {
			case 'n': parity = P_NONE; break;
			case 'e': parity = P_EVEN; break;
			case 'o': parity = P_ODD; break;
			default: show_usage = 1; break;
			}/* switch */
			break;
		case 'P':
			port = strtol(optarg, NULL, 0);
			break;
		case 'r':
			if (set_rn_opt(optarg) < 0) {
				fprintf(stderr, "%s: %s: wrong format\n", myname, optarg);
				show_usage++;
			}
			break;
		case 's':
			if (strcmp(optarg, "1") == 0)
				stop_bit = 1;
			else if (strcmp(optarg, "2") == 0)
				stop_bit = 2;
			else if (strcmp(optarg, "1.5") == 0)
				stop_bit = 3;
			else	
				show_usage = 1;
			break;
		case 'x':
			hex_mode = 1;
			break;
		case 'z':
			truncate_log = 1;
			break;
		}/* switch */
	}/* while */
	if (argc - optind != 1)
		show_usage++;
	if (show_usage) {
		fprintf(stderr, "%s\n", copyright);
		fprintf(stderr, usage_msg, myname, myname, myname);
	}
	else {
		int err = 0;
		FILE *logfp = NULL;
		if (logfile != NULL) {
			logfp = fopen(logfile, (truncate_log ? "w" : "a"));
			if (logfp == NULL) {
				perror(logfile);
				err = -1;
			}
		}
		if (err == 0) {
			if (hex_mode)
				hexdump_start();
			if (daemon)
				server(argv[optind], port, family, logfp);
			else {
				char *target = argv[optind];
				fprintf(stderr, "%s\n", copyright);
				fprintf(stderr, "Type \"Ctrl-M ~ .\" to exit.\n");
				if (*target == '/')
					nain(0, argv[optind], logfp);
				else
					oain(argv[optind], port, family, logfp);
			}
			ex = 0;
		}
	}
	return ex;
}/* main */
