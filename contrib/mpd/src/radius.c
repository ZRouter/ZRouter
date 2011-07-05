/*
 * See ``COPYRIGHT.mpd''
 *
 * $Id: radius.c,v 1.155 2011/07/01 06:51:48 dmitryluhtionov Exp $
 *
 */

#include "ppp.h"
#ifdef PHYSTYPE_PPPOE
#include "pppoe.h"
#endif
#ifdef PHYSTYPE_PPTP
#include "pptp.h"
#endif
#ifdef PHYSTYPE_L2TP
#include "l2tp.h"
#endif
#ifdef PHYSTYPE_TCP
#include "tcp.h"
#endif
#ifdef PHYSTYPE_UDP
#include "udp.h"
#endif
#ifdef PHYSTYPE_MODEM
#include "modem.h"
#endif
#ifdef PHYSTYPE_NG_SOCKET
#include "ng.h"
#endif
#include "util.h"

#include <sys/types.h>

#include <radlib.h>
#include <radlib_vs.h>


/* Global variables */

  static int	RadiusSetCommand(Context ctx, int ac, char *av[], void *arg);
  static int	RadiusAddServer(AuthData auth, short request_type);
  static int	RadiusOpen(AuthData auth, short request_type);
  static int	RadiusStart(AuthData auth, short request_type);  
  static int	RadiusPutAuth(AuthData auth);
  static int	RadiusPutAcct(AuthData auth);
  static int	RadiusGetParams(AuthData auth, int eap_proxy);
  static int	RadiusSendRequest(AuthData auth);

/* Set menu options */

  enum {
    SET_SERVER,
    SET_ME,
    SET_MEV6,
    SET_IDENTIFIER,
    SET_TIMEOUT,
    SET_RETRIES,
    SET_CONFIG,
    SET_ENABLE,
    SET_DISABLE
  };

/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab RadiusSetCmds[] = {
    { "server {name} {secret} [{auth port}] [{acct port}]", "Set radius server parameters" ,
	RadiusSetCommand, NULL, 2, (void *) SET_SERVER },
    { "me {ip}",			"Set NAS IP address" ,
	RadiusSetCommand, NULL, 2, (void *) SET_ME },
    { "v6me {ip}",			"Set NAS IPv6 address" ,
	RadiusSetCommand, NULL, 2, (void *) SET_MEV6 },
    { "identifier {name}",		"Set NAS identifier string" ,
	RadiusSetCommand, NULL, 2, (void *) SET_IDENTIFIER },
    { "timeout {seconds}",		"Set timeout in seconds",
	RadiusSetCommand, NULL, 2, (void *) SET_TIMEOUT },
    { "retries {# retries}",		"set number of retries",
	RadiusSetCommand, NULL, 2, (void *) SET_RETRIES },
    { "config {path to radius.conf}",	"set path to config file for libradius",
	RadiusSetCommand, NULL, 2, (void *) SET_CONFIG },
    { "enable [opt ...]",		"Enable option",
	RadiusSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",		"Disable option",
	RadiusSetCommand, NULL, 2, (void *) SET_DISABLE },
    { NULL },
  };

/*
 * INTERNAL VARIABLES
 */

  static struct confinfo	gConfList[] = {
    { 0,	RADIUS_CONF_MESSAGE_AUTHENTIC,	"message-authentic"	},
    { 0,	0,				NULL			},
  };

  #define RAD_NACK		0
  #define RAD_ACK		1

static int
rad_put_string_tag(struct rad_handle *h, int type, u_char tag, const char *str);

static int
rad_put_string_tag(struct rad_handle *h, int type, u_char tag, const char *str)
{
    char *tmp;
    int res;
    int len = strlen(str);
    
    if (tag == 0) {
	res = rad_put_attr(h, type, str, len);
    } else if (tag <= 0x1F) {
	tmp = Malloc(MB_RADIUS, len + 1);
	tmp[0] = tag;
	memcpy(tmp + 1, str, len);
	res = rad_put_attr(h, type, tmp, len + 1);
	Freee(tmp);
    } else {
	res = -1;
    }
    return (res);
}

/*
 * RadiusInit()
 */

void
RadiusInit(Link l)
{
    RadConf       const conf = &l->lcp.auth.conf.radius;

    memset(conf, 0, sizeof(*conf));
    conf->radius_retries = 3;
    conf->radius_timeout = 5;
}

int
RadiusAuthenticate(AuthData auth) 
{
    Log(LG_RADIUS, ("[%s] RADIUS: Authenticating user '%s'", 
	auth->info.lnkname, auth->params.authname));

    if ((RadiusStart(auth, RAD_ACCESS_REQUEST) == RAD_NACK) ||
	(RadiusPutAuth(auth) == RAD_NACK) ||
	(RadiusSendRequest(auth) == RAD_NACK)) {
	    return (-1);
    }
  
    return (0);
}

/*
 * RadiusAccount()
 *
 * Do RADIUS accounting
 * NOTE: thread-safety is needed here
 */
 
int 
RadiusAccount(AuthData auth) 
{
    Log(auth->acct_type != AUTH_ACCT_UPDATE ? LG_RADIUS : LG_RADIUS2,
	("[%s] RADIUS: Accounting user '%s' (Type: %d)",
	auth->info.lnkname, auth->params.authname, auth->acct_type));

    if ((RadiusStart(auth, RAD_ACCOUNTING_REQUEST) == RAD_NACK) ||
	(RadiusPutAcct(auth) == RAD_NACK) ||
	(RadiusSendRequest(auth) == RAD_NACK)) {
	    return (-1);
    }

    return (0);
}

/*
 * RadiusEapProxy()
 *
 * paction handler for RADIUS EAP Proxy requests.
 * Thread-Safety is needed here
 * auth->status must be set to AUTH_STATUS_FAIL, if the 
 * request couldn't sent, because for EAP a successful
 * RADIUS request is mandatory
 */
 
void
RadiusEapProxy(void *arg)
{
    AuthData	auth = (AuthData)arg;
    int		pos = 0, mlen = RAD_MAX_ATTR_LEN;

    Log(LG_RADIUS, ("[%s] RADIUS: EAP proxying user '%s'", 
	auth->info.lnkname, auth->params.authname));

    if (RadiusStart(auth, RAD_ACCESS_REQUEST) == RAD_NACK) {
	auth->status = AUTH_STATUS_FAIL;  
	return;
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_USER_NAME: %s", 
	auth->info.lnkname, auth->params.authname));
    if (rad_put_string(auth->radius.handle, RAD_USER_NAME, auth->params.authname) == -1) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_USER_NAME failed %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	auth->status = AUTH_STATUS_FAIL;    
	return;
    }

    for (pos = 0; pos <= auth->params.eapmsg_len; pos += RAD_MAX_ATTR_LEN) {
	char	chunk[RAD_MAX_ATTR_LEN];

	if (pos + RAD_MAX_ATTR_LEN > auth->params.eapmsg_len)
    	    mlen = auth->params.eapmsg_len - pos;

    	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_EAP_MESSAGE: len %d of %d",
	    auth->info.lnkname, mlen, auth->params.eapmsg_len));
	memcpy(chunk, &auth->params.eapmsg[pos], mlen);
	if (rad_put_attr(auth->radius.handle, RAD_EAP_MESSAGE, chunk, mlen) == -1) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_EAP_MESSAGE failed %s",
		auth->info.lnkname, rad_strerror(auth->radius.handle)));
    	    auth->status = AUTH_STATUS_FAIL;      
    	    return;
	}
    }

    if (RadiusSendRequest(auth) == RAD_NACK) {
	auth->status = AUTH_STATUS_FAIL;
	return;
    }

    return;
}

void
RadiusClose(AuthData auth) 
{
    if (auth->radius.handle != NULL)
	rad_close(auth->radius.handle);  
    auth->radius.handle = NULL;
}

int
RadStat(Context ctx, int ac, char *av[], void *arg)
{
  Auth		const a = &ctx->lnk->lcp.auth;
  RadConf	const conf = &a->conf.radius;
  int		i;
  char		*buf;
  RadServe_Conf	server;
  char		buf1[64];

  Printf("Configuration:\r\n");
  Printf("\tTimeout      : %d\r\n", conf->radius_timeout);
  Printf("\tRetries      : %d\r\n", conf->radius_retries);
  Printf("\tConfig-file  : %s\r\n", (conf->file ? conf->file : "none"));
  Printf("\tMe (NAS-IP)  : %s\r\n", inet_ntoa(conf->radius_me));
  Printf("\tv6Me (NAS-IP): %s\r\n", u_addrtoa(&conf->radius_mev6, buf1, sizeof(buf1)));
  Printf("\tIdentifier   : %s\r\n", (conf->identifier ? conf->identifier : ""));
  
  if (conf->server != NULL) {

    server = conf->server;
    i = 1;

    while (server) {
      Printf("\t---------------  Radius Server %d ---------------\r\n", i);
      Printf("\thostname   : %s\r\n", server->hostname);
      Printf("\tsecret     : *********\r\n");
      Printf("\tauth port  : %d\r\n", server->auth_port);
      Printf("\tacct port  : %d\r\n", server->acct_port);
      i++;
      server = server->next;
    }

  }

  Printf("RADIUS options\r\n");
  OptStat(ctx, &conf->options, gConfList);

  Printf("Data:\r\n");
  Printf("\tAuthenticated  : %s\r\n", a->params.authentic == AUTH_CONF_RADIUS_AUTH ? 
  	"yes" : "no");
  
  buf = Bin2Hex(a->params.state, a->params.state_len); 
  Printf("\tState          : 0x%s\r\n", buf);
  Freee(buf);
  
  buf = Bin2Hex(a->params.class, a->params.class_len);
  Printf("\tClass          : 0x%s\r\n", buf);
  Freee(buf);
  
  return (0);
}

static int
RadiusAddServer(AuthData auth, short request_type)
{
  RadConf	const c = &auth->conf.radius;
  RadServe_Conf	s;

  if (c->server == NULL)
    return (RAD_ACK);

  s = c->server;
  while (s) {

    if (request_type == RAD_ACCESS_REQUEST) {
      if (s->auth_port != 0) {
	Log(LG_RADIUS2, ("[%s] RADIUS: Adding server %s %d", auth->info.lnkname, s->hostname, s->auth_port));
        if (rad_add_server (auth->radius.handle, s->hostname,
	    s->auth_port,
	    s->sharedsecret,
	    c->radius_timeout,
	    c->radius_retries) == -1) {
		Log(LG_RADIUS, ("[%s] RADIUS: Adding server error: %s", auth->info.lnkname,
		    rad_strerror(auth->radius.handle)));
		return (RAD_NACK);
        }
      }
    } else if (s->acct_port != 0) {
	Log(LG_RADIUS2, ("[%s] RADIUS: Adding server %s %d", auth->info.lnkname, s->hostname, s->acct_port));
        if (rad_add_server (auth->radius.handle, s->hostname,
	    s->acct_port,
	    s->sharedsecret,
	    c->radius_timeout,
	    c->radius_retries) == -1) {
		Log(LG_RADIUS, ("[%s] RADIUS: Adding server error: %s", auth->info.lnkname, 
		    rad_strerror(auth->radius.handle)));
		return (RAD_NACK);
        }
    }

    s = s->next;
  }

  return (RAD_ACK);
}
  
/* Set menu options */
static int
RadiusSetCommand(Context ctx, int ac, char *av[], void *arg) 
{
  RadConf	const conf = &ctx->lnk->lcp.auth.conf.radius;
  RadServe_Conf	server;
  RadServe_Conf	t_server;
  int		val, count;
  struct u_addr t;
  int 		auth_port = 1812;
  int 		acct_port = 1813;

  if (ac == 0)
      return(-1);

    switch ((intptr_t)arg) {

      case SET_SERVER:
	if (ac > 4 || ac < 2) {
	  return(-1);
	}

	count = 0;
	for ( t_server = conf->server ; t_server ;
	  t_server = t_server->next) {
	  count++;
	}
	if (count > RADIUS_MAX_SERVERS) {
	    Error("cannot configure more than %d servers",
		RADIUS_MAX_SERVERS);
	}
	if (strlen(av[0]) > MAXHOSTNAMELEN)
	    Error("Hostname too long. > %d char.", MAXHOSTNAMELEN);
	if (strlen(av[1]) > 127)
	    Error("Shared Secret too long. > 127 char.");
	if (ac > 2) {
	    auth_port = atoi(av[2]);
	    if (auth_port < 0 || auth_port >= 65535)
		Error("Auth Port number too high. > 65535");
	}
	if (ac > 3) {
	    acct_port = atoi(av[3]);
	    if (acct_port < 0 || acct_port >= 65535)
		Error("Acct Port number too high > 65535");
	}
	if (auth_port == 0 && acct_port == 0)
	    Error("At least one port must be specified.");

	server = Malloc(MB_RADIUS, sizeof(*server));
	server->auth_port = auth_port;
	server->acct_port = acct_port;
	server->next = NULL;
	server->hostname = Mstrdup(MB_RADIUS, av[0]);
	server->sharedsecret = Mstrdup(MB_RADIUS, av[1]);
	if (conf->server != NULL)
	    server->next = conf->server;
	conf->server = server;

	break;

      case SET_ME:
        if (ParseAddr(*av, &t, ALLOW_IPV4)) {
	    u_addrtoin_addr(&t,&conf->radius_me);
	} else
	    Error("Bad NAS address '%s'.", *av);
	break;

      case SET_MEV6:
        if (!ParseAddr(*av, &conf->radius_mev6, ALLOW_IPV6))
	    Error("Bad NAS address '%s'.", *av);
	break;

      case SET_TIMEOUT:
	val = atoi(*av);
	  if (val <= 0)
	    Error("Timeout must be positive.");
	  else
	    conf->radius_timeout = val;
	break;

      case SET_RETRIES:
	val = atoi(*av);
	if (val <= 0)
	  Error("Retries must be positive.");
	else
	  conf->radius_retries = val;
	break;

      case SET_CONFIG:
	if (strlen(av[0]) > PATH_MAX) {
	  Error("RADIUS: Config file name too long.");
	} else {
	  Freee(conf->file);
	  conf->file = Mstrdup(MB_RADIUS, av[0]);
	}
	break;

      case SET_IDENTIFIER:
	if (strlen(av[0]) > RAD_MAX_ATTR_LEN) {
	  Error("RADIUS: Identifier too long.");
	} else {
	  Freee(conf->identifier);
	  if (av[0][0] == 0)
		conf->identifier = NULL;
	  else
		conf->identifier = Mstrdup(MB_RADIUS, av[0]);
	}
	break;

    case SET_ENABLE:
      EnableCommand(ac, av, &conf->options, gConfList);
      break;

    case SET_DISABLE:
      DisableCommand(ac, av, &conf->options, gConfList);
      break;

      default:
	assert(0);
    }

    return 0;
}

static int
RadiusOpen(AuthData auth, short request_type)
{
    RadConf 	const conf = &auth->conf.radius;

    if (request_type == RAD_ACCESS_REQUEST) {
  
	if ((auth->radius.handle = rad_open()) == NULL) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: rad_open failed", auth->info.lnkname));
    	    return (RAD_NACK);
	}
    } else { /* RAD_ACCOUNTING_REQUEST */
  
	if ((auth->radius.handle = rad_acct_open()) == NULL) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: rad_acct_open failed", auth->info.lnkname));
    	    return (RAD_NACK);
	}
    }
  
    if (conf->file && strlen(conf->file)) {
	Log(LG_RADIUS2, ("[%s] RADIUS: using %s", auth->info.lnkname, conf->file));
	if (rad_config(auth->radius.handle, conf->file) != 0) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: rad_config: %s", auth->info.lnkname, 
    		rad_strerror(auth->radius.handle)));
    	    return (RAD_NACK);
	}
    }

    if (RadiusAddServer(auth, request_type) == RAD_NACK)
	return (RAD_NACK);
  
    return (RAD_ACK);
}

static int
RadiusStart(AuthData auth, short request_type)
{
  RadConf 	const conf = &auth->conf.radius;  
  char		host[MAXHOSTNAMELEN];
  int		porttype;
  char		buf[48];
  char		*tmpval;

  if (RadiusOpen(auth, request_type) == RAD_NACK) 
    return RAD_NACK;

  if (rad_create_request(auth->radius.handle, request_type) == -1) {
    Log(LG_RADIUS, ("[%s] RADIUS: rad_create_request: %s", 
      auth->info.lnkname, rad_strerror(auth->radius.handle)));
    return (RAD_NACK);
  }

    if (conf->identifier) {
	tmpval = conf->identifier;
    } else {
	if (gethostname(host, sizeof(host)) == -1) {
	    Log(LG_RADIUS, ("[%s] RADIUS: gethostname() for RAD_NAS_IDENTIFIER failed", 
    		auth->info.lnkname));
	    return (RAD_NACK);
	}
	tmpval = host;
    }
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_NAS_IDENTIFIER: %s", 
	auth->info.lnkname, tmpval));
    if (rad_put_string(auth->radius.handle, RAD_NAS_IDENTIFIER, tmpval) == -1)  {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_NAS_IDENTIFIER failed %s", auth->info.lnkname,
    	    rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }
  
  if (conf->radius_me.s_addr != 0) {
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_NAS_IP_ADDRESS: %s", 
      auth->info.lnkname, inet_ntoa(conf->radius_me)));
    if (rad_put_addr(auth->radius.handle, RAD_NAS_IP_ADDRESS, conf->radius_me) == -1) {
      Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_NAS_IP_ADDRESS failed %s", 
	auth->info.lnkname, rad_strerror(auth->radius.handle)));
      return (RAD_NACK);
    }
  }

  if (!u_addrempty(&conf->radius_mev6)) {
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_NAS_IPV6_ADDRESS: %s", 
      auth->info.lnkname, u_addrtoa(&conf->radius_mev6,buf,sizeof(buf))));
    if (rad_put_attr(auth->radius.handle, RAD_NAS_IPV6_ADDRESS, &conf->radius_mev6.u.ip6, sizeof(conf->radius_mev6.u.ip6)) == -1) {
      Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_NAS_IPV6_ADDRESS failed %s", 
	auth->info.lnkname, rad_strerror(auth->radius.handle)));
      return (RAD_NACK);
    }
  }

  /* Insert the Message Authenticator RFC 3579
   * If using EAP this is mandatory
   */
  if ((Enabled(&conf->options, RADIUS_CONF_MESSAGE_AUTHENTIC)
	|| auth->proto == PROTO_EAP)
	&& request_type != RAD_ACCOUNTING_REQUEST) {
    Log(LG_RADIUS2, ("[%s] RADIUS: Put Message Authenticator", auth->info.lnkname));
    if (rad_put_message_authentic(auth->radius.handle) == -1) {
      Log(LG_RADIUS, ("[%s] RADIUS: Put message_authentic failed %s", 
        auth->info.lnkname, rad_strerror(auth->radius.handle)));
      return (RAD_NACK);
    }
  }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_SESSION_ID: %s", 
	auth->info.lnkname, auth->info.session_id));
    if (rad_put_string(auth->radius.handle, RAD_ACCT_SESSION_ID, auth->info.session_id) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_SESSION_ID: %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_NAS_PORT: %d", 
	auth->info.lnkname, auth->info.linkID));
    if (rad_put_int(auth->radius.handle, RAD_NAS_PORT, auth->info.linkID) == -1)  {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_NAS_PORT failed %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

#ifdef PHYSTYPE_MODEM
    if (auth->info.phys_type == &gModemPhysType) {
	porttype = RAD_ASYNC;
    } else 
#endif
#ifdef PHYSTYPE_NG_SOCKET
    if (auth->info.phys_type == &gNgPhysType) {
	porttype = RAD_SYNC;
    } else 
#endif
#ifdef PHYSTYPE_PPPOE
    if (auth->info.phys_type == &gPppoePhysType) {
	porttype = RAD_ETHERNET;
    } else 
#endif
    {
	porttype = RAD_VIRTUAL;
    };
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_NAS_PORT_TYPE: %d", 
	auth->info.lnkname, porttype));
    if (rad_put_int(auth->radius.handle, RAD_NAS_PORT_TYPE, porttype) == -1) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_NAS_PORT_TYPE failed %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_SERVICE_TYPE: RAD_FRAMED", 
	auth->info.lnkname));
    if (rad_put_int(auth->radius.handle, RAD_SERVICE_TYPE, RAD_FRAMED) == -1) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_SERVICE_TYPE failed %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }
  
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_FRAMED_PROTOCOL: RAD_PPP", 
	auth->info.lnkname));
    if (rad_put_int(auth->radius.handle, RAD_FRAMED_PROTOCOL, RAD_PPP) == -1) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_FRAMED_PROTOCOL failed %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    if (auth->params.state != NULL) {
	tmpval = Bin2Hex(auth->params.state, auth->params.state_len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_STATE: 0x%s", auth->info.lnkname, tmpval));
	Freee(tmpval);
	if (rad_put_attr(auth->radius.handle, RAD_STATE, auth->params.state, auth->params.state_len) == -1) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_STATE failed %s", 
    		auth->info.lnkname, rad_strerror(auth->radius.handle)));
    	    return (RAD_NACK);
	}
    }

    if (auth->params.class != NULL) {
	tmpval = Bin2Hex(auth->params.class, auth->params.class_len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_CLASS: 0x%s", auth->info.lnkname, tmpval));
	Freee(tmpval);
	if (rad_put_attr(auth->radius.handle, RAD_CLASS, auth->params.class, auth->params.class_len) == -1) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_CLASS failed %s", 
    		auth->info.lnkname, rad_strerror(auth->radius.handle)));
    	    return (RAD_NACK);
	}
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_CALLING_STATION_ID: %s",
        auth->info.lnkname, auth->params.callingnum));
    if (rad_put_string(auth->radius.handle, RAD_CALLING_STATION_ID,
    	    auth->params.callingnum) == -1) {
        Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_CALLING_STATION_ID failed %s",
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    	return (RAD_NACK);
    }
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_CALLED_STATION_ID: %s",
	auth->info.lnkname, auth->params.callednum));
    if (rad_put_string(auth->radius.handle, RAD_CALLED_STATION_ID,
    	    auth->params.callednum) == -1) {
    	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_CALLED_STATION_ID failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    	return (RAD_NACK);
    }

    if (auth->params.peeriface[0]) {
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_NAS_PORT_ID: %s",
    	auth->info.lnkname, auth->params.peeriface));
	if (rad_put_string(auth->radius.handle, RAD_NAS_PORT_ID,
	        auth->params.peeriface) == -1) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_NAS_PORT_ID failed %s",
		auth->info.lnkname, rad_strerror(auth->radius.handle)));
		return (RAD_NACK);
	}
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_LINK: %s", 
        auth->info.lnkname, auth->info.lnkname));
    if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_LINK, auth->info.lnkname) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_LINK: %s", auth->info.lnkname,
	    rad_strerror(auth->radius.handle)));
    	return (RAD_NACK);
    }

#ifdef PHYSTYPE_PPTP
    if (auth->info.phys_type == &gPptpPhysType) {
	porttype = 1;
    } else 
#endif
#ifdef PHYSTYPE_L2TP
    if (auth->info.phys_type == &gL2tpPhysType) {
	porttype = 3;
    } else 
#endif
#ifdef PHYSTYPE_TCP
    if (auth->info.phys_type == &gTcpPhysType) {
	porttype = -1;
    } else 
#endif
#ifdef PHYSTYPE_UDP
    if (auth->info.phys_type == &gUdpPhysType) {
	porttype = -2;
    } else 
#endif
#ifdef PHYSTYPE_PPPOE
    if (auth->info.phys_type == &gPppoePhysType) {
	porttype = -3;
    } else 
#endif
    {
	porttype = 0;
    };
    if (porttype > 0) {
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_TUNNEL_TYPE: %d", 
	    auth->info.lnkname, porttype));
	if (rad_put_int(auth->radius.handle, RAD_TUNNEL_TYPE, porttype) == -1) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_TUNNEL_TYPE failed %s", 
    		auth->info.lnkname, rad_strerror(auth->radius.handle)));
	    return (RAD_NACK);
	}
    }
    if (porttype != 0) {
	if (porttype == -3) {
	    porttype = 6;
	} else {
	    struct in6_addr ip6;
	    if (inet_pton(AF_INET6, auth->params.peeraddr, &ip6) == 1) {
		porttype = 2;
	    } else {
		porttype = 1;
	    }
	}
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_TUNNEL_MEDIUM_TYPE: %d", 
	    auth->info.lnkname, porttype));
	if (rad_put_int(auth->radius.handle, RAD_TUNNEL_MEDIUM_TYPE, porttype) == -1) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_TUNNEL_MEDIUM_TYPE failed %s", 
    		auth->info.lnkname, rad_strerror(auth->radius.handle)));
	    return (RAD_NACK);
	}
	if (auth->info.originate == LINK_ORIGINATE_LOCAL) {
	    tmpval = auth->params.peeraddr;
	} else {
	    tmpval = auth->params.selfaddr;
	}
	if (tmpval[0]) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_TUNNEL_SERVER_ENDPOINT: %s",
    		auth->info.lnkname, tmpval));
	    if (rad_put_string_tag(auth->radius.handle, RAD_TUNNEL_SERVER_ENDPOINT,
    		    0, tmpval) == -1) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_TUNNEL_SERVER_ENDPOINT failed %s",
    	    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	    }
	}
	if (auth->info.originate == LINK_ORIGINATE_LOCAL) {
	    tmpval = auth->params.selfaddr;
	} else {
	    tmpval = auth->params.peeraddr;
	}
	if (tmpval[0]) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_TUNNEL_CLIENT_ENDPOINT: %s",
    		auth->info.lnkname, tmpval));
	    if (rad_put_string_tag(auth->radius.handle, RAD_TUNNEL_CLIENT_ENDPOINT,
    		    0, tmpval) == -1) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_TUNNEL_CLIENT_ENDPOINT failed %s",
    	    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	    }
	}
	if (auth->info.originate == LINK_ORIGINATE_LOCAL) {
	    tmpval = auth->params.peername;
	} else {
	    tmpval = auth->params.selfname;
	}
	if (tmpval[0]) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_TUNNEL_SERVER_AUTH_ID: %s",
    		auth->info.lnkname, tmpval));
	    if (rad_put_string_tag(auth->radius.handle, RAD_TUNNEL_SERVER_AUTH_ID,
    		    0, tmpval) == -1) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_TUNNEL_SERVER_AUTH_ID failed %s",
    	    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	    }
	}
	if (auth->info.originate == LINK_ORIGINATE_LOCAL) {
	    tmpval = auth->params.selfname;
	} else {
	    tmpval = auth->params.peername;
	}
	if (tmpval[0]) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_TUNNEL_CLIENT_AUTH_ID: %s",
    		auth->info.lnkname, tmpval));
	    if (rad_put_string_tag(auth->radius.handle, RAD_TUNNEL_CLIENT_AUTH_ID,
    		    0, tmpval) == -1) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_TUNNEL_CLIENT_AUTH_ID failed %s",
    	    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	    }
	}
    }
#ifdef PHYSTYPE_PPPOE
    if (auth->info.phys_type == &gPppoePhysType) {
	if (auth->params.selfname[0]) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put ADSL-Agent-Circuit-Id: %s",
    		auth->info.lnkname, auth->params.selfname));
	    if (rad_put_vendor_string(auth->radius.handle, 3561, 1,
    		    auth->params.selfname) == -1) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put ADSL-Agent-Circuit-Id failed %s",
    	    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	    }
	}
	if (auth->params.peername[0]) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put ADSL-Agent-Remote-Id: %s",
    		auth->info.lnkname, auth->params.peername));
	    if (rad_put_vendor_string(auth->radius.handle, 3561, 2,
    		    auth->params.peername) == -1) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put ADSL-Agent-Remote-Id failed %s",
    	    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	    }
	}
    }
#endif

    return (RAD_ACK);
}

static int 
RadiusPutAuth(AuthData auth)
{
  ChapParams		const cp = &auth->params.chap;
  PapParams		const pp = &auth->params.pap;
  
  struct rad_chapvalue		rad_chapval;
  struct rad_mschapvalue	rad_mschapval;
  struct rad_mschapv2value	rad_mschapv2val;
  struct mschapvalue		*mschapval;
  struct mschapv2value		*mschapv2val;  

  Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_USER_NAME: %s", 
    auth->info.lnkname, auth->params.authname));
  if (rad_put_string(auth->radius.handle, RAD_USER_NAME, auth->params.authname) == -1) {
    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_USER_NAME failed %s", 
      auth->info.lnkname, rad_strerror(auth->radius.handle)));
    return (RAD_NACK);
  }

  if (auth->proto == PROTO_CHAP || auth->proto == PROTO_EAP) {
    switch (auth->alg) {

      case CHAP_ALG_MSOFT:
	if (cp->value_len != 49) {
	  Log(LG_RADIUS, ("[%s] RADIUS: RADIUS_CHAP (MSOFTv1) unrecognised key length %d/%d",
	    auth->info.lnkname, cp->value_len, 49));
	  return (RAD_NACK);
	}

	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP_CHALLENGE",
	  auth->info.lnkname));
	if (rad_put_vendor_attr(auth->radius.handle, RAD_VENDOR_MICROSOFT, RAD_MICROSOFT_MS_CHAP_CHALLENGE,
	    cp->chal_data, cp->chal_len) == -1)  {
	  Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP_CHALLENGE failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return (RAD_NACK);
	}

	mschapval = (struct mschapvalue *)cp->value;
	rad_mschapval.ident = auth->id;
	rad_mschapval.flags = 0x01;
	memcpy(rad_mschapval.lm_response, mschapval->lmHash, 24);
	memcpy(rad_mschapval.nt_response, mschapval->ntHash, 24);

	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP_RESPONSE",
	  auth->info.lnkname));
	if (rad_put_vendor_attr(auth->radius.handle, RAD_VENDOR_MICROSOFT, RAD_MICROSOFT_MS_CHAP_RESPONSE,
	    &rad_mschapval, sizeof(rad_mschapval)) == -1)  {
	  Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP_RESPONSE failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return (RAD_NACK);
	}
	break;

      case CHAP_ALG_MSOFTv2:
	if (cp->value_len != sizeof(*mschapv2val)) {
	  Log(LG_RADIUS, ("[%s] RADIUS: RADIUS_CHAP (MSOFTv2) unrecognised key length %d/%d",
	    auth->info.lnkname, cp->value_len, (int)sizeof(*mschapv2val)));
	  return (RAD_NACK);
	}
      
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP_CHALLENGE",
	  auth->info.lnkname));
	if (rad_put_vendor_attr(auth->radius.handle, RAD_VENDOR_MICROSOFT,
	    RAD_MICROSOFT_MS_CHAP_CHALLENGE, cp->chal_data, cp->chal_len) == -1) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP_CHALLENGE failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return (RAD_NACK);
	}

	mschapv2val = (struct mschapv2value *)cp->value;
	rad_mschapv2val.ident = auth->id;
	rad_mschapv2val.flags = mschapv2val->flags;
	memcpy(rad_mschapv2val.response, mschapv2val->ntHash,
	  sizeof(rad_mschapv2val.response));
	memset(rad_mschapv2val.reserved, '\0',
	  sizeof(rad_mschapv2val.reserved));
	memcpy(rad_mschapv2val.pchallenge, mschapv2val->peerChal,
	  sizeof(rad_mschapv2val.pchallenge));

	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MICROSOFT_MS_CHAP2_RESPONSE",
	  auth->info.lnkname));
	if (rad_put_vendor_attr(auth->radius.handle, RAD_VENDOR_MICROSOFT, RAD_MICROSOFT_MS_CHAP2_RESPONSE,
	    &rad_mschapv2val, sizeof(rad_mschapv2val)) == -1)  {
	  Log(LG_RADIUS, ("[%s] RADIUS: Put vendor_attr(RAD_MICROSOFT_MS_CHAP2_RESPONSE failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return (RAD_NACK);
	}
	break;

      case CHAP_ALG_MD5:
	/* RADIUS requires the CHAP Ident in the first byte of the CHAP-Password */
	rad_chapval.ident = auth->id;
	memcpy(rad_chapval.response, cp->value, cp->value_len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_CHAP_CHALLENGE",
	  auth->info.lnkname));
	if (rad_put_attr(auth->radius.handle, RAD_CHAP_CHALLENGE, cp->chal_data, cp->chal_len) == -1) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_CHAP_CHALLENGE failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return (RAD_NACK);
	}
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_CHAP_PASSWORD",
	  auth->info.lnkname));
	if (rad_put_attr(auth->radius.handle, RAD_CHAP_PASSWORD, &rad_chapval, cp->value_len + 1) == -1) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_CHAP_PASSWORD failed %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return (RAD_NACK);
	}
	break;
      
      default:
	Log(LG_RADIUS, ("[%s] RADIUS: RADIUS unkown CHAP ALG %d", 
	  auth->info.lnkname, auth->alg));
	return (RAD_NACK);
    }
  } else if (auth->proto == PROTO_PAP) {
        
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_USER_PASSWORD",
      auth->info.lnkname));
    if (rad_put_string(auth->radius.handle, RAD_USER_PASSWORD, pp->peer_pass) == -1) {
      Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_USER_PASSWORD failed %s", 
	auth->info.lnkname, rad_strerror(auth->radius.handle)));
      return (RAD_NACK);
    }
    
  } else {
    Log(LG_RADIUS, ("[%s] RADIUS: RADIUS unkown Proto %d", 
      auth->info.lnkname, auth->proto));
    return (RAD_NACK);
  }
  
  return (RAD_ACK);

}

static int 
RadiusPutAcct(AuthData auth)
{
    char *username;
    int	authentic;

    if (auth->acct_type == AUTH_ACCT_START) {
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_STATUS_TYPE: RAD_START", 
    	    auth->info.lnkname));
	if (rad_put_int(auth->radius.handle, RAD_ACCT_STATUS_TYPE, RAD_START)) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put STATUS_TYPE: %s", 
		auth->info.lnkname, rad_strerror(auth->radius.handle)));
    	    return (RAD_NACK);
	}
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_FRAMED_IP_ADDRESS: %s", 
	auth->info.lnkname, inet_ntoa(auth->info.peer_addr)));
    if (rad_put_addr(auth->radius.handle, RAD_FRAMED_IP_ADDRESS, auth->info.peer_addr)) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_FRAMED_IP_ADDRESS: %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    if (auth->params.netmask != 0) {
        struct in_addr	ip;
	widthtoin_addr(auth->params.netmask, &ip);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_FRAMED_IP_NETMASK: %s", 
	    auth->info.lnkname, inet_ntoa(ip)));
	if (rad_put_addr(auth->radius.handle, RAD_FRAMED_IP_NETMASK, ip)) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_FRAMED_IP_NETMASK: %s", 
    		auth->info.lnkname, rad_strerror(auth->radius.handle)));
	    return (RAD_NACK);
	}
    }

    username = auth->params.authname;
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_USER_NAME: %s", 
	auth->info.lnkname, username));
    if (rad_put_string(auth->radius.handle, RAD_USER_NAME, username) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_USER_NAME: %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_MULTI_SESSION_ID: %s", 
	auth->info.lnkname, auth->info.msession_id));
    if (rad_put_string(auth->radius.handle, RAD_ACCT_MULTI_SESSION_ID, auth->info.msession_id) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_MULTI_SESSION_ID: %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_BUNDLE: %s", 
        auth->info.lnkname, auth->info.bundname));
    if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_BUNDLE, auth->info.bundname) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_BUNDLE: %s", auth->info.lnkname,
	    rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_IFACE: %s", 
        auth->info.lnkname, auth->info.ifname));
    if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_IFACE, auth->info.ifname) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_IFACE: %s", auth->info.lnkname,
	    rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_IFACE_INDEX: %u", 
        auth->info.lnkname, auth->info.ifindex));
    if (rad_put_vendor_int(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_IFACE_INDEX, auth->info.ifindex) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_IFACE_INDEX: %s", auth->info.lnkname,
	    rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_LINK_COUNT: %d", 
	auth->info.lnkname, auth->info.n_links));
    if (rad_put_int(auth->radius.handle, RAD_ACCT_LINK_COUNT, auth->info.n_links) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_LINK_COUNT failed: %s", 
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

    if (auth->params.authentic == AUTH_CONF_RADIUS_AUTH) {
	authentic = RAD_AUTH_RADIUS;
    } else {
	authentic = RAD_AUTH_LOCAL;
    }
    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_AUTHENTIC: %d", 
	auth->info.lnkname, authentic));
    if (rad_put_int(auth->radius.handle, RAD_ACCT_AUTHENTIC, authentic) != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_AUTHENTIC failed: %s",
    	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	return (RAD_NACK);
    }

  if (auth->acct_type == AUTH_ACCT_STOP 
      || auth->acct_type == AUTH_ACCT_UPDATE) {
#ifdef USE_NG_BPF
    struct svcstatrec *ssr;
#endif

    if (auth->acct_type == AUTH_ACCT_STOP) {
        int	termCause = RAD_TERM_PORT_ERROR;

        Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_STATUS_TYPE: RAD_STOP", 
	    auth->info.lnkname));
        if (rad_put_int(auth->radius.handle, RAD_ACCT_STATUS_TYPE, RAD_STOP)) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_STATUS_TYPE: %s", 
		auth->info.lnkname, rad_strerror(auth->radius.handle)));
	    return (RAD_NACK);
        }

	if ((auth->info.downReason == NULL) || (!strcmp(auth->info.downReason, ""))) {
	  termCause = RAD_TERM_NAS_REQUEST;
	} else if (!strncmp(auth->info.downReason, STR_MANUALLY, strlen(STR_MANUALLY))) {
	  termCause = RAD_TERM_ADMIN_RESET;
	} else if (!strncmp(auth->info.downReason, STR_PEER_DISC, strlen(STR_PEER_DISC))) {
	  termCause = RAD_TERM_USER_REQUEST;
	} else if (!strncmp(auth->info.downReason, STR_ADMIN_SHUTDOWN, strlen(STR_ADMIN_SHUTDOWN))) {
	  termCause = RAD_TERM_ADMIN_REBOOT;
	} else if (!strncmp(auth->info.downReason, STR_FATAL_SHUTDOWN, strlen(STR_FATAL_SHUTDOWN))) {
	  termCause = RAD_TERM_NAS_REBOOT;
	} else if (!strncmp(auth->info.downReason, STR_IDLE_TIMEOUT, strlen(STR_IDLE_TIMEOUT))) {
	  termCause = RAD_TERM_IDLE_TIMEOUT;
	} else if (!strncmp(auth->info.downReason, STR_SESSION_TIMEOUT, strlen(STR_SESSION_TIMEOUT))) {
	  termCause = RAD_TERM_SESSION_TIMEOUT;
	} else if (!strncmp(auth->info.downReason, STR_DROPPED, strlen(STR_DROPPED))) {
	  termCause = RAD_TERM_LOST_CARRIER;
	} else if (!strncmp(auth->info.downReason, STR_ECHO_TIMEOUT, strlen(STR_ECHO_TIMEOUT))) {
	  termCause = RAD_TERM_LOST_SERVICE;
	} else if (!strncmp(auth->info.downReason, STR_PROTO_ERR, strlen(STR_PROTO_ERR))) {
	  termCause = RAD_TERM_SERVICE_UNAVAILABLE;
	} else if (!strncmp(auth->info.downReason, STR_LOGIN_FAIL, strlen(STR_LOGIN_FAIL))) {
	  termCause = RAD_TERM_USER_ERROR;
	} else if (!strncmp(auth->info.downReason, STR_PORT_UNNEEDED, strlen(STR_PORT_UNNEEDED))) {
	  termCause = RAD_TERM_PORT_UNNEEDED;
	};
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_TERMINATE_CAUSE: %s, RADIUS: %d",
	  auth->info.lnkname, auth->info.downReason, termCause));

        if (rad_put_int(auth->radius.handle, RAD_ACCT_TERMINATE_CAUSE, termCause) != 0) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_TERMINATE_CAUSE failed: %s",
		auth->info.lnkname, rad_strerror(auth->radius.handle)));
	    return (RAD_NACK);
        } 
    } else {
        Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_STATUS_TYPE: RAD_UPDATE", 
	    auth->info.lnkname));
        if (rad_put_int(auth->radius.handle, RAD_ACCT_STATUS_TYPE, RAD_UPDATE)) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_STATUS_TYPE: %s", 
		auth->info.lnkname, rad_strerror(auth->radius.handle)));
	    return (RAD_NACK);
        }
    }

    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_SESSION_TIME: %ld", 
        auth->info.lnkname, (long int)(time(NULL) - auth->info.last_up)));
    if (rad_put_int(auth->radius.handle, RAD_ACCT_SESSION_TIME, time(NULL) - auth->info.last_up) != 0) {
        Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_ACCT_SESSION_TIME failed: %s",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
        return (RAD_NACK);
    }

#ifdef USE_NG_BPF
    if (auth->params.std_acct[0][0] == 0) {
#endif
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_INPUT_OCTETS: %lu", 
    	    auth->info.lnkname, (long unsigned int)(auth->info.stats.recvOctets % MAX_U_INT32)));
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_INPUT_GIGAWORDS: %lu", 
    	    auth->info.lnkname, (long unsigned int)(auth->info.stats.recvOctets / MAX_U_INT32)));
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_INPUT_PACKETS: %lu", 
    	    auth->info.lnkname, (long unsigned int)(auth->info.stats.recvFrames)));
	if (rad_put_int(auth->radius.handle, RAD_ACCT_INPUT_OCTETS, auth->info.stats.recvOctets % MAX_U_INT32) != 0 ||
	    rad_put_int(auth->radius.handle, RAD_ACCT_INPUT_PACKETS, auth->info.stats.recvFrames) != 0 ||
	    rad_put_int(auth->radius.handle, RAD_ACCT_INPUT_GIGAWORDS, auth->info.stats.recvOctets / MAX_U_INT32) != 0) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put input stats: %s", auth->info.lnkname,
		    rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	}
#ifdef USE_NG_BPF
    }
    if (auth->params.std_acct[1][0] == 0) {
#endif
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_OUTPUT_OCTETS: %lu", 
    	    auth->info.lnkname, (long unsigned int)(auth->info.stats.xmitOctets % MAX_U_INT32)));
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_OUTPUT_GIGAWORDS: %lu", 
    	    auth->info.lnkname, (long unsigned int)(auth->info.stats.xmitOctets / MAX_U_INT32)));
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_OUTPUT_PACKETS: %lu", 
    	    auth->info.lnkname, (long unsigned int)(auth->info.stats.xmitFrames)));
	if (rad_put_int(auth->radius.handle, RAD_ACCT_OUTPUT_OCTETS, auth->info.stats.xmitOctets % MAX_U_INT32) != 0 ||
	    rad_put_int(auth->radius.handle, RAD_ACCT_OUTPUT_PACKETS, auth->info.stats.xmitFrames) != 0 ||
	    rad_put_int(auth->radius.handle, RAD_ACCT_OUTPUT_GIGAWORDS, auth->info.stats.xmitOctets / MAX_U_INT32) != 0) {
    		Log(LG_RADIUS, ("[%s] RADIUS: Put output stats: %s", auth->info.lnkname,
		    rad_strerror(auth->radius.handle)));
    		return (RAD_NACK);
	}
#ifdef USE_NG_BPF
    }
    SLIST_FOREACH(ssr, &auth->info.ss.stat[0], next) {
	char str[64];
	snprintf(str, sizeof(str), "%s:%llu",
	    ssr->name, (long long unsigned)ssr->Octets);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_INPUT_OCTETS: %s", 
    	    auth->info.lnkname, str));
	if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_INPUT_OCTETS, str) != 0) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_INPUT_OCTETS: %s", auth->info.lnkname,
		rad_strerror(auth->radius.handle)));
	}
	snprintf(str, sizeof(str), "%s:%llu",
	    ssr->name, (long long unsigned)ssr->Packets);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_INPUT_PACKETS: %s", 
    	    auth->info.lnkname, str));
	if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_INPUT_PACKETS, str) != 0) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_INPUT_PACKETS: %s", auth->info.lnkname,
		rad_strerror(auth->radius.handle)));
	}
	if (strcmp(ssr->name,auth->params.std_acct[0]) == 0) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_INPUT_OCTETS: %lu", 
    		auth->info.lnkname, (long unsigned int)(ssr->Octets % MAX_U_INT32)));
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_INPUT_GIGAWORDS: %lu", 
    		auth->info.lnkname, (long unsigned int)(ssr->Octets / MAX_U_INT32)));
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_INPUT_PACKETS: %lu", 
    		auth->info.lnkname, (long unsigned int)(ssr->Packets)));
	    if (rad_put_int(auth->radius.handle, RAD_ACCT_INPUT_OCTETS, ssr->Octets % MAX_U_INT32) != 0 ||
		rad_put_int(auth->radius.handle, RAD_ACCT_INPUT_PACKETS, ssr->Packets) != 0 ||
		rad_put_int(auth->radius.handle, RAD_ACCT_INPUT_GIGAWORDS, ssr->Octets / MAX_U_INT32) != 0) {
    		    Log(LG_RADIUS, ("[%s] RADIUS: Put input stats: %s", auth->info.lnkname,
			rad_strerror(auth->radius.handle)));
    		    return (RAD_NACK);
	    }
	}
    }
    SLIST_FOREACH(ssr, &auth->info.ss.stat[1], next) {
	char str[64];
	snprintf(str, sizeof(str), "%s:%llu",
	    ssr->name, (long long unsigned)ssr->Octets);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_OUTPUT_OCTETS: %s", 
    	    auth->info.lnkname, str));
	if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_OUTPUT_OCTETS, str) != 0) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_OUTPUT_OCTETS: %s", auth->info.lnkname,
		rad_strerror(auth->radius.handle)));
	}
	snprintf(str, sizeof(str), "%s:%llu",
	    ssr->name, (long long unsigned)ssr->Packets);
	Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_MPD_OUTPUT_PACKETS: %s", 
    	    auth->info.lnkname, str));
	if (rad_put_vendor_string(auth->radius.handle, RAD_VENDOR_MPD, RAD_MPD_OUTPUT_PACKETS, str) != 0) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: Put RAD_MPD_OUTPUT_PACKETS: %s", auth->info.lnkname,
		rad_strerror(auth->radius.handle)));
	}
	if (strcmp(ssr->name,auth->params.std_acct[1]) == 0) {
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_OUTPUT_OCTETS: %lu", 
    		auth->info.lnkname, (long unsigned int)(ssr->Octets % MAX_U_INT32)));
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_OUTPUT_GIGAWORDS: %lu", 
    		auth->info.lnkname, (long unsigned int)(ssr->Octets / MAX_U_INT32)));
	    Log(LG_RADIUS2, ("[%s] RADIUS: Put RAD_ACCT_OUTPUT_PACKETS: %lu", 
    		auth->info.lnkname, (long unsigned int)(ssr->Packets)));
	    if (rad_put_int(auth->radius.handle, RAD_ACCT_OUTPUT_OCTETS, ssr->Octets % MAX_U_INT32) != 0 ||
		rad_put_int(auth->radius.handle, RAD_ACCT_OUTPUT_PACKETS, ssr->Packets) != 0 ||
		rad_put_int(auth->radius.handle, RAD_ACCT_OUTPUT_GIGAWORDS, ssr->Octets / MAX_U_INT32) != 0) {
    		    Log(LG_RADIUS, ("[%s] RADIUS: Put output stats: %s", auth->info.lnkname,
			rad_strerror(auth->radius.handle)));
    		    return (RAD_NACK);
	    }
	}
    }
#endif /* USE_NG_BPF */

  }
  return (RAD_ACK);
}

static int 
RadiusSendRequest(AuthData auth)
{
    struct timeval	timelimit;
    struct timeval	tv;
    int 		fd, n;

    Log(LG_RADIUS2, ("[%s] RADIUS: Send request for user '%s'", 
	auth->info.lnkname, auth->params.authname));
    n = rad_init_send_request(auth->radius.handle, &fd, &tv);
    if (n != 0) {
	Log(LG_RADIUS, ("[%s] RADIUS: rad_init_send_request failed: %d %s",
    	    auth->info.lnkname, n, rad_strerror(auth->radius.handle)));
        return (RAD_NACK);
    }

    gettimeofday(&timelimit, NULL);
    timeradd(&tv, &timelimit, &timelimit);

    for ( ; ; ) {
	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	n = poll(fds,1,tv.tv_sec*1000+tv.tv_usec/1000);

	if (n == -1) {
    	    Log(LG_RADIUS, ("[%s] RADIUS: poll failed %s", auth->info.lnkname, 
    		strerror(errno)));
    	    return (RAD_NACK);
	}

	if ((fds[0].revents&POLLIN)!=POLLIN) {
    	    /* Compute a new timeout */
    	    gettimeofday(&tv, NULL);
    	    timersub(&timelimit, &tv, &tv);
    	    if (tv.tv_sec > 0 || (tv.tv_sec == 0 && tv.tv_usec > 0))
		continue;	/* Continue the select */
	}

	Log(LG_RADIUS2, ("[%s] RADIUS: Sending request for user '%s'", 
    	    auth->info.lnkname, auth->params.authname));
	n = rad_continue_send_request(auth->radius.handle, n, &fd, &tv);
	if (n != 0)
    	    break;

	gettimeofday(&timelimit, NULL);
	timeradd(&tv, &timelimit, &timelimit);
    }

    switch (n) {

	case RAD_ACCESS_ACCEPT:
    	    Log(LG_RADIUS, ("[%s] RADIUS: Rec'd RAD_ACCESS_ACCEPT for user '%s'", 
    		auth->info.lnkname, auth->params.authname));
    	    auth->status = AUTH_STATUS_SUCCESS;
    	    break;

	case RAD_ACCESS_CHALLENGE:
    	    Log(LG_RADIUS, ("[%s] RADIUS: Rec'd RAD_ACCESS_CHALLENGE for user '%s'", 
    		auth->info.lnkname, auth->params.authname));
    	    break;

	case RAD_ACCESS_REJECT:
    	    Log(LG_RADIUS, ("[%s] RADIUS: Rec'd RAD_ACCESS_REJECT for user '%s'", 
    		auth->info.lnkname, auth->params.authname));
    	    auth->status = AUTH_STATUS_FAIL;
    	    break;

	case RAD_ACCOUNTING_RESPONSE:
    	    Log(auth->acct_type != AUTH_ACCT_UPDATE ? LG_RADIUS : LG_RADIUS2,
		("[%s] RADIUS: Rec'd RAD_ACCOUNTING_RESPONSE for user '%s'", 
    		auth->info.lnkname, auth->params.authname));
    	    break;

	case -1:
    	    Log(LG_RADIUS, ("[%s] RADIUS: rad_send_request for user '%s' failed: %s",
    		auth->info.lnkname, auth->params.authname,
		rad_strerror(auth->radius.handle)));
    	    return (RAD_NACK);
      
	default:
    	    Log(LG_RADIUS, ("[%s] RADIUS: rad_send_request: unexpected return value: %d", 
    		auth->info.lnkname, n));
    	    return (RAD_NACK);
    }

    return (RadiusGetParams(auth, n == RAD_ACCESS_CHALLENGE));
}

static int
RadiusGetParams(AuthData auth, int eap_proxy)
{
  int		res, i, j;
  size_t	len;
  const void	*data;
  u_int32_t	vendor;
  char		*route;
  char		*tmpval;
  struct in_addr	ip;
#if defined(USE_NG_BPF) || defined(USE_IPFW)
  struct acl		**acls, *acls1;
  char		*acl, *acl1, *acl2, *acl3;
#endif
  struct ifaceroute	*r, *r1;
  struct u_range	range;
#ifdef CCP_MPPC
  u_char	*tmpkey;
  size_t	tmpkey_len;
#endif

  Freee(auth->params.eapmsg);
  auth->params.eapmsg = NULL;
  
  while ((res = rad_get_attr(auth->radius.handle, &data, &len)) > 0) {

    switch (res) {

      case RAD_STATE:
	tmpval = Bin2Hex(data, len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_STATE: 0x%s", auth->info.lnkname, tmpval));
	Freee(tmpval);
	auth->params.state_len = len;
	Freee(auth->params.state);
	auth->params.state = Mdup(MB_AUTH, data, len);
	continue;

      case RAD_CLASS:
	tmpval = Bin2Hex(data, len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_CLASS: 0x%s", auth->info.lnkname, tmpval));
	Freee(tmpval);
	auth->params.class_len = len;
	Freee(auth->params.class);
	auth->params.class = Mdup(MB_AUTH, data, len);
	continue;

	/* libradius already checks the message-authenticator, so simply ignore it */
      case RAD_MESSAGE_AUTHENTIC:
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MESSAGE_AUTHENTIC", auth->info.lnkname));
	continue;

      case RAD_EAP_MESSAGE:
	if (auth->params.eapmsg != NULL) {
	  char *tbuf;
	  Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_EAP_MESSAGE: len %d of %d",
	    auth->info.lnkname, (int)len, (int)(auth->params.eapmsg_len + len)));
	  tbuf = Malloc(MB_AUTH, auth->params.eapmsg_len + len);
	  memcpy(tbuf, auth->params.eapmsg, auth->params.eapmsg_len);
	  memcpy(&tbuf[auth->params.eapmsg_len], data, len);
	  auth->params.eapmsg_len += len;
	  Freee(auth->params.eapmsg);
	  auth->params.eapmsg = tbuf;
	} else {
	  Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_EAP_MESSAGE: len %d",
	    auth->info.lnkname, (int)len));
	  auth->params.eapmsg = Mdup(MB_AUTH, data, len);
	  auth->params.eapmsg_len = len;
	}
	continue;
    }

    if (!eap_proxy)
      switch (res) {

      case RAD_FRAMED_IP_ADDRESS:
        ip = rad_cvt_addr(data);
        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_IP_ADDRESS: %s ",
          auth->info.lnkname, inet_ntoa(ip)));
	  
	if (strcmp(inet_ntoa(ip), "255.255.255.255") == 0) {
	  /* the peer can choose an address */
	  Log(LG_RADIUS2, ("[%s]   the peer can choose an address", auth->info.lnkname));
	  ip.s_addr=0;
	  in_addrtou_range(&ip, 0, &auth->params.range);
	  auth->params.range_valid = 1;
	} else if (strcmp(inet_ntoa(ip), "255.255.255.254") == 0) {
	  /* we should choose the ip */
	  Log(LG_RADIUS2, ("[%s]   we should choose an address", auth->info.lnkname));
	  auth->params.range_valid = 0;
	} else {
	  /* or use IP from Radius-server */
	  in_addrtou_range(&ip, 32, &auth->params.range);
	  auth->params.range_valid = 1;
	}  
        break;

      case RAD_USER_NAME:
	tmpval = rad_cvt_string(data, len);
	/* copy it into the persistent data struct */
	strlcpy(auth->params.authname, tmpval, sizeof(auth->params.authname));
	free(tmpval);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_USER_NAME: %s ",
	  auth->info.lnkname, auth->params.authname));
        break;

      case RAD_FRAMED_IP_NETMASK:
        ip = rad_cvt_addr(data);
	auth->params.netmask = in_addrtowidth(&ip);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_IP_NETMASK: %s (/%d) ",
	  auth->info.lnkname, inet_ntoa(ip), auth->params.netmask));
	break;

      case RAD_FRAMED_ROUTE:
	route = rad_cvt_string(data, len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_ROUTE: %s ",
	  auth->info.lnkname, route));
	if (!ParseRange(route, &range, ALLOW_IPV4)) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Get RAD_FRAMED_ROUTE: Bad route \"%s\"",
	    auth->info.lnkname, route));
	  free(route);
	  break;
	}
	r = Malloc(MB_AUTH, sizeof(struct ifaceroute));
	r->dest = range;
	r->ok = 0;
	j = 0;
	SLIST_FOREACH(r1, &auth->params.routes, next) {
	  if (!u_rangecompare(&r->dest, &r1->dest)) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Duplicate route %s",
		auth->info.lnkname, route));
	    j = 1;
	  }
	};
	free(route);
	if (j == 0) {
	    SLIST_INSERT_HEAD(&auth->params.routes, r, next);
	} else {
	    Freee(r);
	}
	break;

      case RAD_FRAMED_IPV6_ROUTE:
	route = rad_cvt_string(data, len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_IPV6_ROUTE: %s ",
	  auth->info.lnkname, route));
	if (!ParseRange(route, &range, ALLOW_IPV6)) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Get RAD_FRAMED_IPV6_ROUTE: Bad route \"%s\"", auth->info.lnkname, route));
	  free(route);
	  break;
	}
	r = Malloc(MB_AUTH, sizeof(struct ifaceroute));
	r->dest = range;
	r->ok = 0;
	j = 0;
	SLIST_FOREACH(r1, &auth->params.routes, next) {
	  if (!u_rangecompare(&r->dest, &r1->dest)) {
	    Log(LG_RADIUS, ("[%s] RADIUS: Duplicate route %s",
		auth->info.lnkname, route));
	    j = 1;
	  }
	};
	free(route);
	if (j == 0) {
	    SLIST_INSERT_HEAD(&auth->params.routes, r, next);
	} else {
	    Freee(r);
	}
	break;

      case RAD_SESSION_TIMEOUT:
        auth->params.session_timeout = rad_cvt_int(data);
        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_SESSION_TIMEOUT: %u ",
          auth->info.lnkname, auth->params.session_timeout));
        break;

      case RAD_IDLE_TIMEOUT:
        auth->params.idle_timeout = rad_cvt_int(data);
        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_IDLE_TIMEOUT: %u ",
          auth->info.lnkname, auth->params.idle_timeout));
        break;

     case RAD_ACCT_INTERIM_INTERVAL:
	auth->params.acct_update = rad_cvt_int(data);
        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_ACCT_INTERIM_INTERVAL: %u ",
          auth->info.lnkname, auth->params.acct_update));
	break;

      case RAD_FRAMED_MTU:
	i = rad_cvt_int(data);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_MTU: %u ",
	  auth->info.lnkname, i));
	if (i < IFACE_MIN_MTU || i > IFACE_MAX_MTU) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Get RAD_FRAMED_MTU: invalid MTU: %u ",
	    auth->info.lnkname, i));
	  auth->params.mtu = 0;
	  break;
	}
	auth->params.mtu = i;
        break;

      case RAD_FRAMED_COMPRESSION:
        i = rad_cvt_int(data);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_COMPRESSION: %d",
	  auth->info.lnkname, i));
	if (i == RAD_COMP_VJ)
	    auth->params.vjc_enable = 1;
        break;

      case RAD_FRAMED_PROTOCOL:
	Log(LG_RADIUS2, ("[%s] RADIUS: Get (RAD_FRAMED_PROTOCOL: %d)",
	  auth->info.lnkname, rad_cvt_int(data)));
        break;

      case RAD_FRAMED_ROUTING:
	Log(LG_RADIUS2, ("[%s] RADIUS: Get (RAD_FRAMED_ROUTING: %d)",
	  auth->info.lnkname, rad_cvt_int(data)));
        break;

      case RAD_FILTER_ID:
	tmpval = rad_cvt_string(data, len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get (RAD_FILTER_ID: %s)",
	  auth->info.lnkname, tmpval));
	free(tmpval);
        break;

      case RAD_SERVICE_TYPE:
	Log(LG_RADIUS2, ("[%s] RADIUS: Get (RAD_SERVICE_TYPE: %d)",
	  auth->info.lnkname, rad_cvt_int(data)));
        break;

      case RAD_REPLY_MESSAGE:
	tmpval = rad_cvt_string(data, len);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_REPLY_MESSAGE: %s ",
	  auth->info.lnkname, tmpval));
	auth->reply_message = Mdup(MB_AUTH, tmpval, len + 1);
	free(tmpval);
        break;

      case RAD_FRAMED_POOL:
	tmpval = rad_cvt_string(data, len);
	/* copy it into the persistent data struct */
	strlcpy(auth->params.ippool, tmpval, sizeof(auth->params.ippool));
	free(tmpval);
	Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_FRAMED_POOL: %s ",
	  auth->info.lnkname, auth->params.ippool));
        break;

      case RAD_VENDOR_SPECIFIC:
	if ((res = rad_get_vendor_attr(&vendor, &data, &len)) == -1) {
	  Log(LG_RADIUS, ("[%s] RADIUS: Get vendor attr failed: %s ",
	    auth->info.lnkname, rad_strerror(auth->radius.handle)));
	  return RAD_NACK;
	}

	switch (vendor) {

	  case RAD_VENDOR_MICROSOFT:
	    switch (res) {

	      case RAD_MICROSOFT_MS_CHAP_ERROR:
	    	Freee(auth->mschap_error);
		auth->mschap_error = NULL;
		if (len == 0)
		    break;

		/* there is a nullbyte on the first pos, don't know why */
		if (((const char *)data)[0] == '\0') {
		  data = (const char *)data + 1;
		  len--;
		}
		tmpval = rad_cvt_string(data, len);
		auth->mschap_error = Mdup(MB_AUTH, tmpval, len + 1);
		free(tmpval);

		Log(LG_RADIUS2, ("[%s] RADIUS: Get MS-CHAP-Error: %s",
		  auth->info.lnkname, auth->mschap_error));
		break;

	      /* this was taken from userland ppp */
	      case RAD_MICROSOFT_MS_CHAP2_SUCCESS:
	    	Freee(auth->mschapv2resp);
		auth->mschapv2resp = NULL;
		if (len == 0)
		    break;
		if (len < 3 || ((const char *)data)[1] != '=') {
		  /*
		   * Only point at the String field if we don't think the
		   * peer has misformatted the response.
		   */
		  data = (const char *)data + 1;
		  len--;
		} else {
		  Log(LG_RADIUS, ("[%s] RADIUS: Warning: The MS-CHAP2-Success attribute is mis-formatted. Compensating",
		    auth->info.lnkname));
		}
		if ((tmpval = rad_cvt_string((const char *)data, len)) == NULL) {
		    Log(LG_RADIUS, ("[%s] RADIUS: rad_cvt_string failed: %s",
			auth->info.lnkname, rad_strerror(auth->radius.handle)));
		    return RAD_NACK;
		}
		auth->mschapv2resp = Mdup(MB_AUTH, tmpval, len + 1);
		free(tmpval);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_CHAP2_SUCCESS: %s",
		  auth->info.lnkname, auth->mschapv2resp));
		break;

	      case RAD_MICROSOFT_MS_CHAP_DOMAIN:
		Freee(auth->params.msdomain);
		tmpval = rad_cvt_string(data, len);
		auth->params.msdomain = Mdup(MB_AUTH, tmpval, len + 1);
		free(tmpval);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_CHAP_DOMAIN: %s",
		  auth->info.lnkname, auth->params.msdomain));
		break;

#ifdef CCP_MPPC
              /* MPPE Keys MS-CHAPv2 and EAP-TLS */
	      case RAD_MICROSOFT_MS_MPPE_RECV_KEY:
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_MPPE_RECV_KEY",
		  auth->info.lnkname));
		tmpkey = rad_demangle_mppe_key(auth->radius.handle, data, len, &tmpkey_len);
		if (!tmpkey) {
		  Log(LG_RADIUS, ("[%s] RADIUS: rad_demangle_mppe_key failed: %s",
		    auth->info.lnkname, rad_strerror(auth->radius.handle)));
		  return RAD_NACK;
		}

		memcpy(auth->params.msoft.recv_key, tmpkey, MPPE_KEY_LEN);
		free(tmpkey);
		auth->params.msoft.has_keys = TRUE;
		break;

	      case RAD_MICROSOFT_MS_MPPE_SEND_KEY:
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_MPPE_SEND_KEY",
		  auth->info.lnkname));
		tmpkey = rad_demangle_mppe_key(auth->radius.handle, data, len, &tmpkey_len);
		if (!tmpkey) {
		  Log(LG_RADIUS, ("[%s] RADIUS: rad_demangle_mppe_key failed: %s",
		    auth->info.lnkname, rad_strerror(auth->radius.handle)));
		  return RAD_NACK;
		}
		memcpy(auth->params.msoft.xmit_key, tmpkey, MPPE_KEY_LEN);
		free(tmpkey);
		auth->params.msoft.has_keys = TRUE;
		break;

              /* MPPE Keys MS-CHAPv1 */
	      case RAD_MICROSOFT_MS_CHAP_MPPE_KEYS:
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_CHAP_MPPE_KEYS",
		  auth->info.lnkname));

		if (len != 32) {
		  Log(LG_RADIUS, ("[%s] RADIUS: Server returned garbage %d of expected %d Bytes",
		    auth->info.lnkname, (int)len, 32));
		  return RAD_NACK;
		}

		tmpkey = rad_demangle(auth->radius.handle, data, len);
		if (tmpkey == NULL) {
		  Log(LG_RADIUS, ("[%s] RADIUS: rad_demangle failed: %s",
		    auth->info.lnkname, rad_strerror(auth->radius.handle)));
		  return RAD_NACK;
		}
		memcpy(auth->params.msoft.lm_hash, tmpkey, sizeof(auth->params.msoft.lm_hash));
		auth->params.msoft.has_lm_hash = TRUE;
		memcpy(auth->params.msoft.nt_hash_hash, &tmpkey[8], sizeof(auth->params.msoft.nt_hash_hash));
		auth->params.msoft.has_nt_hash = TRUE;
		free(tmpkey);
		break;
#endif

	      case RAD_MICROSOFT_MS_MPPE_ENCRYPTION_POLICY:
		auth->params.msoft.policy = rad_cvt_int(data);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_MPPE_ENCRYPTION_POLICY: %d (%s)",
		  auth->info.lnkname, auth->params.msoft.policy, AuthMPPEPolicyname(auth->params.msoft.policy)));
		break;

	      case RAD_MICROSOFT_MS_MPPE_ENCRYPTION_TYPES:
	        {
		    char	buf[64];
		    auth->params.msoft.types = rad_cvt_int(data);
		    Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_MPPE_ENCRYPTION_TYPES: %d (%s)",
			auth->info.lnkname, auth->params.msoft.types, 
			AuthMPPETypesname(auth->params.msoft.types, buf, sizeof(buf))));
		}
		break;

    	     case RAD_MICROSOFT_MS_PRIMARY_DNS_SERVER:
    		auth->params.peer_dns[0] = rad_cvt_addr(data);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_PRIMARY_DNS_SERVER: %s",
		    auth->info.lnkname, inet_ntoa(auth->params.peer_dns[0])));
		break;

    	     case RAD_MICROSOFT_MS_SECONDARY_DNS_SERVER:
    		auth->params.peer_dns[1] = rad_cvt_addr(data);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_SECONDARY_DNS_SERVER: %s",
		    auth->info.lnkname, inet_ntoa(auth->params.peer_dns[1])));
		break;

    	     case RAD_MICROSOFT_MS_PRIMARY_NBNS_SERVER:
    		auth->params.peer_nbns[0] = rad_cvt_addr(data);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_PRIMARY_NBNS_SERVER: %s",
		    auth->info.lnkname, inet_ntoa(auth->params.peer_nbns[0])));
		break;

    	     case RAD_MICROSOFT_MS_SECONDARY_NBNS_SERVER:
    		auth->params.peer_nbns[1] = rad_cvt_addr(data);
		Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MICROSOFT_MS_SECONDARY_NBNS_SERVER: %s",
		    auth->info.lnkname, inet_ntoa(auth->params.peer_nbns[1])));
		break;

	      default:
		Log(LG_RADIUS2, ("[%s] RADIUS: Dropping MICROSOFT vendor specific attribute: %d ",
		  auth->info.lnkname, res));
		break;
	    }
	    break;

	  case RAD_VENDOR_MPD:

	    if (res == RAD_MPD_DROP_USER) {
	        auth->drop_user = rad_cvt_int(data);
	        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_DROP_USER: %d",
		    auth->info.lnkname, auth->drop_user));
	        break;
	    } else if (res == RAD_MPD_ACTION) {
		tmpval = rad_cvt_string(data, len);
	        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_ACTION: %s",
	    	    auth->info.lnkname, tmpval));
		strlcpy(auth->params.action, tmpval,
		    sizeof(auth->params.action));
		free(tmpval);
		break;
	    } else
#ifdef USE_IPFW
	    if (res == RAD_MPD_RULE) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_RULE: %s",
		auth->info.lnkname, acl));
	      acls = &(auth->params.acl_rule);
	    } else if (res == RAD_MPD_PIPE) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_PIPE: %s",
	        auth->info.lnkname, acl));
	      acls = &(auth->params.acl_pipe);
	    } else if (res == RAD_MPD_QUEUE) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_QUEUE: %s",
	        auth->info.lnkname, acl));
	      acls = &(auth->params.acl_queue);
	    } else if (res == RAD_MPD_TABLE) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_TABLE: %s",
	        auth->info.lnkname, acl));
	      acls = &(auth->params.acl_table);
	    } else if (res == RAD_MPD_TABLE_STATIC) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_TABLE_STATIC: %s",
	        auth->info.lnkname, acl));
	      acls = &(auth->params.acl_table);
	    } else
#endif /* USE_IPFW */
#ifdef USE_NG_BPF
	    if (res == RAD_MPD_FILTER) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_FILTER: %s",
	        auth->info.lnkname, acl));
	      acl2 = strsep(&acl1, "#");
	      i = atol(acl2);
	      if (i <= 0 || i > ACL_FILTERS) {
	        Log(LG_RADIUS, ("[%s] RADIUS: Wrong filter number: %i",
		  auth->info.lnkname, i));
	        free(acl);
	        break;
	      }
	      acls = &(auth->params.acl_filters[i - 1]);
	    } else if (res == RAD_MPD_LIMIT) {
	      acl1 = acl = rad_cvt_string(data, len);
	      Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_LIMIT: %s",
	        auth->info.lnkname, acl));
	      acl2 = strsep(&acl1, "#");
	      if (strcasecmp(acl2, "in") == 0) {
	        i = 0;
	      } else if (strcasecmp(acl2, "out") == 0) {
	        i = 1;
	      } else {
	        Log(LG_ERR, ("[%s] RADIUS: Wrong limit direction: '%s'",
		  auth->info.lnkname, acl2));
	        free(acl);
	        break;
	      }
	      acls = &(auth->params.acl_limits[i]);
	    } else if (res == RAD_MPD_INPUT_ACCT) {
		tmpval = rad_cvt_string(data, len);
	        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_INPUT_ACCT: %s",
	    	    auth->info.lnkname, tmpval));
		strlcpy(auth->params.std_acct[0], tmpval,
		    sizeof(auth->params.std_acct[0]));
		free(tmpval);
		break;
	    } else if (res == RAD_MPD_OUTPUT_ACCT) {
		tmpval = rad_cvt_string(data, len);
	        Log(LG_RADIUS2, ("[%s] RADIUS: Get RAD_MPD_OUTPUT_ACCT: %s",
	    	    auth->info.lnkname, tmpval));
		strlcpy(auth->params.std_acct[1], tmpval,
		    sizeof(auth->params.std_acct[1]));
		free(tmpval);
		break;
	    } else
#endif /* USE_NG_BPF */
	    {
	      Log(LG_RADIUS2, ("[%s] RADIUS: Dropping MPD vendor specific attribute: %d",
		auth->info.lnkname, res));
	      break;
	    }
#if defined(USE_NG_BPF) || defined(USE_IPFW)
	    if (acl1 == NULL) {
	      Log(LG_ERR, ("[%s] RADIUS: Incorrect acl!",
		auth->info.lnkname));
	      free(acl);
	      break;
	    }
	    
	    acl3 = acl1;
	    strsep(&acl3, "=");
	    acl2 = acl1;
	    strsep(&acl2, "#");
	    i = atol(acl1);
	    if (i <= 0) {
	      Log(LG_ERR, ("[%s] RADIUS: Wrong acl number: %i",
		auth->info.lnkname, i));
	      free(acl);
	      break;
	    }
	    if ((acl3 == NULL) || (acl3[0] == 0)) {
	      Log(LG_ERR, ("[%s] RADIUS: Wrong acl", auth->info.lnkname));
	      free(acl);
	      break;
	    }
	    acls1 = Malloc(MB_AUTH, sizeof(struct acl) + strlen(acl3));
	    if (res != RAD_MPD_TABLE_STATIC) {
		    acls1->number = i;
		    acls1->real_number = 0;
	    } else {
		    acls1->number = 0;
		    acls1->real_number = i;
	    }
	    if (acl2)
		strlcpy(acls1->name, acl2, sizeof(acls1->name));
	    strcpy(acls1->rule, acl3);
	    while ((*acls != NULL) && ((*acls)->number < acls1->number))
	      acls = &((*acls)->next);

	    if (*acls == NULL) {
	      acls1->next = NULL;
	    } else if (((*acls)->number == acls1->number) &&
		(res != RAD_MPD_TABLE) &&
		(res != RAD_MPD_TABLE_STATIC)) {
	      Log(LG_ERR, ("[%s] RADIUS: Duplicate acl",
		auth->info.lnkname));
	      Freee(acls1);
	      free(acl);
	      break;
	    } else {
	      acls1->next = *acls;
	    }
	    *acls = acls1;

	    free(acl);
	    break;
#endif /* USE_NG_BPF or USE_IPFW */

	  default:
	    Log(LG_RADIUS2, ("[%s] RADIUS: Dropping vendor %d attribute: %d ", 
	      auth->info.lnkname, vendor, res));
	    break;
	}
	break;

      default:
	Log(LG_RADIUS2, ("[%s] RADIUS: Dropping attribute: %d ", 
	  auth->info.lnkname, res));
	break;
    }
  }

    if (auth->acct_type == 0) {

	/* sanity check, this happens when FreeRADIUS has no msoft-dictionary loaded */
	if (auth->proto == PROTO_CHAP && auth->alg == CHAP_ALG_MSOFTv2
		&& auth->mschapv2resp == NULL && auth->status == AUTH_STATUS_SUCCESS) {
	    Log(LG_RADIUS, ("[%s] RADIUS: PANIC no MS-CHAP2-Success received from server!",
    		auth->info.lnkname));
	    return RAD_NACK;
	}
  
	/* MPPE allowed or required, but no MPPE keys returned */
	/* print warning, because MPPE doesen't work */
	if (!(auth->params.msoft.has_keys || auth->params.msoft.has_nt_hash || auth->params.msoft.has_lm_hash) &&
		auth->params.msoft.policy != MPPE_POLICY_NONE) {
	    Log(LG_RADIUS, ("[%s] RADIUS: WARNING no MPPE-Keys received, MPPE will not work",
    		auth->info.lnkname));
	}

	/* If no MPPE-Infos are returned by the RADIUS server, then allow all */
	/* MSoft IAS sends no Infos if all MPPE-Types are enabled and if encryption is optional */
	if (auth->params.msoft.policy == MPPE_POLICY_NONE &&
    	    auth->params.msoft.types == MPPE_TYPE_0BIT &&
    	    (auth->params.msoft.has_keys || auth->params.msoft.has_nt_hash || auth->params.msoft.has_lm_hash)) {
		auth->params.msoft.policy = MPPE_POLICY_ALLOWED;
		auth->params.msoft.types = MPPE_TYPE_40BIT | MPPE_TYPE_128BIT | MPPE_TYPE_56BIT;
		Log(LG_RADIUS, ("[%s] RADIUS: MPPE-Keys present, but no MPPE-Infos received => allowing MPPE with all types",
    		    auth->info.lnkname));
	}
  
	/* If Framed-IP-Address is present and Framed-Netmask != 32 add route */
	if (auth->params.range_valid && auth->params.range.width == 32 &&
		auth->params.netmask != 0 && auth->params.netmask != 32) {
	    struct in_addr tmpmask;
	    widthtoin_addr(auth->params.netmask, &tmpmask);
	    r = Malloc(MB_AUTH, sizeof(struct ifaceroute));
	    r->dest.addr = auth->params.range.addr;
	    r->dest.addr.u.ip4.s_addr &= tmpmask.s_addr;
	    r->dest.width = auth->params.netmask;
	    r->ok = 0;
	    j = 0;
	    SLIST_FOREACH(r1, &auth->params.routes, next) {
		if (!u_rangecompare(&r->dest, &r1->dest)) {
		    Log(LG_RADIUS, ("[%s] RADIUS: Duplicate route", auth->info.lnkname));
		    j = 1;
		}
	    };
	    if (j == 0) {
		SLIST_INSERT_HEAD(&auth->params.routes, r, next);
	    } else {
		Freee(r);
	    }
	}
    }
  
    return (RAD_ACK);
}
