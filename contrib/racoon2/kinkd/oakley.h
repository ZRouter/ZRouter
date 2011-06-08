/* $Id: oakley.h,v 1.8 2007/07/04 11:54:49 fukumoto Exp $ */
/*	$KAME: oakley.h,v 1.28 2001/12/12 18:23:42 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* refer to RFC 2409 */

/* Attribute Classes */
#define OAKLEY_ATTR_ENC_ALG		1 /* B */
#define   OAKLEY_ATTR_ENC_ALG_DES		1
#define   OAKLEY_ATTR_ENC_ALG_IDEA		2
#define   OAKLEY_ATTR_ENC_ALG_BLOWFISH		3
#define   OAKLEY_ATTR_ENC_ALG_RC5		4
#define   OAKLEY_ATTR_ENC_ALG_3DES		5
#define   OAKLEY_ATTR_ENC_ALG_CAST		6
#define   OAKLEY_ATTR_ENC_ALG_RIJNDAEL		7
#define   OAKLEY_ATTR_ENC_ALG_AES		7
					/*	65001 - 65535 Private Use */
#define OAKLEY_ATTR_HASH_ALG		2 /* B */
#define   OAKLEY_ATTR_HASH_ALG_MD5		1
#define   OAKLEY_ATTR_HASH_ALG_SHA		2
#define   OAKLEY_ATTR_HASH_ALG_TIGER		3
#define   OAKLEY_ATTR_HASH_ALG_SHA2_256		4
#define   OAKLEY_ATTR_HASH_ALG_SHA2_384		5
#define   OAKLEY_ATTR_HASH_ALG_SHA2_512		6
					/*	65001 - 65535 Private Use */
#define OAKLEY_ATTR_AUTH_METHOD		3 /* B */
#define   OAKLEY_ATTR_AUTH_METHOD_PSKEY		1
#define   OAKLEY_ATTR_AUTH_METHOD_DSSSIG	2
#define   OAKLEY_ATTR_AUTH_METHOD_RSASIG	3
#define   OAKLEY_ATTR_AUTH_METHOD_RSAENC	4
#define   OAKLEY_ATTR_AUTH_METHOD_RSAREV	5
#define   OAKLEY_ATTR_AUTH_METHOD_EGENC		6
#define   OAKLEY_ATTR_AUTH_METHOD_EGREV		7
					/*	65001 - 65535 Private Use */
	/*
	 * The following are valid when the Vendor ID is one of
	 * the following:
	 *
	 *	MD5("A GSS-API Authentication Method for IKE")
	 *	MD5("GSSAPI") (recognized by Windows 2000)
	 *	MD5("MS NT5 ISAKMPOAKLEY") (sent by Windows 2000)
	 */
#define   OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB	65001
#define OAKLEY_ATTR_GRP_DESC		4 /* B */
#define   OAKLEY_ATTR_GRP_DESC_MODP768		1
#define   OAKLEY_ATTR_GRP_DESC_MODP1024		2
#define   OAKLEY_ATTR_GRP_DESC_EC2N155		3
#define   OAKLEY_ATTR_GRP_DESC_EC2N185		4
#define   OAKLEY_ATTR_GRP_DESC_MODP1536		5
#define   OAKLEY_ATTR_GRP_DESC_MODP2048		42048	/* these value are */
#define   OAKLEY_ATTR_GRP_DESC_MODP3072		43072	/* make consensus  */
#define   OAKLEY_ATTR_GRP_DESC_MODP4096		44096	/* at the bake off */
#define   OAKLEY_ATTR_GRP_DESC_MODP8192		48192	/* in helsinki     */
#define   OAKLEY_ATTR_GRP_DESC_MODP6144		46144	/* XXX */
					/*	32768 - 65535 Private Use */
#define OAKLEY_ATTR_GRP_TYPE		5 /* B */
#define   OAKLEY_ATTR_GRP_TYPE_MODP		1
#define   OAKLEY_ATTR_GRP_TYPE_ECP		2
#define   OAKLEY_ATTR_GRP_TYPE_EC2N		3
					/*	65001 - 65535 Private Use */
#define OAKLEY_ATTR_GRP_PI		6 /* V */
#define OAKLEY_ATTR_GRP_GEN_ONE		7 /* V */
#define OAKLEY_ATTR_GRP_GEN_TWO		8 /* V */
#define OAKLEY_ATTR_GRP_CURVE_A		9 /* V */
#define OAKLEY_ATTR_GRP_CURVE_B		10 /* V */
#define OAKLEY_ATTR_SA_LD_TYPE		11 /* B */
#define   OAKLEY_ATTR_SA_LD_TYPE_DEFAULT	1
#define   OAKLEY_ATTR_SA_LD_TYPE_SEC		1
#define   OAKLEY_ATTR_SA_LD_TYPE_KB		2
#define   OAKLEY_ATTR_SA_LD_TYPE_MAX		3
					/*	65001 - 65535 Private Use */
#define OAKLEY_ATTR_SA_LD		12 /* V */
#define   OAKLEY_ATTR_SA_LD_SEC_DEFAULT		28800 /* 8 hours */
#define OAKLEY_ATTR_PRF			13 /* B */
#define OAKLEY_ATTR_KEY_LEN		14 /* B */
#define OAKLEY_ATTR_FIELD_SIZE		15 /* B */
#define OAKLEY_ATTR_GRP_ORDER		16 /* V */
#define OAKLEY_ATTR_BLOCK_SIZE		17 /* B */
				/*	16384 - 32767 Private Use */

	/*
	 * The following are valid when the Vendor ID is one of
	 * the following:
	 *
	 *	MD5("A GSS-API Authentication Method for IKE")
	 *	MD5("GSSAPI") (recognized by Windows 2000)
	 *	MD5("MS NT5 ISAKMPOAKLEY") (sent by Windows 2000)
	 */
#define OAKLEY_ATTR_GSS_ID		16384

#define MAXPADLWORD	20


/* XXX */
#define OUTBOUND_SA 0
#define INBOUND_SA 1

struct saproto;
struct saprop;

struct oakley_keymat {
	int (*func)(struct oakley_keymat *obj,
	    struct saproto *pr, size_t octet);
	void *tag;
	int side;
	int sa_dir;
};

struct oakley_prf {
	rc_vchar_t *(*func)(struct oakley_prf *obj, rc_vchar_t *src);
	void *tag;
};

int oakley_compute_keymats(struct oakley_keymat *keymat, struct saprop *pp);
rc_vchar_t *oakley_compute_expanded_keymat(struct oakley_prf *prf,
    size_t octet, rc_vchar_t *src);
