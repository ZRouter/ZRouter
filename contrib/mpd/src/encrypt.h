
/*
 * encrypt.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _ENCRYPT_H_
#define	_ENCRYPT_H_

/*
 * DEFINITIONS
 */

/* Descriptor for one type of encryption */

  struct ecpstate;

  struct enctype
  {
    const char	*name;
    u_char	type;
    int		(*Init)(Bund b, int dir);
    void	(*Configure)(Bund b);
    void	(*UnConfigure)(Bund b);
    int		(*SubtractBloat)(Bund b, int size);
    void	(*Cleanup)(Bund b, int dir);
    u_char	*(*BuildConfigReq)(Bund b, u_char *cp);
    void	(*DecodeConfig)(Fsm fp, FsmOption opt, int mode);
    Mbuf	(*SendResetReq)(Bund b);
    Mbuf	(*RecvResetReq)(Bund b, int id, Mbuf bp);
    void	(*RecvResetAck)(Bund b, int id, Mbuf bp);
    int         (*Stat)(Context ctx, int dir);
    Mbuf	(*Encrypt)(Bund b, Mbuf plain);
    Mbuf	(*Decrypt)(Bund b, Mbuf cypher);
  };
  typedef const struct enctype	*EncType;

#endif

