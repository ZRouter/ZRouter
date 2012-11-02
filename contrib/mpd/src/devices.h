
/*
 * devices.h
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#if !defined(_WANT_DEVICE_TYPES) && !defined(_WANT_DEVICE_CMDS)
#ifdef PHYSTYPE_MODEM
#include "modem.h"
#endif
#ifdef PHYSTYPE_NG_SOCKET
#include "ng.h"
#endif
#ifdef PHYSTYPE_TCP
#include "tcp.h"
#endif
#ifdef PHYSTYPE_UDP
#include "udp.h"
#endif
#ifdef PHYSTYPE_PPTP
#include "pptp.h"
#endif
#ifdef PHYSTYPE_L2TP
#include "l2tp.h"
#endif
#ifdef PHYSTYPE_PPPOE
#include "pppoe.h"
#endif
#endif

#ifdef _WANT_DEVICE_CMDS
#ifdef PHYSTYPE_MODEM
    { "modem ...",			"Modem specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) ModemSetCmds },
#endif
#ifdef PHYSTYPE_NG_SOCKET
    { "ng ...",				"Netgraph specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) NgSetCmds },
#endif
#ifdef PHYSTYPE_TCP
    { "tcp ...",			"TCP specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) TcpSetCmds },
#endif
#ifdef PHYSTYPE_UDP
    { "udp ...",			"UDP specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) UdpSetCmds },
#endif
#ifdef PHYSTYPE_PPTP
    { "pptp ...",			"PPTP specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) PptpSetCmds },
#endif
#ifdef PHYSTYPE_L2TP
    { "l2tp ...",			"L2TP specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) L2tpSetCmds },
#endif
#ifdef PHYSTYPE_PPPOE
    { "pppoe ...",			"PPPoE specific stuff",
	CMD_SUBMENU, AdmitDev, 2, (void *) PppoeSetCmds },
#endif
#endif

#ifdef _WANT_DEVICE_TYPES
#ifdef PHYSTYPE_MODEM
    (const PhysType) &gModemPhysType,
#endif
#ifdef PHYSTYPE_NG_SOCKET
    (const PhysType) &gNgPhysType,
#endif
#ifdef PHYSTYPE_TCP
    (const PhysType) &gTcpPhysType,
#endif
#ifdef PHYSTYPE_UDP
    (const PhysType) &gUdpPhysType,
#endif
#ifdef PHYSTYPE_PPTP
    (const PhysType) &gPptpPhysType,
#endif
#ifdef PHYSTYPE_L2TP
    (const PhysType) &gL2tpPhysType,
#endif
#ifdef PHYSTYPE_PPPOE
    (const PhysType) &gPppoePhysType,
#endif
#endif

