
/*
 * msoft.c
 *
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1998-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "msoft.h"
#include <openssl/sha.h>
#include <openssl/md4.h>
#include <openssl/des.h>

/*
 * This stuff is described in:
 *
 * MS-CHAPv1
 *   http://www.es.net/pub/rfcs/rfc2433.txt
 *
 * MS-CHAPv2
 *   http://www.es.net/pub/rfcs/rfc2759.txt
 *
 * Deriving MPPE keys from MS-CHAPv1 and MS-CHAPv2
 *   http://www.es.net/pub/rfcs/rfc3079.txt
 */

/* Magic constants */
#define MS_MAGIC_1	"This is the MPPE Master Key"
#define MS_MAGIC_2	"On the client side, this is the send key;" \
			" on the server side, it is the receive key."
#define MS_MAGIC_3	"On the client side, this is the receive key;" \
			" on the server side, it is the send key."
#define MS_AR_MAGIC_1	"Magic server to client signing constant"
#define MS_AR_MAGIC_2	"Pad to make it do more than one iteration"

/*
 * INTERNAL FUNCTIONS
 */

  static void	ChallengeResponse(const u_char *chal,
			const char *pwHash, u_char *hash);
  static void	DesEncrypt(const u_char *clear, u_char *key0, u_char *cypher);
  static void	ChallengeHash(const u_char *peerchal, const u_char *authchal,
			const char *username, u_char *hash);

/*
 * LMPasswordHash()
 *
 * password	ASCII password
 * hash		16 byte output LanManager hash
 */

void
LMPasswordHash(const char *password, u_char *hash)
{
  const u_char	*const clear = (u_char *) "KGS!@#$%%";
  u_char	up[14];		/* upper case password */
  int		k;

  memset(&up, 0, sizeof(up));
  for (k = 0; k < sizeof(up) && password[k]; k++)
    up[k] = toupper(password[k]);

  DesEncrypt(clear, &up[0], &hash[0]);
  DesEncrypt(clear, &up[7], &hash[8]);
}

/*
 * NTPasswordHash()
 *
 * password	ASCII (NOT Unicode) password
 * hash		16 byte output NT hash
 */

void
NTPasswordHash(const char *password, u_char *hash)
{
  u_int16_t	unipw[128];
  int		unipwLen;
  MD4_CTX	md4ctx;
  const char	*s;

/* Convert password to Unicode */

  for (unipwLen = 0, s = password; unipwLen < sizeof(unipw) / 2 && *s; s++)
    unipw[unipwLen++] = htons(*s << 8);

/* Compute MD4 of Unicode password */

  MD4_Init(&md4ctx);
  MD4_Update(&md4ctx, (u_char *) unipw, unipwLen * sizeof(*unipw));
  MD4_Final(hash, &md4ctx);
}

/*
 * NTPasswordHashHash()
 *
 * nthash	16 bytes NT-Hash
 * hash		16 bytes MD4 of NT-hash
 */

void
NTPasswordHashHash(const u_char *nthash, u_char *hash)
{
  MD4_CTX	md4ctx;

  MD4_Init(&md4ctx);
  MD4_Update(&md4ctx, (u_char *) nthash, 16);
  MD4_Final(hash, &md4ctx);
}

/*
 * NTChallengeResponse()
 *
 * chal		8 byte challenge
 * nthash	NT-Hash
 * hash		24 byte response
 */

void
NTChallengeResponse(const u_char *chal, const char *nthash, u_char *hash)
{
  ChallengeResponse(chal, nthash, hash);
}

/*
 * ChallengeResponse()
 *
 * chal		8 byte challenge
 * pwHash	16 byte password hash
 * hash		24 byte response
 */

static void
ChallengeResponse(const u_char *chal, const char *pwHash, u_char *hash)
{
  u_char	buf[21];
  int		k;

  memset(&buf, 0, sizeof(buf));
  memcpy(buf, pwHash, 16);

/* Use DES to hash the hash */

  for (k = 0; k < 3; k++)
  {
    u_char	*const key = &buf[k * 7];
    u_char	*const output = &hash[k * 8];

    DesEncrypt(chal, key, output);
  }
}

/*
 * DesEncrypt()
 *
 * clear	8 byte cleartext
 * key		7 byte key
 * cypher	8 byte cyphertext
 */

static void
DesEncrypt(const u_char *clear, u_char *key0, u_char *cypher)
{
  des_key_schedule	ks;
  u_char		key[8];

/* Create DES key */

  key[0] = key0[0] & 0xfe;
  key[1] = (key0[0] << 7) | (key0[1] >> 1);
  key[2] = (key0[1] << 6) | (key0[2] >> 2);
  key[3] = (key0[2] << 5) | (key0[3] >> 3);
  key[4] = (key0[3] << 4) | (key0[4] >> 4);
  key[5] = (key0[4] << 3) | (key0[5] >> 5);
  key[6] = (key0[5] << 2) | (key0[6] >> 6);
  key[7] = key0[6] << 1;
  des_set_key((des_cblock *) key, ks);

/* Encrypt using key */

  des_ecb_encrypt((des_cblock *) clear, (des_cblock *) cypher, ks, 1);
}

/*
 * MsoftGetStartKey()
 */

void
MsoftGetStartKey(u_char *chal, u_char *h)
{
  SHA_CTX	c;
  u_char	hash[20];

  SHA1_Init(&c);
  SHA1_Update(&c, h, 16);
  SHA1_Update(&c, h, 16);
  SHA1_Update(&c, chal, 8);
  SHA1_Final(hash, &c);
  memcpy(h, hash, 16);
}

/*
 * GenerateNTResponse()
 *
 * authchal	16 byte authenticator challenge
 * peerchal	16 byte peer challenge
 * username	ASCII username
 * nthash	NT-Hash
 * hash		24 byte response
 */

void
GenerateNTResponse(const u_char *authchal, const u_char *peerchal,
  const char *username, const char *nthash, u_char *hash)
{
  u_char	chal[8];

  ChallengeHash(peerchal, authchal, username, chal);
  ChallengeResponse(chal, nthash, hash);
}

/*
 * ChallengeHash()
 *
 * peerchal	16 byte peer challenge
 * authchal	16 byte authenticator challenge
 * username	ASCII username
 * hash		8 byte response
 */

static void
ChallengeHash(const u_char *peerchal, const u_char *authchal,
  const char *username, u_char *hash)
{
  SHA_CTX	c;
  u_char	digest[20];

  SHA1_Init(&c);
  SHA1_Update(&c, peerchal, 16);
  SHA1_Update(&c, authchal, 16);
  SHA1_Update(&c, username, strlen(username));
  SHA1_Final(digest, &c);
  memcpy(hash, digest, 8);
}

/*
 * Generate response to MS-CHAPv2 piggy-backed challenge.
 *
 * "authresp" must point to a 20 byte buffer.
 */
void
GenerateAuthenticatorResponse(const u_char *nthash,
  const u_char *ntresp, const u_char *peerchal,
  const u_char *authchal, const char *username, u_char *authresp)
{
  u_char hash[16];
  u_char digest[SHA_DIGEST_LENGTH];
  u_char chal[8];
  MD4_CTX md4ctx;
  SHA_CTX shactx;

  MD4_Init(&md4ctx);
  MD4_Update(&md4ctx, nthash, 16);
  MD4_Final(hash, &md4ctx);

  SHA1_Init(&shactx);
  SHA1_Update(&shactx, hash, 16);
  SHA1_Update(&shactx, ntresp, 24);
  SHA1_Update(&shactx, MS_AR_MAGIC_1, 39);
  SHA1_Final(digest, &shactx);

  ChallengeHash(peerchal, authchal, username, chal);

  SHA1_Init(&shactx);
  SHA1_Update(&shactx, digest, sizeof(digest));
  SHA1_Update(&shactx, chal, 8);
  SHA1_Update(&shactx, MS_AR_MAGIC_2, 41);
  SHA1_Final(authresp, &shactx);
}

/*
 * MsoftGetMasterKey()
 */

void
MsoftGetMasterKey(u_char *resp, u_char *h)
{
  SHA_CTX	c;
  u_char	hash[20];

  SHA1_Init(&c);
  SHA1_Update(&c, h, 16);
  SHA1_Update(&c, resp, 24);
  SHA1_Update(&c, MS_MAGIC_1, 27);
  SHA1_Final(hash, &c);
  memcpy(h, hash, 16);
}

/*
 * MsoftGetAsymetricStartKey()
 */

void
MsoftGetAsymetricStartKey(u_char *h, int server_recv)
{
  SHA_CTX		c;
  u_char		pad[40];
  u_char		hash[20];

  SHA1_Init(&c);
  SHA1_Update(&c, h, 16);
  memset(pad, 0x00, sizeof(pad));
  SHA1_Update(&c, pad, sizeof(pad));
  SHA1_Update(&c, server_recv ? MS_MAGIC_2 : MS_MAGIC_3, 84);
  memset(pad, 0xf2, sizeof(pad));
  SHA1_Update(&c, pad, sizeof(pad));
  SHA1_Final(hash, &c);
  memcpy(h, hash, 16);
}

