
/*
 * web.c
 *
 * Written by Alexander Motin <mav@FreeBSD.org>
 */

#include "ppp.h"
#include "web.h"
#include "util.h"


/*
 * DEFINITIONS
 */

  /* Set menu options */
  enum {
    SET_OPEN,
    SET_CLOSE,
    SET_SELF,
    SET_DISABLE,
    SET_ENABLE
  };


/*
 * INTERNAL FUNCTIONS
 */

  static int	WebSetCommand(Context ctx, int ac, char *av[], void *arg);

  static int	WebServletRun(struct http_servlet *servlet,
                         struct http_request *req, struct http_response *resp);
  static void	WebServletDestroy(struct http_servlet *servlet);

  static void	WebLogger(int sev, const char *fmt, ...);

  static void	WebConsoleSessionWrite(ConsoleSession cs, const char *fmt, ...);
  static void	WebConsoleSessionWriteV(ConsoleSession cs, const char *fmt, va_list vl);
  static void	WebConsoleSessionShowPrompt(ConsoleSession cs);

  static void	WebRunBinCmd(FILE *f, const char *query, int priv);
  static void	WebRunCmd(FILE *f, const char *query, int priv);
  static void	WebShowSummary(FILE *f, int priv);

/*
 * GLOBAL VARIABLES
 */

  const struct cmdtab WebSetCmds[] = {
    { "open",			"Open the web" ,
  	WebSetCommand, NULL, 2, (void *) SET_OPEN },
    { "close",			"Close the web" ,
  	WebSetCommand, NULL, 2, (void *) SET_CLOSE },
    { "self {ip} [{port}]",	"Set web ip and port" ,
  	WebSetCommand, NULL, 2, (void *) SET_SELF },
    { "enable [opt ...]",	"Enable web option" ,
  	WebSetCommand, NULL, 2, (void *) SET_ENABLE },
    { "disable [opt ...]",	"Disable web option" ,
  	WebSetCommand, NULL, 2, (void *) SET_DISABLE },
    { NULL },
  };


/*
 * INTERNAL VARIABLES
 */

  static const struct confinfo	gConfList[] = {
    { 0,	WEB_AUTH,	"auth"	},
    { 0,	0,		NULL	},
  };

  static struct pevent_ctx *gWebCtx = NULL;
    
/*
 * WebInit()
 */

int
WebInit(Web w)
{
  /* setup web-defaults */
  memset(w, 0, sizeof(*w));

  Enable(&w->options, WEB_AUTH);  
  
  ParseAddr(DEFAULT_WEB_IP, &w->addr, ALLOW_IPV4|ALLOW_IPV6);
  w->port = DEFAULT_WEB_PORT;

  return 0;
}

/*
 * WebOpen()
 */

int
WebOpen(Web w)
{
  char		addrstr[INET6_ADDRSTRLEN];

  if (w->srv) {
    Log(LG_ERR, ("web: web already running"));
    return -1;
  }

  gWebCtx = pevent_ctx_create(MB_WEB, NULL);
  if (!gWebCtx) {
    Log(LG_ERR, ("%s: error pevent_ctx_create: %d", __FUNCTION__, errno));
    return(-1);
  }
  
  if (!(w->srv = http_server_start(gWebCtx, w->addr.u.ip4,
           w->port, NULL, "mpd web server", WebLogger))) {
    Log(LG_ERR, ("%s: error http_server_start: %d", __FUNCTION__, errno));
    return(-1);
  }

  w->srvlet.arg=NULL;
  w->srvlet.hook=NULL;
  w->srvlet.run=WebServletRun;
  w->srvlet.destroy=WebServletDestroy;
	   
  if (http_server_register_servlet(w->srv, &w->srvlet, NULL, ".*", 10) < 0) {
    Log(LG_ERR, ("%s: error http_server_register_servlet: %d", __FUNCTION__, errno));
    return(-1);
  }
  
  Log(LG_ERR, ("web: listening on %s %d", 
	u_addrtoa(&w->addr,addrstr,sizeof(addrstr)), w->port));
  return 0;
}

/*
 * WebClose()
 */

int
WebClose(Web w)
{
  if (!w->srv) {
    Log(LG_ERR, ("web: web is not running"));
    return -1;
  }

  http_server_stop(&w->srv);
  if (gWebCtx) pevent_ctx_destroy(&gWebCtx);
  
  return 0;
}

/*
 * WebStat()
 */

int
WebStat(Context ctx, int ac, char *av[], void *arg)
{
  Web		w = &gWeb;
  char		addrstr[64];

  Printf("Web configuration:\r\n");
  Printf("\tState         : %s\r\n", w->srv ? "OPENED" : "CLOSED");
  Printf("\tIP-Address    : %s\r\n", u_addrtoa(&w->addr,addrstr,sizeof(addrstr)));
  Printf("\tPort          : %d\r\n", w->port);

  Printf("Web options:\r\n");
  OptStat(ctx, &w->options, gConfList);

  return 0;
}

/*
 * ConsoleSessionWriteV()
 */

static void 
WebConsoleSessionWriteV(ConsoleSession cs, const char *fmt, va_list vl)
{
  vfprintf((FILE *)(cs->cookie), fmt, vl);
}

/*
 * WebConsoleSessionWrite()
 */

static void 
WebConsoleSessionWrite(ConsoleSession cs, const char *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  WebConsoleSessionWriteV(cs, fmt, vl);
  va_end(vl);
}

/*
 * WebConsoleShowPrompt()
 */

static void
WebConsoleSessionShowPrompt(ConsoleSession cs)
{
    if (cs->context.lnk)
	cs->write(cs, "[%s] ", cs->context.lnk->name);
    else if (cs->context.bund)
	cs->write(cs, "[%s] ", cs->context.bund->name);
    else if (cs->context.rep)
	cs->write(cs, "[%s] ", cs->context.rep->name);
    else
	cs->write(cs, "[] ");
}

static void
WebShowCSS(FILE *f)
{
  fprintf(f, "body {font : Arial, Helvetica, sans-serif; background-color: #EEEEEE; }\n");
  fprintf(f, "table {background-color: #FFFFFF; }\n");
  fprintf(f, "th {background-color: #00B000; }\n");
  fprintf(f, "td {background-color: #EEEEEE; }\n");
  fprintf(f, "td.r {background-color: #EECCCC; }\n");
  fprintf(f, "td.y {background-color: #EEEEBB; }\n");
  fprintf(f, "td.g {background-color: #BBEEBB; }\n");
  fprintf(f, "td.d {background-color: #CCCCCC; }\n");
  fprintf(f, "pre {background-color: #FFFFFF; }\n");
  fprintf(f, "a, a:visited, a:link { color: blue; }\n");
}

static void
WebShowSummary(FILE *f, int priv)
{
  int		b,l;
  Bund		B;
  Link  	L;
  Rep		R;
  char		buf[64],buf2[64];

  fprintf(f, "<H2>Current status summary</H2>\n");
  fprintf(f, "<table>\n");
  fprintf(f, "<TR><TH>Bund</TH><TH colspan=2>Iface</TH><TH>IPCP</TH><TH>IPV6CP</TH><TH>CCP</TH><TH>ECP</TH>"
	     "<TH>Link</TH><TH>LCP</TH><TH>User</TH><TH colspan=2>Device</TH><TH>Peer</TH><TH colspan=3></TH>%s</TR>",
	     priv?"<TH></TH>":"");
#define FSM_COLOR(s) (((s)==ST_OPENED)?"g":(((s)==ST_INITIAL)?"r":"y"))
#define PHYS_COLOR(s) (((s)==PHYS_STATE_UP)?"g":(((s)==PHYS_STATE_DOWN)?"r":"y"))
    for (b = 0; b<gNumLinks; b++) {
	if ((L=gLinks[b]) != NULL && L->bund == NULL && L->rep == NULL) {
	    fprintf(f, "<TR>\n");
	    fprintf(f, "<TD colspan=\"7\">&nbsp;</a></TD>\n");
	    fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20link\">%s</a></TD>\n", 
	        L->tmpl?"d":FSM_COLOR(L->lcp.fsm.state), L->name, L->name);
	    fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20lcp\">%s</a></TD>\n", 
	        L->tmpl?"d":FSM_COLOR(L->lcp.fsm.state), L->name, FsmStateName(L->lcp.fsm.state));
	    fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20auth\">%s</a></TD>\n", 
	        L->tmpl?"d":FSM_COLOR(L->lcp.fsm.state), L->name, L->lcp.auth.params.authname);
	    fprintf(f, "<TD class=\"L=%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
	        L->tmpl?"d":PHYS_COLOR(L->state), L->name, L->type?L->type->name:"");
	    fprintf(f, "<TD class=\"L=%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
	        L->tmpl?"d":PHYS_COLOR(L->state), L->name, gPhysStateNames[L->state]);
	    if (L->state != PHYS_STATE_DOWN) {
		PhysGetPeerAddr(L, buf, sizeof(buf));
		fprintf(f, "<TD>%s</TD>\n", buf);
		PhysGetCallingNum(L, buf, sizeof(buf));
		PhysGetCalledNum(L, buf2, sizeof(buf2));
		if (PhysGetOriginate(L) == LINK_ORIGINATE_REMOTE) {
		    fprintf(f, "<TD>%s</TD><TD><=</TD><TD>%s</TD>\n", 
			buf2, buf);
		} else {
		    fprintf(f, "<TD>%s</TD><TD>=></TD><TD>%s</TD>\n", 
			buf, buf2);
		}
	    } else {
	    	fprintf(f, "<TD></TD>\n");
	    	fprintf(f, "<TD colspan=3></TD>\n");
	    }
	    if (priv) {
		if (!L->tmpl) {
		    fprintf(f, "<TD><A href=\"/cmd?link%%20%s&amp;open\">[Open]</a><A href=\"/cmd?link%%20%s&amp;close\">[Close]</a></TD>\n", 
	    		L->name, L->name);
		} else {
		    fprintf(f, "<TD></TD>\n");
		}
	    }
	    fprintf(f, "</TR>\n");
	}
    }
  for (b = 0; b<gNumBundles; b++) {
    if ((B=gBundles[b]) != NULL) {
	int rows = B->n_links?B->n_links:1;
	int first = 1;
	fprintf(f, "<TR>\n");
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20bund\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":(B->iface.up?"g":"r"), B->name, B->name);
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20iface\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":(B->iface.up?"g":"r"), B->name, B->iface.ifname);
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20iface\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":(B->iface.up?"g":"r"), B->name, (B->iface.up?"Up":"Down"));
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20ipcp\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":FSM_COLOR(B->ipcp.fsm.state), B->name,FsmStateName(B->ipcp.fsm.state));
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20ipv6cp\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":FSM_COLOR(B->ipv6cp.fsm.state), B->name,FsmStateName(B->ipv6cp.fsm.state));
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20ccp\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":FSM_COLOR(B->ccp.fsm.state), B->name,FsmStateName(B->ccp.fsm.state));
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?bund%%20%s&amp;show%%20ecp\">%s</a></TD>\n", 
	    rows, B->tmpl?"d":FSM_COLOR(B->ecp.fsm.state), B->name,FsmStateName(B->ecp.fsm.state));
	if (B->n_links == 0) {
	    fprintf(f, "<TD colspan=\"10\">&nbsp;</a></TD>\n</TR>\n");
	}
	for (l = 0; l < NG_PPP_MAX_LINKS; l++) {
	    if ((L=B->links[l]) != NULL) {
		if (first)
		    first = 0;
		else
		    fprintf(f, "<TR>\n");
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20link\">%s</a></TD>\n", 
		    L->tmpl?"d":FSM_COLOR(L->lcp.fsm.state), L->name, L->name);
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20lcp\">%s</a></TD>\n", 
		    L->tmpl?"d":FSM_COLOR(L->lcp.fsm.state), L->name, FsmStateName(L->lcp.fsm.state));
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20auth\">%s</a></TD>\n", 
		    L->tmpl?"d":FSM_COLOR(L->lcp.fsm.state), L->name, L->lcp.auth.params.authname);
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
		    L->tmpl?"d":PHYS_COLOR(L->state), L->name, L->type?L->type->name:"");
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
		    L->tmpl?"d":PHYS_COLOR(L->state), L->name, gPhysStateNames[L->state]);
		if (L->state != PHYS_STATE_DOWN) {
		    PhysGetPeerAddr(L, buf, sizeof(buf));
		    fprintf(f, "<TD>%s</TD>\n", buf);
		    PhysGetCallingNum(L, buf, sizeof(buf));
		    PhysGetCalledNum(L, buf2, sizeof(buf2));
		    if (PhysGetOriginate(L) == LINK_ORIGINATE_REMOTE) {
			    fprintf(f, "<TD>%s</TD><TD><=</TD><TD>%s</TD>\n", 
				buf2, buf);
		    } else {
			    fprintf(f, "<TD>%s</TD><TD>=></TD><TD>%s</TD>\n", 
				buf, buf2);
		    }
		} else {
			fprintf(f, "<TD></TD>\n");
			fprintf(f, "<TD colspan=3></TD>\n");
		}
		if (priv) {
		    fprintf(f, "<TD><A href=\"/cmd?link%%20%s&amp;open\">[Open]</a><A href=\"/cmd?link%%20%s&amp;close\">[Close]</a></TD>\n", 
		    	L->name, L->name);
		}
		fprintf(f, "</TR>\n");
	    }
	}
    }
  }
  for (b = 0; b<gNumReps; b++) {
    if ((R=gReps[b]) != NULL) {
	int shown = 0;
#define FSM_COLOR(s) (((s)==ST_OPENED)?"g":(((s)==ST_INITIAL)?"r":"y"))
#define PHYS_COLOR(s) (((s)==PHYS_STATE_UP)?"g":(((s)==PHYS_STATE_DOWN)?"r":"y"))
	int rows = (R->links[0]?1:0) + (R->links[1]?1:0);
	if (rows == 0)
	    rows = 1;
	fprintf(f, "<TR>\n");
	fprintf(f, "<TD rowspan=\"%d\" colspan=6>Repeater</TD>\n", rows);
	fprintf(f, "<TD rowspan=\"%d\" class=\"%s\"><A href=\"/cmd?rep%%20%s&amp;show%%20repeater\">%s</a></TD>\n", 
	     rows, R->p_up?"g":"r", R->name, R->name);
	for (l = 0; l < 2; l++) {
	    if ((L=R->links[l]) != NULL) {
		if (shown)
		    fprintf(f, "<TR>\n");
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
		    PHYS_COLOR(L->state), L->name, L->name);
		fprintf(f, "<TD colspan=2></TD>\n");
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
		    PHYS_COLOR(L->state), L->name, L->type?L->type->name:"");
		fprintf(f, "<TD class=\"%s\"><A href=\"/cmd?link%%20%s&amp;show%%20device\">%s</a></TD>\n", 
		    PHYS_COLOR(L->state), L->name, gPhysStateNames[L->state]);
		if (L->state != PHYS_STATE_DOWN) {
		    PhysGetPeerAddr(L, buf, sizeof(buf));
		    fprintf(f, "<TD>%s</TD>\n", buf);
		    PhysGetCallingNum(L, buf, sizeof(buf));
		    PhysGetCalledNum(L, buf2, sizeof(buf2));
		    if (PhysGetOriginate(L) == LINK_ORIGINATE_REMOTE) {
			    fprintf(f, "<TD>%s</TD><TD><=</TD><TD>%s</TD>\n", 
				buf2, buf);
		    } else {
			    fprintf(f, "<TD>%s</TD><TD>=></TD><TD>%s</TD>\n", 
				buf, buf2);
		    }
		} else {
			fprintf(f, "<TD></TD>\n");
			fprintf(f, "<TD colspan=3></TD>\n");
		}
		fprintf(f, "<TD></TD>\n");
		fprintf(f, "</TR>\n");
		
		shown = 1;
	    }
	}
	if (!shown) {
	    fprintf(f, "<TD colspan = \"10\"></TD>\n");
	    fprintf(f, "</TR>\n");
	}
    }
  }
  fprintf(f, "</TABLE>\n");
}

static void 
WebRunBinCmd(FILE *f, const char *query, int priv)
{
    Console		c = &gConsole;
    struct console_session css;
    ConsoleSession	cs = &css;
    char		*buf;
    char		*tmp;
    int			argc, k;
    char		*argv[MAX_CONSOLE_ARGS];
  
    memset(cs, 0, sizeof(*cs));

    cs->cookie = f;
    cs->console = c;
    cs->close = NULL;
    cs->write = WebConsoleSessionWrite;
    cs->writev = WebConsoleSessionWriteV;
    cs->prompt = WebConsoleSessionShowPrompt;
    cs->context.cs = cs;
    cs->context.priv = priv;

    tmp = buf = Mstrdup(MB_WEB, query);
    for (argc = 0; (argv[argc] = strsep(&tmp, "&")) != NULL;)
	if (argv[argc][0] != '\0')
    	    if (++argc >= MAX_CONSOLE_ARGS)
        	break;

    for (k = 0; k < argc; k++) {
	int	ac, rtn;
	char	*av[MAX_CONSOLE_ARGS];
	char	*buf1;

	buf1 = Malloc(MB_WEB, strlen(argv[k]) + 1);
	http_request_url_decode(argv[k], buf1);
        Log2(LG_CONSOLE, ("[%s] WEB: %s", 
	    cs->context.lnk ? cs->context.lnk->name :
		(cs->context.bund? cs->context.bund->name : ""), 
	    buf1));
	ac = ParseLine(buf1, av, sizeof(av) / sizeof(*av), 0);
	cs->context.errmsg[0] = 0;
	rtn = DoCommandTab(&cs->context, gCommands, ac, av);
	Freee(buf1);
	fprintf(f, "RESULT: %d %s\n", rtn, cs->context.errmsg);
    }
    Freee(buf);
    RESETREF(cs->context.lnk, NULL);
    RESETREF(cs->context.bund, NULL);
    RESETREF(cs->context.rep, NULL);
}

static void 
WebRunCmd(FILE *f, const char *query, int priv)
{
    Console		c = &gConsole;
    struct console_session css;
    ConsoleSession	cs = &css;
    char		*buf;
    char		*tmp;
    int			argc, k;
    char		*argv[MAX_CONSOLE_ARGS];
  
    memset(cs, 0, sizeof(*cs));

    cs->cookie = f;
    cs->console = c;
    cs->close = NULL;
    cs->write = WebConsoleSessionWrite;
    cs->writev = WebConsoleSessionWriteV;
    cs->prompt = WebConsoleSessionShowPrompt;
    cs->context.cs = cs;
    cs->context.priv = priv;

    tmp = buf = Mstrdup(MB_WEB, query);
    for (argc = 0; (argv[argc] = strsep(&tmp, "&")) != NULL;)
	if (argv[argc][0] != '\0')
    	    if (++argc >= MAX_CONSOLE_ARGS)
        	break;

    fprintf(f, "<P><A href=\"/\"><< Back</A></P>\n");

    if (argc == 0) {
	fprintf(f, "<P>No command cpecified!</P>\n");
	goto done;
    }

    fprintf(f, "<PRE>\n");
    for (k = 0; k < argc; k++) {
	int	ac;
	char	*av[MAX_CONSOLE_ARGS];
	char	*buf1;

	buf1 = Malloc(MB_WEB, strlen(argv[k]) + 1);
	http_request_url_decode(argv[k], buf1);
        Log2(LG_CONSOLE, ("[%s] WEB: %s", 
	    cs->context.lnk ? cs->context.lnk->name :
		(cs->context.bund? cs->context.bund->name : ""), 
	    buf1));

	cs->prompt(cs);
	cs->write(cs, "%s\n", buf1);
    
	ac = ParseLine(buf1, av, sizeof(av) / sizeof(*av), 0);
	DoCommand(&cs->context, ac, av, NULL, 0);
	Freee(buf1);
    }
    fprintf(f, "</PRE>\n");
done:
    Freee(buf);
    fprintf(f, "<P><A href=\"/\"><< Back</A></P>\n");
    RESETREF(cs->context.lnk, NULL);
    RESETREF(cs->context.bund, NULL);
    RESETREF(cs->context.rep, NULL);
}

static void
WebServletRunCleanup(void *cookie) {
    GIANT_MUTEX_UNLOCK();
}

static int	
WebServletRun(struct http_servlet *servlet,
                         struct http_request *req, struct http_response *resp)
{
    FILE *f;
    const char *path;
    const char *query;
    int priv = 0;
    
    if (Enabled(&gWeb.options, WEB_AUTH)) {
	const char *username;
	const char *password;
	ConsoleUser		u;
	struct console_user	iu;

	/* Get username and password */
	if ((username = http_request_get_username(req)) == NULL)
    	    username = "";
	if ((password = http_request_get_password(req)) == NULL)
    	    password = "";

	strlcpy(iu.username, username, sizeof(iu.username));
	RWLOCK_RDLOCK(gUsersLock);
	u = ghash_get(gUsers, &iu);
	RWLOCK_UNLOCK(gUsersLock);

	if ((u == NULL) || strcmp(u->password, password)) {
		http_response_send_basic_auth(resp, "Access Restricted");
		return (1);
	}
	priv = u->priv;
    }

    if (!(f = http_response_get_output(resp, 1))) {
	return 0;
    }
    if (!(path = http_request_get_path(req)))
	return 0;
    if (!(query = http_request_get_query_string(req)))
	return 0;

    if (!strcmp(path,"/mpd.css")) {
	http_response_set_header(resp, 0, "Content-Type", "text/css");
	WebShowCSS(f);
    } else if (!strcmp(path,"/bincmd")) {
	http_response_set_header(resp, 0, "Content-Type", "text/plain");
	http_response_set_header(resp, 1, "Pragma", "no-cache");
	http_response_set_header(resp, 1, "Cache-Control", "no-cache, must-revalidate");
	
	pthread_cleanup_push(WebServletRunCleanup, NULL);
	GIANT_MUTEX_LOCK();
	WebRunBinCmd(f, query, priv);
	GIANT_MUTEX_UNLOCK();
	pthread_cleanup_pop(0);
    } else if (!strcmp(path,"/") || !strcmp(path,"/cmd")) {
	http_response_set_header(resp, 0, "Content-Type", "text/html");
	http_response_set_header(resp, 1, "Pragma", "no-cache");
	http_response_set_header(resp, 1, "Cache-Control", "no-cache, must-revalidate");
	
	pthread_cleanup_push(WebServletRunCleanup, NULL);
	GIANT_MUTEX_LOCK();
	fprintf(f, "<!DOCTYPE HTML "
	    "PUBLIC \"-//W3C//DTD HTML 4.01//EN\" "
	    "\"http://www.w3.org/TR/html4/strict.dtd\">\n");
	fprintf(f, "<HTML>\n");
	fprintf(f, "<HEAD><TITLE>Multi-link PPP Daemon for FreeBSD (mpd)</TITLE>\n");
	fprintf(f, "<LINK rel='stylesheet' href='/mpd.css' type='text/css'>\n");
	fprintf(f, "</HEAD>\n<BODY>\n");
	fprintf(f, "<H1>Multi-link PPP Daemon for FreeBSD</H1>\n");
    
	if (!strcmp(path,"/"))
	    WebShowSummary(f, priv);
	else if (!strcmp(path,"/cmd"))
	    WebRunCmd(f, query, priv);
	    
	GIANT_MUTEX_UNLOCK();
	pthread_cleanup_pop(0);
	
	fprintf(f, "</BODY>\n</HTML>\n");
    } else {
	http_response_send_error(resp, 404, NULL);
    }
    return 1;
}

static void	
WebServletDestroy(struct http_servlet *servlet)
{
}

static void
WebLogger(int sev, const char *fmt, ...)
{
  va_list       args;

  va_start(args, fmt);
  vLogPrintf(fmt, args);
  va_end(args);
}

/*
 * WebSetCommand()
 */

static int
WebSetCommand(Context ctx, int ac, char *av[], void *arg) 
{
  Web	 		w = &gWeb;
  int			port;

  switch ((intptr_t)arg) {

    case SET_OPEN:
      WebOpen(w);
      break;

    case SET_CLOSE:
      WebClose(w);
      break;

    case SET_ENABLE:
	EnableCommand(ac, av, &w->options, gConfList);
      break;

    case SET_DISABLE:
	DisableCommand(ac, av, &w->options, gConfList);
      break;

    case SET_SELF:
      if (ac < 1 || ac > 2)
	return(-1);

      if (!ParseAddr(av[0],&w->addr, ALLOW_IPV4)) 
	Error("Bogus IP address given %s", av[0]);

      if (ac == 2) {
        port =  strtol(av[1], NULL, 10);
        if (port < 1 || port > 65535)
	    Error("Bogus port given %s", av[1]);
        w->port=port;
      }
      break;

    default:
      return(-1);

  }

  return 0;
}
