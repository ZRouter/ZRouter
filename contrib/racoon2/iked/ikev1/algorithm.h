/* $Id: algorithm.h,v 1.5 2008/02/06 05:49:39 mk Exp $ */

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

#ifndef _ALGORITHM_H
#define _ALGORITHM_H

#include "gnuc.h"

/* algorithm class */
enum {
	algclass_ipsec_enc,
	algclass_ipsec_auth,
	algclass_ipsec_comp,
	algclass_isakmp_enc,
	algclass_isakmp_hash,
	algclass_isakmp_dh,
	algclass_isakmp_ameth	/* authentication method. */
#define MAXALGCLASS	7
};

#define ALG_DEFAULT_KEYLEN	64

#define ALGTYPE_NOTHING		0

#if 0
/* algorithm type */
enum algtype {
	algtype_nothing = 0,

	/* enc */
	algtype_des_iv64,
	algtype_des,
	algtype_3des,
	algtype_rc5,
	algtype_idea,
	algtype_cast128,
	algtype_blowfish,
	algtype_3idea,
	algtype_des_iv32,
	algtype_rc4,
	algtype_null_enc,
	algtype_aes,
	algtype_twofish,

	/* ipsec auth */
	algtype_hmac_md5,
	algtype_hmac_sha1,
	algtype_des_mac,
	algtype_kpdk,
	algtype_non_auth,
	algtype_hmac_sha2_256,
	algtype_hmac_sha2_384,
	algtype_hmac_sha2_512,

	/* ipcomp */
	algtype_oui,
	algtype_deflate,
	algtype_lzs,

	/* hash */
	algtype_md5,
	algtype_sha1,
	algtype_tiger,
	algtype_sha2_256,
	algtype_sha2_384,
	algtype_sha2_512,

	/* dh_group */
	algtype_modp768,
	algtype_modp1024,
	algtype_ec2n155,
	algtype_ec2n185,
	algtype_modp1536,
	algtype_modp2048,
	algtype_modp3072,
	algtype_modp4096,
	algtype_modp6144,
	algtype_modp8192,

	/* authentication method. */
	algtype_psk,
	algtype_dsssig,
	algtype_rsasig,
	algtype_rsaenc,
	algtype_rsarev,
	algtype_gssapikrb,
#ifdef ENABLE_HYBRID
	algtype_hybrid_rsa_s,
	algtype_hybrid_dss_s,
	algtype_hybrid_rsa_c,
	algtype_hybrid_dss_c,
	algtype_xauth_psk_s,
	algtype_xauth_psk_c,
	algtype_xauth_rsa_s,
	algtype_xauth_rsa_c,
#endif
};
#endif

struct hmac_algorithm {
	char *name;
	rc_type type;
	int doi;
	caddr_t (*init) (rc_vchar_t *);
	void (*update) (caddr_t, rc_vchar_t *);
	rc_vchar_t *(*final) (caddr_t);
	int (*hashlen) (void);
	rc_vchar_t *(*one) (rc_vchar_t *, rc_vchar_t *);
};

struct hash_algorithm {
	char *name;
	rc_type type;
	int doi;
	caddr_t (*init) (void);
	void (*update) (caddr_t, rc_vchar_t *);
	rc_vchar_t *(*final) (caddr_t);
	int (*hashlen) (void);
	rc_vchar_t *(*one) (rc_vchar_t *);
};

struct enc_algorithm {
	char *name;
	rc_type type;
	int doi;
	int blocklen;
	rc_vchar_t *(*encrypt) (rc_vchar_t *, rc_vchar_t *, rc_vchar_t *);
	rc_vchar_t *(*decrypt) (rc_vchar_t *, rc_vchar_t *, rc_vchar_t *);
	int (*weakkey) (rc_vchar_t *);
	int (*keylen) (int);
};

/* dh group */
struct dh_algorithm {
	char *name;
	rc_type type;
	int doi;
	struct dhgroup *dhgroup;
};

/* ipcomp, auth meth, dh group */
struct misc_algorithm {
	char *name;
	int type;
	int doi;
};

extern int alg_oakley_hashdef_ok (int);
extern int alg_oakley_hashdef_doi (int);
extern int alg_oakley_hashdef_hashlen (int);
extern rc_vchar_t *alg_oakley_hashdef_one (int, rc_vchar_t *);

extern int alg_oakley_hmacdef_doi (int);
extern rc_vchar_t *alg_oakley_hmacdef_one (int, rc_vchar_t *,
					       rc_vchar_t *);

extern int alg_oakley_encdef_ok (int);
extern int alg_oakley_encdef_doi (int);
extern int alg_oakley_encdef_keylen (int, int);
extern int alg_oakley_encdef_blocklen (int);
extern rc_vchar_t *alg_oakley_encdef_decrypt (int, rc_vchar_t *,
						  rc_vchar_t *, rc_vchar_t *);
extern rc_vchar_t *alg_oakley_encdef_encrypt (int, rc_vchar_t *,
						  rc_vchar_t *, rc_vchar_t *);

extern int alg_ipsec_encdef_doi (int);
extern int alg_ipsec_encdef_keylen (int, int);

extern int alg_ipsec_hmacdef_doi (int);
extern int alg_ipsec_hmacdef_hashlen (int);

extern int alg_ipsec_compdef_doi (int);

extern int alg_oakley_dhdef_doi (int);
extern int alg_oakley_dhdef_ok (int);
extern struct dhgroup *alg_oakley_dhdef_group (int);

extern int alg_oakley_authdef_doi (int);

#if 0
extern int default_keylen (int, int);
extern int check_keylen (int, int, int);
#endif
extern int algtype2doi (int, int);
extern int algclass2doi (int);

extern const char *alg_oakley_encdef_name (int);
extern const char *alg_oakley_hashdef_name (int);
extern const char *alg_oakley_dhdef_name (int);
extern const char *alg_oakley_authdef_name (int);

#endif /* _ALGORITHM_H */
