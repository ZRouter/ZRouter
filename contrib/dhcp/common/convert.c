/* convert.c

   Safe copying of option values into and out of the option buffer, which
   can't be assumed to be aligned. */

/*
 * Copyright (c) 1995, 1996 The Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: convert.c,v 1.4.2.1 1998/06/25 21:11:28 mellon Exp $ Copyright (c) 1995, 1996 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

u_int32_t getULong (buf)
	unsigned char *buf;
{
	unsigned long ibuf;

	memcpy (&ibuf, buf, sizeof (u_int32_t));
	return ntohl (ibuf);
}

int32_t getLong (buf)
	unsigned char *buf;
{
	long ibuf;

	memcpy (&ibuf, buf, sizeof (int32_t));
	return ntohl (ibuf);
}

u_int16_t getUShort (buf)
	unsigned char *buf;
{
	unsigned short ibuf;

	memcpy (&ibuf, buf, sizeof (u_int16_t));
	return ntohs (ibuf);
}

int16_t getShort (buf)
	unsigned char *buf;
{
	short ibuf;

	memcpy (&ibuf, buf, sizeof (int16_t));
	return ntohs (ibuf);
}

void putULong (obuf, val)
	unsigned char *obuf;
	u_int32_t val;
{
	u_int32_t tmp = htonl (val);
	memcpy (obuf, &tmp, sizeof tmp);
}

void putLong (obuf, val)
	unsigned char *obuf;
	int32_t val;
{
	int32_t tmp = htonl (val);
	memcpy (obuf, &tmp, sizeof tmp);
}

void putUShort (obuf, val)
	unsigned char *obuf;
	unsigned int val;
{
	u_int16_t tmp = htons (val);
	memcpy (obuf, &tmp, sizeof tmp);
}

void putShort (obuf, val)
	unsigned char *obuf;
	int val;
{
	int16_t tmp = htons (val);
	memcpy (obuf, &tmp, sizeof tmp);
}

