
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

#include <sys/types.h>
#include <sys/syslog.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <netinet/in.h>

#include <openssl/ssl.h>

#include "http/http_defs.h"
#include "http/http_server.h"

struct http_status {
	int		code;
	const char	*msg;
};

static const struct http_status codes[] = {
{ HTTP_STATUS_CONTINUE, "Continue" },
{ HTTP_STATUS_SWITCHING_PROTOCOLS, "Switching Protocols" },
{ HTTP_STATUS_OK, "OK" },
{ HTTP_STATUS_CREATED, "Created" },
{ HTTP_STATUS_ACCEPTED, "Accepted" },
{ HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION, "Non-Authoritative Information" },
{ HTTP_STATUS_NO_CONTENT, "No Content" },
{ HTTP_STATUS_RESET_CONTENT, "Reset Content" },
{ HTTP_STATUS_PARTIAL_CONTENT, "Partial Content" },
{ HTTP_STATUS_MULTIPLE_CHOICES, "Multiple Choices" },
{ HTTP_STATUS_MOVED_PERMANENTLY, "Moved Permanently" },
{ HTTP_STATUS_FOUND, "Found" },
{ HTTP_STATUS_SEE_OTHER, "See Other" },
{ HTTP_STATUS_NOT_MODIFIED, "Not Modified" },
{ HTTP_STATUS_USE_PROXY, "Use Proxy" },
{ HTTP_STATUS_TEMPORARY_REDIRECT, "Temporary Redirect" },
{ HTTP_STATUS_BAD_REQUEST, "Bad Request" },
{ HTTP_STATUS_UNAUTHORIZED, "Authorization Required" },
{ HTTP_STATUS_PAYMENT_REQUIRED, "Payment Required" },
{ HTTP_STATUS_FORBIDDEN, "Forbidden" },
{ HTTP_STATUS_NOT_FOUND, "Not Found" },
{ HTTP_STATUS_METHOD_NOT_ALLOWED, "Method Not Allowed" },
{ HTTP_STATUS_NOT_ACCEPTABLE, "Not Acceptable" },
{ HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED, "Proxy Authentication Required" },
{ HTTP_STATUS_REQUEST_TIME_OUT, "Request Time-out" },
{ HTTP_STATUS_CONFLICT, "Conflict" },
{ HTTP_STATUS_GONE, "Gone" },
{ HTTP_STATUS_LENGTH_REQUIRED, "Length Required" },
{ HTTP_STATUS_PRECONDITION_FAILED, "Precondition Failed" },
{ HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large" },
{ HTTP_STATUS_REQUEST_URI_TOO_LARGE, "Request-URI Too Large" },
{ HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type" },
{ HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE, "Requested Range Not Satisfiable" },
{ HTTP_STATUS_EXPECTATION_FAILED, "Expectation Failed" },
{ HTTP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error" },
{ HTTP_STATUS_NOT_IMPLEMENTED, "Not Implemented" },
{ HTTP_STATUS_BAD_GATEWAY, "Bad Gateway" },
{ HTTP_STATUS_SERVICE_UNAVAILABLE, "Service Unavailable" },
{ HTTP_STATUS_GATEWAY_TIME_OUT, "Gateway Time-out" },
{ HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported" },
};
static const int num_codes = sizeof(codes) / sizeof(*codes);

/*
 * Internal functions
 */
static int	code_cmp(const void *v1, const void *v2);

const char *
http_response_status_msg(int code)
{
	struct http_status key;
	struct http_status *status;
	static char buf[32];

	key.code = code;
	if ((status = bsearch(&key,
	    codes, num_codes, sizeof(*codes), code_cmp)) == NULL) {
		snprintf(buf, sizeof(buf), "Unknown status code %d", code);
		return (buf);
	}
	return (status->msg);
}

static int
code_cmp(const void *v1, const void *v2)
{
	const struct http_status *const s1 = v1;
	const struct http_status *const s2 = v2;

	return (s1->code - s2->code);
}

#if 0

/*
 * Generate the C #define's from the above list.
 */
int
main(int ac, char **av)
{
	const char *prefix = "HTTP_STATUS_";
	char buf[128];
	int i;

	for (i = 0; i < num_codes; i++) {
		const struct http_status *const code = &codes[i];
		int ntabs;
		char *s;
		int j;

		strlcpy(buf, code->msg, sizeof(buf));
		for (s = buf; *s != '\0'; s++) {
			if (isalnum(*s))
				*s = toupper(*s);
			else
				*s = '_';
		}
		printf("#define\t%s%s", prefix, buf);
		ntabs = 6 - (strlen(buf) + strlen(prefix)) / 8;
		for (j = 0; j < ntabs; j++)
			printf("\t");
		printf("%d\n", code->code);
	}
	return (0);
}

#endif
