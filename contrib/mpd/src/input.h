
/*
 * input.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _INPUT_H_
#define _INPUT_H_

#include "defs.h"

/*
 * FUNCTIONS
 */

  extern void	InputFrame(Bund b, Link l, int proto, Mbuf bp);

#endif

