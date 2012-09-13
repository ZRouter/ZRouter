/*
 * See ``COPYRIGHT.mpd''
 *
 * $Id: radius.h,v 1.45 2012/03/19 08:30:45 amotin Exp $
 *
 */

#ifdef CCP_MPPC
#include <netgraph/ng_mppc.h>
#endif
#include <radlib.h>

#include <net/if.h>
#include <net/if_types.h>

#include "iface.h"

#ifndef _RADIUS_H_
#define _RADIUS_H_

/*
 * DEFINITIONS
 */

  #define RADIUS_CHAP		1
  #define RADIUS_PAP		2
  #define RADIUS_EAP		3
  #define RADIUS_MAX_SERVERS	10

  #ifndef RAD_UPDATE
  #define RAD_UPDATE 3
  #endif

  #ifndef RAD_ACCT_INPUT_GIGAWORDS
  #define RAD_ACCT_INPUT_GIGAWORDS 52
  #endif

  #ifndef RAD_ACCT_OUTPUT_GIGAWORDS
  #define RAD_ACCT_OUTPUT_GIGAWORDS 53
  #endif

  #ifndef RAD_TUNNEL_TYPE
  #define RAD_TUNNEL_TYPE 64
  #endif

  #ifndef RAD_TUNNEL_MEDIUM_TYPE
  #define RAD_TUNNEL_MEDIUM_TYPE 65
  #endif

  #ifndef RAD_TUNNEL_CLIENT_ENDPOINT
  #define RAD_TUNNEL_CLIENT_ENDPOINT 66
  #endif

  #ifndef RAD_TUNNEL_SERVER_ENDPOINT
  #define RAD_TUNNEL_SERVER_ENDPOINT 67
  #endif

  #ifndef RAD_EAP_MESSAGE
  #define RAD_EAP_MESSAGE 79
  #endif

  #ifndef RAD_MESSAGE_AUTHENTIC
  #define RAD_MESSAGE_AUTHENTIC 80
  #endif

  #ifndef RAD_ACCT_INTERIM_INTERVAL
  #define RAD_ACCT_INTERIM_INTERVAL 85
  #endif

  #ifndef RAD_NAS_PORT_ID
  #define RAD_NAS_PORT_ID	87
  #endif

  #ifndef RAD_FRAMED_POOL
  #define RAD_FRAMED_POOL	88
  #endif

  #ifndef RAD_TUNNEL_CLIENT_AUTH_ID
  #define RAD_TUNNEL_CLIENT_AUTH_ID 90
  #endif

  #ifndef RAD_TUNNEL_SERVER_AUTH_ID
  #define RAD_TUNNEL_SERVER_AUTH_ID 91
  #endif

  #ifndef RAD_MAX_ATTR_LEN
  #define RAD_MAX_ATTR_LEN 253
  #endif

  /* for mppe-keys */
  #define AUTH_LEN		16
  #define SALT_LEN		2

  /* max. length of RAD_ACCT_SESSION_ID, RAD_ACCT_MULTI_SESSION_ID */
  #define RAD_ACCT_MAX_SESSIONID	256

  #define RAD_VENDOR_MPD	12341
  #define RAD_MPD_RULE		1
  #define RAD_MPD_PIPE		2
  #define RAD_MPD_QUEUE		3
  #define RAD_MPD_TABLE		4
  #define RAD_MPD_TABLE_STATIC	5
  #define RAD_MPD_FILTER	6
  #define RAD_MPD_LIMIT		7
  #define RAD_MPD_INPUT_OCTETS	8
  #define RAD_MPD_INPUT_PACKETS	9
  #define RAD_MPD_OUTPUT_OCTETS	10
  #define RAD_MPD_OUTPUT_PACKETS	11
  #define RAD_MPD_LINK		12
  #define RAD_MPD_BUNDLE	13
  #define RAD_MPD_IFACE		14
  #define RAD_MPD_IFACE_INDEX	15
  #define RAD_MPD_INPUT_ACCT	16
  #define RAD_MPD_OUTPUT_ACCT	17
  #define RAD_MPD_ACTION	18
  #define RAD_MPD_PEER_IDENT	19
  #define RAD_MPD_IFACE_NAME	20
  #define RAD_MPD_IFACE_DESCR	21
  #define RAD_MPD_IFACE_GROUP	22
  #define RAD_MPD_DROP_USER	154

  /* Configuration options */
  enum {
    RADIUS_CONF_MESSAGE_AUTHENTIC
  };

  extern const	struct cmdtab RadiusSetCmds[];
  extern const	struct cmdtab RadiusUnSetCmds[];

  /* Configuration for a radius server */
  struct radiusserver_conf {
    char	*hostname;
    char	*sharedsecret;
    in_port_t	auth_port;
    in_port_t	acct_port;
    struct	radiusserver_conf *next;
  };
  typedef struct radiusserver_conf *RadServe_Conf;

  struct radiusconf {
    int		radius_timeout;
    int		radius_retries;
    struct	in_addr radius_me;
    struct	u_addr radius_mev6;
    char	*identifier;
    char	*file;
    struct radiusserver_conf *server;
    struct optinfo	options;	/* Configured options */
  };
  typedef struct radiusconf *RadConf;

  struct rad_chapvalue {
    u_char	ident;
    u_char	response[CHAP_MAX_VAL];
  };

  struct rad_mschapvalue {
    u_char	ident;
    u_char	flags;
    u_char	lm_response[24];
    u_char	nt_response[24];
  };

  struct rad_mschapv2value {
    u_char	ident;
    u_char	flags;
    u_char	pchallenge[16];
    u_char	reserved[8];
    u_char	response[24];
  };

  struct authdata;

/*
 * FUNCTIONS
 */

  extern void	RadiusInit(Link l);
  extern int	RadiusAuthenticate(struct authdata *auth);
  extern int	RadiusAccount(struct authdata *auth);
  extern void	RadiusClose(struct authdata *auth);
  extern void	RadiusEapProxy(void *arg);
  extern int	RadStat(Context ctx, int ac, char *av[], void *arg);

#endif
