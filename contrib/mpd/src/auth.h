
/*
 * auth.h
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#ifndef _AUTH_H_
#define	_AUTH_H_

#include "timer.h"
#include "ppp.h"
#include "pap.h"
#include "chap.h"
#include "eap.h"
#include "radius.h"

#ifdef USE_SYSTEM
#include <pwd.h>
#endif
#ifdef USE_OPIE
#include <opie.h>
#endif

/*
 * DEFINITIONS
 */

  #define AUTH_RETRIES		5

  #define AUTH_MSG_WELCOME	"Welcome"
  #define AUTH_MSG_INVALID	"Login incorrect"
  #define AUTH_MSG_BAD_PACKET	"Incorrectly formatted packet"
  #define AUTH_MSG_NOT_ALLOWED	"Login not allowed for this account"
  #define AUTH_MSG_NOT_EXPECTED	"Unexpected packet"
  #define AUTH_MSG_ACCT_DISAB	"Account disabled"
  #define AUTH_MSG_RESTR_HOURS	"Login hours restricted"

  #define AUTH_PEER_TO_SELF	0
  #define AUTH_SELF_TO_PEER	1

  #define AUTH_FAIL_INVALID_LOGIN	0
  #define AUTH_FAIL_ACCT_DISABLED	1
  #define AUTH_FAIL_NO_PERMISSION	2
  #define AUTH_FAIL_RESTRICTED_HOURS	3
  #define AUTH_FAIL_INVALID_PACKET	4
  #define AUTH_FAIL_NOT_EXPECTED	5
  
  #define AUTH_STATUS_UNDEF		0
  #define AUTH_STATUS_FAIL		1
  #define AUTH_STATUS_SUCCESS		2
  #define AUTH_STATUS_BUSY		3
  
  #define AUTH_PW_HASH_NONE		0
  #define AUTH_PW_HASH_NT		1
  
  #define AUTH_ACCT_START		1
  #define AUTH_ACCT_STOP		2
  #define AUTH_ACCT_UPDATE		3
  
  #define MPPE_POLICY_NONE	0
  #define MPPE_POLICY_ALLOWED	1
  #define MPPE_POLICY_REQUIRED	2

  #define MPPE_TYPE_0BIT	0	/* No encryption required */
  #define MPPE_TYPE_40BIT	2
  #define MPPE_TYPE_128BIT	4
  #define MPPE_TYPE_56BIT	8
  
  /* Configuration options */
  enum {
    AUTH_CONF_RADIUS_AUTH = 1,
    AUTH_CONF_RADIUS_ACCT,
    AUTH_CONF_INTERNAL,
    AUTH_CONF_EXT_AUTH,
    AUTH_CONF_EXT_ACCT,
    AUTH_CONF_SYSTEM_AUTH,
    AUTH_CONF_SYSTEM_ACCT,
    AUTH_CONF_PAM_AUTH,
    AUTH_CONF_PAM_ACCT,
    AUTH_CONF_OPIE,
    AUTH_CONF_ACCT_MANDATORY
  };  

#if defined(USE_NG_BPF) || defined(USE_IPFW)
  struct acl {			/* List of ACLs received from auth */
    u_short number;		/* ACL number given by auth server */
    u_short real_number;	/* ACL number allocated my mpd */
    struct acl *next;
    char name[ACL_NAME_LEN]; 	/* Name of ACL */
    char rule[1]; 		/* Text of ACL (Dynamically sized!) */
  };
#endif

  struct authparams {
    char		authname[AUTH_MAX_AUTHNAME];
    char		password[AUTH_MAX_PASSWORD];

    struct papparams	pap;
    struct chapparams	chap;

    struct u_range	range;		/* IP range allowed to user */
    u_char		range_valid;	/* range is valid */
    u_char		netmask;	/* IP Netmask */
    u_char		vjc_enable;	/* VJC requested by AAA */

    u_char		ippool_used;
    char		ippool[LINK_MAX_NAME];

    struct in_addr	peer_dns[2];	/* DNS servers for peer to use */
    struct in_addr	peer_nbns[2];	/* NBNS servers for peer to use */

    char		*eapmsg;	/* EAP Msg for forwarding to RADIUS server */
    int			eapmsg_len;
    u_char		*state;		/* copy of the state attribute, needed for accounting */
    int			state_len;
    u_char		*class;		/* copy of the class attribute, needed for accounting */
    int			class_len;

    char		action[8 + LINK_MAX_NAME];

#ifdef USE_IPFW
    struct acl		*acl_rule;	/* ipfw rules */
    struct acl		*acl_pipe;	/* ipfw pipes */
    struct acl		*acl_queue;	/* ipfw queues */
    struct acl		*acl_table;	/* ipfw tables */
#endif

#ifdef USE_NG_BPF
    struct acl		*acl_filters[ACL_FILTERS]; /* mpd's internal bpf filters */
    struct acl		*acl_limits[ACL_DIRS];	/* traffic limits based on mpd's filters */

    char 		std_acct[ACL_DIRS][ACL_NAME_LEN]; /* Names of ACL rerurned in standard accounting */
#endif
    
    u_int		session_timeout;	/* Session-Timeout */
    u_int		idle_timeout;		/* Idle-Timeout */
    u_int		acct_update;		/* interval for accouting updates */
    u_int		acct_update_lim_recv;
    u_int		acct_update_lim_xmit;
    char		*msdomain;		/* Microsoft domain */
    SLIST_HEAD(, ifaceroute) routes;
    u_short		mtu;			/* MTU */

    u_char		authentic;	/* wich backend was used */

    char		callingnum[128];/* hr representation of the calling number */
    char		callednum[128];	/* hr representation of the called number */
    char		selfname[64];	/* hr representation of the self name */
    char		peername[64];	/* hr representation of the peer name */
    char		selfaddr[64];	/* hr representation of the self address */
    char		peeraddr[64];	/* hr representation of the peer address */
    char		peerport[6];	/* hr representation of the peer port */
    char		peermacaddr[32];	/* hr representation of the peer MAC address */
    char		peeriface[IFNAMSIZ];	/* hr representation of the peer interface */

    struct {
      int	policy;			/* MPPE_POLICY_* */
      int	types;			/* MPPE_TYPE_*BIT bitmask */
      u_char	lm_hash[16];		/* LM-Hash */
      u_char	nt_hash[16];		/* NT-Hash */
      u_char	nt_hash_hash[16];	/* NT-Hash-Hash */
      u_char	has_lm_hash;
      u_char	has_nt_hash;
      u_char	has_keys;

      u_char	chap_alg;		/* Callers's CHAP algorithm */

      u_char	msChal[CHAP_MSOFTv2_CHAL_LEN]; /* MSOFT challng */
      u_char	ntResp[CHAP_MSOFTv2_RESP_LEN]; /* MSOFT response */

#ifdef CCP_MPPC
      /* Keys when using MS-CHAPv2 or EAP */
      u_char	xmit_key[MPPE_KEY_LEN];	/* xmit start key */
      u_char	recv_key[MPPE_KEY_LEN];	/* recv start key */
#endif
    } msoft;
  };

  struct authconf {
    struct radiusconf	radius;		/* RADIUS configuration */
    char		authname[AUTH_MAX_AUTHNAME];	/* Configured username */
    char		password[AUTH_MAX_PASSWORD];	/* Configured password */
    u_int		acct_update;
    u_int		acct_update_lim_recv;
    u_int		acct_update_lim_xmit;
    int			timeout;	/* Authorization timeout in seconds */
    struct optinfo	options;	/* Configured options */
    char		*extauth_script;/*  External auth script */
    char		*extacct_script;/*  External acct script */
    char		ippool[LINK_MAX_NAME];
  };
  typedef struct authconf	*AuthConf;

  /* State of authorization process during authorization phase,
   * contains params set by the auth-backend */
  struct auth {
    u_short		peer_to_self;	/* What I need from peer */
    u_short		self_to_peer;	/* What peer needs from me */
    u_char		peer_to_self_alg;	/* What alg I need from peer */
    u_char		self_to_peer_alg;	/* What alg peer needs from me */
    struct pppTimer	timer;		/* Max time to spend doing auth */
    struct pppTimer	acct_timer;	/* Timer for accounting updates */
    struct papinfo	pap;		/* PAP state */
    struct chapinfo	chap;		/* CHAP state */
    struct eapinfo	eap;		/* EAP state */
    struct paction	*thread;	/* async auth thread */
    struct paction	*acct_thread;	/* async accounting auth thread */
    struct authconf	conf;		/* Auth backends, RADIUS, etc. */
    struct authparams	params;		/* params to pass to from auth backend */
    struct ng_ppp_link_stat64	prev_stats;	/* Previous link statistics */
  };
  typedef struct auth	*Auth;

  struct radiusconf	radius;			/* RADIUS configuration */
  /* Interface between the auth-backend (secret file, RADIUS, etc.)
   * and Mpd's internal structs.
   */
  struct authdata {
    struct authconf	conf;		/* a copy of bundle's authconf */
    u_short		proto;		/* wich proto are we using, PAP, CHAP, ... */
    u_char		alg;		/* proto specific algoruthm */
    u_int		id;		/* Actual, packet id */    
    u_int		code;		/* Proto specific code */
    u_char		acct_type;	/* Accounting type, Start, Stop, Update */
    u_char		eap_radius;
    u_char		status;
    u_char		why_fail;
    char		*reply_message;	/* Text wich may displayed to the user */
    char		*mschap_error;	/* MSCHAP Error Message */
    char		*mschapv2resp;	/* Response String for MSCHAPv2 */
    void		(*finish)(Link l, struct authdata *auth); /* Finish handler */
    int			drop_user;	/* RAD_MPD_DROP_USER value sent by RADIUS server */
    struct {
      struct rad_handle	*handle;	/* the RADIUS handle */
    } radius;
#ifdef USE_OPIE
    struct {
      struct opie	data;
    } opie;
#endif
    struct {		/* informational (read-only) data needed for e.g. accouting */
      char		msession_id[AUTH_MAX_SESSIONID]; /* multi-session-id */
      char		session_id[AUTH_MAX_SESSIONID];	/* session-id */
      char		ifname[IFNAMSIZ];	/* interface name */
      uint		ifindex;		/* System interface index */
      char		bundname[LINK_MAX_NAME];/* name of the bundle */
      char		lnkname[LINK_MAX_NAME];	/* name of the link */
      struct ng_ppp_link_stat64	stats;		/* Current link statistics */
#ifdef USE_NG_BPF
      struct svcstat	ss;
#endif
      char		*downReason;	/* Reason for link going down */
      time_t		last_up;	/* Time this link last got up */
      PhysType		phys_type;	/* Device type descriptor */
      int		linkID;		/* Absolute link number */
      char		peer_ident[64];	/* LCP ident received from peer */
      struct in_addr	peer_addr;	/* currently assigned IP-Address of the client */
      short		n_links;	/* number of links in the bundle */
      u_char		originate;	/* Who originated the connection */
    } info;
    struct authparams	params;		/* params to pass to from auth backend */
  };
  typedef struct authdata	*AuthData;
  
  extern const struct cmdtab AuthSetCmds[];

/*
 * GLOBAL VARIABLES
 */
  extern const u_char	gMsoftZeros[32];
  extern int		gMaxLogins;	/* max number of concurrent logins per user */
  extern int		gMaxLoginsCI;

/*
 * FUNCTIONS
 */

  extern void		AuthInit(Link l);
  extern void		AuthInst(Auth auth, Auth autht);
  extern void		AuthShutdown(Link l);
  extern void		AuthStart(Link l);
  extern void		AuthStop(Link l);
  extern void		AuthInput(Link l, int proto, Mbuf bp);
  extern void		AuthOutput(Link l, int proto, u_int code, u_int id,
			  const u_char *ptr, int len, int add_len, 
			  u_char eap_type);
  extern void		AuthFinish(Link l, int which, int ok);
  extern void		AuthCleanup(Link l);
  extern int		AuthStat(Context ctx, int ac, char *av[], void *arg);
  extern void		AuthAccountStart(Link l, int type);
  extern void		AuthAccountTimeout(void *arg);
  extern AuthData	AuthDataNew(Link l);
  extern void		AuthDataDestroy(AuthData auth);
  extern int		AuthGetData(char *authname, char *password, size_t passlen, 
			    struct u_range *range, u_char *range_valid);
  extern void		AuthAsyncStart(Link l, AuthData auth);
  extern const char	*AuthFailMsg(AuthData auth, char *buf, size_t len);
  extern const char	*AuthStatusText(int status);
  extern const char	*AuthMPPEPolicyname(int policy);
  extern const char	*AuthMPPETypesname(int types, char *buf, size_t len);

#if defined(USE_NG_BPF) || defined(USE_IPFW)
  extern void		ACLCopy(struct acl *src, struct acl **dst);
  extern void		ACLDestroy(struct acl *acl);
#endif
  extern void		authparamsInit(struct authparams *ap);
  extern void		authparamsCopy(struct authparams *src, struct authparams *dst);
  extern void		authparamsMove(struct authparams *src, struct authparams *dst);
  extern void		authparamsDestroy(struct authparams *ap);

#endif
