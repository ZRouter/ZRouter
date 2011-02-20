
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
#include <sys/queue.h>

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>

#include <openssl/ssl.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "http/http_defs.h"
#include "http/http_server.h"
#include "http/http_internal.h"
#include "util/typed_mem.h"

#define MAX_NVP_LEN		(8 * 1024)
#define MAX_NVP_DATA		(64 * 1024)

/*
 * Error strings
 */
#define PARSE_REQUST_MSG	"Error parsing request header"
#define REQUEST_TIMEOUT_MSG 	"Press reload to reestablish a connection"

/*
 * Internal functions
 */
static int	http_request_decode_query(struct http_request *req);
static int	nvp_cmp(const void *v1, const void *v2);
static char	*read_string_until(const char *mtype,
			FILE *fp, const char *term);
static int	http_request_add_nvp(struct http_request *req, int encoded,
			const char *ename, const char *evalue);
static void	http_request_decode_auth(struct http_request *req);

/*
 * Internal variables
 */
static const	char base64[65]
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*********************************************************************
			MAIN ROUTINES
*********************************************************************/

/*
 * Create a new request structure.
 */
int
_http_request_new(struct http_connection *conn)
{
	struct http_request *req;

	/* Create request structure */
	assert(conn->req == NULL);
	if ((req = MALLOC("http_request", sizeof(*req))) == NULL) {
		(*conn->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		return (-1);
	}
	memset(req, 0, sizeof(*req));

	/* Attach message structure */
	if ((req->msg = _http_message_new()) == NULL) {
		(*conn->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		FREE("http_request", req);
		return (-1);
	}

	/* Link it up */
	req->msg->conn = conn;
	conn->req = req;
	return (0);
}

/*
 * Read in, validate, and prep an HTTP request from a connection.
 *
 * If there is a problem, load an error response page if appropriate.
 */
int
_http_request_read(struct http_connection *conn)
{
	struct http_request *const req = conn->req;
	struct http_response *const resp = conn->resp;
	struct http_message *const msg = req->msg;
	const char *path = NULL;
	const char *method;
	const char *colon;
	const char *slash;
	const char *proto;
	const char *uri;
	const char *s;
	char scheme[32];
	int plen = -1;
	int hlen;

	/* Load in message from HTTP connection */
	if (_http_message_read(msg, 1) == -1) {
		const int errno_save = errno;

		/* If nothing read, the peer must have closed the connection */
		if (!_http_message_has_anything(msg)) {
			errno = ENOTCONN;
			return (-1);
		}

		/* Return an appropriate error message */
		switch (errno) {
		case EINVAL:
			http_response_send_error(resp, HTTP_STATUS_BAD_REQUEST,
			    PARSE_REQUST_MSG);
			break;

		case ETIMEDOUT:
			http_response_send_error(resp,
			    HTTP_STATUS_REQUEST_TIME_OUT, REQUEST_TIMEOUT_MSG);
			break;

		default:
			http_response_send_error(resp,
			    HTTP_STATUS_INTERNAL_SERVER_ERROR,
			    "%s", strerror(errno));
			break;
		}

		/* in all cases return -1 */
		errno = errno_save;
		return (-1);
	}

	/* Get method, URI, and protocol */
	method = _http_head_get(msg->head, HDR_REQUEST_METHOD);
	uri = _http_head_get(msg->head, HDR_REQUEST_URI);
	proto = _http_head_get(msg->head, HDR_REQUEST_VERSION);

	/* Check method and reset input length if not POST */
	if (strcmp(method, HTTP_METHOD_GET) == 0
	    || strcmp(method, HTTP_METHOD_HEAD) == 0
	    || strcmp(method, HTTP_METHOD_POST) == 0
	    || strcmp(method, HTTP_METHOD_CONNECT) == 0)
		;						/* ok */
	else if (strcmp(method, HTTP_METHOD_OPTIONS) == 0
	    || strcmp(method, HTTP_METHOD_PUT) == 0
	    || strcmp(method, HTTP_METHOD_DELETE) == 0
	    || strcmp(method, HTTP_METHOD_TRACE) == 0) {
		http_response_send_error(resp,
		    HTTP_STATUS_METHOD_NOT_ALLOWED, NULL);
		return (-1);
	} else {
		http_response_send_error(resp,
		    HTTP_STATUS_NOT_IMPLEMENTED, NULL);
		return (-1);
	}

	/* If request was HEAD, omit sending the body */
	if (strcasecmp(method, HTTP_METHOD_HEAD) == 0)
		resp->msg->skip_body = 1;

	/* Check protocol is known */
	if (strcmp(proto, HTTP_PROTO_0_9) != 0
	    && strcmp(proto, HTTP_PROTO_1_0) != 0
	    && strcmp(proto, HTTP_PROTO_1_1) != 0) {
		http_response_send_error(resp,
		    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED, NULL);
		return (-1);
	}

	/* If protocol is HTTP/0.9, don't send back any headers */
	if (strcmp(proto, HTTP_PROTO_0_9) == 0)
		resp->msg->no_headers = 1;

	/* Check for CONNECT method */
	if (strcmp(method, HTTP_METHOD_CONNECT) == 0) {
		if ((msg->host = STRDUP("http_message.host", uri)) == NULL)
			goto malloc_fail;
		conn->proxy = 1;
		goto no_path;
	}

	/* Get explicit scheme, if any */
	colon = strchr(uri, ':');
	slash = strchr(uri, '/');
	if (colon != NULL && slash != NULL && colon < slash) {
		char *c;

		strlcpy(scheme, uri, sizeof(scheme));
		if ((c = strchr(scheme, ':')) != NULL)
			*c = '\0';
		conn->proxy = 1;	/* explicit scheme => proxy request */
	} else
		strlcpy(scheme, "http", sizeof(scheme));

	/* We only support HTTP */
	if (strcasecmp(scheme, "http") != 0) {
		http_response_send_error(resp, HTTP_STATUS_BAD_REQUEST,
		    "Unsupported scheme \"%s\"", scheme);
		return (-1);
	}

	/* Separate out host, path and query parts from URI */
	if (strncasecmp(uri, "http://", 7) == 0) {
		if ((path = strchr(uri + 7, '/')) == NULL) {
			http_response_send_error(resp, HTTP_STATUS_BAD_REQUEST,
			    "Request URI has no path component");
			return (-1);
		}
		hlen = path - (uri + 7);
		if ((msg->host = MALLOC("http_message.host", hlen + 1)) == NULL)
			goto malloc_fail;
		memcpy(msg->host, uri + 7, hlen);
		msg->host[hlen] = '\0';
		conn->proxy = 1;
	} else if (*uri != '/') {
		http_response_send_error(resp,
		    HTTP_STATUS_BAD_REQUEST, "Bogus non-absolute URI");
		return (-1);
	} else
		path = uri;
	if ((s = strchr(path, '?')) == NULL)
		plen = strlen(path);
	else {
		plen = s - path;
		if ((msg->query = STRDUP("http_message.query", s + 1)) == NULL)
			goto malloc_fail;
	}

no_path:
	/* Check body input length for POST, all others have no input body */
	if (strcmp(method, HTTP_METHOD_POST) == 0) {
		if (msg->input_len == UINT_MAX
		    && _http_head_want_keepalive(req->msg->head)) {
			http_response_send_error(resp, HTTP_STATUS_BAD_REQUEST,
			    "Keep-Alive requires %s",
			    HTTP_HEADER_CONTENT_LENGTH);
			return (-1);
		}
	} else
		msg->input_len = 0;			/* ignore C-L header */

	/* Decode URL path part */
	if (path == NULL)
		goto no_path2;
	if ((msg->path = MALLOC("http_message.path", plen + 1)) == NULL) {
malloc_fail:
		(*conn->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		http_response_send_error(resp,
		    HTTP_STATUS_INTERNAL_SERVER_ERROR, "%s", strerror(errno));
		return (-1);
	}
	memcpy(msg->path, path, plen);
	msg->path[plen] = '\0';
	http_request_url_decode(msg->path, msg->path);

no_path2:
	/* Check Host: header specified in a HTTP/1.1 request */
	if (strcmp(proto, HTTP_PROTO_1_1) == 0
	    && msg->host == NULL
	    && _http_head_get(msg->head, HTTP_HEADER_HOST) == NULL) {
		http_response_send_error(resp,
		    HTTP_STATUS_BAD_REQUEST, "No host specified");
		return (-1);
	}

	/* Decode name, value pairs from query string */
	if (msg->query != NULL)
		http_request_decode_query(req);

	/* Done */
	return (0);
}

/*
 * Free a request structure.
 */
void
_http_request_free(struct http_request **reqp)
{
	struct http_request *const req = *reqp;
	int i;

	if (req == NULL)
		return;
	_http_message_free(&req->msg);
	FREE("http_request.username", req->username);
	FREE("http_request.password", req->password);
	for (i = 0; i < req->num_nvp; i++) {
		struct http_nvp *const nvp = &req->nvp[i];

		FREE("http_request.nvp.name", nvp->name);
		FREE("http_request.nvp.value", nvp->value);
	}
	FREE("http_request.nvp", req->nvp);
	FREE("http_request", req);
	*reqp = NULL;
}

/*********************************************************************
			MESSAGE WRAPPER ROUTINES
*********************************************************************/

/*
 * Get a request header.
 *
 * For headers listed multiple times, this only gets the first instance.
 */
const char *
http_request_get_header(struct http_request *req, const char *name)
{
	return (_http_head_get(req->msg->head, name));
}

/*
 * Get the number of headers
 */
int
http_request_num_headers(struct http_request *req)
{
	return (_http_head_num_headers(req->msg->head));
}

/*
 * Get header by index.
 */
int
http_request_get_header_by_index(struct http_request *req,
	u_int index, const char **namep, const char **valuep)
{
	return (_http_head_get_by_index(req->msg->head, index, namep, valuep));
}

/*
 * Set a request header.
 */
int
http_request_set_header(struct http_request *req, int append,
	const char *name, const char *valfmt, ...)
{
	va_list args;
	int ret;

	/* Set header */
	va_start(args, valfmt);
	ret = _http_message_vset_header(req->msg, append, name, valfmt, args);
	va_end(args);
	return (ret);
}

/*
 * Remove a header.
 */
int
http_request_remove_header(struct http_request *req, const char *name)
{
	return (_http_message_remove_header(req->msg, name));
}

/*
 * Send request headers to peer.
 */
int
http_request_send_headers(struct http_request *req)
{
	const char *const method = http_request_get_method(req);
	const char *const path = http_request_get_path(req);
	const char *const uri = http_request_get_uri(req);
	char *epath;

	/* Sanity checks */
	if (req->msg->hdrs_sent)
		return (0);
	if (method == NULL || (path == NULL && uri == NULL)) {
		errno = EINVAL;
		return (-1);
	}

	/* If request is GET, put name, value pairs in the query string */
	if (strcmp(method, HTTP_METHOD_GET) == 0
	    && http_request_set_query_from_values(req) == -1)
		return (-1);

	/* Set request URI */
	if (uri == NULL) {
		if ((epath = http_request_url_encode(
		    TYPED_MEM_TEMP, path)) == NULL)
			return (-1);
		if (http_request_set_header(req, 0,
		    HDR_REQUEST_URI, "%s%s%s", epath,
		    req->msg->query != NULL ? "?" : "",
		    req->msg->query != NULL ? req->msg->query : "") == -1) {
			FREE(TYPED_MEM_TEMP, epath);
			return (-1);
		}
		FREE(TYPED_MEM_TEMP, epath);
	}

	/* Set other headers */
	if (http_request_set_header(req, 0, HDR_REQUEST_METHOD,
	    "%s", http_request_get_method(req)) == -1)
		return (-1);
	if (http_request_set_header(req, 0, HDR_REQUEST_VERSION,
	    "%s", HTTP_PROTO_1_0) == -1)
		return (-1);
	if (http_request_set_header(req, 0,
	    _http_message_connection_header(req->msg), "%s",
	    req->msg->conn->keep_alive ? "Keep-Alive" : "Close") == -1)
		return (-1);

	/* Send headers */
	_http_message_send_headers(req->msg, 0);
	return (0);
}

/*
 * Get the encoded query string.
 */
const char *
http_request_get_query_string(struct http_request *req)
{
	return (req->msg->query == NULL ? "" : req->msg->query);
}

/*
 * Get specified host, if any.
 */
const char *
http_request_get_host(struct http_request *req)
{
	struct http_connection *const conn = req->msg->conn;
	const char *host;

	if (!conn->proxy
	    && (host = http_request_get_header(req, HTTP_HEADER_HOST)) != NULL)
		return (host);
	return (req->msg->host);
}

/*
 * Get query method.
 */
const char *
http_request_get_method(struct http_request *req)
{
	return (http_request_get_header(req, HDR_REQUEST_METHOD));
}

/*
 * Get query URI.
 */
const char *
http_request_get_uri(struct http_request *req)
{
	return (http_request_get_header(req, HDR_REQUEST_URI));
}

/*
 * Get query version.
 */
const char *
http_request_get_version(struct http_request *req)
{
	return (http_request_get_header(req, HDR_REQUEST_VERSION));
}

/*
 * Set query method.
 */
int
http_request_set_method(struct http_request *req, const char *method)
{
	/* Must be GET, POST, or HEAD for now */
	if (strcmp(method, HTTP_METHOD_GET) != 0
	    && strcmp(method, HTTP_METHOD_POST) != 0
	    && strcmp(method, HTTP_METHOD_HEAD) != 0) {
		errno = EINVAL;
		return (-1);
	}

	/* Set method */
	if (http_request_set_header(req, 0,
	    HDR_REQUEST_METHOD, "%s", method) == -1)
		return (-1);

	/* Only POST is allowed to have a body */
	req->msg->no_body = (strcmp(method, HTTP_METHOD_POST) != 0);
	return (0);
}

/*
 * Set proxy request bit.
 */
void
http_request_set_proxy(struct http_request *req, int whether)
{
	struct http_connection *const conn = req->msg->conn;

	conn->proxy = !!whether;
}

/*
 * Get the URL path.
 */
const char *
http_request_get_path(struct http_request *req)
{
	return (req->msg->path);
}

/*
 * Set the URL path.
 */
int
http_request_set_path(struct http_request *req, const char *path)
{
	char *copy;

	if (path == NULL || *path != '/') {
		errno = EINVAL;
		return (-1);
	}
	if ((copy = STRDUP("http_message.path", path)) == NULL)
		return (-1);
	FREE("http_message.path", req->msg->path);
	req->msg->path = copy;
	return (0);
}

/*
 * Get SSL context.
 */
SSL_CTX *
http_request_get_ssl(struct http_request *req)
{
	return (req->msg->conn->ssl);
}

/*
 * Get request input stream.
 */
FILE *
http_request_get_input(struct http_request *req)
{
	if (!req->msg->conn->server) {
		errno = EINVAL;
		return (NULL);
	}
	return (req->msg->input);
}

/*
 * Get raw i/o stream as a file descriptor.
 */
int
http_request_get_raw_socket(struct http_request *req)
{
	return (_http_message_get_raw_socket(req->msg));
}

/*
 * Get request output stream.
 *
 * If "buffer" is true, the entire output will be buffered unless
 * the headers have already been sent.
 */
FILE *
http_request_get_output(struct http_request *req, int buffer)
{
	if (req->msg->conn->server) {
		errno = EINVAL;
		return (NULL);
	}
	return (_http_message_get_output(req->msg, buffer));
}

/*
 * Get remote IP address.
 */
struct in_addr
http_request_get_remote_ip(struct http_request *req)
{
	return (_http_message_get_remote_ip(req->msg));
}

/*
 * Get remote port.
 */
u_int16_t
http_request_get_remote_port(struct http_request *req)
{
	return (_http_message_get_remote_port(req->msg));
}

/*********************************************************************
			AUTHORIZATION ROUTINES
*********************************************************************/

/*
 * Get request remote username.
 *
 * Returns NULL if none specified.
 */
const char *
http_request_get_username(struct http_request *req)
{
	if (req->username == NULL)
		http_request_decode_auth(req);
	return (req->username);
}

/*
 * Get request remote password
 *
 * Returns NULL if none specified.
 */
const char *
http_request_get_password(struct http_request *req)
{
	if (req->password == NULL)
		http_request_decode_auth(req);
	return (req->password);
}

/*
 * Compute base64 encoded username and password, suitable for
 * passing in a basic authentication header.
 */
char *
http_request_encode_basic_auth(const char *mtype,
	const char *username, const char *password)
{
	char *buf;
	char *auth;
	int len;
	int i, j;

	/* Get raw data buffer */
	len = strlen(username) + 1 + strlen(password) + 1;
	if ((buf = MALLOC(TYPED_MEM_TEMP, len)) == NULL)
		return (NULL);
	strcpy(buf, username);
	strcat(buf, ":");
	strcat(buf, password);

	/* Get encoded buffer */
	len = (4 * len) / 3 + 32;
	if ((auth = MALLOC(mtype, len)) == NULL) {
		FREE(TYPED_MEM_TEMP, buf);
		return (NULL);
	}

	/* Encode bits */
	len = strlen(buf);
	for (j = i = 0; i < len; i += 3) {
		const u_char b0 = ((u_char *)buf)[i];
		const u_char b1 = (i < len - 1) ? ((u_char *)buf)[i + 1] : 0;
		const u_char b2 = (i < len - 2) ? ((u_char *)buf)[i + 2] : 0;

		auth[j++] = base64[(b0 >> 2) & 0x3f];
		auth[j++] = base64[((b0 << 4) & 0x30) | ((b1 >> 4) & 0x0f)];
		if (i == len - 1)
			break;
		auth[j++] = base64[((b1 << 2) & 0x3c) | ((b2 >> 6) & 0x03)];
		if (i == len - 2)
			break;
		auth[j++] = base64[b2 & 0x3f];
	}
	FREE(TYPED_MEM_TEMP, buf);

	/* Pad encoding to an even multiple with equals signs */
	switch (len % 3) {
	case 1:	auth[j++] = '=';		/* fall through */
	case 2:	auth[j++] = '=';
	case 0:	break;
	}
	auth[j] = '\0';

	/* Done */
	return (auth);
}

/*
 * Decode basic Authorization: header.
 */
static void
http_request_decode_auth(struct http_request *req)
{
	static u_char table[256];
	u_int val, pval = 0;
	const char *s;
	char buf[128];
	char *pw;
	int step;
	int len;

	/* Initialize table (first time only) */
	if (table[0] == 0) {
		for (val = 0; val < 0x100; val++) {
			table[val] = ((s = strchr(base64, (char)val)) != NULL) ?
			    s - base64 : 0xff;
		}
	}

	/* Get basic authentication header */
	if ((s = http_request_get_header(req,
	      HTTP_HEADER_AUTHORIZATION)) == NULL
	    || strncmp(s, "Basic ", 6) != 0)
		return;

	/* Decode it */
	for (len = step = 0, s += 6; len < sizeof(buf) - 1 && *s != '\0'; s++) {

		/* Decode character */
		if ((val = table[(u_char)*s]) == 0xff)
			continue;

		/* Glom on bits */
		switch (step % 4) {
		case 1:
			buf[len++] = (pval << 2) | ((val & 0x30) >> 4);
			break;
		case 2:
			buf[len++] = ((pval & 0x0f) << 4) | ((val & 0x3c) >> 2);
			break;
		case 3:
			buf[len++] = ((pval & 0x03) << 6) | val;
			break;
		}
		pval = val;
		step++;
	}
	buf[len] = '\0';

	/* Find password */
	if ((pw = strchr(buf, ':')) == NULL)
		return;
	*pw++ = '\0';

	/* Allocate strings */
	if ((req->username = STRDUP("http_request.username", buf)) == NULL)
		return;
	if ((req->password = STRDUP("http_request.password", pw)) == NULL) {
		FREE("http_request.username", req->username);
		req->username = NULL;
		return;
	}
}

/*********************************************************************
			FORM FILE UPLOAD
*********************************************************************/

#define WHITESPACE		" \t\r\n\f\v"

struct upload_info {
	const char	*field;
	FILE		*fp;
	int		error;
	size_t		max;
};

static http_mime_handler_t http_request_file_upload_handler;

/*
 * Read an HTTP POST containing MIME multipart data and write
 * the contents of the named field into the supplied stream.
 * The stream is NOT closed.
 *
 * If "max" is non-zero and more than "max" bytes are read,
 * an error is returned with errno = EFBIG.
 *
 * Returns zero if successful, otherwise -1 and sets errno.
 */
int
http_request_file_upload(struct http_request *req,
	const char *field, FILE *fp, size_t max)
{
	struct upload_info info;
	const char *hval;
	char *s;

	/* Verify proper submit was done */
	if ((hval = http_request_get_header(req,
	    HTTP_HEADER_CONTENT_TYPE)) == NULL) {
		errno = EINVAL;
		return (-1);
	}
	if ((s = strchr(hval, ';')) == NULL
	    || strncasecmp(hval,
	      HTTP_CTYPE_MULTIPART_FORMDATA, s - hval) != 0) {
		errno = EINVAL;
		return (-1);
	}

	/* Read form data into file */
	memset(&info, 0, sizeof(info));
	info.field = field;
	info.fp = fp;
	info.max = max;
	if (http_request_get_mime_multiparts(req,
	    http_request_file_upload_handler, &info) < 0) {
		if (info.error == 0)
			info.error = errno;
	}
	if (info.error != 0) {
		errno = info.error;
		return (-1);
	}

	/* Done */
	return (0);
}

static int
http_request_file_upload_handler(void *arg, struct mime_part *part, FILE *fp)
{
	struct upload_info *const info = arg;
	const char *hval = http_mime_part_get_header(part,
	    HTTP_HEADER_CONTENT_DISPOSITION);
	char buf[256];
	char *tokctx;
	char *s, *t;
	size_t len;
	size_t nr;

	/* Parse content dispostion to get the field name */
	strlcpy(buf, hval, sizeof(buf));
	for (s = strtok_r(buf, WHITESPACE ";", &tokctx);
	    s != NULL;
	    s = strtok_r(NULL, WHITESPACE ";", &tokctx)) {
		if (strncmp(s, "name=\"", 6) != 0)
			continue;
		s += 6;
		if ((t = strchr(s, '"')) == NULL)
			continue;
		*t = '\0';
		if (strcmp(s, info->field) == 0)	/* is this the field? */
			break;
	}
	if (s == NULL)					/* wrong field */
		return (0);

	/* Read data and write it to caller-supplied stream */
	for (len = 0; (nr = fread(buf, 1, sizeof(buf), fp)) != 0; len += nr) {
		if (info->max != 0 && len + nr > info->max) {
			errno = EFBIG;
			goto fail;
		}
		if (fwrite(buf, 1, nr, info->fp) != nr)
			goto fail;
	}
	if (ferror(fp)) {
fail:		info->error = errno;
		return (-1);
	}

	/* Done */
	return (0);
}

/*********************************************************************
			NAME VALUE PAIR ROUTINES
*********************************************************************/

/*
 * Get a value from a request.
 */
const char *
http_request_get_value(struct http_request *req, const char *name, int instance)
{
	struct http_nvp key;
	struct http_nvp *nvp;

	key.name = (char *)name;
	if ((nvp = bsearch(&key,
	    req->nvp, req->num_nvp, sizeof(*req->nvp), nvp_cmp)) == NULL)
		return (NULL);
	while (instance-- > 0) {
		if (++nvp - req->nvp >= req->num_nvp
		    || strcmp(nvp->name, name) != 0)
			return (NULL);
	}
	return (nvp->value);
}

/*
 * Set a value associated with a query.
 */
int
http_request_set_value(struct http_request *req,
	const char *name, const char *value)
{
	int ret;

	if ((ret = http_request_add_nvp(req, 0, name, value)) == -1)
		return (-1);
	mergesort(req->nvp, req->num_nvp, sizeof(*req->nvp), nvp_cmp);
	return (0);
}

/*
 * Get the number of NVP's.
 */
int
http_request_get_num_values(struct http_request *req)
{
	return (req->num_nvp);
}

/*
 * Get the name of an NVP by index.
 */
int
http_request_get_value_by_index(struct http_request *req, int i,
	const char **name, const char **value)
{
	if (i < 0 || i >= req->num_nvp) {
		errno = EINVAL;
		return (-1);
	}
	if (name != NULL)
		*name = req->nvp[i].name;
	if (value != NULL)
		*value = req->nvp[i].value;
	return (0);
}

/*
 * Reset the query string from the list of name, value pairs.
 */
int
http_request_set_query_from_values(struct http_request *req)
{
	char *buf;
	int qlen;
	int i;

	/* If no values, no query string */
	if (req->num_nvp == 0) {
		buf = NULL;
		goto done;
	}

	/* Get bound on encoded query string length */
	for (qlen = i = 0; i < req->num_nvp; i++) {
		const struct http_nvp *const nvp = &req->nvp[i];

		qlen += strlen(nvp->name) * 3 + 1 + strlen(nvp->value) * 3;
	}

	/* Allocate new buffer */
	if ((buf = MALLOC("http_message.query", qlen + 1)) == NULL)
		return (-1);

	/* Encode name, value pairs into buffer */
	for (qlen = i = 0; i < req->num_nvp; i++) {
		const struct http_nvp *const nvp = &req->nvp[i];
		char *ename;
		char *evalue;

		if ((ename = http_request_url_encode(TYPED_MEM_TEMP,
		    nvp->name)) == NULL) {
			FREE("http_message.query", buf);
			return (-1);
		}
		if ((evalue = http_request_url_encode(TYPED_MEM_TEMP,
		    nvp->value)) == NULL) {
			FREE(TYPED_MEM_TEMP, ename);
			FREE("http_message.query", buf);
			return (-1);
		}
		qlen += sprintf(buf + qlen, "%s%s=%s",
		    i > 0 ? "&" : "", ename, evalue);
		FREE(TYPED_MEM_TEMP, ename);
		FREE(TYPED_MEM_TEMP, evalue);
	}

done:
	/* Set new query string */
	FREE("http_message.query", req->msg->query);
	req->msg->query = buf;
	return (0);
}

/*
 * Read in name, value pairs as URL-encoded form data.
 * Typically this is used for receiving POST requests.
 */
int
http_request_read_url_encoded_values(struct http_request *req)
{
	FILE *input;
	char *name;
	char *value;
	int count;
	int totlen;
	int ret;
	int ch;

	/* Get input stream */
	if ((input = http_request_get_input(req)) == NULL)
		return (-1);

	/* Read in URL-encoded name, value pairs */
	for (count = totlen = 0; totlen < MAX_NVP_DATA; totlen++) {

		/* Read name */
		if ((name = read_string_until(TYPED_MEM_TEMP,
		    req->msg->input, "&=")) == NULL)
			return (-1);

		/* Read value, if any */
		if ((ch = getc(req->msg->input)) == '=') {
			if ((value = read_string_until(TYPED_MEM_TEMP,
			    req->msg->input, "&")) == NULL) {
				FREE(TYPED_MEM_TEMP, name);
				return (-1);
			}
		} else
			value = NULL;

		/* Slurp next char */
		(void)getc(req->msg->input);

		/* Add name, value pair */
		ret = 0;
		if (*name != '\0') {
			ret = http_request_add_nvp(req, 1,
			    name, (value != NULL) ? value : "");
			count++;
		}
		FREE(TYPED_MEM_TEMP, name);
		FREE(TYPED_MEM_TEMP, value);
		if (ret == -1)
			return (-1);
	}

	/* Sort name using mergesort() which preserves order of duplicates */
	if (mergesort(req->nvp, req->num_nvp, sizeof(*req->nvp), nvp_cmp) == -1)
		return (-1);

	/* Done */
	return (count);
}

/*
 * Write out name, value pairs as URL-encoded form data.
 * Typically this is used for sending POST requests.
 */
int
http_request_write_url_encoded_values(struct http_request *req)
{
	int i;

	/* Get output stream */
	if (req->msg->output == NULL) {
		errno = EINVAL;
		return (-1);
	}

	/* Encode name, value pairs into buffer */
	for (i = 0; i < req->num_nvp; i++) {
		const struct http_nvp *const nvp = &req->nvp[i];
		char *ename;
		char *evalue;

		if ((ename = http_request_url_encode(TYPED_MEM_TEMP,
		    nvp->name)) == NULL)
			return (-1);
		if ((evalue = http_request_url_encode(TYPED_MEM_TEMP,
		    nvp->value)) == NULL) {
			FREE(TYPED_MEM_TEMP, ename);
			return (-1);
		}
		fprintf(req->msg->output, "%s%s=%s",
		    i > 0 ? "&" : "", ename, evalue);
		FREE(TYPED_MEM_TEMP, ename);
		FREE(TYPED_MEM_TEMP, evalue);
	}
	return (0);
}

/*
 * Parse out query string into name, value pairs.
 */
static int
http_request_decode_query(struct http_request *req)
{
	char *tokctx;
	char *qbuf;
	char *name;
	char *value;

	/* Sanity */
	if (req->msg->query == NULL)
		return (0);

	/* Copy original encoded query string */
	if ((qbuf = STRDUP(TYPED_MEM_TEMP, req->msg->query)) == NULL)
		return (-1);

	/* Separate out and decode name, value pairs */
	for (name = strtok_r(qbuf, "&", &tokctx);
	    name != NULL;
	    name = strtok_r(NULL, "&", &tokctx)) {

		/* Find value */
		if ((value = strchr(name, '=')) == NULL)
			continue;
		*value++ = '\0';

		/* Add name, value pair */
		if (http_request_add_nvp(req, 1, name, value) == -1) {
			FREE(TYPED_MEM_TEMP, qbuf);
			return (-1);
		}
	}
	FREE(TYPED_MEM_TEMP, qbuf);

	/* Sort name using mergesort() which preserves order of duplicates */
	if (mergesort(req->nvp, req->num_nvp, sizeof(*req->nvp), nvp_cmp) == -1)
		return (-1);
	return (0);
}

/*
 * Add an (optionally URL-encoded) name, value pair.
 *
 * Array must be sorted again by caller.
 */
static int
http_request_add_nvp(struct http_request *req, int encoded,
	const char *ename, const char *evalue)
{
	char *name;
	char *value;
	void *mem;

	/* Create buffers */
	if ((name = MALLOC("http_request.nvp.name", strlen(ename) + 1)) == NULL)
		return (-1);
	if ((value = MALLOC("http_request.nvp.value",
	    strlen(evalue) + 1)) == NULL) {
		FREE("http_request.nvp.name", name);
		return (-1);
	}

	/* URL-decode name and value */
	if (encoded) {
		http_request_url_decode(ename, name);
		http_request_url_decode(evalue, value);
	} else {
		strcpy(name, ename);
		strcpy(value, evalue);
	}

	/* Add new pointer struct */
	if ((mem = REALLOC("http_request.nvp", req->nvp,
	    (req->num_nvp + 1) * sizeof(*req->nvp))) == NULL) {
		FREE("http_request.nvp.name", name);
		FREE("http_request.nvp.value", value);
		return (-1);
	}
	req->nvp = mem;
	req->nvp[req->num_nvp].name = name;
	req->nvp[req->num_nvp].value = value;
	req->num_nvp++;
	return (0);
}

/*
 * Comparator for name, value pairs
 */
static int
nvp_cmp(const void *v1, const void *v2)
{
	const struct http_nvp *const q1 = v1;
	const struct http_nvp *const q2 = v2;

	return (strcmp(q1->name, q2->name));
}

/*
 * Read a string until a terminating character or EOF is seen.
 */
static char *
read_string_until(const char *mtype, FILE *fp, const char *term)
{
	size_t alloc = 32;
	void *mem;
	char *s;
	int len = 0;
	int ch;

	if ((s = MALLOC(mtype, alloc)) == NULL)
		return (NULL);
	while (len < MAX_NVP_LEN) {
		if ((ch = getc(fp)) == EOF) {
			s[len] = '\0';
			return (s);
		}
		if (strchr(term, ch) != NULL && ch != '\0') {
			ungetc(ch, fp);
			s[len] = '\0';
			return (s);
		}
		s[len++] = ch;
		if (len == alloc) {
			alloc <<= 1;
			if ((mem = REALLOC(mtype, s, alloc)) == NULL)
				goto fail;
			s = mem;
		}
	}
	errno = E2BIG;
fail:
	FREE(mtype, s);
	return (NULL);
}

/*********************************************************************
			MISC ROUTINES
*********************************************************************/

/*
 * URL-encode a string.
 */
char *
http_request_url_encode(const char *mtype, const char *s)
{
	char *enc;
	char *t;

	if ((enc = MALLOC(mtype, (strlen(s) * 3) + 1)) == NULL)
		return (NULL);
	for (t = enc; *s != '\0'; s++) {
		if (!isalnum((u_char)*s) && strchr("-_.!~*'()/:", *s) == NULL)
			t += sprintf(t, "%%%02x", (u_char)*s);
		else
			*t++ = *s;
	}
	*t = '\0';
	return (enc);
}

/*
 * URL-decode a string.
 */
void
http_request_url_decode(const char *s, char *t)
{
	for (; *s != '\0'; s++, t++) {
		switch (s[0]) {
		case '+':
			*t = ' ';
			break;
		case '%':
			if (isxdigit((u_char)s[1]) && isxdigit((u_char)s[2])) {
				*t = isdigit((u_char)s[1]) ?
				    s[1] - '0' : tolower(s[1]) - 'a' + 10;
				*t <<= 4;
				*t |= isdigit((u_char)s[2]) ?
				    s[2] - '0' : tolower(s[2]) - 'a' + 10;
				s += 2;
				break;
			}
			/* fall through */
		default:
			*t = *s;
			break;
		}
	}
	*t = '\0';
}

/*
 * Parse an HTTP time string.
 *
 * Returns (time_t)-1 if there was an error.
 */
time_t
http_request_parse_time(const char *string)
{
	static const char *fmts[] = {
		HTTP_TIME_FMT_RFC1123,
		HTTP_TIME_FMT_RFC850,
		HTTP_TIME_FMT_CTIME,
	};
	struct tm whentm;
	time_t when;
	int i;

	for (i = 0; i < sizeof(fmts) / sizeof(*fmts); i++) {
		memset(&whentm, 0, sizeof(whentm));
		if (strptime(string, fmts[i], &whentm) != NULL
		    && (when = timegm(&whentm)) != (time_t)-1)
			return (when);
	}
	return ((time_t)-1);
}

