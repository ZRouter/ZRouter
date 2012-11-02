
/*
 * phys.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _PHYS_H_
#define _PHYS_H_

#include "mbuf.h"
#include "msg.h"

/*
 * DEFINITIONS
 */

  enum {
    PHYS_STATE_DOWN = 0,
    PHYS_STATE_CONNECTING,
    PHYS_STATE_READY,
    PHYS_STATE_UP
  };

  /* Descriptor for a given type of physical layer */
  struct phystype {
    const char	*name;				/* Name of device type */
    const char	*descr;				/* Description of device type */
    u_short	mtu, mru;			/* Not incl. addr/ctrl/fcs */
    int		tmpl;				/* This type is template, not an instance */
    int		(*tinit)(void);			/* Initialize device type info */
    void	(*tshutdown)(void);		/* Destroy device type info */
    int		(*init)(Link l);		/* Initialize device info */
    int		(*inst)(Link l, Link lt);	/* Instantiate device */
    void	(*open)(Link l);		/* Initiate connection */
    void	(*close)(Link l);		/* Disconnect */
    void	(*update)(Link l);		/* Update config */
    void	(*shutdown)(Link l);	/* Destroy all nodes */
    void	(*showstat)(Context ctx);	/* Shows type specific stats */
    int		(*originate)(Link l);	/* We originated connection? */
    int		(*issync)(Link l);		/* Link is synchronous */
    int		(*setaccm)(Link l, u_int32_t xmit, u_int32_t recv);	/* Set async accm */
    int		(*setcallingnum)(Link l, void *buf); 
						/* sets the calling number */
    int		(*setcallednum)(Link l, void *buf); 
						/* sets the called number */
    int		(*selfname)(Link l, void *buf, size_t buf_len); 
						/* returns the self-name */
    int		(*peername)(Link l, void *buf, size_t buf_len); 
						/* returns the peer-name */
    int		(*selfaddr)(Link l, void *buf, size_t buf_len); 
						/* returns the self-address (IP, MAC, whatever) */
    int		(*peeraddr)(Link l, void *buf, size_t buf_len); 
						/* returns the peer-address (IP, MAC, whatever) */
    int		(*peerport)(Link l, void *buf, size_t buf_len); 
						/* returns the peer-port */
    int		(*peermacaddr)(Link l, void *buf, size_t buf_len);
						/* returns the peer MAC address */
    int		(*peeriface)(Link l, void *buf, size_t buf_len);
						/* returns the peer interface */
    int		(*callingnum)(Link l, void *buf, size_t buf_len); 
						/* returns the calling number (IP, MAC, whatever) */
    int		(*callednum)(Link l, void *buf, size_t buf_len); 
						/* returns the called number (IP, MAC, whatever) */
  };
  typedef struct phystype	*PhysType;

/*
 * VARIABLES
 */

  extern const PhysType	gPhysTypes[];
  extern const char *gPhysStateNames[];

/*
 * FUNCTIONS
 */

  extern int		PhysOpenCmd(Context ctx);
  extern void		PhysOpen(Link l);
  extern int		PhysCloseCmd(Context ctx);
  extern void		PhysClose(Link l);
  extern void		PhysUp(Link l);
  extern void		PhysDown(Link l, const char *reason, const char *details);
  extern void		PhysIncoming(Link l);
  extern int		PhysGetUpperHook(Link l, char *path, char *hook);

  extern int		PhysSetAccm(Link l, uint32_t xmit, u_int32_t recv);
  extern int		PhysSetCallingNum(Link l, char *buf);
  extern int		PhysSetCalledNum(Link l, char *buf);
  extern int		PhysGetSelfName(Link l, char *buf, size_t buf_len);
  extern int		PhysGetPeerName(Link l, char *buf, size_t buf_len);
  extern int		PhysGetSelfAddr(Link l, char *buf, size_t buf_len);
  extern int		PhysGetPeerAddr(Link l, char *buf, size_t buf_len);
  extern int		PhysGetPeerPort(Link l, char *buf, size_t buf_len);
  extern int		PhysGetPeerMacAddr(Link l, char *buf, size_t buf_len);
  extern int		PhysGetPeerIface(Link l, char *buf, size_t buf_len);
  extern int		PhysGetCallingNum(Link l, char *buf, size_t buf_len);
  extern int		PhysGetCalledNum(Link l, char *buf, size_t buf_len);
  extern int		PhysIsBusy(Link l);
 
  extern int		PhysInit(Link l);
  extern int		PhysInst(Link l, Link lt);
  extern int		PhysGet(Link l);
  extern void		PhysShutdown(Link l);
  extern void		PhysSetDeviceType(Link l, char *typename);
  extern int		PhysGetOriginate(Link l);
  extern int		PhysIsSync(Link l);
  extern int		PhysStat(Context ctx, int ac, char *av[], void *arg);

#endif

