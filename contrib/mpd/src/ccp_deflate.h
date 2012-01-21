
/*
 * ccp_deflate.h
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#ifndef _CCP_DEFLATE_H_
#define _CCP_DEFLATE_H_

#include <netgraph/ng_message.h>
#include <netgraph/ng_deflate.h>

#include "defs.h"
#include "mbuf.h"
#include "comp.h"

/*
 * DEFINITIONS
 */

  struct deflateinfo {
	int	xmit_windowBits;
	int	recv_windowBits;
  };
  typedef struct deflateinfo	*DeflateInfo;

/*
 * VARIABLES
 */

  extern const struct comptype	gCompDeflateInfo;

#endif

