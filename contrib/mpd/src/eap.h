/*
 * See ``COPYRIGHT.mpd''
 *
 * $Id: eap.h,v 1.12 2008/01/06 15:10:52 amotin Exp $
 *
 */


#ifndef _EAP_H_
#define	_EAP_H_

#include "mbuf.h"
#include "timer.h"

/*
 * DEFINITIONS
 */

  #define EAP_NUM_TYPES		EAP_TYPE_MSCHAP_V2
  #define EAP_NUM_STDTYPES	3

  /* Configuration options */
  enum {
    EAP_CONF_RADIUS,
    EAP_CONF_MD5
  };

  enum {
    EAP_REQUEST = 1,
    EAP_RESPONSE,
    EAP_SUCCESS,
    EAP_FAILURE
  };

  enum {
    EAP_TYPE_IDENT = 1,
    EAP_TYPE_NOTIF,
    EAP_TYPE_NAK,
    EAP_TYPE_MD5CHAL,		/* MD5 Challenge */
    EAP_TYPE_OTP,		/* One Time Password */
    EAP_TYPE_GTC,		/* Generic Token Card */
    EAP_TYPE_RSA_PUB_KEY_AUTH = 9,	/* RSA Public Key Authentication */
    EAP_TYPE_DSS_UNILITERAL,	/* DSS Unilateral */
    EAP_TYPE_KEA,		/* KEA */
    EAP_TYPE_TYPE_KEA_VALIDATE,	/* KEA-VALIDATE */
    EAP_TYPE_EAP_TLS,		/* EAP-TLS RFC 2716 */
    EAP_TYPE_DEFENDER_TOKEN,	/* Defender Token (AXENT) */
    EAP_TYPE_RSA_SECURID,	/* RSA Security SecurID EAP */
    EAP_TYPE_ARCOT,		/* Arcot Systems EAP */
    EAP_TYPE_LEAP,		/* EAP-Cisco LEAP (MS-CHAP) */
    EAP_TYPE_NOKIA_IP_SC,	/* Nokia IP smart card authentication */
    EAP_TYPE_SRP_SHA1_1,	/* SRP-SHA1 Part 1 */
    EAP_TYPE_SRP_SHA1_2,	/* SRP-SHA1 Part 2 */
    EAP_TYPE_EAP_TTLS,		/* EAP-TTLS */
    EAP_TYPE_RAS,		/* Remote Access Service */
    EAP_TYPE_UMTS,		/* UMTS Authentication and Key Argreement */
    EAP_TYPE_3COM_WIRELESS,	/* EAP-3Com Wireless */
    EAP_TYPE_PEAP,		/* PEAP */
    EAP_TYPE_MS,		/* MS-EAP-Authentication */
    EAP_TYPE_MAKE,		/* MAKE, Mutual Authentication w/Key Exchange */
    EAP_TYPE_CRYPTOCARD,	/* CRYPTOCard */
    EAP_TYPE_MSCHAP_V2,		/* EAP-MSCHAP-V2 */
    EAP_TYPE_DYNAMID,		/* DynamID */
    EAP_TYPE_ROB,		/* Rob EAP */
    EAP_TYPE_SECURID,		/* SecurID EAP */
    EAP_TYPE_MS_AUTH_TLV,	/* MS-Authentication-TLV */
    EAP_TYPE_SENTRINET,		/* SentriNET */
    EAP_TYPE_ACTIONTEC_WIRELESS,/* EAP-Actiontec Wireless */
    EAP_TYPE_COGENT,		/* Cogent Systems Biometrics Authentication EAP */
    EAP_TYPE_AIRFORTRESS,	/* AirFortress EAP */
    EAP_TYPE_HTTP_DIGEST,	/* EAP-HTTP Digest */
    EAP_TYPE_SECURESUITE,	/* SecureSuite EAP */
    EAP_TYPE_DEVICECONNECT,	/* DeviceConnect */
    EAP_TYPE_SPEKE,		/* EAP-SPEKE */
    EAP_TYPE_MOBAC,		/* EAP-MOBAC */
    EAP_TYPE_FAST		/* EAP-FAST */
  };

  extern const	struct cmdtab EapSetCmds[];

  /* Configuration for a link */
  struct eapconf {
    struct optinfo	options;	/* Configured options */
  };

  struct eapinfo {
    short		next_id;		/* Packet id */
    short		retry;			/* Resend count */
    struct pppTimer	identTimer;		/* Identity timer */
    struct pppTimer	reqTimer;		/* Request timer */
    char		identity[AUTH_MAX_AUTHNAME];	/* Identity */
    u_char		peer_types[EAP_NUM_TYPES];	/* list of acceptable types */
    u_char		want_types[EAP_NUM_TYPES];	/* list of requestable types */
    struct eapconf	conf;			/* Configured options */
  };
  typedef struct eapinfo	*EapInfo;

  struct authdata;
/*
 * FUNCTIONS
 */

  extern void	EapInit(Link l);
  extern void	EapStart(Link l, int which);
  extern void	EapStop(EapInfo eap);
  extern void	EapInput(Link l, struct authdata *auth, const u_char *pkt, u_short len);
  extern const	char *EapCode(u_char code, char *buf, size_t len);
  extern const	char *EapType(u_char type);
  extern int	EapStat(Context ctx, int ac, char *av[], void *arg);

#endif

