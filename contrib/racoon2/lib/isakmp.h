/*	$KAME: isakmp.h,v 1.19 2001/04/11 06:11:55 sakane Exp $	*/

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

/* refer to RFC 2408 */

/* Exchange Type */
#define ISAKMP_ETYPE_BASE	1	/* Base */
#define ISAKMP_ETYPE_IDENT	2	/* Identity Proteciton */
#define ISAKMP_ETYPE_AGG	4	/* Aggressive */

/* Certificate Type */
#define ISAKMP_CERT_PKCS7	1
#define ISAKMP_CERT_PGP		2
#define ISAKMP_CERT_DNS		3
#define ISAKMP_CERT_X509SIGN	4
#define ISAKMP_CERT_X509KE	5
#define ISAKMP_CERT_KERBEROS	6
#define ISAKMP_CERT_CRL		7
#define ISAKMP_CERT_ARL		8
#define ISAKMP_CERT_SPKI	9
#define ISAKMP_CERT_X509ATTR	10

