
/*
 * comp.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _COMP_H_
#define	_COMP_H_

/*
 * DEFINITIONS
 */

  #define COMP_DIR_XMIT		1
  #define COMP_DIR_RECV		2

  /* Compression type descriptor */
  struct ccpstate;

  struct comptype {
    const char	*name;
    u_char	type;
    u_char	mode;
    /*
     * This function should initialize internal state according
     * to the direction parameter (recv or xmit or both).
     */
    int		(*Init)(Bund b, int dir);
    /*
     * Reset any type-specific configuration options to their defaults.
     */
    int		(*Configure)(Bund b);
    /*
     * Do the opposite of Configure
     */
    void	(*UnConfigure)(Bund b);
    /*
     * This returns a string describing the configuration (optional).
     */
    char	*(*Describe)(Bund b, int dir, char *buf, size_t len);
    /*
     * Given that "size" is our MTU, return the maximum length frame
     * we can compress without the result being longer than "size".
     */
    int		(*SubtractBloat)(Bund b, int size);
    /*
     * Do the opposite of Init: ie., free memory, etc.
     */
    void	(*Cleanup)(Bund b, int dir);
    /*
     * This should add the type-specific stuff for a config-request
     * to the building config-request packet
     */
    u_char	*(*BuildConfigReq)(Bund b, u_char *cp, int *ok);
    /*
     * This should decode type-specific config request stuff.
     */
    void	(*DecodeConfig)(Fsm fp, FsmOption opt, int mode);
    /*
     * This should return an mbuf containing type-specific reset-request
     * contents if any, or else NULL.
     */
    Mbuf	(*SendResetReq)(Bund b);
    /*
     * Receive type-specific reset-request contents (possibly NULL).
     * Should return contents of reset-ack (NULL for empty). If no
     * reset-ack is desired, set *noAck to non-zero.
     */
    Mbuf	(*RecvResetReq)(Bund b, int id, Mbuf bp, int *noAck);
    /*
     * Receive type-specific reset-ack contents (possibly NULL).
     */
    void	(*RecvResetAck)(Bund b, int id, Mbuf bp);
    /*
     * Return true if compression was successfully negotiated in
     * the indicated direction.
     */
    int		(*Negotiated)(Bund b, int dir);
    /*
     * Prints current compressor status
     */
    int		(*Stat)(Context ctx, int dir);
    /*
     * For compression methods which is not implemented in kernel
     * here is support for user level functions.
     */
    Mbuf	(*Compress)(Bund b, Mbuf plain);
    Mbuf	(*Decompress)(Bund b, Mbuf comp);
  };
  typedef const struct comptype	*CompType;

#endif

