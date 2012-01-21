
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
#include <sys/param.h>
#include <sys/stat.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>

#include <openssl/ssl.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "http/http_defs.h"
#include "http/http_server.h"
#include "http/http_servlet.h"
#include "http/servlet/basicauth.h"
#include "util/typed_mem.h"

#define MEM_TYPE		"http_servlet_basicauth"

static http_servlet_run_t	http_servlet_basicauth_run;
static http_servlet_destroy_t	http_servlet_basicauth_destroy;

struct http_servlet_basicauth {
	http_servlet_basicauth_t	*auth;
	void				(*destroy)(void *);
	void				*arg;
};

/*
 * Create a new auth servlet.
 */
struct http_servlet *
http_servlet_basicauth_create(http_servlet_basicauth_t *auth,
	void *arg, void (*destroy)(void *))
{
	struct http_servlet_basicauth *info;
	struct http_servlet *servlet;

	/* Sanity check */
	if (auth == NULL) {
		errno = EINVAL;
		return (NULL);
	}

	/* Create servlet */
	if ((servlet = MALLOC(MEM_TYPE, sizeof(*servlet))) == NULL)
		return (NULL);
	memset(servlet, 0, sizeof(*servlet));
	servlet->run = http_servlet_basicauth_run;
	servlet->destroy = http_servlet_basicauth_destroy;

	/* Add info */
	if ((info = MALLOC(MEM_TYPE, sizeof(*info))) == NULL) {
		FREE(MEM_TYPE, servlet);
		return (NULL);
	}
	memset(info, 0, sizeof(*info));
	info->auth = auth;
	info->arg = arg;
	info->destroy = destroy;
	servlet->arg = info;

	/* Done */
	return (servlet);
}

/*
 * Execute authorization servlet.
 */
static int
http_servlet_basicauth_run(struct http_servlet *servlet,
	struct http_request *req, struct http_response *resp)
{
	struct http_servlet_basicauth *const info = servlet->arg;
	const char *username;
	const char *password;
	const char *realm;

	/* Get username and password */
	if ((username = http_request_get_username(req)) == NULL)
		username = "";
	if ((password = http_request_get_password(req)) == NULL)
		password = "";

	/* Check authorization and return error if it fails */
	if ((realm = (*info->auth)(info->arg,
	    req, username, password)) != NULL) {
		http_response_send_basic_auth(resp, realm);
		return (1);
	}

	/* Continue */
	return (0);
}

/*
 * Destroy an auth servlet.
 */
static void
http_servlet_basicauth_destroy(struct http_servlet *servlet)
{
	struct http_servlet_basicauth *const info = servlet->arg;

	if (info->destroy != NULL)
		(*info->destroy)(info->arg);
	FREE(MEM_TYPE, servlet->arg);
	FREE(MEM_TYPE, servlet);
}

