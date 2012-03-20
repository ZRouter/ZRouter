
/*
 * util.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include "ip.h"

  /*-
   * The following macro is used to update an
   * internet checksum.  "acc" is a 32-bit
   * accumulation of all the changes to the
   * checksum (adding in old 16-bit words and
   * subtracting out new words), and "cksum"
   * is the checksum value to be updated.
   */
  #define ADJUST_CHECKSUM(acc, cksum) { \
    acc += cksum; \
    if (acc < 0) { \
      acc = -acc; \
      acc = (acc >> 16) + (acc & 0xffff); \
      acc += acc >> 16; \
      cksum = (u_short) ~acc; \
    } else { \
      acc = (acc >> 16) + (acc & 0xffff); \
      acc += acc >> 16; \
      cksum = (u_short) acc; \
    } \
  }

  #define MAX_U_INT32 0xffffffffU

  #define IFCONF_BUFFSIZE	16384
  #define IFCONF_BUFFMAXSIZE	1048576

  struct configfile {
    char		*label;
    off_t		seek;
    int			linenum;
    struct configfile	*next;
  };

  struct configfiles {
    char		*filename;
    struct configfile	*sections;
    struct configfiles	*next;
  };
/*
 * FUNCTIONS
 */

  extern FILE		*OpenConfFile(const char *name, struct configfile **cf);
  extern int		SeekToLabel(FILE *fp, const char *label, int *lineNum, struct configfile *cf);

  extern char		*ReadFullLine(FILE *fp, int *lineNum, char *result, int resultlen);
  extern int		ReadFile(const char *filename, const char *target,
				int (*func)(Context ctx, int ac, char *av[], const char *file, int line), Context ctx);
  extern int		ParseLine(char *line, char *vec[], int max_args, int copy);
  extern void		FreeArgs(int ac, char *av[]);

  extern int		TcpGetListenPort(struct u_addr *addr, in_port_t port, int block);
  extern int		TcpAcceptConnection(int sock, struct sockaddr_storage *addr, int block);
  extern int		GetInetSocket(int type, struct u_addr *addr, in_port_t port, int block, char *ebuf, size_t len);

  extern int		OpenSerialDevice(const char *label, const char *path, int baudrate);
  extern int		ExclusiveOpenDevice(const char *label, const char *path);
  extern void		ExclusiveCloseDevice(const char *label, int fd, const char *path);

  extern int		PIDCheck(const char *lockfile, int killem);

  extern void		LengthenArray(void *arrayp, size_t esize,
				int *alenp, const char *type);

  extern int		ExecCmd(int log, const char *label, const char *fmt, ...)
				__printflike(3, 4);
  extern int		ExecCmdNosh(int log, const char *label, const char *fmt, ...)
				__printflike(3, 4);
  extern void		ShowMesg(int log, const char *pref, const char *buf, int len);
  extern char		*Bin2Hex(const unsigned char *bin, size_t len);
  extern u_char		*Hex2Bin(char *hexstr);
  extern u_short	Crc16(u_short fcs, u_char *cp, int len);
  extern u_long		GenerateMagic(void);

  extern int		GetAnyIpAddress(struct u_addr *ipaddr, const char *ifname);
  extern int		GetEther(struct u_addr *addr,
			    struct sockaddr_dl *hwaddr);
  extern int		GetPeerEther(struct u_addr *addr, struct sockaddr_dl *hwaddr);
  extern void     	ppp_util_ascify(char *buf, size_t max,
			    const char *bytes, size_t len);

#endif

