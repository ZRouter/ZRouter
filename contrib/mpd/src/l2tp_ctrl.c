
/*
 * Copyright (c) 2001-2002 Packet Design, LLC.
 * All rights reserved.
 * 
 * Subject to the following obligations and disclaimer of warranty,
 * use and redistribution of this software, in source or object code
 * forms, with or without modifications are expressly permitted by
 * Packet Design; provided, however, that:
 * 
 *    (i)  Any and all reproductions of the source or object code
 *         must include the copyright notice above and the following
 *         disclaimer of warranties; and
 *    (ii) No rights are granted, in any manner or form, to use
 *         Packet Design trademarks, including the mark "PACKET DESIGN"
 *         on advertising, endorsements, or otherwise except as such
 *         appears in the above copyright notice or in the software.
 * 
 * THIS SOFTWARE IS BEING PROVIDED BY PACKET DESIGN "AS IS", AND
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, PACKET DESIGN MAKES NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, REGARDING
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY AND ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT.  PACKET DESIGN DOES NOT WARRANT, GUARANTEE,
 * OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF, OR THE RESULTS
 * OF THE USE OF THIS SOFTWARE IN TERMS OF ITS CORRECTNESS, ACCURACY,
 * RELIABILITY OR OTHERWISE.  IN NO EVENT SHALL PACKET DESIGN BE
 * LIABLE FOR ANY DAMAGES RESULTING FROM OR ARISING OUT OF ANY USE
 * OF THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE, OR CONSEQUENTIAL
 * DAMAGES, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, LOSS OF
 * USE, DATA OR PROFITS, HOWEVER CAUSED AND UNDER ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF PACKET DESIGN IS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Archie Cobbs <archie@freebsd.org>
 */

#include "ppp.h"
#include <netgraph.h>
#include <netgraph/ng_message.h>
#include <netgraph/ng_socket.h>
#include <netgraph/ng_l2tp.h>
#include <netgraph/ng_tee.h>
#include <openssl/md5.h>
#include "l2tp_avp.h"
#include "l2tp_ctrl.h"
#include "ngfunc.h"

#ifndef __FreeBSD__
#define __printflike(x,y)
#endif

#define CTRL_MEM_TYPE		"ppp_l2tp_ctrl"
#define SESS_MEM_TYPE		"ppp_l2tp_sess"

/* Retransmit timeout parameters */
#define L2TP_REXMIT_MAX		8
#define L2TP_REXMIT_MAX_TO	10

#define L2TP_CHALLENGE_LEN	16

#define L2TP_CONNECT_SPEED	10000000		/* XXX hardcoded */
#define L2TP_FRAMING_TYPE	L2TP_FRAMING_SYNC	/* XXX hardcoded */

/* Idle timeout for sending 'HELLO' message */
#define L2TP_IDLE_TIMEOUT	60

/* Reply timeout for messages */
#define L2TP_REPLY_TIMEOUT	60

/* Linger time for dying tunnels and sessions */
#define L2TP_CTRL_DEATH_TIMEOUT	11
#define L2TP_SESS_DEATH_TIMEOUT	11

#define LOG_ERR		LG_ERR
#define LOG_WARNING	LG_PHYS2
#define LOG_NOTICE	LG_PHYS2
#define LOG_INFO	LG_PHYS3
#define LOG_DEBUG	LG_PHYS3

/* Control message types */
enum l2tp_msg_type {
	SCCRQ		=1,
	SCCRP		=2,
	SCCCN		=3,
	StopCCN		=4,
	HELLO		=6,
	OCRQ		=7,
	OCRP		=8,
	OCCN		=9,
	ICRQ		=10,
	ICRP		=11,
	ICCN		=12,
	CDN		=14,
	WEN		=15,
	SLI		=16
};

/* Control connection states */
enum l2tp_ctrl_state {
	CS_IDLE = 0,
	CS_WAIT_CTL_REPLY,
	CS_WAIT_CTL_CONNECT,
	CS_ESTABLISHED,
	CS_DYING
};

/* Session states */
enum l2tp_sess_state {
	SS_WAIT_REPLY = 1,
	SS_WAIT_CONNECT,
	SS_WAIT_ANSWER,
	SS_ESTABLISHED,
	SS_DYING
};

/* Session origination */
enum l2tp_sess_orig {
	ORIG_LOCAL	=0,
	ORIG_REMOTE	=1
};

/* Session side */
enum l2tp_sess_side {
	SIDE_LNS	=0,
	SIDE_LAC	=1
};

/*
 * Control message handler function types.
 *
 * Messages are either session-specific, or apply to the entire tunnel.
 */
typedef int	l2tp_ctrl_msg_handler_t(struct ppp_l2tp_ctrl *ctrl,
			const struct ppp_l2tp_avp_list *avps,
			struct ppp_l2tp_avp_ptrs *ptrs);
typedef int	l2tp_sess_msg_handler_t(struct ppp_l2tp_sess *sess,
			const struct ppp_l2tp_avp_list *avps,
			struct ppp_l2tp_avp_ptrs *ptrs);

/* Descriptor for one control message */
struct l2tp_msg_info {
	const char		*name;
	enum l2tp_msg_type	type;
	l2tp_ctrl_msg_handler_t	*ctrl_handler;
	l2tp_sess_msg_handler_t	*sess_handler;
	int			valid_states[12];
	int			valid_orig[3];
	int			valid_side[3];
	int			req_avps[AVP_MAX + 1];
};

/* Control connection */
struct ppp_l2tp_ctrl {
	enum l2tp_ctrl_state	state;			/* control state */
	const struct ppp_l2tp_ctrl_cb *cb;		/* link callbacks */
	struct pevent_ctx	*ctx;			/* event context */
	pthread_mutex_t		*mutex;			/* mutex */
	ng_ID_t			node_id;		/* l2tp node id */
	u_int32_t		peer_id;		/* peer unique id */
	char			path[32];		/* l2tp node path */
	int			csock;			/* netgraph ctrl sock */
	int			dsock;			/* netgraph data sock */
	u_char			*secret;		/* shared secret */
	u_int			seclen;			/* share secret len */
	u_char			chal[L2TP_CHALLENGE_LEN]; /* our L2TP challenge */
	struct ng_l2tp_config	config;			/* netgraph node cfg. */
	struct ghash		*sessions;		/* sessions */
	struct ppp_l2tp_avp_list *avps;			/* avps for SCCR[QP] */
	struct pevent		*idle_timer;		/* ctrl idle timer */
	struct pevent		*reply_timer;		/* reply timer */
	struct pevent		*close_timer;		/* close timer */
	struct pevent		*death_timer;		/* death timer */
	struct pevent		*ctrl_event;		/* ctrl socket event */
	struct pevent		*data_event;		/* data socket event */
	void			*link_cookie;		/* opaque link cookie */
	u_int16_t		result;			/* close result code */
	u_int16_t		error;			/* close error code */
	u_int32_t		peer_bearer;		/* peer bearer types */
	u_int32_t		peer_framing;		/* peer framing types */
	u_int			active_sessions;	/* # of sessns */
	char			*errmsg;		/* close error msg */
	u_char			link_notified;		/* link notified down */
	u_char			peer_notified;		/* peer notified down */
	u_char			hide_avps;		/* enable AVPs hiding */
	char 			self_name[MAXHOSTNAMELEN]; /* L2TP local hostname */
	char 			peer_name[MAXHOSTNAMELEN]; /* L2TP remote hostname */
};

/* Session */
struct ppp_l2tp_sess {
	struct ppp_l2tp_ctrl	*ctrl;			/* associated ctrl */
	enum l2tp_sess_state	state;			/* session state */
	enum l2tp_sess_orig	orig;			/* who originated it? */
	enum l2tp_sess_side	side;			/* are we lac or lns? */
	u_int32_t		serial;			/* call serial number */
	struct ng_l2tp_sess_config config;		/* netgraph hook cfg. */
	struct ppp_l2tp_avp_list *my_avps;		/* avps in [IO]CR[QP] */
	struct ppp_l2tp_avp_list *peer_avps;		/* avps in [IO]CCN */
	u_int16_t		session_id;		/* session id */
	u_int16_t		peer_id;		/* peer session id */
	ng_ID_t			node_id;		/* tee node id */
	char			hook[NG_HOOKSIZ];	/* session hook name */
	void			*link_cookie;		/* opaque link cookie */
	u_int16_t		result;			/* close result code */
	u_int16_t		error;			/* close error code */
	char			*errmsg;		/* close error msg */
	struct pevent		*reply_timer;		/* reply timer */
	struct pevent		*notify_timer;		/* link notify timer */
	struct pevent		*close_timer;		/* close timer */
	struct pevent		*death_timer;		/* death timer */
	u_char			link_responded;		/* link notified up */
	u_char			peer_responded;		/* got icrp from lns */
	u_char			dseq_required;		/* data seq. req'd */
	u_char			link_notified;		/* link notified down */
	u_char			peer_notified;		/* peer notified down */
	u_char			include_length;		/* enable Length field in data packets */
	u_char			enable_dseq;		/* enable sequence fields in data packets */
};

/***

Notes

    - "link_notified" means the link side has been notified that
      the control connection or session has gone down.
    - "peer_notified" means the peer has been notified that
      the control connection or session has gone down.
    - "link_responded" and "peer_responded" are only used for
      outgoing calls when we are the LAC; they indicate acceptance
      from the link side and the remote peer, respectively. Both
      must be true before we send the OCCN.
    - "sess->my_avps" are the AVP's included my ICRQ or OCRQ. In case
      of ICRQ, these get overwritten by AVP's included in ICCN.
    - "sess->peer_avps" are the AVPS included peer's ICCN or OCCN

***/

/************************************************************************
			INTERNAL FUNCTIONS
************************************************************************/

static int	ppp_l2tp_ctrl_setup_1(struct ppp_l2tp_ctrl *ctrl,
			struct ppp_l2tp_avp_ptrs *ptrs);
static int	ppp_l2tp_ctrl_setup_2(struct ppp_l2tp_ctrl *ctrl,
			struct ppp_l2tp_avp_ptrs *ptrs);
static void	ppp_l2tp_ctrl_send(struct ppp_l2tp_ctrl *ctrl,
			u_int16_t session_id, enum l2tp_msg_type msgtype,
			const struct ppp_l2tp_avp_list *avps0);
static void	ppp_l2tp_ctrl_check_reply(struct ppp_l2tp_ctrl *ctrl);
static void	ppp_l2tp_ctrl_close(struct ppp_l2tp_ctrl *ctrl,
			u_int16_t result, u_int16_t error, const char *errmsg);
static void	ppp_l2tp_ctrl_death_start(struct ppp_l2tp_ctrl *ctrl);

static struct	ppp_l2tp_sess *ppp_l2tp_sess_create(struct ppp_l2tp_ctrl *ctrl,
			enum l2tp_sess_orig orig, enum l2tp_sess_side side, 
			u_int32_t serial);
static void	ppp_l2tp_sess_destroy(struct ppp_l2tp_sess **sessp);
static void	ppp_l2tp_sess_close(struct ppp_l2tp_sess *sess,
			u_int16_t result, u_int16_t error, const char *errmsg);
static int	ppp_l2tp_sess_setup(struct ppp_l2tp_sess *sess);
static int	ppp_l2tp_sess_check_liic(struct ppp_l2tp_sess *sess);
static void	ppp_l2tp_sess_check_reply(struct ppp_l2tp_sess *sess);

static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_SCCRQ;
static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_SCCRP;
static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_SCCCN;
static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_StopCCN;
static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_HELLO;
static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_OCRQ;
static l2tp_ctrl_msg_handler_t	ppp_l2tp_handle_ICRQ;

static l2tp_sess_msg_handler_t	ppp_l2tp_handle_OCRP;
static l2tp_sess_msg_handler_t	ppp_l2tp_handle_xCCN;
static l2tp_sess_msg_handler_t	ppp_l2tp_handle_ICRP;
static l2tp_sess_msg_handler_t	ppp_l2tp_handle_CDN;
static l2tp_sess_msg_handler_t	ppp_l2tp_handle_WEN;
static l2tp_sess_msg_handler_t	ppp_l2tp_handle_SLI;

static pevent_handler_t		ppp_l2tp_ctrl_event;
static pevent_handler_t		ppp_l2tp_data_event;

static pevent_handler_t		ppp_l2tp_idle_timeout;
static pevent_handler_t		ppp_l2tp_unused_timeout;
static pevent_handler_t		ppp_l2tp_ctrl_do_close;
static pevent_handler_t		ppp_l2tp_ctrl_death_timeout;

static pevent_handler_t		ppp_l2tp_sess_notify;
static pevent_handler_t		ppp_l2tp_sess_do_close;
static pevent_handler_t		ppp_l2tp_sess_death_timeout;

static void	ppp_l2tp_ctrl_dump(struct ppp_l2tp_ctrl *ctrl,
			struct ppp_l2tp_avp_list *list, const char *fmt, ...)
			__printflike(3, 4);
static const	char *ppp_l2tp_ctrl_state_str(enum l2tp_ctrl_state state);
static const	char *ppp_l2tp_sess_state_str(enum l2tp_ctrl_state state);
static const	char *ppp_l2tp_sess_orig_str(enum l2tp_sess_orig orig);
static const	char *ppp_l2tp_sess_side_str(enum l2tp_sess_side side);

static ghash_hash_t	ppp_l2tp_ctrl_hash;
static ghash_equal_t	ppp_l2tp_ctrl_equal;

static ghash_hash_t	ppp_l2tp_sess_hash;
static ghash_equal_t	ppp_l2tp_sess_equal;

/************************************************************************
			INTERNAL VARIABLES
************************************************************************/

/* Descriptors for each L2TP message type */
static const	struct l2tp_msg_info ppp_l2tp_msg_info[] = {

	/* Control connection messages */
	{ "SCCRQ", SCCRQ,	ppp_l2tp_handle_SCCRQ, NULL,
	    { CS_IDLE, -1 },
	    { -1 }, { -1 },
	    { AVP_PROTOCOL_VERSION, AVP_HOST_NAME,
		AVP_FRAMING_CAPABILITIES, AVP_ASSIGNED_TUNNEL_ID, -1 } },
	{ "SCCRP", SCCRP,	ppp_l2tp_handle_SCCRP, NULL,
	    { CS_WAIT_CTL_REPLY, -1 },
	    { -1 }, { -1 },
	    { AVP_PROTOCOL_VERSION, AVP_HOST_NAME,
		AVP_FRAMING_CAPABILITIES, AVP_ASSIGNED_TUNNEL_ID, -1 } },
	{ "SCCCN", SCCCN,	ppp_l2tp_handle_SCCCN, NULL,
	    { CS_WAIT_CTL_CONNECT, -1 },
	    { -1 }, { -1 },
	    { -1 } },
	{ "StopCCN", StopCCN,	ppp_l2tp_handle_StopCCN, NULL,
	    { CS_IDLE, CS_WAIT_CTL_REPLY, CS_WAIT_CTL_CONNECT,
		CS_ESTABLISHED, CS_DYING, -1 },
	    { -1 }, { -1 },
	    { AVP_ASSIGNED_TUNNEL_ID, AVP_RESULT_CODE, -1 } },
	{ "HELLO", HELLO,	ppp_l2tp_handle_HELLO, NULL,
	    { CS_IDLE, CS_WAIT_CTL_REPLY, CS_WAIT_CTL_CONNECT,
		CS_ESTABLISHED, CS_DYING, -1 },
	    { -1 }, { -1 },
	    { -1 } },
	{ "ICRQ", ICRQ,		ppp_l2tp_handle_ICRQ, NULL,
	    { CS_ESTABLISHED, -1 },
	    { -1 }, { -1 },
	    { AVP_ASSIGNED_SESSION_ID, AVP_CALL_SERIAL_NUMBER, -1 } },
	{ "OCRQ", OCRQ,		ppp_l2tp_handle_OCRQ, NULL,
	    { CS_ESTABLISHED, -1 },
	    { -1 }, { -1 },
	    { AVP_ASSIGNED_SESSION_ID, AVP_CALL_SERIAL_NUMBER,
		AVP_MINIMUM_BPS, AVP_MAXIMUM_BPS, AVP_BEARER_TYPE,
		AVP_FRAMING_TYPE, AVP_CALLED_NUMBER, -1 } },

	/* Session connection messages */
	{ "ICRP", ICRP,		NULL, ppp_l2tp_handle_ICRP,
	    { SS_WAIT_REPLY, SS_DYING, -1 },
	    { ORIG_LOCAL, -1 }, { SIDE_LAC, -1 },
	    { AVP_ASSIGNED_SESSION_ID, -1 } },
	{ "OCRP", OCRP,		NULL, ppp_l2tp_handle_OCRP,
	    { SS_WAIT_REPLY, SS_DYING, -1 },
	    { ORIG_LOCAL, -1 }, { SIDE_LNS, -1 },
	    { AVP_ASSIGNED_SESSION_ID, -1 } },
	{ "ICCN", ICCN,		NULL, ppp_l2tp_handle_xCCN,
	    { SS_WAIT_CONNECT, SS_DYING, -1 },
	    { ORIG_REMOTE, -1 }, { SIDE_LNS, -1 },
	    { AVP_TX_CONNECT_SPEED, AVP_FRAMING_TYPE, -1 } },
	{ "OCCN", OCCN,		NULL, ppp_l2tp_handle_xCCN,
	    { SS_WAIT_CONNECT, SS_DYING, -1 },
	    { ORIG_LOCAL, -1 }, { SIDE_LNS, -1 },
	    { AVP_TX_CONNECT_SPEED, AVP_FRAMING_TYPE, -1 } },
	{ "CDN", CDN,		NULL, ppp_l2tp_handle_CDN,
	    { SS_WAIT_REPLY, SS_WAIT_CONNECT,
		SS_WAIT_ANSWER, SS_ESTABLISHED, SS_DYING, -1 },
	    { ORIG_LOCAL, ORIG_REMOTE, -1 }, { SIDE_LAC, SIDE_LNS, -1 },
	    { AVP_RESULT_CODE, AVP_ASSIGNED_SESSION_ID, -1 } },
	{ "WEN", WEN,		NULL, ppp_l2tp_handle_WEN,
	    { SS_ESTABLISHED, SS_DYING, -1 },
	    { ORIG_LOCAL, ORIG_REMOTE, -1 }, { SIDE_LNS, -1 },
	    { AVP_CALL_ERRORS, -1 } },
	{ "SLI", SLI,		NULL, ppp_l2tp_handle_SLI,
	    { SS_ESTABLISHED, SS_DYING, -1 },
	    { ORIG_LOCAL, ORIG_REMOTE, -1 }, { SIDE_LAC, -1 },
	    { AVP_ACCM, -1 } },
	{ NULL }
};

/* Descriptors for each AVP */

#define AVP_ITEM(x,h,m,min,max)	\
	{ #x, ppp_l2tp_avp_decode_ ## x, 0, AVP_ ## x, h, m, min, max }

static const	struct ppp_l2tp_avp_info ppp_l2tp_avp_info_list[] = {
	AVP_ITEM(MESSAGE_TYPE,		0,  1,  2,  2),
	AVP_ITEM(RANDOM_VECTOR,		0,  1,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(RESULT_CODE,		0,  1,  2,  AVP_MAX_LENGTH),
	AVP_ITEM(PROTOCOL_VERSION,	0,  1,  2,  2),
	AVP_ITEM(FRAMING_CAPABILITIES,	1,  1,  4,  4),
	AVP_ITEM(BEARER_CAPABILITIES,	1,  1,  4,  4),
	AVP_ITEM(TIE_BREAKER,		0,  0,  8,  8),
	AVP_ITEM(FIRMWARE_REVISION,	1,  0,  2,  2),
	AVP_ITEM(HOST_NAME,		0,  1,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(VENDOR_NAME,		1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(ASSIGNED_TUNNEL_ID,	1,  1,  2,  2),
	AVP_ITEM(RECEIVE_WINDOW_SIZE,	0,  1,  2,  2),
	AVP_ITEM(CHALLENGE,		1,  1,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(CHALLENGE_RESPONSE,	1,  1, 16,  16),
	AVP_ITEM(CAUSE_CODE,		0,  1,  3,  AVP_MAX_LENGTH),
	AVP_ITEM(ASSIGNED_SESSION_ID,	1,  1,  2,  2),
	AVP_ITEM(CALL_SERIAL_NUMBER,	1,  1,  4,  4),
	AVP_ITEM(MINIMUM_BPS,		1,  1,  4,  4),
	AVP_ITEM(MAXIMUM_BPS,		1,  1,  4,  4),
	AVP_ITEM(BEARER_TYPE,		1,  1,  4,  4),
	AVP_ITEM(FRAMING_TYPE,		1,  1,  4,  4),
	AVP_ITEM(CALLED_NUMBER,		1,  1,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(CALLING_NUMBER,	1,  1,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(SUB_ADDRESS,		1,  1,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(TX_CONNECT_SPEED,	1,  1,  4,  4),
	AVP_ITEM(RX_CONNECT_SPEED,	1,  0,  4,  4),
	AVP_ITEM(PHYSICAL_CHANNEL_ID,	1,  0,  4,  4),
	AVP_ITEM(PRIVATE_GROUP_ID,	1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(SEQUENCING_REQUIRED,	0,  1,  0,  0),
	AVP_ITEM(INITIAL_RECV_CONFREQ,	1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(LAST_SENT_CONFREQ,	1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(LAST_RECV_CONFREQ,	1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(PROXY_AUTHEN_TYPE,	1,  0,  2,  2),
	AVP_ITEM(PROXY_AUTHEN_NAME,	1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(PROXY_AUTHEN_CHALLENGE,1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(PROXY_AUTHEN_ID,	1,  0,  2,  2),
	AVP_ITEM(PROXY_AUTHEN_RESPONSE,	1,  0,  0,  AVP_MAX_LENGTH),
	AVP_ITEM(CALL_ERRORS,		1,  1, 26,  26),
	AVP_ITEM(ACCM,			1,  1, 10,  10),
	{ NULL, NULL, 0, 0, 0, 0, 0, 0 }
};

/* All control connections */
struct ghash	*ppp_l2tp_ctrls;

static uint32_t gNextSerial = 0;

/************************************************************************
			PUBLIC FUNCTIONS
************************************************************************/

/*
 * Create a new control connection.
 */
struct ppp_l2tp_ctrl *
ppp_l2tp_ctrl_create(struct pevent_ctx *ctx, pthread_mutex_t *mutex,
	const struct ppp_l2tp_ctrl_cb *cb,
	u_int32_t peer_id, ng_ID_t *nodep, char *hook,
	const struct ppp_l2tp_avp_list *avps, const void *secret, size_t seclen,
	u_char hide_avps)
{
	struct ppp_l2tp_ctrl *ctrl;
	struct ngm_mkpeer mkpeer;
	u_int16_t value16;
	int index, i;
	
	/* Init Call Serial Number */
	if (gNextSerial == 0)
	    gNextSerial = (random() % 900) * 10000;

	/* Create global control structure hash table */
	if (ppp_l2tp_ctrls == NULL
	    && (ppp_l2tp_ctrls = ghash_create(NULL, 0, 0, CTRL_MEM_TYPE,
	      ppp_l2tp_ctrl_hash, ppp_l2tp_ctrl_equal, NULL, NULL)) == NULL)
		return (NULL);

	/* Create control connection */
	ctrl = Malloc(CTRL_MEM_TYPE, sizeof(*ctrl));
	ctrl->ctx = ctx;
	ctrl->mutex = mutex;
	ctrl->cb = cb;
	ctrl->peer_id = peer_id;
	ctrl->csock = -1;
	ctrl->dsock = -1;

	/* Debugging */
	Log(LOG_DEBUG, ("L2TP: %s invoked", __FUNCTION__));

	/* Select an unused, non-zero local tunnel ID */
	while (ctrl->config.tunnel_id == 0
	    || ghash_get(ppp_l2tp_ctrls, ctrl) != NULL)
		ctrl->config.tunnel_id = random() & 0xffff;

	/* Add control structure to hash table */
	if (ghash_put(ppp_l2tp_ctrls, ctrl) == -1)
		goto fail;

	/* Copy shared secret, if any */
	if (seclen > 0)
		ctrl->secret = Mdup(CTRL_MEM_TYPE, secret, seclen);
	ctrl->seclen = seclen;
	ctrl->hide_avps = hide_avps;

	/* Create sessions hash table */
	if ((ctrl->sessions = ghash_create(NULL, 0, 0, CTRL_MEM_TYPE,
	    ppp_l2tp_sess_hash, ppp_l2tp_sess_equal, NULL, NULL)) == NULL)
		goto fail;

	/* Create netgraph node */
	if (NgMkSockNode(NULL, &ctrl->csock, &ctrl->dsock) == -1)
		goto fail;
	memset(&mkpeer, 0, sizeof(mkpeer));
	strlcpy(mkpeer.type, NG_L2TP_NODE_TYPE, sizeof(mkpeer.type));
	strlcpy(mkpeer.ourhook, NG_L2TP_HOOK_CTRL, sizeof(mkpeer.ourhook));
	strlcpy(mkpeer.peerhook, NG_L2TP_HOOK_CTRL, sizeof(mkpeer.peerhook));
	if (NgSendMsg(ctrl->csock, ".:", NGM_GENERIC_COOKIE,
	    NGM_MKPEER, &mkpeer, sizeof(mkpeer)) == -1)
		goto fail;

	/* Get l2tp node ID */
	if ((ctrl->node_id = NgGetNodeID(ctrl->csock, NG_L2TP_HOOK_CTRL)) == 0) {
	    Perror("L2TP: Cannot get %s node id", NG_L2TP_NODE_TYPE);
	    goto fail;
	};
	snprintf(ctrl->path, sizeof(ctrl->path),
	    "[%lx]:", (u_long)ctrl->node_id);

	/* Configure netgraph node with initial configuration */
	ctrl->config.enabled = 1;
	ctrl->config.peer_win = 1;		/* we increase this later */
	ctrl->config.rexmit_max = L2TP_REXMIT_MAX;
	ctrl->config.rexmit_max_to = L2TP_REXMIT_MAX_TO;
	if (NgSendMsg(ctrl->csock, NG_L2TP_HOOK_CTRL, NGM_L2TP_COOKIE,
	    NGM_L2TP_SET_CONFIG, &ctrl->config, sizeof(ctrl->config)) == -1)
		goto fail;

	/* Listen for control messages and control packets */
	if (pevent_register(ctrl->ctx, &ctrl->ctrl_event, PEVENT_RECURRING,
	    ctrl->mutex, ppp_l2tp_ctrl_event, ctrl, PEVENT_READ,
	    ctrl->csock) == -1)
		goto fail;
	if (pevent_register(ctrl->ctx, &ctrl->data_event, PEVENT_RECURRING,
	    ctrl->mutex, ppp_l2tp_data_event, ctrl, PEVENT_READ,
	    ctrl->dsock) == -1)
		goto fail;

	/* Copy initial AVP list */
	ctrl->avps = (avps == NULL) ?
	    ppp_l2tp_avp_list_create() :
	    ppp_l2tp_avp_list_copy(avps);
	if (ctrl->avps == NULL)
		goto fail;

	/* Add required AVP's */
	if (ppp_l2tp_avp_list_find(ctrl->avps,
	    0, AVP_PROTOCOL_VERSION)== -1) {
		const u_char pv[] = { L2TP_PROTO_VERS, L2TP_PROTO_REV };

		if (ppp_l2tp_avp_list_append(ctrl->avps, 1,
		    0, AVP_PROTOCOL_VERSION, pv, sizeof(pv)) == -1)
			goto fail;
	}
	if ((index = ppp_l2tp_avp_list_find(ctrl->avps,
	    0, AVP_HOST_NAME)) == -1) {
		(void)gethostname(ctrl->self_name, sizeof(ctrl->self_name) - 1);
		ctrl->self_name[sizeof(ctrl->self_name) - 1] = '\0';
		if (ppp_l2tp_avp_list_append(ctrl->avps, 1,
		    0, AVP_HOST_NAME, ctrl->self_name, strlen(ctrl->self_name)) == -1)
			goto fail;
	} else {
	    int len = ctrl->avps->avps[index].vlen;
	    if (len >= sizeof(ctrl->self_name))
		len = sizeof(ctrl->self_name) - 1;
	    memcpy(ctrl->self_name, ctrl->avps->avps[index].value, len);
	    ctrl->self_name[len] = 0;
	}
	if (ppp_l2tp_avp_list_find(ctrl->avps,
	    0, AVP_FRAMING_CAPABILITIES) == -1) {
		const u_int32_t value = htonl(L2TP_FRAMING_SYNC|L2TP_FRAMING_ASYNC);

		if (ppp_l2tp_avp_list_append(ctrl->avps, 1,
		    0, AVP_FRAMING_CAPABILITIES, &value, sizeof(value)) == -1)
			goto fail;
	}
	if ((index = ppp_l2tp_avp_list_find(ctrl->avps,
	    0, AVP_ASSIGNED_TUNNEL_ID)) != -1)
		ppp_l2tp_avp_list_remove(ctrl->avps, index);
	value16 = htons(ctrl->config.tunnel_id);
	if (ppp_l2tp_avp_list_append(ctrl->avps, 1,
	    0, AVP_ASSIGNED_TUNNEL_ID, &value16, sizeof(value16)) == -1)
		goto fail;

	if ((index = ppp_l2tp_avp_list_find(ctrl->avps,
	    0, AVP_CHALLENGE)) != -1)
		ppp_l2tp_avp_list_remove(ctrl->avps, index);

	if (ctrl->secret != NULL) {
	    for (i = 0; i < sizeof(ctrl->chal); i++)
		    ctrl->chal[i] = random();
	    if (ppp_l2tp_avp_list_append(ctrl->avps, 1,
		0, AVP_CHALLENGE, &ctrl->chal, sizeof(ctrl->chal)) == -1)
		    goto fail;
	}

	ctrl->state = CS_IDLE;		/* wait for peer's sccrq */

	/* Expect some sort of reply */
	ppp_l2tp_ctrl_check_reply(ctrl);

	/* Done */
	*nodep = ctrl->node_id;
	strlcpy(hook, NG_L2TP_HOOK_LOWER, NG_HOOKSIZ);
	return (ctrl);

fail:
	/* Clean up after failure */
	if (ctrl->csock >= 0)
		(void)close(ctrl->csock);	/* l2tp node will go away too */
	if (ctrl->dsock >= 0)
		(void)close(ctrl->dsock);
	pevent_unregister(&ctrl->reply_timer);
	pevent_unregister(&ctrl->ctrl_event);
	pevent_unregister(&ctrl->data_event);
	ppp_l2tp_avp_list_destroy(&ctrl->avps);
	ghash_remove(ppp_l2tp_ctrls, ctrl);
	ghash_destroy(&ctrl->sessions);
	Freee(ctrl->secret);
	Freee(ctrl);
	if (ppp_l2tp_ctrls != NULL && ghash_size(ppp_l2tp_ctrls) == 0)
		ghash_destroy(&ppp_l2tp_ctrls);
	return (NULL);
}

/*
 * Create a new control connection.
 */
struct ppp_l2tp_ctrl *
ppp_l2tp_ctrl_initiate(struct ppp_l2tp_ctrl *ctrl)
{
	/* Debugging */
	Log(LOG_DEBUG, ("L2TP: %s invoked", __FUNCTION__));

	/* Initiate connection, we're the initiator */
	ctrl->state = CS_WAIT_CTL_REPLY;
	ppp_l2tp_ctrl_send(ctrl, 0, SCCRQ, ctrl->avps);

	/* Expect some sort of reply */
	ppp_l2tp_ctrl_check_reply(ctrl);

	/* Done */
	return (ctrl);
}

/*
 * Shutdown a control connection.
 */
void
ppp_l2tp_ctrl_shutdown(struct ppp_l2tp_ctrl *ctrl,
        u_int16_t result, u_int16_t error, const char *errmsg)
{
	/* Debugging */
	Log(LOG_DEBUG, ("L2TP: %s invoked, ctrl=%p errmsg=\"%s\"",
	    __FUNCTION__, ctrl, errmsg));

	/* Close control connection */
	ppp_l2tp_ctrl_close(ctrl, result, error, errmsg);
}

/*
 * Initiate a new session.
 */
struct ppp_l2tp_sess *
ppp_l2tp_initiate(struct ppp_l2tp_ctrl *ctrl, int out,
	u_char include_length, u_char enable_dseq,
	const struct ppp_l2tp_avp_list *avps)
{
	struct ppp_l2tp_sess *sess;
	u_int32_t value32;
	int i;

	/* Debugging */
	Log(LOG_DEBUG, ("L2TP: %s invoked, ctrl=%p out=%d", __FUNCTION__, ctrl, out));

	/* Check control connection */
	/* XXX add support for sessions waiting for open ctrl conection */
	if (ctrl->state != CS_ESTABLISHED) {
		errno = ENXIO;
		return (NULL);
	}

	/* Verify peer supports outgoing calls */
	if (out
	    && (ctrl->peer_bearer
	      & (L2TP_BEARER_DIGITAL|L2TP_BEARER_ANALOG)) == 0) {
		errno = EOPNOTSUPP;
		return (NULL);
	}

	/* Create new session */
	if ((sess = ppp_l2tp_sess_create(ctrl,
	    ORIG_LOCAL, out ? SIDE_LNS : SIDE_LAC,
	    gNextSerial++)) == NULL)
		return (NULL);
	
	sess->include_length = include_length;
	sess->enable_dseq = enable_dseq;

	/* Copy AVP's supplied by caller */
	if (avps) {
	    for (i = 0; i < avps->length; i++) {
		const struct ppp_l2tp_avp *const avp = &avps->avps[i];

		if (ppp_l2tp_avp_list_append(sess->my_avps,
		    avp->mandatory, avp->vendor, avp->type,
		    avp->value, avp->vlen) == -1)
			goto fail;
	    }
	}

	/* Add other AVP's required by OCRQ */
	if (!out)
		goto send_msg;
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_MINIMUM_BPS) == -1) {
		value32 = htonl(0);
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_MINIMUM_BPS, &value32, sizeof(value32)) == -1)
			goto fail;
	}
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_MAXIMUM_BPS) == -1) {
		value32 = htonl(0x7fffffff);
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_MAXIMUM_BPS, &value32, sizeof(value32)) == -1)
			goto fail;
	}
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_BEARER_TYPE) == -1) {
		value32 = (ctrl->peer_bearer
		    & (L2TP_BEARER_DIGITAL|L2TP_BEARER_ANALOG));
		value32 = htonl(value32);
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_BEARER_TYPE, &value32, sizeof(value32)) == -1)
			goto fail;
	}
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_FRAMING_TYPE) == -1) {
		value32 = (ctrl->peer_framing
		    & (L2TP_FRAMING_SYNC|L2TP_FRAMING_ASYNC));
		value32 = htonl(value32);
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_FRAMING_TYPE, &value32, sizeof(value32)) == -1)
			goto fail;
	}
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_CALLED_NUMBER) == -1) {
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_CALLED_NUMBER, NULL, 0) == -1)
			goto fail;
	}

send_msg:
	/* Send request to peer */
	ppp_l2tp_ctrl_send(ctrl, 0, out ? OCRQ : ICRQ, sess->my_avps);

	/* Await reply from peer */
	ppp_l2tp_sess_check_reply(sess);
	return (sess);

fail:
	/* Clean up after failure */
	ppp_l2tp_sess_destroy(&sess);
	return (NULL);
}

/*
 * Report successful connection of a remotely initiated outgoing call
 * or a locally initiated incoming call.
 */
int
ppp_l2tp_connected(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;
	u_int32_t value32;
	int i;

	/* Debugging */
	Log(LOG_DEBUG, ("L2TP: %s invoked, sess=%p", __FUNCTION__, sess));

	/* Check control connection */
	if (ctrl->state != CS_ESTABLISHED) {
		Log(LOG_ERR, ("L2TP: inappropriate call to %s", __FUNCTION__));
		errno = ENXIO;
		return (-1);
	}

	/* Check session state */
	if (sess->side != SIDE_LAC
	    || !((sess->orig == ORIG_REMOTE && sess->state == SS_WAIT_ANSWER)
	      || (sess->orig == ORIG_LOCAL && sess->state == SS_WAIT_REPLY))) {
		Log(LOG_ERR, ("L2TP: inappropriate call to %s", __FUNCTION__));
		errno = ENOTTY;
		return (-1);
	}

	/* Copy AVP's supplied by caller */
	while (sess->my_avps->length > 0)
		ppp_l2tp_avp_list_remove(sess->my_avps, 0);
	if (avps != NULL) {
		for (i = 0; i < avps->length; i++) {
			struct ppp_l2tp_avp *const avp = &avps->avps[i];

			if (ppp_l2tp_avp_list_append(sess->my_avps,
			    avp->mandatory, avp->vendor, avp->type,
			    avp->value, avp->vlen) == -1)
				goto fail;
		}
	}

	/* Add other AVP's required by ICCN and OCCN */
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_TX_CONNECT_SPEED) == -1) {
		value32 = htonl(L2TP_CONNECT_SPEED);
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_TX_CONNECT_SPEED, &value32, sizeof(value32)) == -1)
			goto fail;
	}
	if (ppp_l2tp_avp_list_find(sess->my_avps,
	    0, AVP_FRAMING_TYPE) == -1) {
		value32 = htonl(L2TP_FRAMING_TYPE);
		if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		    0, AVP_FRAMING_TYPE, &value32, sizeof(value32)) == -1)
			goto fail;
	}

	/* Handle incoming or outgoing call */
	if (sess->orig == ORIG_LOCAL) {
		sess->link_responded = 1;
		if (ppp_l2tp_sess_check_liic(sess) == -1)
			goto fail;
	} else {
		if (ppp_l2tp_sess_setup(sess) == -1)
			goto fail;
		ppp_l2tp_ctrl_send(ctrl, sess->peer_id, OCCN, sess->my_avps);
	}

	/* Done */
	return (0);

fail:
	/* Clean up after failure */
	ppp_l2tp_sess_close(sess, L2TP_RESULT_ERROR,
	    L2TP_ERROR_GENERIC, strerror(errno));
	return (-1);
}

/*
 * Terminate a session.
 */
void
ppp_l2tp_terminate(struct ppp_l2tp_sess *sess,
	u_int16_t result, u_int16_t error, const char *errmsg)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	/* Debugging */
	Log(LOG_DEBUG, ("L2TP: %s invoked, sess=%p errmsg=\"%s\"",
	    __FUNCTION__, sess, errmsg != NULL ? errmsg : ""));

	/* Check control connection state */
	if (ctrl->state == CS_DYING) {
		sess->link_notified = 1;
		return;
	}
	if (ctrl->state != CS_ESTABLISHED) {
		Log(LOG_ERR, ("L2TP: inappropriate call to %s", __FUNCTION__));
		return;
	}

	/* Close connection */
	sess->link_notified = 1;
	ppp_l2tp_sess_close(sess, result, error, errmsg);
}

/*
 * Send SLI to peer.
 */
int
ppp_l2tp_set_link_info(struct ppp_l2tp_sess *sess,
			u_int32_t xmit, u_int32_t recv)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;
	struct ppp_l2tp_avp_list *avps;
	uint16_t accm[5];

	/* Only LNS should send SLI */
	if (sess->side != SIDE_LNS)
		return (0);

	avps = ppp_l2tp_avp_list_create();

	accm[0] = 0;
	accm[1] = htons(xmit >> 16);
	accm[2] = htons(xmit & 0xFFFF);
	accm[3] = htons(recv >> 16);
	accm[4] = htons(recv & 0xFFFF);
	if (ppp_l2tp_avp_list_append(avps, 0,
	    0, AVP_ACCM, &accm, sizeof(accm)) == -1) {
		ppp_l2tp_avp_list_destroy(&avps);
		return (-1);
	}

	ppp_l2tp_ctrl_send(ctrl, sess->peer_id, SLI, avps);
	ppp_l2tp_avp_list_destroy(&avps);
	return (0);
}

/*
 * Get the link side cookie corresponding to a control connection.
 */
void *
ppp_l2tp_ctrl_get_cookie(struct ppp_l2tp_ctrl *ctrl)
{
	return (ctrl->link_cookie);
}

/*
 * Set the link side cookie corresponding to a control connection.
 */
void
ppp_l2tp_ctrl_set_cookie(struct ppp_l2tp_ctrl *ctrl, void *cookie)
{
	ctrl->link_cookie = cookie;
}

/*
 * Get the link side cookie corresponding to a session.
 */
void *
ppp_l2tp_sess_get_cookie(struct ppp_l2tp_sess *sess)
{
	return (sess->link_cookie);
}

/*
 * Set the link side cookie corresponding to a session.
 */
void
ppp_l2tp_sess_set_cookie(struct ppp_l2tp_sess *sess, void *cookie)
{
	sess->link_cookie = cookie;
}

/*
 * Get session's Call Serial Number.
 */
uint32_t
ppp_l2tp_sess_get_serial(struct ppp_l2tp_sess *sess)
{
	return(sess->serial);
}

int
ppp_l2tp_ctrl_get_self_name(struct ppp_l2tp_ctrl *ctrl,
			void *buf, size_t buf_len) {
	strlcpy(buf, ctrl->self_name, buf_len);
	return (0);
};

int
ppp_l2tp_ctrl_get_peer_name(struct ppp_l2tp_ctrl *ctrl,
			void *buf, size_t buf_len) {
	strlcpy(buf, ctrl->peer_name, buf_len);
	return (0);
};

/*
 * Get the node path and hook name for the hook that corresponds
 * to a control connection's L2TP frames.
 */
void
ppp_l2tp_ctrl_get_hook(struct ppp_l2tp_ctrl *ctrl,
	ng_ID_t *nodep, const char **hookp)
{
	if (nodep != NULL)
		*nodep = ctrl->node_id;
	if (hookp != NULL)
		*hookp = NG_L2TP_HOOK_LOWER;
}

/*
 * Get the node path and hook name for the hook that corresponds
 * to a session's data packets.
 */
void
ppp_l2tp_sess_get_hook(struct ppp_l2tp_sess *sess,
	ng_ID_t *nodep, const char **hookp)
{
	if (nodep != NULL) {
	    if (sess->node_id)
		*nodep = sess->node_id;
	    else
		*nodep = sess->ctrl->node_id;
	}
	if (hookp != NULL) {
	    if (sess->node_id)
		*hookp = NG_TEE_HOOK_RIGHT;
	    else
		*hookp = sess->hook;
	}
}

/*
 * Informs that hook has been connected and temporal tee can be shutted down.
 */
void
ppp_l2tp_sess_hooked(struct ppp_l2tp_sess *sess) {
	char path[32];

	snprintf(path, sizeof(path), "[%lx]:", (u_long)sess->node_id);
	(void)NgSendMsg(sess->ctrl->csock, path,
	    NGM_GENERIC_COOKIE, NGM_SHUTDOWN, NULL, 0);
	sess->node_id = 0;
}

/************************************************************************
		INTERNAL FUNCTIONS: CONTROL CONNECTION
************************************************************************/

/*
 * Extract peer information from AVP's contained in a SCCRQ or SCCRP.
 * This is the first part of tunnel setup.
 */
static int
ppp_l2tp_ctrl_setup_1(struct ppp_l2tp_ctrl *ctrl,
	struct ppp_l2tp_avp_ptrs *ptrs)
{
	/* Log */
	Log(LOG_INFO, ("L2TP: connected to \"%s\", version=%u.%u",
	    ptrs->hostname->hostname, ptrs->protocol->version,
	    ptrs->protocol->revision));

	/* Get peer's tunnel ID */
	ctrl->config.peer_id = ptrs->tunnelid->id;

	/* Get peer's receive window size */
	ctrl->config.peer_win = L2TP_DEFAULT_PEER_WIN;
	if (ptrs->winsize != NULL) {
		if (ptrs->winsize->size == 0)
			Log(LOG_WARNING, ("L2TP: ignoring zero recv window size AVP"));
		else
			ctrl->config.peer_win = ptrs->winsize->size;
	}

	/* Get peer's bearer and framing types */
	if (ptrs->bearercap != NULL) {
		if (ptrs->bearercap->digital)
			ctrl->peer_bearer |= L2TP_BEARER_DIGITAL;
		if (ptrs->bearercap->analog)
			ctrl->peer_bearer |= L2TP_BEARER_ANALOG;
	}
	if (ptrs->framingcap->sync)
		ctrl->peer_framing |= L2TP_FRAMING_SYNC;
	if (ptrs->framingcap->async)
		ctrl->peer_framing |= L2TP_FRAMING_ASYNC;

	strlcpy(ctrl->peer_name, ptrs->hostname->hostname, sizeof(ctrl->peer_name));

	/* Update netgraph node configuration */
	if (NgSendMsg(ctrl->csock, NG_L2TP_HOOK_CTRL, NGM_L2TP_COOKIE,
	    NGM_L2TP_SET_CONFIG, &ctrl->config, sizeof(ctrl->config)) == -1)
		return (-1);

	/* See if there is a challenge AVP */
	if (ptrs->challenge != NULL) {
                MD5_CTX md5ctx;
		u_char hash[MD5_DIGEST_LENGTH];
		uint8_t t;

		/* Make sure response was included */
		if (ctrl->secret == NULL) {
			Log(LOG_WARNING, ("L2TP: tunnel challenge received but"
			    " no shared secret is configured"));
			ppp_l2tp_ctrl_close(ctrl,
			    L2TP_RESULT_NOT_AUTH, 0, NULL);
			return (-1);
		}

		/* Calculate challenge response */
		t = (ptrs->message->mesgtype == SCCRQ)?SCCRP:SCCCN;
		MD5_Init(&md5ctx);
		MD5_Update(&md5ctx, &t, 1);
		MD5_Update(&md5ctx, ctrl->secret, ctrl->seclen);
		MD5_Update(&md5ctx, &ptrs->challenge->value, ptrs->challenge->length);
		MD5_Final(hash, &md5ctx);

		if (ppp_l2tp_avp_list_append(ctrl->avps, 0,
		    0, AVP_CHALLENGE_RESPONSE, hash, sizeof(hash)) == -1)
		return (0);
	}

	/* Done */
	return (0);
}

/*
 * Extract peer information from AVP's contained in a SCCRP or SCCCN.
 * This is the second part of tunnel setup.
 */
static int
ppp_l2tp_ctrl_setup_2(struct ppp_l2tp_ctrl *ctrl,
	struct ppp_l2tp_avp_ptrs *ptrs)
{
	/* Peer now knows our tunnel ID */
	ctrl->config.match_id = 1;
	if (NgSendMsg(ctrl->csock, NG_L2TP_HOOK_CTRL, NGM_L2TP_COOKIE,
	    NGM_L2TP_SET_CONFIG, &ctrl->config, sizeof(ctrl->config)) == -1)
		return (-1);

	/* Verify peer's challenge response */
	if (ctrl->secret != NULL) {
                MD5_CTX md5ctx;
		u_char hash[MD5_DIGEST_LENGTH];
		uint8_t t;

		/* Make sure response was included */
		if (ptrs->challengresp == NULL) {
			Log(LOG_WARNING, ("L2TP: SCCRP lacks challenge response"));
			ppp_l2tp_ctrl_close(ctrl,
			    L2TP_RESULT_NOT_AUTH, 0, NULL);
			return (0);
		}

		/* Calculate challenge response */
		t = ptrs->message->mesgtype;
		MD5_Init(&md5ctx);
		MD5_Update(&md5ctx, &t, 1);
		MD5_Update(&md5ctx, ctrl->secret, ctrl->seclen);
		MD5_Update(&md5ctx, ctrl->chal, sizeof(ctrl->chal));
		MD5_Final(hash, &md5ctx);
		
		/* Check challenge response */
		if (bcmp(hash, &ptrs->challengresp->value, sizeof(hash))) {
		    Log(LOG_WARNING, ("L2TP: tunnel challenge/response incorrect"));
		    ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_NOT_AUTH, 0, NULL);
		    return (-1);
		}
	}

	if (pevent_register(ctrl->ctx, &ctrl->death_timer, 0,
	    ctrl->mutex, ppp_l2tp_unused_timeout, ctrl,
	    PEVENT_TIME, gL2TPto * 1000) == -1) {
		Perror("L2TP: error starting unused timer");
	}

	/* Done */
	return (0);
}

/*
 * Create a new session.
 */
static struct ppp_l2tp_sess *
ppp_l2tp_sess_create(struct ppp_l2tp_ctrl *ctrl,
	enum l2tp_sess_orig orig, enum l2tp_sess_side side, u_int32_t serial)
{
	struct ppp_l2tp_sess *sess = NULL;
	u_int16_t value16;
	u_int32_t value32;

	/* Create new session object */
	sess = Malloc(SESS_MEM_TYPE, sizeof(*sess));
	sess->ctrl = ctrl;
	sess->orig = orig;
	sess->side = side;
	sess->serial = serial;
	sess->state = (orig == ORIG_LOCAL) ? SS_WAIT_REPLY :
	    (side == SIDE_LNS) ? SS_WAIT_CONNECT : SS_WAIT_ANSWER;

	/* Get unique session ID */
	while (sess->config.session_id == 0
	    || ghash_get(ctrl->sessions, sess) != NULL)
		sess->config.session_id = random() & 0xffff;
	snprintf(sess->hook, sizeof(sess->hook),
	    NG_L2TP_HOOK_SESSION_F, sess->config.session_id);

	pevent_unregister(&ctrl->death_timer);
	ctrl->active_sessions++;

	/* Add to control connection hash table */
	if (ghash_put(ctrl->sessions, sess) == -1) {
		ppp_l2tp_sess_destroy(&sess);
		return (NULL);
	}

	/* Create session AVP list to send to peer */
	sess->my_avps = ppp_l2tp_avp_list_create();

	/* Add assigned session ID AVP */
	value16 = htons(sess->config.session_id);
	if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
	    0, AVP_ASSIGNED_SESSION_ID, &value16, sizeof(value16)) == -1) {
		ppp_l2tp_sess_destroy(&sess);
		return (NULL);
	}

	if (orig == ORIG_LOCAL) {
	    /* Add call serial number AVP */
	    value32 = htonl(sess->serial);
	    if (ppp_l2tp_avp_list_append(sess->my_avps, 1,
		0, AVP_CALL_SERIAL_NUMBER, &value32, sizeof(value32)) == -1) {
		    ppp_l2tp_sess_destroy(&sess);
		    return (NULL);
	    }
	}

	/* Done */
	Log(LOG_DEBUG, ("L2TP: created new session #%u id 0x%04x orig=%s side=%s"
	    " state=%s", sess->serial, sess->config.session_id,
	    ppp_l2tp_sess_orig_str(sess->orig),
	    ppp_l2tp_sess_side_str(sess->side),
	    ppp_l2tp_sess_state_str(sess->state)));
	return (sess);
}

/*
 * Send a control message.
 */
static void
ppp_l2tp_ctrl_send(struct ppp_l2tp_ctrl *ctrl, u_int16_t session_id,
	enum l2tp_msg_type msgtype, const struct ppp_l2tp_avp_list *avps0)
{
	struct ppp_l2tp_avp_list *avps = NULL;
	struct ppp_l2tp_avp *avp = NULL;
	u_char *data = NULL;
	u_int16_t value;
	int index;
	int len;

	/* Copy AVP list */
	avps = (avps0 == NULL) ? ppp_l2tp_avp_list_create()
	    : ppp_l2tp_avp_list_copy(avps0);
	if (avps == NULL)
		goto fail;

	/* Remove any message type AVP in the way */
	if ((index = ppp_l2tp_avp_list_find(avps, 0, AVP_MESSAGE_TYPE)) != -1)
		ppp_l2tp_avp_list_remove(avps, index);

	/* Add message type AVP as first in the list */
	value = htons(msgtype);
	avp = ppp_l2tp_avp_create(1, 0, AVP_MESSAGE_TYPE, &value, sizeof(value));
	if (ppp_l2tp_avp_list_insert(avps, &avp, 0) == -1)
		goto fail;

	/* Encoded AVP's into a packet */
	if ((len = ppp_l2tp_avp_pack(ppp_l2tp_avp_info_list,
	    avps, (ctrl->hide_avps?ctrl->secret:NULL), ctrl->seclen, NULL)) == -1)
		goto fail;
	data = Malloc(TYPED_MEM_TEMP, 2 + len);
	session_id = htons(session_id);
	memcpy(data, &session_id, 2);
	(void)ppp_l2tp_avp_pack(ppp_l2tp_avp_info_list,
	    avps, (ctrl->hide_avps?ctrl->secret:NULL), ctrl->seclen, data + 2);

	/* Write packet */
	if (session_id == 0)
		ppp_l2tp_ctrl_dump(ctrl, avps, "L2TP: XMIT ");
	else {
		ppp_l2tp_ctrl_dump(ctrl, avps, "L2TP: XMIT(0x%04x) ",
		    ntohs(session_id));
	}
	if (NgSendData(ctrl->dsock, NG_L2TP_HOOK_CTRL, data, 2 + len) == -1)
		goto fail;

	/* Done */
	goto done;

fail:
	/* Close up shop */
	Perror("L2TP: error sending ctrl packet");
	ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
	    L2TP_ERROR_GENERIC, strerror(errno));

done:
	/* Clean up */
	ppp_l2tp_avp_destroy(&avp);
	ppp_l2tp_avp_list_destroy(&avps);
	Freee(data);
}

/*
 * Close a control connection gracefully, after the next context switch.
 */
static void
ppp_l2tp_ctrl_close(struct ppp_l2tp_ctrl *ctrl,
	u_int16_t result, u_int16_t error, const char *errmsg)
{
	/* Sanity check */
	if (ctrl->state == CS_DYING)
		return;
	ctrl->state = CS_DYING;

	/* Save result code and error string */
	ctrl->result = result;
	ctrl->error = error;
	Freee(ctrl->errmsg);
	ctrl->errmsg = (errmsg == NULL) ? NULL : Mstrdup(CTRL_MEM_TYPE, errmsg);

	/* Notify peer if necessary */
	if (!ctrl->peer_notified) {
		struct ppp_l2tp_avp_list *avps = NULL;
		const size_t elen = (ctrl->errmsg == NULL) ?
		    0 : strlen(ctrl->errmsg);
		struct ppp_l2tp_sess *sess;
		struct ghash_walk walk;
		u_char *rbuf = NULL;
		u_int16_t value16;

		/* Create AVP list */
		ctrl->peer_notified = 1;
		avps = ppp_l2tp_avp_list_create();

		/* Add assigned tunnel ID AVP */
		value16 = htons(ctrl->config.tunnel_id);
		if (ppp_l2tp_avp_list_append(avps, 1, 0,
		    AVP_ASSIGNED_TUNNEL_ID, &value16, sizeof(value16)) == -1) {
			Perror("L2TP: ppp_l2tp_avp_list_append");
			goto notify_done;
		}

		/* Add result code AVP */
		rbuf = Malloc(TYPED_MEM_TEMP, 4 + elen);
		value16 = htons(ctrl->result);
		memcpy(rbuf, &value16, sizeof(value16));
		value16 = htons(ctrl->error);
		memcpy(rbuf + 2, &value16, sizeof(value16));
		memcpy(rbuf + 4, ctrl->errmsg, elen);
		if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_RESULT_CODE,
		    rbuf, 4 + elen) == -1) {
			Perror("L2TP: ppp_l2tp_avp_list_append");
			goto notify_done;
		}

		/* Send StopCCN */
		ppp_l2tp_ctrl_send(ctrl, 0, StopCCN, avps);

		/* StopCCN implies closing all sessions */
		ghash_walk_init(ctrl->sessions, &walk);
		while ((sess = ghash_walk_next(ctrl->sessions, &walk)) != NULL)
			sess->peer_notified = 1;

notify_done:
		/* Clean up */
		ppp_l2tp_avp_list_destroy(&avps);
		Freee(rbuf);
	}

	/* Stop all timers */
	pevent_unregister(&ctrl->idle_timer);
	pevent_unregister(&ctrl->reply_timer);
	pevent_unregister(&ctrl->close_timer);
	pevent_unregister(&ctrl->death_timer);

	/* Start timer to call ppp_l2tp_ctrl_do_close() */
	if (pevent_register(ctrl->ctx, &ctrl->close_timer, 0, ctrl->mutex,
	    ppp_l2tp_ctrl_do_close, ctrl, PEVENT_TIME, 0) == -1)
		Perror("L2TP: error starting close timer");
}

/*
 * Close a control connection gracefully.
 *
 * We call this in a separate event thread to avoid reentrancy problems.
 */
static void
ppp_l2tp_ctrl_do_close(void *arg)
{
	struct ppp_l2tp_ctrl *ctrl = arg;
	struct ppp_l2tp_sess *sess;
	struct ghash_walk walk;

	/* Remove event */
	pevent_unregister(&ctrl->close_timer);

	/* Notify link side about all sessions first */
	ghash_walk_init(ctrl->sessions, &walk);
	while ((sess = ghash_walk_next(ctrl->sessions, &walk)) != NULL) {
		if (sess->link_notified)
			continue;
		sess->link_notified = 1;
		sess->result = L2TP_RESULT_ERROR;
		sess->error = L2TP_ERROR_GENERIC;
		Freee(sess->errmsg);
		sess->errmsg = Mdup(SESS_MEM_TYPE,
		    "control connection closing", strlen("control connection closing") + 1);
		(*ctrl->cb->terminated)(sess,
		    sess->result, sess->error, sess->errmsg);
	}

	/* Now notify link side about control connection */
	if (!ctrl->link_notified) {
		ctrl->link_notified = 1;
		(*ctrl->cb->ctrl_terminated)(ctrl, ctrl->result, ctrl->error,
		    (ctrl->errmsg != NULL) ? ctrl->errmsg : "");
	}

	/* If no active sessions exist, start dying */
	if (ctrl->active_sessions == 0) {
		ppp_l2tp_ctrl_death_start(ctrl);
		return;
	}

	/* Close all active sessions */
	ghash_walk_init(ctrl->sessions, &walk);
	while ((sess = ghash_walk_next(ctrl->sessions, &walk)) != NULL) {
		sess->peer_notified = 1;	/* no need to notify peer */
		ppp_l2tp_sess_close(sess, L2TP_RESULT_ERROR,
		    L2TP_ERROR_GENERIC, "control connection closing");
	}
}

/*
 * Notify link side that the control connection has gone away
 * and begin death timer.
 *
 * We rig things so that all the session death notifications happen
 * before the control connection notification, which happens here.
 */
static void
ppp_l2tp_ctrl_death_start(struct ppp_l2tp_ctrl *ctrl)
{
	/* Sanity */
	assert(ctrl->state == CS_DYING);

	/* Stop timers */
	pevent_unregister(&ctrl->idle_timer);
	pevent_unregister(&ctrl->reply_timer);
	pevent_unregister(&ctrl->death_timer);
	/* close_timer must not be touched! */

	/* Linger for a while before going away */
	if (pevent_register(ctrl->ctx, &ctrl->death_timer, 0,
	    ctrl->mutex, ppp_l2tp_ctrl_death_timeout, ctrl,
	    PEVENT_TIME, L2TP_CTRL_DEATH_TIMEOUT * 1000) == -1)
		Perror("L2TP: error starting death timer");
}

/*
 * Handle idle timeout on control connection.
 */
static void
ppp_l2tp_idle_timeout(void *arg)
{
	struct ppp_l2tp_ctrl *const ctrl = arg;

	/* Remove event */
	pevent_unregister(&ctrl->idle_timer);

	/* Restart idle timer */
	if (pevent_register(ctrl->ctx, &ctrl->idle_timer, 0,
	    ctrl->mutex, ppp_l2tp_idle_timeout, ctrl, PEVENT_TIME,
	    L2TP_IDLE_TIMEOUT * 1000) == -1)
		Perror("L2TP: error restarting idle timer");

	/* Send a 'hello' packet */
	ppp_l2tp_ctrl_send(ctrl, 0, HELLO, NULL);
}

/*
 * Handle unused timeout on control connection.
 */
static void
ppp_l2tp_unused_timeout(void *arg)
{
	struct ppp_l2tp_ctrl *const ctrl = arg;

	assert(ctrl->active_sessions == 0);
	assert(ctrl->state != CS_DYING);

	ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_CLEARED,
	    0, "no more sessions exist in this tunnel");
}

/*
 * Remove a control connection that has been dead for a while.
 */
static void
ppp_l2tp_ctrl_death_timeout(void *arg)
{
	struct ppp_l2tp_ctrl *ctrl = arg;

	pevent_unregister(&ctrl->death_timer);
	if (*ctrl->cb->ctrl_destroyed != NULL)
	    (*ctrl->cb->ctrl_destroyed)(ctrl);
	ppp_l2tp_ctrl_destroy(&ctrl);
}

/************************************************************************
			INTERNAL FUNCTIONS: SESSIONS
************************************************************************/

/*
 * This function handles the situation of a locally initiated incoming
 * call, i.e., we are the LAC. Before sending the ICCN to the LNS, two
 * events must happen: the LNS must reply with a ICRP, and the link
 * side must invoke the ppp_l2tp_connected() function.
 */
static int
ppp_l2tp_sess_check_liic(struct ppp_l2tp_sess *sess)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	/* Are we ready to send ICCN yet? */
	if (!sess->link_responded || !sess->peer_responded)
		return (0);

	/* Set up session */
	if (ppp_l2tp_sess_setup(sess) == -1)
		return (-1);

	/* Now send ICCN to peer */
	ppp_l2tp_ctrl_send(ctrl, sess->peer_id, ICCN, sess->my_avps);

	/* Done */
	return (0);
}

/*
 * Set up a session that has reached the established state.
 */
static int
ppp_l2tp_sess_setup(struct ppp_l2tp_sess *sess)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;
	struct ngm_mkpeer mkpeer;
	char path[64];

	/* If link side is waiting for confirmation, schedule it */
	if (sess->side == SIDE_LNS || sess->orig == ORIG_LOCAL) {
		pevent_unregister(&sess->notify_timer);
		if (pevent_register(ctrl->ctx, &sess->notify_timer, 0,
		    ctrl->mutex, ppp_l2tp_sess_notify, sess, PEVENT_TIME, 0) == -1) {
			Perror("L2TP: error starting notify timer");
			goto fail;
		}
	}

	/* Attach a 'tee' node so l2tp node never sees hook disconnect */
	memset(&mkpeer, 0, sizeof(mkpeer));
	strlcpy(mkpeer.type, NG_TEE_NODE_TYPE, sizeof(mkpeer.type));
	strlcpy(mkpeer.ourhook, sess->hook, sizeof(mkpeer.ourhook));
	strlcpy(mkpeer.peerhook, NG_TEE_HOOK_LEFT, sizeof(mkpeer.peerhook));
	if (NgSendMsg(ctrl->csock, NG_L2TP_HOOK_CTRL, NGM_GENERIC_COOKIE,
	    NGM_MKPEER, &mkpeer, sizeof(mkpeer)) == -1) {
		Perror("L2TP: mkpeer");
		goto fail;
	}

	/* Get ng_tee node ID */
	snprintf(path, sizeof(path), "%s.%s", NG_L2TP_HOOK_CTRL, sess->hook);
	if ((sess->node_id = NgGetNodeID(ctrl->csock, path)) == 0) {
	    Perror("L2TP: Cannot get %s node id", NG_TEE_NODE_TYPE);
	    goto fail;
	};

	/* Configure session hook */
	if (sess->dseq_required) {
	    sess->config.control_dseq = sess->dseq_required;
	    sess->config.enable_dseq = 1;
	} else {
	    sess->config.control_dseq = (sess->side == SIDE_LNS)?1:0;
	    sess->config.enable_dseq = sess->enable_dseq;
	}
	sess->config.peer_id = sess->peer_id;
	sess->config.include_length = sess->include_length;
	if (NgSendMsg(ctrl->csock, NG_L2TP_HOOK_CTRL, NGM_L2TP_COOKIE,
	    NGM_L2TP_SET_SESS_CONFIG, &sess->config,
	    sizeof(sess->config)) == -1) {
		Perror("L2TP: error configuring session hook");
		goto fail;
	}

	/* Call is now established */
	sess->state = SS_ESTABLISHED;
	return (0);

fail:
	/* Clean up after failure */
	pevent_unregister(&sess->notify_timer);
	return (-1);
}

/*
 * Close a session gracefully, after the next context switch.
 */
static void
ppp_l2tp_sess_close(struct ppp_l2tp_sess *sess,
	u_int16_t result, u_int16_t error, const char *errmsg)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	/* Sanity check */
	if (sess->state == SS_DYING)
		return;
	sess->state = SS_DYING;

	/* Save result code and error string */
	sess->result = result;
	sess->error = error;
	Freee(sess->errmsg);
	sess->errmsg = (errmsg == NULL) ? NULL : Mstrdup(SESS_MEM_TYPE, errmsg);

	/* Notify peer if necessary */
	if (!sess->peer_notified) {
		struct ppp_l2tp_avp_list *avps = NULL;
		const size_t elen = (sess->errmsg == NULL) ?
		    0 : strlen(sess->errmsg);
		u_char *rbuf = NULL;
		u_int16_t value16;

		/* Create AVP list */
		sess->peer_notified = 1;
		avps = ppp_l2tp_avp_list_create();

		/* Add assigned session ID AVP */
		value16 = htons(sess->config.session_id);
		if (ppp_l2tp_avp_list_append(avps, 1, 0,
		    AVP_ASSIGNED_SESSION_ID, &value16, sizeof(value16)) == -1) {
			Perror("L2TP: ppp_l2tp_avp_list_append");
			goto notify_done;
		}

		/* Add result code AVP */
		rbuf = Malloc(TYPED_MEM_TEMP, 4 + elen);
		value16 = htons(sess->result);
		memcpy(rbuf, &value16, sizeof(value16));
		value16 = htons(sess->error);
		memcpy(rbuf + 2, &value16, sizeof(value16));
		memcpy(rbuf + 4, sess->errmsg, elen);
		if (ppp_l2tp_avp_list_append(avps, 1, 0, AVP_RESULT_CODE,
		    rbuf, 4 + elen) == -1) {
			Perror("L2TP: ppp_l2tp_avp_list_append");
			goto notify_done;
		}

		/* Send CDN */
		ppp_l2tp_ctrl_send(ctrl, sess->peer_id, CDN, avps);

notify_done:
		/* Clean up */
		ppp_l2tp_avp_list_destroy(&avps);
		Freee(rbuf);
	}

	/* Stop all session timers */
	pevent_unregister(&sess->notify_timer);
	pevent_unregister(&sess->reply_timer);
	pevent_unregister(&sess->death_timer);
	pevent_unregister(&sess->close_timer);

	/* Start timer to call ppp_l2tp_sess_do_close() */
	if (pevent_register(ctrl->ctx, &sess->close_timer, 0,
	    ctrl->mutex, ppp_l2tp_sess_do_close, sess, PEVENT_TIME, 0) == -1)
		Perror("L2TP: error starting close timer");
}

/*
 * Close a session gracefully.
 *
 * We call this in a separate event thread to avoid reentrancy problems.
 */
static void
ppp_l2tp_sess_do_close(void *arg)
{
	struct ppp_l2tp_sess *const sess = arg;
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	/* Remove event */
	pevent_unregister(&sess->close_timer);

	ctrl->active_sessions--;

	/* Linger for a while before going away */
	if (pevent_register(ctrl->ctx, &sess->death_timer, 0,
	    ctrl->mutex, ppp_l2tp_sess_death_timeout, sess, PEVENT_TIME,
	    L2TP_SESS_DEATH_TIMEOUT * 1000) == -1)
		Perror("L2TP: error starting death timer");

	/* Notify link side about session if necessary */
	if (!sess->link_notified) {
		sess->link_notified = 1;
		(*ctrl->cb->terminated)(sess, sess->result, sess->error,
		    (sess->errmsg != NULL) ? sess->errmsg : "");
	}

	/* Close control connection after last session closes */
	if (ctrl->active_sessions == 0) {
		if (ctrl->state == CS_DYING) {
			ppp_l2tp_ctrl_death_start(ctrl);
		} else if (pevent_register(ctrl->ctx, &ctrl->death_timer, 0,
		    ctrl->mutex, ppp_l2tp_unused_timeout, ctrl,
		    PEVENT_TIME, gL2TPto * 1000) == -1) {
			Perror("L2TP: error starting unused timer");
		}
	}
}

/*
 * Notify link side that a session is connected.
 *
 * We call this in a separate event thread to avoid reentrancy problems.
 */
static void
ppp_l2tp_sess_notify(void *arg)
{
	struct ppp_l2tp_sess *const sess = arg;
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	pevent_unregister(&sess->notify_timer);
	(*ctrl->cb->connected)(sess, sess->peer_avps);
}

/*
 * Remove a session that has been dead for a while.
 */
static void
ppp_l2tp_sess_death_timeout(void *arg)
{
	struct ppp_l2tp_sess *sess = arg;

	pevent_unregister(&sess->death_timer);
	ppp_l2tp_sess_destroy(&sess);
}

/************************************************************************
			NETGRAPH SOCKET READERS
************************************************************************/

/*
 * Read from netgraph data socket. This is where incoming L2TP
 * control connection messages appear.
 */
static void
ppp_l2tp_data_event(void *arg)
{
	struct ppp_l2tp_ctrl *const ctrl = arg;
	const struct l2tp_msg_info *msg_info;
	struct ppp_l2tp_avp_list *avps = NULL;
	struct ppp_l2tp_avp_ptrs *ptrs = NULL;
	struct ppp_l2tp_sess *sess;
	struct ppp_l2tp_sess key;
	static u_char buf[4096];
	u_int16_t msgtype;
	char ebuf[64];
	int len;
	int i;
	int j;

	/* Restart idle timer */
	pevent_unregister(&ctrl->idle_timer);
	if (pevent_register(ctrl->ctx, &ctrl->idle_timer, 0,
	    ctrl->mutex, ppp_l2tp_idle_timeout, ctrl, PEVENT_TIME,
	    L2TP_IDLE_TIMEOUT * 1000) == -1) {
		Perror("L2TP: error restarting idle timer");
		goto fail_errno;
	}

	/* Read packet */
	if ((len = read(ctrl->dsock, buf, sizeof(buf))) == -1) {
		Perror("L2TP: error reading ctrl hook");
		goto fail_errno;
	}

	/* Extract session ID */
	memcpy(&key.config.session_id, buf, 2);
	key.config.session_id = ntohs(key.config.session_id);

	/* Parse out AVP's */
	if ((avps = ppp_l2tp_avp_unpack(ppp_l2tp_avp_info_list,
	    buf + 2, len - 2, ctrl->secret, ctrl->seclen)) == NULL) {
		switch (errno) {
		case EILSEQ:
			Log(LOG_ERR,
			    ("L2TP: rec'd improperly formatted control message"));
			goto fail_invalid;
		case EAUTH:
			Log(LOG_ERR,
			    ("L2TP: rec'd hidden AVP, but no shared secret"));
			ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
			    L2TP_ERROR_GENERIC, "hidden AVP found"
			    " but no shared secret configured");
			goto done;
		case ENOSYS:
			Log(LOG_ERR, ("L2TP: rec'd mandatory but unknown AVP"));
			ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
			    L2TP_ERROR_MANDATORY, NULL);
			goto done;
		default:
			Perror("L2TP: error decoding control message");
			goto fail_errno;
		}
	}

	/* Debugging */
	if (key.config.session_id == 0)
		ppp_l2tp_ctrl_dump(ctrl, avps, "L2TP: RECV ");
	else {
		ppp_l2tp_ctrl_dump(ctrl, avps, "L2TP: RECV(0x%04x) ",
		    ntohs(key.config.session_id));
	}

	/* Message type AVP must be present and first */
	if (avps->length == 0 || avps->avps[0].type != AVP_MESSAGE_TYPE) {
		Log(LOG_WARNING,
		    ("L2TP: rec'd ctrl message lacking message type AVP"));
		goto fail_invalid;
	}

	/* Get message type from message type AVP */
	memcpy(&msgtype, avps->avps[0].value, 2);
	msgtype = ntohs(msgtype);

	/* Find descriptor for this message type */
	for (i = 0; ppp_l2tp_msg_info[i].name != NULL
	    && msgtype != ppp_l2tp_msg_info[i].type; i++);
	if ((msg_info = &ppp_l2tp_msg_info[i])->name == NULL) {
		if (avps->avps[0].mandatory) {
			snprintf(ebuf, sizeof(ebuf), "rec'd unsupported"
			    " but mandatory message type %u", msgtype);
			Log(LOG_WARNING, ("L2TP: %s", ebuf));
			ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
			    L2TP_ERROR_BAD_VALUE, ebuf);
			goto done;
		}
		Log(LOG_NOTICE,
		    ("L2TP: rec'd unknown message type %u; ignoring", msgtype));
		goto done;				/* just ignore it */
	}

	/* Check for missing required AVP's */
	for (i = 0; msg_info->req_avps[i] != -1; i++) {
		for (j = 0; j < avps->length
		    && avps->avps[j].type != msg_info->req_avps[i]; j++);
		if (j == avps->length) {
			snprintf(ebuf, sizeof(ebuf), "rec'd %s control"
			    " message lacking required AVP #%u",
			    msg_info->name, msg_info->req_avps[i]);
			ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
			    L2TP_ERROR_BAD_VALUE, ebuf);
			goto done;
		}
	}

	/* Convert AVP's to friendly form */
	if ((ptrs = ppp_l2tp_avp_list2ptrs(avps)) == NULL) {
		Perror("L2TP: error decoding AVP list");
		goto fail_errno;
	}

	/* If this is a tunnel-level command, do it */
	if (msg_info->ctrl_handler != NULL) {

		/* Check for valid control connection state */
		for (i = 0; msg_info->valid_states[i] != -1
		    && msg_info->valid_states[i] != ctrl->state; i++);
		if (msg_info->valid_states[i] == -1) {

			/* Could be in CS_DYING if we just closed the tunnel */
			if (ctrl->state == CS_DYING) {
				snprintf(ebuf, sizeof(ebuf),
				    "ignoring %s in state %s", msg_info->name,
				    ppp_l2tp_ctrl_state_str(ctrl->state));
				Log(LOG_INFO, ("L2TP: %s", ebuf));
				goto done;
			}

			/* Log a warning message because the peer is broken */
			snprintf(ebuf, sizeof(ebuf),
			    "rec'd %s in state %s", msg_info->name,
			    ppp_l2tp_ctrl_state_str(ctrl->state));
			Log(LOG_WARNING, ("L2TP: %s", ebuf));
			ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_FSM, 0, ebuf);
			goto done;
		}

		/* Cancel reply timer and invoke handler */
		Log((msgtype == HELLO) ? LOG_DEBUG : LOG_INFO,
		    ("L2TP: rec'd %s in state %s", msg_info->name,
		    ppp_l2tp_ctrl_state_str(ctrl->state)));
		pevent_unregister(&ctrl->reply_timer);
		if ((*msg_info->ctrl_handler)(ctrl, avps, ptrs) == -1)
			goto fail_errno;

		/* If we're now expecting a reply, start expecting it */
		ppp_l2tp_ctrl_check_reply(ctrl);
		goto done;
	}

	/* Find associated session */
	if (key.config.session_id == 0) {
		struct ghash_walk walk;

		/* This should only happen with CDN messages */
		if (msgtype != CDN) {
			Log(LOG_NOTICE, ("L2TP: rec'd %s with zero session ID",
			    msg_info->name));
			goto done;
		}

		/* Find session with 'reverse lookup' using peer's session ID */
		ghash_walk_init(ctrl->sessions, &walk);
		while ((sess = ghash_walk_next(ctrl->sessions, &walk)) != NULL
		    && sess->peer_id != key.config.session_id);
		if (sess == NULL)
			goto done;
	} else if ((sess = ghash_get(ctrl->sessions, &key)) == NULL) {
		Log(LOG_NOTICE, ("L2TP: rec'd %s for unknown session 0x%04x",
		    msg_info->name, key.config.session_id));
		goto done;
	}

	/* Check for valid session state, origination, and side */
	for (i = 0; msg_info->valid_states[i] != -1
	    && msg_info->valid_states[i] != sess->state; i++);
	if (msg_info->valid_states[i] == -1) {
		snprintf(ebuf, sizeof(ebuf), "rec'd %s in state %s",
		    msg_info->name, ppp_l2tp_sess_state_str(sess->state));
		Log(LOG_WARNING, ("L2TP: %s", ebuf));
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_FSM, 0, ebuf);
		goto done;
	}
	for (i = 0; msg_info->valid_orig[i] != -1
	    && msg_info->valid_orig[i] != sess->orig; i++);
	if (msg_info->valid_orig[i] == -1) {
		snprintf(ebuf, sizeof(ebuf), "rec'd %s in state %s,"
		    " but session originated %sly", msg_info->name,
		    ppp_l2tp_sess_state_str(sess->state),
		    ppp_l2tp_sess_orig_str(sess->orig));
		Log(LOG_WARNING, ("L2TP: %s", ebuf));
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_FSM, 0, ebuf);
		goto done;
	}
	for (i = 0; msg_info->valid_side[i] != -1
	    && msg_info->valid_side[i] != sess->side; i++);
	if (msg_info->valid_side[i] == -1) {
		snprintf(ebuf, sizeof(ebuf), "rec'd %s in state %s,"
		    " but we are %s for this session", msg_info->name,
		    ppp_l2tp_sess_state_str(sess->state),
		    ppp_l2tp_sess_side_str(sess->side));
		Log(LOG_WARNING, ("L2TP: %s", ebuf));
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_FSM, 0, ebuf);
		goto done;
	}

	/* Cancel reply timer and invoke handler */
	Log(LOG_INFO, ("L2TP: rec'd %s in state %s",
	    msg_info->name, ppp_l2tp_sess_state_str(sess->state)));
	pevent_unregister(&sess->reply_timer);
	if ((*msg_info->sess_handler)(sess, avps, ptrs) == -1)
		goto fail_errno;

	/* If we're now expecting a reply, start expecting it */
	ppp_l2tp_sess_check_reply(sess);
	goto done;

fail_invalid:
	/* Fail because of a bogus message */
	ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
	    L2TP_ERROR_BAD_VALUE, "improperly formatted control message");
	goto done;

fail_errno:
	/* Fail because of a system error */
	ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
	    L2TP_ERROR_GENERIC, strerror(errno));

done:
	/* Clean up */
	ppp_l2tp_avp_list_destroy(&avps);
	ppp_l2tp_avp_ptrs_destroy(&ptrs);
}

/*
 * Read from netgraph control socket. This is where incoming
 * netgraph control messages appear.
 */
static void
ppp_l2tp_ctrl_event(void *arg)
{
	struct ppp_l2tp_ctrl *const ctrl = arg;
	union {
	    u_char buf[128];
	    struct ng_mesg msg;
	} buf;
	struct ng_mesg *const msg = &buf.msg;
	char raddr[NG_PATHSIZ];

	/* Read netgraph control message */
	if (NgRecvMsg(ctrl->csock, msg, sizeof(buf), raddr) < 0) {
		Perror("L2TP: error reading control message");
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
		    L2TP_ERROR_GENERIC, strerror(errno));
		return;
	}

	/* Examine message */
	switch (msg->header.typecookie) {
	case NGM_L2TP_COOKIE:
		switch (msg->header.cmd) {
		case NGM_L2TP_ACK_FAILURE:
			if (ctrl->state != CS_DYING) {
				Log(LOG_WARNING,
				    ("L2TP: acknowledgement timeout"));
				ppp_l2tp_ctrl_close(ctrl,
				    L2TP_RESULT_CLEARED, 0, NULL);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

/************************************************************************
		    INCOMING CONTROL MESSAGE HANDLERS
************************************************************************/

static int
ppp_l2tp_handle_SCCRQ(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_ctrl *ctrl2;
	const u_char *tiebreaker;
	struct ghash_walk walk;
	int diff;
	int i;

	/* See if there is an outstanding SCCRQ to this peer */
	ghash_walk_init(ppp_l2tp_ctrls, &walk);
	while ((ctrl2 = ghash_walk_next(ppp_l2tp_ctrls, &walk)) != NULL) {
		if (ctrl2 != ctrl
		    && ctrl2->peer_id == ctrl->peer_id
		    && ctrl2->state == CS_WAIT_CTL_REPLY)
			break;
	}
	if (ctrl2 == NULL)
		goto ok;

	/* Determine if we used a tie-breaker in our SCCRQ */
	for (tiebreaker = NULL, i = 0; i < ctrl2->avps->length; i++) {
		struct ppp_l2tp_avp *const avp = &ctrl2->avps->avps[i];

		if (avp->vendor == 0 && avp->type == AVP_TIE_BREAKER) {
			tiebreaker = avp->value;
			break;
		}
	}

	/* If neither side used a tie-breaker, allow this connection */
	if (tiebreaker == NULL && ptrs->tiebreaker == NULL)
		goto ok;

	/* Compare tie-breaker values to see who wins */
	if (tiebreaker == NULL)				/* peer wins */
		diff = 1;
	else if (ptrs->tiebreaker == NULL)		/* i win */
		diff = -1;
	else						/* compare values */
		diff = memcmp(tiebreaker, &ptrs->tiebreaker->value, 8);
	if (diff == 0) {				/* we both lose */
		Log(LOG_NOTICE, ("L2TP: SCCRQ tie: we both lose"));
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_DUP_CTRL, 0, NULL);
		ppp_l2tp_ctrl_close(ctrl2, L2TP_RESULT_DUP_CTRL, 0, NULL);
		return (0);
	}
	if (diff > 0) {					/* i win */
		Log(LOG_NOTICE, ("L2TP: SCCRQ tie: peer loses"));
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_DUP_CTRL, 0, NULL);
		return (0);
	}
	Log(LOG_NOTICE, ("L2TP: SCCRQ tie: peer wins"));
	ppp_l2tp_ctrl_close(ctrl2, L2TP_RESULT_DUP_CTRL, 0, NULL);

ok:
	/* Do control connection setup */
	if (ppp_l2tp_ctrl_setup_1(ctrl, ptrs) == -1)
		return (-1);

	/* Send response and update state */
	ctrl->state = CS_WAIT_CTL_CONNECT;
	ppp_l2tp_ctrl_send(ctrl, 0, SCCRP, ctrl->avps);
	return (0);
}

static int
ppp_l2tp_handle_SCCRP(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	/* Do control connection setup */
	if (ppp_l2tp_ctrl_setup_1(ctrl, ptrs) == -1)
		return (-1);
	if (ppp_l2tp_ctrl_setup_2(ctrl, ptrs) == -1)
		return (-1);

	/* Send response and update state */
	ctrl->state = CS_ESTABLISHED;
	ppp_l2tp_ctrl_send(ctrl, 0, SCCCN, ctrl->avps);
	if (*ctrl->cb->ctrl_connected != NULL)
	    (*ctrl->cb->ctrl_connected)(ctrl);
	return (0);
}

static int
ppp_l2tp_handle_SCCCN(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	/* Do control connection setup */
	if (ppp_l2tp_ctrl_setup_2(ctrl, ptrs) == -1)
		return (-1);

	/* Update state */
	ctrl->state = CS_ESTABLISHED;
	if (*ctrl->cb->ctrl_connected != NULL)
	    (*ctrl->cb->ctrl_connected)(ctrl);
	return (0);
}

static int
ppp_l2tp_handle_StopCCN(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_sess *sess;
	struct ghash_walk walk;

	/* StopCCN implies closing all sessions */
	ctrl->peer_notified = 1;
	ghash_walk_init(ctrl->sessions, &walk);
	while ((sess = ghash_walk_next(ctrl->sessions, &walk)) != NULL)
		sess->peer_notified = 1;

	/* Close control connection */
	ppp_l2tp_ctrl_close(ctrl, ptrs->errresultcode->result,
	    ptrs->errresultcode->error, ptrs->errresultcode->errmsg);
	return (0);
}

static int
ppp_l2tp_handle_HELLO(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	return (0);
}

static int
ppp_l2tp_handle_OCRQ(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_sess *sess;

	/* Create new session */
	if ((sess = ppp_l2tp_sess_create(ctrl, ORIG_REMOTE, SIDE_LAC,
	    ptrs->serialnum->serialnum)) == NULL)
		return (-1);
	sess->peer_id = ptrs->sessionid->id;

	/* Notify link side */
	(*ctrl->cb->initiated)(ctrl, sess, 1, avps, &sess->include_length, &sess->enable_dseq);

	if (sess->state != SS_DYING) {
		/* Send response */
		ppp_l2tp_ctrl_send(ctrl, sess->peer_id, OCRP, sess->my_avps);
	}

	/* Clean up */
	return (0);
}

static int
ppp_l2tp_handle_ICRQ(struct ppp_l2tp_ctrl *ctrl,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_sess *sess;

	/* Create new session */
	if ((sess = ppp_l2tp_sess_create(ctrl, ORIG_REMOTE, SIDE_LNS,
	    ptrs->serialnum->serialnum)) == NULL)
		return (-1);
	sess->peer_id = ptrs->sessionid->id;

	/* Notify link side */
	(*ctrl->cb->initiated)(ctrl, sess, 0, avps, &sess->include_length, &sess->enable_dseq);

	if (sess->state != SS_DYING) {
		/* Send response */
		ppp_l2tp_ctrl_send(ctrl, sess->peer_id, ICRP, sess->my_avps);
		ppp_l2tp_sess_check_reply(sess);
	}

	/* Clean up */
	return (0);
}

static int
ppp_l2tp_handle_OCRP(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	if (sess->state == SS_DYING)
		return (0);

	sess->peer_id = ptrs->sessionid->id;
	sess->state = SS_WAIT_CONNECT;
	return (0);
}

static int
ppp_l2tp_handle_xCCN(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	if (sess->state == SS_DYING)
		return (0);

	/* Save peer's AVP's for this session */
	ppp_l2tp_avp_list_destroy(&sess->peer_avps);
	if ((sess->peer_avps = ppp_l2tp_avp_list_copy(avps)) == NULL)
		return (-1);

	/* Set up session */
	sess->dseq_required = (ptrs->seqrequired != NULL);
	if (ppp_l2tp_sess_setup(sess) == -1)
		return (-1);

	/* Done */
	return (0);
}

static int
ppp_l2tp_handle_ICRP(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;
	char buf[64];
	
	if (sess->state == SS_DYING)
		return (0);

	/* Save peer ID */
	sess->peer_id = ptrs->sessionid->id;

	/* Detect duplicate ICRP's */
	if (sess->peer_responded) {
		snprintf(buf, sizeof(buf), "rec'd duplicate %s in state %s",
		    "ICRP", ppp_l2tp_sess_state_str(sess->state));
		Log(LOG_WARNING, ("L2TP: %s", buf));
		ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_FSM, 0, buf);
		return (0);
	}

	/* Check status for locally initiated incoming call */
	sess->peer_responded = 1;
	if (ppp_l2tp_sess_check_liic(sess) == -1)
		return (-1);

	/* Done */
	return (0);
}

static int
ppp_l2tp_handle_CDN(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	sess->peer_notified = 1;
	ppp_l2tp_sess_close(sess, ptrs->errresultcode->result,
	    ptrs->errresultcode->error, ptrs->errresultcode->errmsg);
	return (0);
}

static int
ppp_l2tp_handle_SLI(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	if (sess->state == SS_DYING)
		return (0);

	if (ctrl->cb->set_link_info == NULL)
		return (0);
	(*ctrl->cb->set_link_info)(sess, ptrs->accm->xmit, ptrs->accm->recv);
	return (0);
}

static int
ppp_l2tp_handle_WEN(struct ppp_l2tp_sess *sess,
	const struct ppp_l2tp_avp_list *avps, struct ppp_l2tp_avp_ptrs *ptrs)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	if (sess->state == SS_DYING)
		return (0);

	if (ctrl->cb->wan_error_notify == NULL)
		return (0);
	(*ctrl->cb->wan_error_notify)(sess, ptrs->callerror->crc,
	    ptrs->callerror->frame, ptrs->callerror->overrun,
	    ptrs->callerror->buffer, ptrs->callerror->timeout,
	    ptrs->callerror->alignment);
	return (0);
}

/************************************************************************
			REPLY EXPECTORS
************************************************************************/

static pevent_handler_t	ppp_l2tp_ctrl_reply_timeout;
static pevent_handler_t	ppp_l2tp_sess_reply_timeout;

static void
ppp_l2tp_ctrl_check_reply(struct ppp_l2tp_ctrl *ctrl)
{
	pevent_unregister(&ctrl->reply_timer);
	switch (ctrl->state) {
	case CS_IDLE:
	case CS_WAIT_CTL_REPLY:
	case CS_WAIT_CTL_CONNECT:
		if (pevent_register(ctrl->ctx, &ctrl->reply_timer, 0,
		    ctrl->mutex, ppp_l2tp_ctrl_reply_timeout, ctrl, PEVENT_TIME,
		    L2TP_REPLY_TIMEOUT * 1000) == -1)
			Perror("L2TP: error starting reply timer");
		break;
	default:
		break;
	}
}

static void
ppp_l2tp_sess_check_reply(struct ppp_l2tp_sess *sess)
{
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	pevent_unregister(&sess->reply_timer);
	switch (sess->state) {
	case SS_WAIT_REPLY:
	case SS_WAIT_CONNECT:
		if (pevent_register(ctrl->ctx, &sess->reply_timer, 0,
		    ctrl->mutex, ppp_l2tp_sess_reply_timeout, sess, PEVENT_TIME,
		    L2TP_REPLY_TIMEOUT * 1000) == -1)
			Perror("L2TP: error starting reply timer");
		break;
	default:
		break;
	}
}

static void
ppp_l2tp_ctrl_reply_timeout(void *arg)
{
	struct ppp_l2tp_ctrl *const ctrl = arg;

	pevent_unregister(&ctrl->reply_timer);
	Log(LOG_NOTICE, ("L2TP: reply timeout in state %s",
	    ppp_l2tp_ctrl_state_str(ctrl->state)));
	ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
	    L2TP_ERROR_GENERIC, "expecting reply; none received");
}

static void
ppp_l2tp_sess_reply_timeout(void *arg)
{
	struct ppp_l2tp_sess *const sess = arg;
	struct ppp_l2tp_ctrl *const ctrl = sess->ctrl;

	pevent_unregister(&sess->reply_timer);
	Log(LOG_NOTICE, ("L2TP: reply timeout in state %s",
	    ppp_l2tp_sess_state_str(sess->state)));
	ppp_l2tp_ctrl_close(ctrl, L2TP_RESULT_ERROR,
	    L2TP_ERROR_GENERIC, "expecting reply; none received");
}

/************************************************************************
		CONTROL AND SESSION OBJECT DESTRUCTORS
************************************************************************/

/*
 * Immediately destroy a control connection and all associated sessions.
 */
void
ppp_l2tp_ctrl_destroy(struct ppp_l2tp_ctrl **ctrlp)
{
	struct ppp_l2tp_ctrl *const ctrl = *ctrlp;

	/* Sanity */
	if (ctrl == NULL)
		return;
	*ctrlp = NULL;

	/* Destroy all sessions */
	while (ghash_size(ctrl->sessions) > 0) {
		struct ppp_l2tp_sess *sess;
		struct ghash_walk walk;

		ghash_walk_init(ctrl->sessions, &walk);
		sess = ghash_walk_next(ctrl->sessions, &walk);
		ppp_l2tp_sess_destroy(&sess);
	}

	/* Destroy netgraph node */
	(void)NgSendMsg(ctrl->csock, NG_L2TP_HOOK_CTRL,
	    NGM_GENERIC_COOKIE, NGM_SHUTDOWN, NULL, 0);

	/* Destroy control connection */
	ghash_remove(ppp_l2tp_ctrls, ctrl);
	if (ghash_size(ppp_l2tp_ctrls) == 0)
		ghash_destroy(&ppp_l2tp_ctrls);
	(void)close(ctrl->csock);
	(void)close(ctrl->dsock);
	pevent_unregister(&ctrl->reply_timer);
	pevent_unregister(&ctrl->close_timer);
	pevent_unregister(&ctrl->death_timer);
	pevent_unregister(&ctrl->idle_timer);
	pevent_unregister(&ctrl->ctrl_event);
	pevent_unregister(&ctrl->data_event);
	ppp_l2tp_avp_list_destroy(&ctrl->avps);
	ghash_destroy(&ctrl->sessions);
	Freee(ctrl->secret);
	Freee(ctrl->errmsg);
	memset(ctrl, 0, sizeof(*ctrl));
	Freee(ctrl);
}

/*
 * Immediately destroy a session.
 */
static void
ppp_l2tp_sess_destroy(struct ppp_l2tp_sess **sessp)
{
	struct ppp_l2tp_sess *const sess = *sessp;
	struct ppp_l2tp_ctrl *ctrl;
	char path[32];

	/* Sanity */
	if (sess == NULL)
		return;
	*sessp = NULL;

	/* Destroy session */
	ctrl = sess->ctrl;
	if (sess->state != SS_DYING || sess->close_timer) {
		ctrl->active_sessions--;
		/* Close control connection after last session closes */
		if (ctrl->active_sessions == 0) {
			if (ctrl->state == CS_DYING) {
				ppp_l2tp_ctrl_death_start(ctrl);
			} else if (pevent_register(ctrl->ctx, &ctrl->death_timer, 0,
			    ctrl->mutex, ppp_l2tp_unused_timeout, ctrl,
			    PEVENT_TIME, gL2TPto * 1000) == -1) {
				Perror("L2TP: error starting unused timer");
			}
		}
	}
	ghash_remove(ctrl->sessions, sess);
	snprintf(path, sizeof(path), "[%lx]:", (u_long)sess->node_id);
	(void)NgSendMsg(ctrl->csock, path,
	    NGM_GENERIC_COOKIE, NGM_SHUTDOWN, NULL, 0);
	ppp_l2tp_avp_list_destroy(&sess->my_avps);
	ppp_l2tp_avp_list_destroy(&sess->peer_avps);
	pevent_unregister(&sess->notify_timer);
	pevent_unregister(&sess->reply_timer);
	pevent_unregister(&sess->close_timer);
	pevent_unregister(&sess->death_timer);
	Freee(sess->errmsg);
	memset(sess, 0, sizeof(*sess));
	Freee(sess);
}

/************************************************************************
			HASH TABLE FUNCTIONS
************************************************************************/

static int
ppp_l2tp_ctrl_equal(struct ghash *g, const void *item1, const void *item2)
{
	const struct ppp_l2tp_ctrl *const ctrl1 = item1;
	const struct ppp_l2tp_ctrl *const ctrl2 = item2;

	return (ctrl1->config.tunnel_id == ctrl2->config.tunnel_id);
}

static u_int32_t
ppp_l2tp_ctrl_hash(struct ghash *g, const void *item)
{
	const struct ppp_l2tp_ctrl *const ctrl = item;

	return ((u_int32_t)ctrl->config.tunnel_id);
}

static int
ppp_l2tp_sess_equal(struct ghash *g, const void *item1, const void *item2)
{
	const struct ppp_l2tp_sess *const sess1 = item1;
	const struct ppp_l2tp_sess *const sess2 = item2;

	return (sess1->config.session_id == sess2->config.session_id);
}

static u_int32_t
ppp_l2tp_sess_hash(struct ghash *g, const void *item)
{
	const struct ppp_l2tp_sess *const sess = item;

	return ((u_int32_t)sess->config.session_id);
}

/************************************************************************
			STRING CONVERTERS
************************************************************************/

/*
 * Dump an AVP list.
 */
static void
ppp_l2tp_ctrl_dump(struct ppp_l2tp_ctrl *ctrl,
	struct ppp_l2tp_avp_list *avps, const char *fmt, ...)
{
	char buf[1024];
	va_list args;
	int i;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	for (i = 0; i < avps->length; i++) {
		struct ppp_l2tp_avp *const avp = &avps->avps[i];
		const struct ppp_l2tp_avp_info *info;
		int j;

		strlcat(buf, i > 0 ? " [" : "[", sizeof(buf));
		for (j = 0; (info = &ppp_l2tp_avp_info_list[j])->name != NULL
		    && (info->vendor != avp->vendor
		      || info->type != avp->type); j++);
		if (info->name != NULL) {
			strlcat(buf, info->name, sizeof(buf));
			strlcat(buf, " ", sizeof(buf));
			(*info->decode)(info, avp,
			    buf + strlen(buf), sizeof(buf) - strlen(buf));
		} else {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			    "%u:%u vlen=%u", avp->vendor, avp->type, avp->vlen);
		}
		strlcat(buf, "]", sizeof(buf));
	}
	Log(LOG_DEBUG, ("%s", buf));
}

static const char *
ppp_l2tp_ctrl_state_str(enum l2tp_ctrl_state state)
{
	switch (state) {
	case CS_IDLE:
		return ("idle");
	case CS_WAIT_CTL_REPLY:
		return ("wait-ctl-reply");
	case CS_WAIT_CTL_CONNECT:
		return ("wait-ctl-conn");
	case CS_ESTABLISHED:
		return ("established");
	case CS_DYING:
		return ("dying");
	default:
		return ("UNKNOWN");
	}
}

static const char *
ppp_l2tp_sess_state_str(enum l2tp_ctrl_state state)
{
	switch (state) {
	case SS_WAIT_REPLY:
		return ("wait-cs-reply");
	case SS_WAIT_CONNECT:
		return ("wait-connect");
	case SS_WAIT_ANSWER:
		return ("wait-answer");
	case SS_ESTABLISHED:
		return ("established");
	case SS_DYING:
		return ("dying");
	default:
		return ("UNKNOWN");
	}
}

static const char *
ppp_l2tp_sess_orig_str(enum l2tp_sess_orig orig)
{
	return (orig == ORIG_LOCAL ? "local" : "remote");
}

static const char *
ppp_l2tp_sess_side_str(enum l2tp_sess_side side)
{
	return (side == SIDE_LNS ? "LNS" : "LAC");
}

char *
ppp_l2tp_ctrl_stats(struct ppp_l2tp_ctrl *ctrl, char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%s",
		ppp_l2tp_ctrl_state_str(ctrl->state));
	return (buf);
}
