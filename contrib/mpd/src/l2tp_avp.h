
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

#ifndef _PPP_L2TP_PDEL_PPP_PPP_L2TP_AVP_H_
#define _PPP_L2TP_PDEL_PPP_PPP_L2TP_AVP_H_

/***********************************************************************
			AVP STRUCTURE DEFINITIONS
***********************************************************************/

/* Constants defined in RFC 2661 */
#define L2TP_PROTO_VERS		1
#define L2TP_PROTO_REV		0
#define L2TP_DEFAULT_PEER_WIN	4
#define L2TP_BEARER_DIGITAL	0x00000001
#define L2TP_BEARER_ANALOG	0x00000002
#define L2TP_FRAMING_SYNC	0x00000001
#define L2TP_FRAMING_ASYNC	0x00000002

#define AVP_MANDATORY		0x8000
#define AVP_HIDDEN		0x4000
#define AVP_RESERVED		0x3c00
#define AVP_LENGTH_MASK		0x03ff
#define AVP_MAX_LENGTH		1023
#define AVP_MAX_VLEN		(AVP_MAX_LENGTH - 6)

struct ppp_l2tp_avp_info;
struct ppp_l2tp_avp;

/* AVP decoder function type */
typedef void	ppp_l2tp_avp_decode_t(const struct ppp_l2tp_avp_info *info,
			const struct ppp_l2tp_avp *avp, char *buf, size_t bmax);

/* Structure describing one AVP type */
struct ppp_l2tp_avp_info {
	const char		*name;		/* textual name */
	ppp_l2tp_avp_decode_t	*decode;	/* decode contents */
	u_int16_t		vendor;		/* vendor id */
	u_int16_t		type;		/* attribute type */
	u_char			hidden_ok;	/* avp allowed to be hidden */
	u_char			mandatory;	/* set mandatory bit (xmit) */
	u_int16_t		min_length;	/* minimum length of value */
	u_int16_t		max_length;	/* maximum length of value */
};

/* Structure describing one AVP */
struct ppp_l2tp_avp {
	u_char			mandatory;	/* mandatory bit */
	u_int16_t		vendor;		/* vendor id */
	u_int16_t		type;		/* attribute type */
	u_int16_t		vlen;		/* length of attribute value */
	void			*value;		/* attribute value */
};

/* Structure describing a list of AVP's */
struct ppp_l2tp_avp_list {
	u_int			length;		/* length of list */
	struct ppp_l2tp_avp	*avps;		/* array of avps in list */
};

/* Individual AVP structures */
struct messagetype_avp {
	u_int16_t	mesgtype;
};

struct codesresulterror_avp {
	u_int16_t	result;
	u_int16_t	error;
	char		errmsg[0];
};

struct protoversion_avp {
	u_int8_t	version;
	u_int8_t	revision;
};

struct framecap_avp {
	u_char		async;
	u_char		sync;
};

struct bearercap_avp {
	u_char		analog;
	u_char		digital;
};

struct tiebreaker_avp {
	u_char		value[8];
};

struct firmware_avp {
	u_int16_t	revision;
};

struct hostname_avp {
	char		hostname[0];
};

struct vendorname_avp {
	char		vendorname[0];
};

struct tunnelid_avp {
	u_int16_t	id;
};

struct sessionid_avp {
	u_int16_t	id;
};

struct windowsize_avp {
	u_int16_t	size;
};

struct challenge_avp {
	u_int		length;
	u_char		value[0];
};

struct challengeresp_avp {
	u_char		value[16];
};

struct causecode_avp {
	u_int16_t	causecode;
	u_int8_t	causemsg;
	char		message[0];
};

struct serialnum_avp {
	u_int32_t	serialnum;
};

struct minbps_avp {
	u_int32_t	minbps;
};

struct maxbps_avp {
	u_int32_t	maxbps;
};

struct bearer_avp {
	u_char		analog;
	u_char		digital;
};

struct framing_avp {
	u_char		async;
	u_char		sync;
};

struct callednum_avp {
	char		number[0];
};

struct callingnum_avp {
	char		number[0];
};

struct subaddress_avp {
	char		number[0];
};

struct txconnect_avp {
	u_int32_t	bps;
};

struct rxconnect_avp {
	u_int32_t	bps;
};

struct channelid_avp {
	u_int32_t	channel;
};

struct groupid_avp {
	u_int		length;
	u_char		data[0];
};

struct recvlcp_avp {
	u_int		length;
	u_char		data[0];
};

struct lastsentlcp_avp {
	u_int		length;
	u_char		data[0];
};

struct lastrecvlcp_avp {
	u_int		length;
	u_char		data[0];
};

struct proxyauth_avp {
	u_int16_t	type;
};

struct proxyname_avp {
	u_int		length;
	char		data[0];
};

struct proxychallenge_avp {
	u_int		length;
	u_char		data[0];
};

struct proxyid_avp {
	u_int16_t	id;
};

struct proxyresp_avp {
	u_int		length;
	u_char		data[0];
};

struct callerror_avp {
	u_int32_t	crc;
	u_int32_t	frame;
	u_int32_t	overrun;
	u_int32_t	buffer;
	u_int32_t	timeout;
	u_int32_t	alignment;
};

struct accm_avp {
	u_int32_t	xmit;
	u_int32_t	recv;
};

struct seqrequired_avp {
};

/*
 * This structure describes a suite of AVPs in host-friendly format.
 * Typically only a sub-set of the AVPs listed will be used.
 */
struct ppp_l2tp_avp_ptrs {
	struct messagetype_avp		*message;
	struct codesresulterror_avp	*errresultcode;
	struct protoversion_avp		*protocol;
	struct framecap_avp		*framingcap;
	struct bearercap_avp		*bearercap;
	struct tiebreaker_avp		*tiebreaker;
	struct firmware_avp		*firmware;
	struct hostname_avp		*hostname;
	struct vendorname_avp		*vendor;
	struct tunnelid_avp		*tunnelid;
	struct sessionid_avp		*sessionid;
	struct windowsize_avp		*winsize;
	struct challenge_avp		*challenge;
	struct challengeresp_avp	*challengresp;
	struct causecode_avp		*causecode;
	struct serialnum_avp		*serialnum;
	struct minbps_avp		*minbps;
	struct maxbps_avp		*maxbps;
	struct bearer_avp		*bearer;
	struct framing_avp		*framing;
	struct callednum_avp		*callednum;
	struct callingnum_avp		*callingnum;
	struct subaddress_avp		*subaddress;
	struct txconnect_avp		*txconnect;
	struct rxconnect_avp		*rxconnect;
	struct channelid_avp		*channelid;
	struct groupid_avp		*groupid;
	struct recvlcp_avp		*recvlcp;
	struct lastsentlcp_avp		*lastsendlcp;
	struct lastrecvlcp_avp		*lastrecvlcp;
	struct proxyauth_avp		*proxyauth;
	struct proxyname_avp		*proxyname;
	struct proxychallenge_avp	*proxychallenge;
	struct proxyid_avp		*proxyid;
	struct proxyresp_avp		*proxyres;
	struct callerror_avp		*callerror;
	struct accm_avp			*accm;
	struct seqrequired_avp		*seqrequired;
};

/* L2TP result codes for StopCCN messages */
#define L2TP_RESULT_CLEARED	1	/* general reqst. to clear connection */
#define L2TP_RESULT_ERROR	2	/* general error: see error code */
#define L2TP_RESULT_DUP_CTRL	3	/* dupliate control channel */
#define L2TP_RESULT_NOT_AUTH	4	/* control channel not authorized */
#define L2TP_RESULT_PROTO_VERS	5	/* unsupported protocol: max=err code */
#define L2TP_RESULT_SHUTDOWN	6	/* being shut down */
#define L2TP_RESULT_FSM		7	/* fsm error */

/* L2TP result codes for CDN messages */
#define L2TP_RESULT_DRP_CARRIER	1	/* lost carrier */
#define L2TP_RESULT_ERROR	2	/* general error: see error code */
#define L2TP_RESULT_ADMIN	3	/* administrative disconnect */
#define L2TP_RESULT_AVAIL_TEMP	4	/* temp. lack of available resources */
#define L2TP_RESULT_AVAIL_PERM	5	/* perm. lack of available resources */
#define L2TP_RESULT_DEST	6	/* invalid destination */
#define L2TP_RESULT_NO_CARRIER	7	/* no carrier detected */
#define L2TP_RESULT_BUSY	8	/* busy signal */
#define L2TP_RESULT_NO_DIALTONE	9	/* no dial tone */
#define L2TP_RESULT_TIMEOUT	10	/* timeout during establishment */
#define L2TP_RESULT_NO_FRAMING	11	/* no appropriate framing detected */

/* L2TP error codes */
#define L2TP_ERROR_NO_CTRL	1	/* no control connection exists */
#define L2TP_ERROR_LENGTH	2	/* length is wrong */
#define L2TP_ERROR_BAD_VALUE	3	/* invalid or field non-zero resv. */
#define L2TP_ERROR_RESOURCES	4	/* insufficient resources */
#define L2TP_ERROR_INVALID_ID	5	/* invalid session id */
#define L2TP_ERROR_GENERIC	6	/* generic vendor-specific error */
#define L2TP_ERROR_TRY_ANOTHER	7	/* try another lns */
#define L2TP_ERROR_MANDATORY	8	/* rec'd unknown mandatory avp */

/***********************************************************************
			AVP TYPES
***********************************************************************/

enum ppp_l2tp_avp_type {
	AVP_MESSAGE_TYPE		=0,
	AVP_RESULT_CODE			=1,
	AVP_PROTOCOL_VERSION		=2,
	AVP_FRAMING_CAPABILITIES	=3,
	AVP_BEARER_CAPABILITIES		=4,
	AVP_TIE_BREAKER			=5,
	AVP_FIRMWARE_REVISION		=6,
	AVP_HOST_NAME			=7,
	AVP_VENDOR_NAME			=8,
	AVP_ASSIGNED_TUNNEL_ID		=9,
	AVP_RECEIVE_WINDOW_SIZE		=10,
	AVP_CHALLENGE			=11,
	AVP_CAUSE_CODE			=12,
	AVP_CHALLENGE_RESPONSE		=13,
	AVP_ASSIGNED_SESSION_ID		=14,
	AVP_CALL_SERIAL_NUMBER		=15,
	AVP_MINIMUM_BPS			=16,
	AVP_MAXIMUM_BPS			=17,
	AVP_BEARER_TYPE			=18,
	AVP_FRAMING_TYPE		=19,
	AVP_CALLED_NUMBER		=21,
	AVP_CALLING_NUMBER		=22,
	AVP_SUB_ADDRESS			=23,
	AVP_TX_CONNECT_SPEED		=24,
	AVP_PHYSICAL_CHANNEL_ID		=25,
	AVP_INITIAL_RECV_CONFREQ	=26,
	AVP_LAST_SENT_CONFREQ		=27,
	AVP_LAST_RECV_CONFREQ		=28,
	AVP_PROXY_AUTHEN_TYPE		=29,
	AVP_PROXY_AUTHEN_NAME		=30,
	AVP_PROXY_AUTHEN_CHALLENGE	=31,
	AVP_PROXY_AUTHEN_ID		=32,
	AVP_PROXY_AUTHEN_RESPONSE	=33,
	AVP_CALL_ERRORS			=34,
	AVP_ACCM			=35,
	AVP_RANDOM_VECTOR		=36,
	AVP_PRIVATE_GROUP_ID		=37,
	AVP_RX_CONNECT_SPEED		=38,
	AVP_SEQUENCING_REQUIRED		=39,
	AVP_MAX				=40
};

/***********************************************************************
			AVP FUNCTIONS
***********************************************************************/

__BEGIN_DECLS

/*
 * Create an AVP pointers structure from an AVP list.
 *
 * AVP's not listed in the pointers structure are simply omitted.
 *
 * Returns:
 *	NULL	If failure (errno is set)
 *	ptrs	New pointers structure
 */
extern struct	ppp_l2tp_avp_ptrs *ppp_l2tp_avp_list2ptrs(
			const struct ppp_l2tp_avp_list *list);

/*
 * Destroy an AVP pointers structure.
 *
 * Arguments:
 *	ptrsp	Pointer to 'AVP pointers' pointer; it gets set to NULL
 */
extern void	ppp_l2tp_avp_ptrs_destroy(struct ppp_l2tp_avp_ptrs **ptrsp);

/*
 * Create a new AVP structure.
 *
 * Arguments:
 *	mandatory Set mandatory bit
 *	vendor	Attribute vendor
 *	type	Attribute type
 *	value	Value contents
 *	vlen	Length of value contents
 *
 * Returns:
 *	NULL	If failure (errno is set)
 *	avp	New AVP structure
 */
extern struct	ppp_l2tp_avp *ppp_l2tp_avp_create(int mandatory,
			u_int16_t vendor, u_int16_t type, const void *value,
			size_t vlen);

/*
 * Copy an AVP struture.
 *
 * Returns:
 *	NULL	If failure (errno is set)
 *	ptr	New AVP structure
 */
extern struct	ppp_l2tp_avp *ppp_l2tp_avp_copy(const struct ppp_l2tp_avp *avp);

/*
 * Destroy an AVP structure.
 *
 * Arguments:
 *	avpp	Pointer to AVP pointer; it gets set to NULL
 */
extern void	ppp_l2tp_avp_destroy(struct ppp_l2tp_avp **avpp);

/*
 * Create a new AVP list.
 *
 * Returns:
 *	NULL	If failure (errno is set)
 *	ptr	New AVP list structure
 */
extern struct	ppp_l2tp_avp_list *ppp_l2tp_avp_list_create(void);

/*
 * Insert an AVP into a list.
 *
 * Arguments:
 *	list	List to insert AVP into
 *	avpp	Pointer to AVP pointer; it gets set to NULL if successful
 *	index	Where in list to insert; special case -1 means 'at the end'
 *
 * Returns:
 *	 0	Successful
 *	-1	If failure (errno is set)
 */
extern int	ppp_l2tp_avp_list_insert(struct ppp_l2tp_avp_list *list,
			struct ppp_l2tp_avp **avpp, int index);

/*
 * Create a new AVP and add it to the end of the given list. That is, this
 * ia a combination of ppp_l2tp_avp_create() and ppp_l2tp_avp_list_insert()
 * added for convenience in a common operation.
 *
 * Arguments:
 *	list	List to append AVP to
 *	mandatory Set mandatory bit
 *	vendor	Attribute vendor
 *	type	Attribute type
 *	value	Value contents
 *	vlen	Length of value contents
 *
 * Returns:
 *	 0	Successful
 *	-1	If failure (errno is set)
 */
extern int	ppp_l2tp_avp_list_append(struct ppp_l2tp_avp_list *list,
		    int mandatory, u_int16_t vendor, u_int16_t type,
		    const void *value, size_t vlen);

/*
 * Extract an AVP from a list.
 *
 * Arguments:
 *	list	List to extract AVP from
 *	index	Which item to extract
 *
 * Returns:
 *	avp	If 'index' is valid, extracted item
 *	NULL	If 'index' is out of range
 */
extern struct	ppp_l2tp_avp *ppp_l2tp_avp_list_extract(
			struct ppp_l2tp_avp_list *list, u_int index);

/*
 * Remove and destroy an AVP from a list.
 *
 * Arguments:
 *	list	List to remove AVP from
 *	index	Which item to remove
 *
 * Returns:
 *	 0	If 'index' is valid and AVP was removed
 *	-1	If 'index' is out of range
 */
extern int	ppp_l2tp_avp_list_remove(
			struct ppp_l2tp_avp_list *list, u_int index);

/*
 * Find an AVP in a list.
 *
 * Arguments:
 *	list	List of AVP's
 *	vendor	AVP vendor
 *	type	AVP attribute type
 *
 * Returns:
 *	index	If found, index of AVP in list
 *	-1	If not found.
 */
extern int	ppp_l2tp_avp_list_find(const struct ppp_l2tp_avp_list *list,
			u_int16_t vendor, u_int16_t type);

/*
 * Copy an AVP list.
 *
 * Returns:
 *	NULL	If failure (errno is set)
 *	ptr	New AVP list structure
 */
extern struct	ppp_l2tp_avp_list *ppp_l2tp_avp_list_copy(
			const struct ppp_l2tp_avp_list *list);

/*
 * Destroy an AVP list.
 *
 * Arguments:
 *	listp	Pointer to AVP list pointer; it gets set to NULL
 */
extern void	ppp_l2tp_avp_list_destroy(struct ppp_l2tp_avp_list **listp);

/*
 * Encode a list of AVP's into a single buffer, preserving the order
 * of the AVP's.  If a shared secret is supplied, and any of the AVP's
 * are hidden, then any required random vector AVP's are created and
 * inserted automatically.
 *
 * Arguments:
 *	info	AVP info list, terminated with NULL name
 *	list	List of AVP structures to encode
 *	secret	Shared secret for hiding AVP's, or NULL for none.
 *	slen	Length of shared secret (if secret != NULL)
 *	buf	Buffer for the data, or NULL to only compute length.
 *
 * Returns:
 *	-1	If failure (errno is set)
 *	length	If successful, length of encoded data (in bytes)
 *
 * Possibilities for errno:
 *	EILSEQ	Invalid data format
 */
extern int	ppp_l2tp_avp_pack(const struct ppp_l2tp_avp_info *info,
			const struct ppp_l2tp_avp_list *list,
			const u_char *secret, size_t slen, u_char *buf);

/*
 * Decode a packet into an array of unpacked AVP structures, preserving
 * the order of the AVP's. Random vector AVP's are automatically removed.
 *
 * Arguments:
 *	info	AVP info list, terminated with NULL name
 *	data	Original packed AVP data packet
 *	dlen	Length of the data pointed to by 'data'
 *	secret	Shared secret for unhiding AVP's, or NULL for none.
 *	slen	Length of shared secret (if secret != NULL)
 *
 * Returns:
 *	NULL	If failure (errno is set)
 *	length	If successful, the unpacked list
 *
 * Possibilities for errno:
 *	EILSEQ	Invalid data format
 *	EAUTH	Hidden AVP found but no shared secret was provided
 *	ENOSYS	Mandatory but unrecognized AVP seen (i.e., AVP not in list)
 */
extern struct	ppp_l2tp_avp_list *ppp_l2tp_avp_unpack(
			const struct ppp_l2tp_avp_info *info,
			u_char *data, size_t dlen,
			const u_char *secret, size_t slen);

/*
 * AVP decoders
 */
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_MESSAGE_TYPE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_RESULT_CODE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PROTOCOL_VERSION;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_FRAMING_CAPABILITIES;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_BEARER_CAPABILITIES;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_TIE_BREAKER;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_FIRMWARE_REVISION;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_HOST_NAME;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_VENDOR_NAME;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_ASSIGNED_TUNNEL_ID;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_RECEIVE_WINDOW_SIZE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CHALLENGE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CAUSE_CODE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CHALLENGE_RESPONSE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_ASSIGNED_SESSION_ID;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CALL_SERIAL_NUMBER;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_MINIMUM_BPS;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_MAXIMUM_BPS;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_BEARER_TYPE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_FRAMING_TYPE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CALLED_NUMBER;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CALLING_NUMBER;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_SUB_ADDRESS;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_TX_CONNECT_SPEED;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PHYSICAL_CHANNEL_ID;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_INITIAL_RECV_CONFREQ;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_LAST_SENT_CONFREQ;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_LAST_RECV_CONFREQ;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PROXY_AUTHEN_TYPE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PROXY_AUTHEN_NAME;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PROXY_AUTHEN_CHALLENGE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PROXY_AUTHEN_ID;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PROXY_AUTHEN_RESPONSE;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_CALL_ERRORS;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_ACCM;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_RANDOM_VECTOR;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_PRIVATE_GROUP_ID;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_RX_CONNECT_SPEED;
extern ppp_l2tp_avp_decode_t	ppp_l2tp_avp_decode_SEQUENCING_REQUIRED;

__END_DECLS

#endif /* _PPP_L2TP_PDEL_PPP_PPP_L2TP_AVP_H_ */
