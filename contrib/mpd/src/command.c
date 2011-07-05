
/*
 * command.c
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "console.h"
#ifndef NOWEB
#include "web.h"
#endif
#include "radsrv.h"
#include "command.h"
#include "ccp.h"
#include "iface.h"
#include "radius.h"
#include "bund.h"
#include "link.h"
#include "lcp.h"
#ifdef	USE_NG_NAT
#include "nat.h"
#endif
#include "ipcp.h"
#include "ip.h"
#include "ippool.h"
#include "devices.h"
#include "netgraph.h"
#include "ngfunc.h"
#ifdef CCP_MPPC
#include "ccp_mppc.h"
#endif
#include "util.h"
#ifdef USE_FETCH
#include <fetch.h>
#endif

/*
 * DEFINITIONS
 */

  struct layer {
    const char	*name;
    int		(*opener)(Context ctx);
    int		(*closer)(Context ctx);
    int		(*admit)(Context ctx, CmdTab cmd);
    const char	*desc;
  };
  typedef struct layer	*Layer;

  #define DEFAULT_OPEN_LAYER	"link"

  /* Set menu options */
  enum {
    SET_ENABLE,
    SET_DISABLE,
#ifdef USE_IPFW
    SET_RULE,
    SET_QUEUE,
    SET_PIPE,
    SET_TABLE,
#endif
#ifdef PHYSTYPE_PPTP
    SET_PPTPTO,
    SET_PPTPLIMIT,
#endif
#ifdef PHYSTYPE_L2TP
    SET_L2TPTO,
    SET_L2TPLIMIT,
#endif
    SET_MAX_CHILDREN,
#ifdef USE_NG_BPF
    SET_FILTER
#endif
  };

  enum {
    SHOW_IFACE,
    SHOW_IP,
    SHOW_USER,
    SHOW_SESSION,
    SHOW_MSESSION,
    SHOW_BUNDLE,
    SHOW_LINK,
    SHOW_PEER
  };

/*
 * INTERNAL FUNCTIONS
 */

  /* Commands */
  static int	ShowVersion(Context ctx, int ac, char *av[], void *arg);
  static int	ShowLayers(Context ctx, int ac, char *av[], void *arg);
  static int	ShowTypes(Context ctx, int ac, char *av[], void *arg);
  static int	ShowSummary(Context ctx, int ac, char *av[], void *arg);
  static int	ShowSessions(Context ctx, int ac, char *av[], void *arg);
  static int	ShowCustomer(Context ctx, int ac, char *av[], void *arg);
  static int	ShowEvents(Context ctx, int ac, char *av[], void *arg);
  static int	ShowGlobal(Context ctx, int ac, char *av[], void *arg);
  static int	OpenCommand(Context ctx, int ac, char *av[], void *arg);
  static int	CloseCommand(Context ctx, int ac, char *av[], void *arg);
  static int	LoadCommand(Context ctx, int ac, char *av[], void *arg);
  static int	ExitCommand(Context ctx, int ac, char *av[], void *arg);
  static int	QuitCommand(Context ctx, int ac, char *av[], void *arg);
  static int	GlobalSetCommand(Context ctx, int ac, char *av[], void *arg);
  static int	SetDebugCommand(Context ctx, int ac, char *av[], void *arg);

  /* Other stuff */
  static Layer	GetLayer(const char *name);

/*
 * INTERNAL VARIABLES
 */

  const struct cmdtab GlobalSetCmds[] = {
    { "enable {opt ...}", 		"Enable option" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable {opt ...}", 		"Disable option" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_DISABLE },
#ifdef USE_IPFW
    { "startrule {num}",		"Initial ipfw rule number" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_RULE },
    { "startqueue {num}", 		"Initial ipfw queue number" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_QUEUE },
    { "startpipe {num}",		"Initial ipfw pipe number" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_PIPE },
    { "starttable {num}", 		"Initial ipfw table number" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_TABLE },
#endif /* USE_IPFW */
#ifdef PHYSTYPE_L2TP
    { "l2tptimeout {sec}", 		"L2TP tunnel unused timeout" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_L2TPTO },
    { "l2tplimit {num}", 		"Calls per L2TP tunnel limit" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_L2TPLIMIT },
#endif
#ifdef PHYSTYPE_PPTP
    { "pptptimeout {sec}", 		"PPTP tunnel unused timeout" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_PPTPTO },
    { "pptplimit {num}", 		"Calls per PPTP tunnel limit" ,
       	GlobalSetCommand, NULL, 2, (void *) SET_PPTPLIMIT },
#endif
    { "max-children {num}",		"Max number of children",
	GlobalSetCommand, NULL, 2, (void *) SET_MAX_CHILDREN },
#ifdef USE_NG_BPF
    { "filter {num} add|clear [\"{flt}\"]",	"Global traffic filters management",
	GlobalSetCommand, NULL, 2, (void *) SET_FILTER },
#endif
    { NULL },
  };

  static const struct confinfo	gGlobalConfList[] = {
#ifdef USE_WRAP
    { 0,	GLOBAL_CONF_TCPWRAPPER,	"tcp-wrapper"	},
#endif
    { 0,	GLOBAL_CONF_ONESHOT,	"one-shot"	},
    { 0,	0,			NULL		},
  };

  static const struct cmdtab CreateCommands[] = {
    { "link [template|static] {name} {template|type}",	"Create link/template",
	LinkCreate, NULL, 2, NULL },
    { "bundle [template|static] {name} {template}",	"Create bundle/template",
	BundCreate, NULL, 2, NULL },
    { NULL },
  };

  static const struct cmdtab DestroyCommands[] = {
    { "link [{name}]",			"Destroy link/template",
	LinkDestroy, NULL, 2, NULL },
    { "bundle [{name}]",		"Destroy bundle/template",
	BundDestroy, NULL, 2, NULL },
    { NULL },
  };

  static const struct cmdtab ShowSessCmds[] = {
    { "iface {name}",			"Filter by iface name",
	ShowSessions, NULL, 2, (void *) SHOW_IFACE },
    { "ip {ip}",			"Filter by IP address",
	ShowSessions, NULL, 2, (void *) SHOW_IP },
    { "user {name}",			"Filter by user name",
	ShowSessions, NULL, 2, (void *) SHOW_USER },
    { "session {ID}",			"Filter by session ID",
	ShowSessions, NULL, 2, (void *) SHOW_SESSION },
    { "msession {ID}",			"Filter by msession ID",
	ShowSessions, NULL, 2, (void *) SHOW_MSESSION },
    { "bundle {name}",			"Filter by bundle name",
	ShowSessions, NULL, 2, (void *) SHOW_BUNDLE },
    { "link {name}",			"Filter by link name",
	ShowSessions, NULL, 2, (void *) SHOW_LINK },
    { "peer {name}",			"Filter by peer name",
	ShowSessions, NULL, 2, (void *) SHOW_PEER },
    { NULL },
  };

  static const struct cmdtab ShowCommands[] = {
    { "bundle [{name}]",		"Bundle status",
	BundStat, AdmitBund, 0, NULL },
    { "customer",			"Customer summary",
	ShowCustomer, NULL, 0, NULL },
    { "repeater [{name}]",		"Repeater status",
	RepStat, AdmitRep, 0, NULL },
    { "ccp",				"CCP status",
	CcpStat, AdmitBund, 0, NULL },
#ifdef CCP_MPPC
    { "mppc",				"MPPC status",
	MppcStat, AdmitBund, 0, NULL },
#endif
    { "ecp",				"ECP status",
	EcpStat, AdmitBund, 0, NULL },
    { "eap",				"EAP status",
	EapStat, AdmitLink, 0, NULL },
    { "events",				"Current events",
	ShowEvents, NULL, 0, NULL },
    { "ipcp",				"IPCP status",
	IpcpStat, AdmitBund, 0, NULL },
    { "ipv6cp",				"IPV6CP status",
	Ipv6cpStat, AdmitBund, 0, NULL },
    { "ippool",				"IP pool status",
	IPPoolStat, NULL, 0, NULL },
    { "iface",				"Interface status",
	IfaceStat, AdmitBund, 0, NULL },
    { "routes",				"IP routing table",
	IpShowRoutes, NULL, 0, NULL },
    { "layers",				"Layers to open/close",
	ShowLayers, NULL, 0, NULL },
    { "device",				"Physical device status",
	PhysStat, AdmitLink, 0, NULL },
#ifdef PHYSTYPE_PPTP
    { "pptp",				"PPTP tunnels status",
	PptpsStat, NULL, 0, NULL },
#endif
#ifdef PHYSTYPE_L2TP
    { "l2tp",				"L2TP tunnels status",
	L2tpsStat, NULL, 0, NULL },
#endif
    { "link",				"Link status",
	LinkStat, AdmitLink, 0, NULL },
    { "auth",				"Auth status",
	AuthStat, AdmitLink, 0, NULL },
    { "radius",				"RADIUS status",
	RadStat, AdmitLink, 0, NULL },
#ifdef RAD_COA_REQUEST
    { "radsrv",				"RADIUS server status",
	RadsrvStat, NULL, 0, NULL },
#endif
    { "lcp",				"LCP status",
	LcpStat, AdmitLink, 0, NULL },
#ifdef	USE_NG_NAT
    { "nat",				"NAT status",
	NatStat, AdmitBund, 0, NULL },
#endif
    { "mem",				"Memory map",
	MemStat, NULL, 0, NULL },
    { "console",			"Console status",
	ConsoleStat, NULL, 0, NULL },
#ifndef NOWEB
    { "web",				"Web status",
	WebStat, NULL, 0, NULL },
#endif
    { "user",				"Console users" ,
      	UserStat, NULL, 0, NULL },
    { "global",				"Global settings",
	ShowGlobal, NULL, 0, NULL },
    { "types",				"Supported device types",
	ShowTypes, NULL, 0, NULL },
    { "version",			"Version string",
	ShowVersion, NULL, 0, NULL },
    { "sessions [ {param} {value} ]",	"Active sessions",
	CMD_SUBMENU, NULL, 2, (void *) ShowSessCmds},
    { "summary",			"Daemon status summary",
	ShowSummary, NULL, 0, NULL },
    { NULL },
  };

  static const struct cmdtab SetCommands[] = {
    { "bundle ...",			"Bundle specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) BundSetCmds },
    { "link ...",			"Link specific stuff",
	CMD_SUBMENU, AdmitLink, 2, (void *) LinkSetCmds },
    { "iface ...",			"Interface specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) IfaceSetCmds },
    { "ipcp ...",			"IPCP specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) IpcpSetCmds },
    { "ipv6cp ...",			"IPV6CP specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) Ipv6cpSetCmds },
    { "ippool ...",			"IP pool specific stuff",
	CMD_SUBMENU, NULL, 2, (void *) IPPoolSetCmds },
    { "ccp ...",			"CCP specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) CcpSetCmds },
#ifdef CCP_MPPC
    { "mppc ...",			"MPPC specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) MppcSetCmds },
#endif
    { "ecp ...",			"ECP specific stuff",
	CMD_SUBMENU, AdmitBund, 2, (void *) EcpSetCmds },
    { "eap ...",			"EAP specific stuff",
	CMD_SUBMENU, AdmitLink, 2, (void *) EapSetCmds },
    { "auth ...",			"Auth specific stuff",
	CMD_SUBMENU, AdmitLink, 2, (void *) AuthSetCmds },
    { "radius ...",			"RADIUS specific stuff",
	CMD_SUBMENU, AdmitLink, 2, (void *) RadiusSetCmds },
#ifdef RAD_COA_REQUEST
    { "radsrv ...",			"RADIUS server specific stuff",
	CMD_SUBMENU, NULL, 2, (void *) RadsrvSetCmds },
#endif
    { "console ...",			"Console specific stuff",
	CMD_SUBMENU, NULL, 0, (void *) ConsoleSetCmds },
#ifndef NOWEB
    { "web ...",			"Web specific stuff",
	CMD_SUBMENU, NULL, 2, (void *) WebSetCmds },
#endif
    { "user {name} {password} [{priv}]",	"Add console user" ,
      	UserCommand, NULL, 2, NULL },
    { "global ...",			"Global settings",
	CMD_SUBMENU, NULL, 2, (void *) GlobalSetCmds },
#ifdef USE_NG_NETFLOW
    { "netflow ...", 			"NetFlow settings",
	CMD_SUBMENU, NULL, 2, (void *) NetflowSetCmds },
#endif
#ifdef	USE_NG_NAT
    { "nat ...", 			"Nat settings",
	CMD_SUBMENU, NULL, 2, (void *) NatSetCmds },
#endif
    { "debug level",			"Set netgraph debug level",
	SetDebugCommand, NULL, 2, NULL },
#define _WANT_DEVICE_CMDS
#include "devices.h"
    { NULL },
  };

  const struct cmdtab gCommands[] = {
    { "authname {name}",		"Choose link by auth name",
	AuthnameCommand, NULL, 0, NULL },
    { "bundle [{name}]",		"Choose/list bundles",
	BundCommand, NULL, 0, NULL },
    { "close [{layer}]",		"Close a layer",
	CloseCommand, NULL, 1, NULL },
    { "create ...",			"Create new item",
    	CMD_SUBMENU, NULL, 2, (void *) CreateCommands },
    { "destroy ...",			"Destroy item",
    	CMD_SUBMENU, NULL, 2, (void *) DestroyCommands },
    { "exit",				"Exit console",
	ExitCommand, NULL, 0, NULL },
    { "iface {iface}",			"Choose bundle by iface",
	IfaceCommand, NULL, 0, NULL },
    { "help ...",			"Help on any command",
	HelpCommand, NULL, 0, NULL },
    { "link {name}",			"Choose link",
	LinkCommand, NULL, 0, NULL },
    { "load [{file}] {label}",		"Read from config file",
	LoadCommand, NULL, 0, NULL },
    { "log [+/-{opt} ...]",		"Set/view log options",
	LogCommand, NULL, 2, NULL },
    { "msession {msesid}",		"Ch. bundle by msession-id",
	MSessionCommand, NULL, 0, NULL },
    { "open [{layer}]",			"Open a layer",
	OpenCommand, NULL, 1, NULL },
    { "quit",				"Quit program",
	QuitCommand, NULL, 2, NULL },
    { "repeater [{name}]",		"Choose/list repeaters",
	RepCommand, NULL, 0, NULL },
    { "session {sesid}",		"Choose link by session-id",
	SessionCommand, NULL, 0, NULL },
    { "set ...",			"Set parameters",
	CMD_SUBMENU, NULL, 0, (void *) SetCommands },
    { "show ...",			"Show status",
	CMD_SUBMENU, NULL, 0, (void *) ShowCommands },
    { NULL },
  };



/*
 * Layers
 */

  struct layer	gLayers[] = {
    { "iface",
      IfaceOpenCmd,
      IfaceCloseCmd,
      AdmitBund,
      "System interface"
    },
    { "ipcp",
      IpcpOpenCmd,
      IpcpCloseCmd,
      AdmitBund,
      "IPCP: IP control protocol"
    },
    { "ipv6cp",
      Ipv6cpOpenCmd,
      Ipv6cpCloseCmd,
      AdmitBund,
      "IPV6CP: IPv6 control protocol"
    },
    { "ccp",
      CcpOpenCmd,
      CcpCloseCmd,
      AdmitBund,
      "CCP: compression ctrl prot."
    },
    { "ecp",
      EcpOpenCmd,
      EcpCloseCmd,
      AdmitBund,
      "ECP: encryption ctrl prot."
    },
    { "bund",
      BundOpenCmd,
      BundCloseCmd,
      AdmitBund,
      "Multilink bundle"
    },
    { "link",
      LinkOpenCmd,
      LinkCloseCmd,
      AdmitLink,
      "Link layer"
    },
    { "device",
      PhysOpenCmd,
      PhysCloseCmd,
      AdmitLink,
      "Physical link layer"
    },
  };

  #define NUM_LAYERS	(sizeof(gLayers) / sizeof(*gLayers))

/*
 * DoCommand()
 *
 * Executes command. Returns TRUE if user wants to quit.
 */

int
DoCommand(Context ctx, int ac, char *av[], const char *file, int line)
{
    int		rtn, i;
    char	filebuf[100], cmd[256];

    ctx->errmsg[0] = 0;
    rtn = DoCommandTab(ctx, gCommands, ac, av);

    if (rtn) {
	if (file) {
	    snprintf(filebuf,sizeof(filebuf),"%s:%d: ", file, line);
	} else {
	    filebuf[0] = 0;
	}
	cmd[0] = 0;
	for (i = 0; i < ac; i++) {
	    if (i)
		strlcat(cmd, " ", sizeof(cmd));
	    strlcat(cmd, av[i], sizeof(cmd));
	}
    }

    switch (rtn) {
	case 0:
	    break;
    
	default:
	case CMD_ERR_USAGE:
	case CMD_ERR_UNFIN:
	    HelpCommand(ctx, ac, av, filebuf);
	    Log2(LG_ERR, ("%sError in '%s'", filebuf, cmd));
	    break;
	case CMD_ERR_UNDEF:
    	    Printf("%sUnknown command: '%s'. Try \"help\".\r\n", filebuf, cmd);
	    Log2(LG_ERR, ("%sUnknown command: '%s'. Try \"help\".", filebuf, cmd));
	    break;
	case CMD_ERR_AMBIG:
    	    Printf("%sAmbiguous command: '%s'. Try \"help\".\r\n", filebuf, cmd);
	    Log2(LG_ERR, ("%sAmbiguous command: '%s'. Try \"help\".", filebuf, cmd));
	    break;
	case CMD_ERR_RECUR:
    	    Printf("%sRecursion detected for: '%s'!\r\n", filebuf, cmd);
	    Log2(LG_ERR, ("%sRecursion detected for: '%s'!", filebuf, cmd));
	    break;
	case CMD_ERR_NOCTX:
    	    Printf("%sIncorrect context for: '%s'\r\n", filebuf, cmd);
	    Log2(LG_ERR, ("%sIncorrect context for: '%s'", filebuf, cmd));
	    break;
	case CMD_ERR_OTHER:
    	    Printf("%sError in '%s': %s\r\n", filebuf, cmd, ctx->errmsg);
	    Log2(LG_ERR, ("%sError in '%s': %s", filebuf, cmd, ctx->errmsg));
	    break;
    }
  
    return(rtn);
}

/*
 * DoCommandTab()
 *
 * Execute command from given command menu
 */

int
DoCommandTab(Context ctx, CmdTab cmdlist, int ac, char *av[])
{
    CmdTab	cmd;
    int		rtn = 0;

    /* Huh? */
    if (ac <= 0)
	return(CMD_ERR_UNFIN);

    /* Find command */
    if ((rtn = FindCommand(ctx, cmdlist, av[0], &cmd)))
	return(rtn);

    /* Check command admissibility */
    if (cmd->admit && !(cmd->admit)(ctx, cmd))
	return(CMD_ERR_NOCTX);

    /* Find command and either execute or recurse into a submenu */
    if (cmd->func == CMD_SUBMENU) {
        if ((intptr_t)cmd->arg == (intptr_t)ShowSessCmds) {
            if (ac > 1)
	        rtn = DoCommandTab(ctx, (CmdTab) cmd->arg, ac - 1, av + 1);
            else
                rtn = ShowSessions(ctx, 0, NULL, NULL);
        } else
            rtn = DoCommandTab(ctx, (CmdTab) cmd->arg, ac - 1, av + 1);
    } else
	rtn = (cmd->func)(ctx, ac - 1, av + 1, cmd->arg);

    return(rtn);
}

/*
 * FindCommand()
 */

int
FindCommand(Context ctx, CmdTab cmds, char *str, CmdTab *cmdp)
{
    int		nmatch;
    int		len = strlen(str);

    for (nmatch = 0; cmds->name; cmds++) {
	if (cmds->name && !strncmp(str, cmds->name, len) &&
	  cmds->priv <= ctx->priv) {
	    *cmdp = cmds;
    	    nmatch++;
	}
    }
    switch (nmatch) {
	case 0:
    	    return(CMD_ERR_UNDEF);
	case 1:
    	    return(0);
	default:
    	    return(CMD_ERR_AMBIG);
    }
}

/********** COMMANDS **********/


/*
 * GlobalSetCommand()
 */

static int
GlobalSetCommand(Context ctx, int ac, char *av[], void *arg) 
{
    int val;

  if (ac == 0)
    return(-1);

  switch ((intptr_t)arg) {
    case SET_ENABLE:
      EnableCommand(ac, av, &gGlobalConf.options, gGlobalConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &gGlobalConf.options, gGlobalConfList);
      break;

#ifdef USE_IPFW
    case SET_RULE:
	if (rule_pool) 
	    Error("Rule pool is not empty. Impossible to set initial number");
	else {
	    val = atoi(*av);
	    if (val <= 0 || val>=65535)
		Error("Incorrect rule number");
	    else
		rule_pool_start = val;
	}
      break;

    case SET_QUEUE:
	if (queue_pool) 
	    Error("Queue pool is not empty. Impossible to set initial number");
	else {
	    val = atoi(*av);
	    if (val <= 0 || val>=65535)
		Error("Incorrect queue number");
	    else
		queue_pool_start = val;
	}
      break;

    case SET_PIPE:
	if (pipe_pool) 
	    Error("Pipe pool is not empty. Impossible to set initial number");
	else {
	    val = atoi(*av);
	    if (val <= 0 || val>=65535)
		Error("Incorrect pipe number");
	    else
		pipe_pool_start = val;
	}
      break;

    case SET_TABLE:
	if (table_pool) 
	    Error("Table pool is not empty. Impossible to set initial number");
	else {
	    val = atoi(*av);
	    if (val <= 0 || val>127) /* table 0 is usually possible but we deny it */
		Error("Incorrect table number");
	    else
		table_pool_start = val;
	}
      break;
#endif /* USE_IPFW */

#ifdef PHYSTYPE_L2TP
    case SET_L2TPTO:
	val = atoi(*av);
	if (val < 0 || val > 1000000)
	    Error("Incorrect L2TP timeout");
	else
	    gL2TPto = val;
      break;

    case SET_L2TPLIMIT:
	val = atoi(*av);
	if (val < 0 || val > 65536)
	    Error("Incorrect L2TP call limit");
	else
	    gL2TPtunlimit = val;
      break;
#endif

#ifdef PHYSTYPE_PPTP
    case SET_PPTPTO:
	val = atoi(*av);
	if (val < 0 || val > 1000000)
	    Error("Incorrect PPTP timeout");
	else
	    gPPTPto = val;
      break;

    case SET_PPTPLIMIT:
	val = atoi(*av);
	if (val <= 0 || val > 65536)
	    Error("Incorrect PPTP call limit");
	else
	    gPPTPtunlimit = val;
      break;
#endif

    case SET_MAX_CHILDREN:
	val = atoi(*av);
	if (val < 0 || val > 65536)
	    Error("Incorrect children links limit");
	else
	    gMaxChildren = val;
      break;

#ifdef USE_NG_BPF
    case SET_FILTER:
	if (ac == 4 && strcasecmp(av[1], "add") == 0) {
	    struct acl	**acls, *acls1;
	    int		i;
	    
	    i = atol(av[0]);
	    if (i <= 0 || i > ACL_FILTERS)
		Error("Wrong filter number\r\n");
	    acls = &(acl_filters[i - 1]);
	    
	    i = atol(av[2]);
	    if (i <= 0)
	        Error("Wrong acl number: %i\r\n", i);

	    acls1 = Malloc(MB_AUTH, sizeof(struct acl) + strlen(av[3]));
	    acls1->number = i;
	    acls1->real_number = 0;
	    strcpy(acls1->rule, av[3]);
	    while ((*acls != NULL) && ((*acls)->number < acls1->number))
	      acls = &((*acls)->next);

	    if (*acls == NULL) {
	        acls1->next = NULL;
	    } else if ((*acls)->number == acls1->number) {
		Freee(acls1);
	        Error("Duplicate acl\r\n");
	    } else {
	        acls1->next = *acls;
	    }
	    *acls = acls1;

	} else if (ac == 2 && strcasecmp(av[1], "clear") == 0) {
	    struct acl          *acls, *acls1;
	    int i;
	    
	    i = atoi(av[0]);
	    if (i <= 0 || i > ACL_FILTERS)
		Error("Wrong filter number\r\n");
	    	    
	    acls = acl_filters[i - 1];
	    while (acls != NULL) {
	        acls1 = acls->next;
	        Freee(acls);
	        acls = acls1;
	    };
	    acl_filters[i - 1] = NULL;
	} else
	    return(-1);
	break;
#endif /* USE_NG_BPF */
	
    default:
      return(-1);
  }

  return 0;
}

/*
 * HelpCommand()
 */

int
HelpCommand(Context ctx, int ac, char *av[], void *arg)
{
  int		depth, i;
  CmdTab	menu, cmd;
  char		*mark, *mark_save;
  const char	*errfmt;
  char		buf[256];
  int		err;

  for (mark = buf, depth = *buf = 0, menu = gCommands;
      depth < ac;
      depth++, menu = (CmdTab) cmd->arg) {
    if ((err = FindCommand(ctx, menu, av[depth], &cmd))) {
      int k;

      for (*buf = k = 0; k <= depth; k++)
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s%c",
	  av[k], k == depth ? '\0' : ' ');
      if (err == CMD_ERR_AMBIG)
	  errfmt = "%sAmbiguous command: '%s'.\r\n";
      else 
          errfmt = "%sUnknown command: '%s'.\r\n";
      if (arg) {
        Printf(errfmt, (char*)arg, buf);
      } else {
        Printf(errfmt, "", buf);
      }
      return(0);
    }
    sprintf(mark, depth ? " %s" : "%s", cmd->name);
    mark_save = mark;
    if ((mark = strchr(mark + 1, ' ')) == NULL)
      mark = mark_save + strlen(mark_save);
    if (cmd->func != CMD_SUBMENU)
    {
      Printf("Usage: %s\r\n", buf);
      return(0);
    }
  }

    /* Show list of available commands in this submenu */
    *mark = 0;
    if (!*buf)
	Printf("Available commands:\r\n");
    else
	Printf("Commands available under \"%s\":\r\n", buf);
    i = 0;
    for (cmd = menu; cmd->name; cmd++) {
	if (cmd->priv > ctx->priv)
	    continue;
	strlcpy(buf, cmd->name, sizeof(buf));
	if ((mark = strchr(buf, ' ')))
    	    *mark = 0;
	Printf(" %-9s: %-20s%s", buf, cmd->desc,
    	    (i & 1)? "\r\n" : "\t");
	i++;
    }
    if (i & 1)
	Printf("\r\n");
    return(0);
}

/*
 * SetDebugCommand()
 */

static int
SetDebugCommand(Context ctx, int ac, char *av[], void *arg)
{
  switch (ac) {
    case 1:
      NgSetDebug(atoi(av[0]));
      break;
    default:
      return(-1);
  }
  return(0);
}

/*
 * ShowVersion()
 */

static int
ShowVersion(Context ctx, int ac, char *av[], void *arg)
{
  Printf("MPD version: %s\r\n", gVersion);
  Printf("  Available features:\r\n");
#ifdef	USE_IPFW
  Printf("	ipfw rules	: yes\r\n");
#else
  Printf("	ipfw rules	: no\r\n");
#endif
#ifdef	USE_FETCH
  Printf("	config fetch	: yes\r\n");
#else
  Printf("	config fetch	: no\r\n");
#endif
#ifdef	USE_NG_BPF
  Printf("	ng_bpf		: yes\r\n");
#else
  Printf("	ng_bpf		: no\r\n");
#endif
#ifdef	USE_NG_CAR
  Printf("	ng_car		: yes\r\n");
#else
  Printf("	ng_car		: no\r\n");
#endif
#ifdef	USE_NG_DEFLATE
  Printf("	ng_deflate	: yes\r\n");
#else
  Printf("	ng_deflate	: no\r\n");
#endif
#ifdef	USE_NG_IPACCT
  Printf("	ng_ipacct	: yes\r\n");
#else
  Printf("	ng_ipacct	: no\r\n");
#endif
#ifdef  CCP_MPPC
  Printf("	ng_mppc (MPPC)	: %s\r\n", MPPCPresent?"yes":"no");
  Printf("	ng_mppc (MPPE)	: %s\r\n", MPPEPresent?"yes":"no");
#else
  Printf("	ng_mppc (MPPC)	: no\r\n");
  Printf("	ng_mppc (MPPE)	: no\r\n");
#endif
#ifdef	USE_NG_NAT
  Printf("	ng_nat		: yes\r\n");
#ifdef NG_NAT_DESC_LENGTH
  Printf("	nat redirect	: yes\r\n");
#else
  Printf("	nat redirect	: no\r\n");
#endif
#else
  Printf("	ng_nat		: no\r\n");
#endif
#ifdef	USE_NG_NETFLOW
  Printf("	ng_netflow	: yes\r\n");
#else
  Printf("	ng_netflow	: no\r\n");
#endif
#ifdef	USE_NG_PRED1
  Printf("	ng_pred1	: yes\r\n");
#else
  Printf("	ng_pred1	: emulated\r\n");
#endif
#ifdef	USE_NG_TCPMSS
  Printf("	ng_tcpmss	: yes\r\n");
#else
  Printf("	ng_tcpmss	: emulated\r\n");
#endif
  return(0);
}

/*
 * ShowEvents()
 */

static int
ShowEvents(Context ctx, int ac, char *av[], void *arg)
{
  EventDump(ctx, "mpd events");
  return(0);
}

/*
 * ShowGlobal()
 */

static int
ShowGlobal(Context ctx, int ac, char *av[], void *arg)
{
#ifdef USE_NG_BPF
    int	k;
#endif

    Printf("Global settings:\r\n");
#ifdef USE_IPFW
    Printf("	startrule	: %d\r\n", rule_pool_start);
    Printf("	startpipe	: %d\r\n", pipe_pool_start);
    Printf("	startqueue	: %d\r\n", queue_pool_start);
    Printf("	starttable	: %d\r\n", table_pool_start);
#endif
#ifdef PHYSTYPE_L2TP
    Printf("	l2tptimeout	: %d\r\n", gL2TPto);
    Printf("	l2tplimit	: %d\r\n", gL2TPtunlimit);
#endif
#ifdef PHYSTYPE_PPTP
    Printf("	pptptimeout	: %d\r\n", gPPTPto);
    Printf("	pptplimit	: %d\r\n", gPPTPtunlimit);
#endif
    Printf("	max-children	: %d\r\n", gMaxChildren);
    Printf("Global options:\r\n");
    OptStat(ctx, &gGlobalConf.options, gGlobalConfList);
#ifdef USE_NG_BPF
    Printf("Global traffic filters:\r\n");
    for (k = 0; k < ACL_FILTERS; k++) {
        struct acl *a = acl_filters[k];
        while (a) {
    	    Printf("\t%d#%d\t: '%s'\r\n", (k + 1), a->number, a->rule);
    	    a = a->next;
	}
    }
#endif
  return 0;
}


/*
 * ExitCommand()
 */

static int
ExitCommand(Context ctx, int ac, char *av[], void *arg)
{
    if (ctx->cs)
	ctx->cs->exit = TRUE;
    return(0);
}

/*
 * QuitCommand()
 */

static int
QuitCommand(Context ctx, int ac, char *av[], void *arg)
{
    if (ctx->cs)
	ctx->cs->exit = TRUE;
    SendSignal(SIGTERM);
    return(0);
}

/*
 * LoadCommand()
 */

static int
LoadCommand(Context ctx, int ac, char *av[], void *arg)
{
    char filename[128];
#ifdef USE_FETCH
    char buf[1024];
    FILE *f = NULL;
    FILE *rf = NULL;
    int fetch = 0;
#endif

    if (ac < 1 || ac > 2)
	return(-1);
    else {
	if (ctx->depth > 20)
    	    return(CMD_ERR_RECUR);
	ctx->depth++;

	if (ac == 1)
	    strlcpy(filename, gConfigFile, sizeof(filename));
	else {
#ifdef USE_FETCH
	    if (strncmp(av[0], "http://", 7) == 0 ||
		strncmp(av[0], "https://", 8) == 0 ||
		strncmp(av[0], "ftp://", 6) == 0) {
		    fetch = 1;
		strcpy(filename, "/tmp/mpd.conf.XXXXXX");
		mkstemp(filename);
		f = fopen(filename, "w");
		if (f == NULL) {
		    Printf("Can't create temporal file\r\n");
		    goto out;
		}
		rf = fetchGetURL(av[0], "");
		if (rf == NULL) {
		    Printf("Can't fetch '%s'\r\n", av[0]);
		    fclose(f);
		    goto out;
		}
		Printf("Fetching '%s' ...", av[0]);
		while (!feof(rf)) {
		    int b = fread(buf, 1, sizeof(buf), rf);
		    fwrite(buf, b, 1, f);
		}
		Printf(" done\r\n");
		fclose(f);
	    } else
#endif
		strlcpy(filename, av[0], sizeof(filename));
	}
	if (ReadFile(filename, av[ac - 1], DoCommand, ctx) < 0)
	    Printf("can't read configuration for \"%s\" from \"%s\"\r\n",
		av[ac -1], filename);
#ifdef USE_FETCH
out:	if (fetch)
	    unlink(filename);
#endif
	ctx->depth--;
    }
    return(0);
}

/*
 * OpenCommand()
 */

static int
OpenCommand(Context ctx, int ac, char *av[], void *arg)
{
    Layer	layer;
    const char	*name;

    switch (ac) {
    case 0:
        name = DEFAULT_OPEN_LAYER;
        break;
    case 1:
        name = av[0];
        break;
    default:
        return(-1);
    }
    if ((layer = GetLayer(name)) != NULL) {
	/* Check command admissibility */
	if (!layer->admit || (layer->admit)(ctx, NULL))
	    return (*layer->opener)(ctx);
	else
	    return (CMD_ERR_NOCTX);
    }
    return(-1);
}

/*
 * CloseCommand()
 */

static int
CloseCommand(Context ctx, int ac, char *av[], void *arg)
{
    Layer	layer;
    const char	*name;

    switch (ac) {
    case 0:
      name = DEFAULT_OPEN_LAYER;
      break;
    case 1:
      name = av[0];
      break;
    default:
      return(-1);
    }
    if ((layer = GetLayer(name)) != NULL) {
	/* Check command admissibility */
	if (!layer->admit || (layer->admit)(ctx, NULL))
	    return (*layer->closer)(ctx);
	else
	    return (CMD_ERR_NOCTX);
    }
    return(-1);
}

/*
 * GetLayer()
 */

static Layer
GetLayer(const char *name)
{
  int	k, found;

  if (name == NULL)
    name = "iface";
  for (found = -1, k = 0; k < NUM_LAYERS; k++) {
    if (!strncasecmp(name, gLayers[k].name, strlen(name))) {
      if (found > 0) {
	Log(LG_ERR, ("%s: ambiguous", name));
	return(NULL);
      } else
	found = k;
    }
  }
  if (found < 0) {
    Log(LG_ERR, ("unknown layer \"%s\": try \"show layers\"", name));
    return(NULL);
  }
  return(&gLayers[found]);
}

/*
 * ShowLayers()
 */

static int
ShowLayers(Context ctx, int ac, char *av[], void *arg)
{
  int	k;

  Printf("\tName\t\tDescription\r\n");
  Printf("\t----\t\t-----------\r\n");
  for (k = 0; k < NUM_LAYERS; k++)
    Printf("\t%s\t\t%s\r\n", gLayers[k].name, gLayers[k].desc);
  return(0);
}

/*
 * ShowTypes()
 */

static int
ShowTypes(Context ctx, int ac, char *av[], void *arg)
{
  PhysType	pt;
  int		k;

  Printf("\tName\t\tDescription\r\n");
  Printf("\t----\t\t-----------\r\n");
  for (k = 0; (pt = gPhysTypes[k]); k++)
    Printf("\t%s\t\t%s\r\n", pt->name, pt->descr);
  return(0);
}

/*
 * ShowSummary()
 */

static int
ShowSummary(Context ctx, int ac, char *av[], void *arg)
{
  int		b, l, f;
  Bund		B;
  Link  	L;
  Rep		R;
  char	buf[64];

  Printf("Current daemon status summary\r\n");
  Printf("Iface\tBund\t\tLink\tLCP\tDevice\t\tUser\t\tFrom\r\n");
  for (b = 0; b<gNumLinks; b++) {
    if ((L=gLinks[b]) != NULL && L->bund == NULL && L->rep == NULL) {
	Printf("\t\t\t");
	Printf("%s\t%s\t", 
	    L->name,
	    FsmStateName(L->lcp.fsm.state));
	PhysGetPeerAddr(L, buf, sizeof(buf));
	Printf("%s\t%s\t%8s\t%s", 
	    (L->type?L->type->name:""),
	    gPhysStateNames[L->state],
	    L->lcp.auth.params.authname,
	    buf
	);
	Printf("\r\n");
    }
  }
  for (b = 0; b<gNumBundles; b++) {
    if ((B=gBundles[b]) != NULL) {
	Printf("%s\t%s\t%s\t", B->iface.ifname, B->name, (B->iface.up?"Up":"Down"));
	f = 1;
	if (B->n_links == 0)
	    Printf("\r\n");
	else for (l = 0; l < NG_PPP_MAX_LINKS; l++) {
	    if ((L=B->links[l]) != NULL) {
		if (f == 1)
		    f = 0;
		else
		    Printf("\t\t\t");
		PhysGetPeerAddr(L, buf, sizeof(buf));
		Printf("%s\t%s\t%s\t%s\t%8s\t%s", 
		    L->name,
		    FsmStateName(L->lcp.fsm.state),
		    (L->type?L->type->name:""),
		    gPhysStateNames[L->state],
		    L->lcp.auth.params.authname,
		    buf
		    );
		Printf("\r\n");
	    }
	}
    }
  }
  for (b = 0; b < gNumReps; b++) {
    if ((R = gReps[b]) != NULL) {
	Printf("Repeater\t%s\t", R->name);
	f = 1;
	for (l = 0; l < 2; l++) {
	    if ((L = R->links[l])!= NULL) {
		if (f)
		    f = 0;
		else
		    Printf("\t\t\t");
		PhysGetPeerAddr(L, buf, sizeof(buf));
		Printf("%s\t%s\t%s\t%s\t%8s\t%s", 
		    L->name,
		    "",
		    (L->type?L->type->name:""),
		    gPhysStateNames[L->state],
		    "",
		    buf
		    );
		Printf("\r\n");
	    }
	}
	if (f)
	    Printf("\r\n");
    }
  }
  return(0);
}

/*
 * ShowSessions()
 */

static int
ShowSessions(Context ctx, int ac, char *av[], void *arg)
{
    int		l;
    Bund	B;
    Link  	L;
    char	peer[64], addr[64];

    if (ac != 0 && ac != 1)
	return (-1);

    for (l = 0; l < gNumLinks; l++) {
	if ((L=gLinks[l]) != NULL && L->session_id[0] && L->bund) {
	    B = L->bund;
	    u_addrtoa(&B->iface.peer_addr, addr, sizeof(addr));
	    PhysGetPeerAddr(L, peer, sizeof(peer));
	    if (ac == 0)
	        goto out;
	    switch ((intptr_t)arg) {
		case SHOW_IFACE:
		    if (strcmp(av[0], B->iface.ifname))
			continue;
		    break;
		case SHOW_IP:
		    if (strcmp(av[0], addr))
			continue;
		    break;
		case SHOW_USER:
		    if (strcmp(av[0], L->lcp.auth.params.authname))
			continue;
		    break;
		case SHOW_MSESSION:
		    if (strcmp(av[0], B->msession_id))
			continue;
		    break;
		case SHOW_SESSION:
		    if (strcmp(av[0], L->session_id))
			continue;
		    break;
		case SHOW_BUNDLE:
		    if (strcmp(av[0], B->name))
			continue;
		    break;
		case SHOW_LINK:
		    if (av[0][0] == '[') {
			int k;
			if (sscanf(av[0], "[%x]", &k) != 1)
			    return (-1);
			else {
			    if (L->id != k)
				continue;
			}
		    } else {
			if (strcmp(av[1], L->name))
			    continue;
		    }
		    break;
		case SHOW_PEER:
		    if (strcmp(av[0], peer))
			continue;
			break;
		default:
		    return (-1);
	    }
out:
	    Printf("%s\t%s\t%s\t%s\t", B->iface.ifname,
		addr, B->name, B->msession_id);
	    Printf("%s\t%d\t%s\t%s\t%s", 
		L->name,
		L->id,
		L->session_id,
		L->lcp.auth.params.authname,
		peer
	    );
	    Printf("\r\n");
	}
    }
    return(0);
}

/*
 * ShowCustomer()
 */

static int
ShowCustomer(Context ctx, int ac, char *av[], void *arg)
{
    Link	l = ctx->lnk;
    Bund	b = ctx->bund;
    IfaceState	iface;
    IfaceRoute	r;
    int		j;
    char	buf[64];
#if defined(USE_NG_BPF) || defined(USE_IPFW)
    struct acl	*a;
#endif

    if (b && b->iface.ifname[0]) {
	iface = &b->iface;
	Printf("Interface:\r\n");
	Printf("\tName            : %s\r\n", iface->ifname);
	Printf("\tStatus          : %s\r\n", iface->up ? (iface->dod?"DoD":"UP") : "DOWN");
	if (iface->up) {
	    Printf("\tSession time    : %ld seconds\r\n", (long int)(time(NULL) - iface->last_up));
#ifdef USE_NG_BPF
	    if (b->params.idle_timeout || iface->idle_timeout)
		Printf("\tIdle timeout    : %d seconds\r\n", b->params.idle_timeout?b->params.idle_timeout:iface->idle_timeout);
#endif
	    if (b->params.session_timeout || iface->session_timeout)
		Printf("\tSession timeout : %d seconds\r\n", b->params.session_timeout?b->params.session_timeout:iface->session_timeout);
	    Printf("\tMTU             : %d bytes\r\n", iface->mtu);
	}
	if (iface->ip_up && !u_rangeempty(&iface->self_addr)) {
	    Printf("\tIP Addresses    : %s -> ", u_rangetoa(&iface->self_addr,buf,sizeof(buf)));
	    Printf("%s\r\n", u_addrtoa(&iface->peer_addr,buf,sizeof(buf)));
	}
	if (iface->ipv6_up && !u_addrempty(&iface->self_ipv6_addr)) {
	    Printf("\tIPv6 Addresses  : %s%%%s -> ", 
		u_addrtoa(&iface->self_ipv6_addr,buf,sizeof(buf)), iface->ifname);
	    Printf("%s%%%s\r\n", u_addrtoa(&iface->peer_ipv6_addr,buf,sizeof(buf)), iface->ifname);
	}
	if (iface->up) {
	    if (!SLIST_EMPTY(&iface->routes) || !SLIST_EMPTY(&b->params.routes)) {
    		Printf("\tRoutes via peer :\r\n");
		SLIST_FOREACH(r, &iface->routes, next)
		    Printf("\t\t%s\r\n", u_rangetoa(&r->dest,buf,sizeof(buf)));
		SLIST_FOREACH(r, &b->params.routes, next)
		    Printf("\t\t%s\r\n", u_rangetoa(&r->dest,buf,sizeof(buf)));
	    }
#ifdef USE_IPFW
	    if (b->params.acl_pipe) {
		Printf("\tIPFW pipes      :\r\n");
		a = b->params.acl_pipe;
		while (a) {
		    Printf("\t\t%d (%d)\t'%s'\r\n", a->number, a->real_number, a->rule);
		    a = a->next;
		}
	    }
	    if (b->params.acl_queue) {
		Printf("\tIPFW queues     :\r\n");
		a = b->params.acl_queue;
		while (a) {
		    Printf("\t\t%d (%d)\t'%s'\r\n", a->number, a->real_number, a->rule);
	    	    a = a->next;
		}
	    }
	    if (b->params.acl_table) {
		Printf("\tIPFW tables     :\r\n");
		a = b->params.acl_table;
		while (a) {
		    if (a->number != 0)
			Printf("\t\t%d (%d)\t'%s'\r\n", a->number, a->real_number, a->rule);
		    else
			Printf("\t\t(%d)\t'%s'\r\n", a->real_number, a->rule);
		    a = a->next;
		}
	    }
	    if (b->params.acl_rule) {
		Printf("\tIPFW rules      :\r\n");
		a = b->params.acl_rule;
		while (a) {
	    	    Printf("\t\t%d (%d)\t'%s'\r\n", a->number, a->real_number, a->rule);
	    	    a = a->next;
		}
	    }
#endif /* USE_IPFW */
#ifdef USE_NG_BPF
	    if (b->params.acl_limits[0] || b->params.acl_limits[1]) {
		int k;
		Printf("\tTraffic filters :\r\n");
		for (k = 0; k < ACL_FILTERS; k++) {
		    a = b->params.acl_filters[k];
		    if (a == NULL)
			a = acl_filters[k];
		    while (a) {
			Printf("\t\t%d#%d\t'%s'\r\n", (k + 1), a->number, a->rule);
			a = a->next;
		    }
		}
		Printf("\tTraffic limits  :\r\n");
		for (k = 0; k < 2; k++) {
		    a = b->params.acl_limits[k];
		    while (a) {
			Printf("\t\t%s#%d%s%s\t'%s'\r\n", (k?"out":"in"), a->number,
			    ((a->name[0])?"#":""), a->name, a->rule);
			a = a->next;
		    }
		}
	    }
#endif /* USE_NG_BPF */
	}
    }
    if (b) {
	Printf("Bundle %s%s:\r\n", b->name, b->tmpl?" (template)":(b->stay?" (static)":""));
	Printf("\tStatus          : %s\r\n", b->open ? "OPEN" : "CLOSED");
	Printf("\tMulti Session Id: %s\r\n", b->msession_id);
	Printf("\tPeer authname   : \"%s\"\r\n", b->params.authname);
	if (b->n_up)
    	    Printf("\tSession time    : %ld seconds\r\n", (long int)(time(NULL) - l->last_up));

	if (b->peer_mrru) {
	    Printf("\tMultilink PPP:\r\n");
    	    Printf("\t\tPeer auth name : \"%s\"\r\n", b->params.authname);
    	    Printf("\t\tPeer discrimin.: %s\r\n", MpDiscrimText(&b->peer_discrim, buf, sizeof(buf)));
	}

	if (!b->tmpl) {
	    /* Show stats */
	    BundUpdateStats(b);
	    Printf("\tTraffic stats:\r\n");

	    Printf("\t\tInput octets   : %llu\r\n", (unsigned long long)b->stats.recvOctets);
	    Printf("\t\tInput frames   : %llu\r\n", (unsigned long long)b->stats.recvFrames);
	    Printf("\t\tOutput octets  : %llu\r\n", (unsigned long long)b->stats.xmitOctets);
	    Printf("\t\tOutput frames  : %llu\r\n", (unsigned long long)b->stats.xmitFrames);
	    Printf("\t\tBad protocols  : %llu\r\n", (unsigned long long)b->stats.badProtos);
	    Printf("\t\tRunts          : %llu\r\n", (unsigned long long)b->stats.runts);
	}
    }
    for (j = 0; j < NG_PPP_MAX_LINKS; j++) {
	if (b)
	    l = b->links[j];
	else if (j != 0)
	    l = NULL;
	if (l) {
	    char	buf[64];
	    Printf("Link %s:\r\n", l->name);
	    Printf("\tDevice type     : %s\r\n", l->type?l->type->name:"");
	    Printf("\tStatus          : %s/%s\r\n",
		FsmStateName(l->lcp.fsm.state),
		gPhysStateNames[l->state]);
	    Printf("\tSession Id      : %s\r\n", l->session_id);
	    Printf("\tPeer ident      : %s\r\n", l->lcp.peer_ident);
	    if (l->state == PHYS_STATE_UP)
    		Printf("\tSession time    : %ld seconds\r\n", (long int)(time(NULL) - l->last_up));

	    PhysGetSelfAddr(l, buf, sizeof(buf));
	    Printf("\tSelf addr (name): %s", buf);
	    PhysGetSelfName(l, buf, sizeof(buf));
	    Printf(" (%s)\r\n", buf);
	    PhysGetPeerAddr(l, buf, sizeof(buf));
	    Printf("\tPeer addr (name): %s", buf);
	    PhysGetPeerName(l, buf, sizeof(buf));
	    Printf(" (%s)\r\n", buf);
	    PhysGetPeerMacAddr(l, buf, sizeof(buf));
	    Printf("\tPeer MAC address: %s\r\n", buf);
	    PhysGetPeerIface(l, buf, sizeof(buf));
	    Printf("\tPeer iface      : %s\r\n", buf);
	    PhysGetCallingNum(l, buf, sizeof(buf));
	    Printf("\tCalling         : %s\r\n", buf);
	    PhysGetCalledNum(l, buf, sizeof(buf));
	    Printf("\tCalled          : %s\r\n", buf);

	    if (l->bund) {
		LinkUpdateStats(l);
		Printf("\tTraffic stats:\r\n");
		Printf("\t\tInput octets   : %llu\r\n", (unsigned long long)l->stats.recvOctets);
		Printf("\t\tInput frames   : %llu\r\n", (unsigned long long)l->stats.recvFrames);
		Printf("\t\tOutput octets  : %llu\r\n", (unsigned long long)l->stats.xmitOctets);
	        Printf("\t\tOutput frames  : %llu\r\n", (unsigned long long)l->stats.xmitFrames);
	        Printf("\t\tBad protocols  : %llu\r\n", (unsigned long long)l->stats.badProtos);
	        Printf("\t\tRunts          : %llu\r\n", (unsigned long long)l->stats.runts);
	    }
	}
    }

    return(0);
}

/*
 * AdmitBund()
 */

int
AdmitBund(Context ctx, CmdTab cmd)
{
    if (!ctx->bund)
	return(FALSE);
    return(TRUE);
}

/*
 * AdmitLink()
 */

int
AdmitLink(Context ctx, CmdTab cmd)
{
    if (!ctx->lnk)
	return(FALSE);
    return(TRUE);
}

/*
 * AdmitRep()
 */

int
AdmitRep(Context ctx, CmdTab cmd)
{
    if (!ctx->rep)
	return(FALSE);
    return(TRUE);
}

/*
 * AdmitDev()
 */

int
AdmitDev(Context ctx, CmdTab cmd)
{
    if (!cmd)
	return(FALSE);
    if (!ctx->lnk)
	return(FALSE);
    if (strncmp(cmd->name, ctx->lnk->type->name, strlen(ctx->lnk->type->name)))
	return(FALSE);
    return(TRUE);
}

