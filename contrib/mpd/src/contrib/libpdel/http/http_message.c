
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
#include <pthread.h>
#include <limits.h>
#include <syslog.h>
#include <errno.h>
#include <ctype.h>

#include <openssl/ssl.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "util/typed_mem.h"
#include "http/http_defs.h"
#include "http/http_server.h"
#include "http/http_internal.h"

/*
 * Internal functions
 */
static int	http_message_input_read(void *cookie, char *buf, int len);
static int	http_message_input_close(void *cookie);
static int	http_message_output_write(void *cookie,
			const char *buf, int len);
static int	http_message_output_close(void *cookie);

/*********************************************************************
			MAIN ROUTINES
*********************************************************************/

/*
 * Create a new message structure.
 */
struct http_message *
_http_message_new(void)
{
	struct http_message *msg;

	if ((msg = MALLOC("http_message", sizeof(*msg))) == NULL)
		return (NULL);
	memset(msg, 0, sizeof(*msg));
	if ((msg->head = _http_head_new()) == NULL) {
		FREE("http_message", msg);
		return (NULL);
	}
	return (msg);
}

/*
 * Initialize a message by loading in HTTP headers from a stream,
 * and set msg->input to be the entity input stream.
 *
 * This reads any Content-Length header and sets msg->input_len
 * accordingly, or UINT_MAX if none found. Messages that by definition
 * have no body must reset msg->input_len to zero.
 */
int
_http_message_read(struct http_message *msg, int req)
{
	struct http_connection *const conn = msg->conn;
	const char *s;

	/* Slurp in HTTP headers */
	if (_http_head_read(msg->head, conn->fp, req) == -1)
		return (-1);

	/* Get content length, if any */
	msg->input_len = UINT_MAX;
	if ((s = _http_head_get(msg->head,
	    HTTP_HEADER_CONTENT_LENGTH)) != NULL) {
		if (sscanf(s, "%u", &msg->input_len) != 1) {
			errno = EINVAL;
			return (-1);
		}
	}

	/* Create input stream */
	if ((msg->input = funopen(msg, http_message_input_read,
	    NULL, NULL, http_message_input_close)) == NULL)
		return (-1);

	/* Debugging */
	if (PDEL_DEBUG_ENABLED(HTTP_HDRS)) {
		DBG(HTTP_HDRS, "dumping %s headers:",
		    msg->conn->server ? "REQUEST" : "RESPONSE");
		_http_head_write(msg->head, stdout);
	}

	/* Done */
	return (0);
}

/*
 * Free a message structure.
 */
void
_http_message_free(struct http_message **msgp)
{
	struct http_message *const msg = *msgp;

	if (msg == NULL)
		return;
	_http_head_free(&msg->head);
	if (msg->input != NULL)
		fclose(msg->input);
	if (msg->output != NULL)
		fclose(msg->output);
	if (msg->output_buf != NULL)
		FREE("http_message.output_buf", msg->output_buf);
	FREE("http_message.query", msg->query);
	FREE("http_message.host", msg->host);
	FREE("http_message.path", msg->path);
	FREE("http_message", msg);
	*msgp = NULL;
}

/*
 * Get message output stream.
 *
 * If "buffer" is true, the entire output will be buffered unless
 * the headers have already been sent.
 */
FILE *
_http_message_get_output(struct http_message *msg, int buffer)
{
	if (msg->output == NULL) {
		if ((msg->output = funopen(msg, NULL,
		    http_message_output_write, NULL,
		    http_message_output_close)) == NULL)
			return (NULL);
		msg->buffered = buffer;
	}
	return (msg->output);
}

/*
 * Get raw i/o stream as a file descriptor.
 */
int
_http_message_get_raw_socket(struct http_message *msg)
{
	if (msg->conn->ssl != NULL) {
		errno = EPROTOTYPE;
		return (-1);
	}
	return (msg->conn->sock);
}

/*
 * Send message headers, if not sent already.
 */
void
_http_message_send_headers(struct http_message *msg, int unbuffer)
{
	/* Do nothing if nothing to do */
	if (msg->hdrs_sent)
		return;

	/* If we're unbuffering message, turn off buffered flag */
	if (!msg->buffered)
		unbuffer = 0;
	else if (unbuffer) {
		if (msg->output != NULL)
			fflush(msg->output);
		msg->buffered = 0;
	}

	/* In buffered mode, set Content-Length header from buffer length.
	   In non-buffed mode, turn off connection keep-alive. If no body,
	   remove content related headers. */
	if (msg->no_body) {
		_http_head_remove(msg->head, HTTP_HEADER_CONTENT_ENCODING);
		_http_head_remove(msg->head, HTTP_HEADER_CONTENT_LENGTH);
		_http_head_remove(msg->head, HTTP_HEADER_CONTENT_TYPE);
		_http_head_remove(msg->head, HTTP_HEADER_LAST_MODIFIED);
	} else if (msg->buffered) {
		if (msg->output != NULL)
			fflush(msg->output);
		_http_head_set(msg->head, 0, HTTP_HEADER_CONTENT_LENGTH,
		    "%u", msg->output_len);
	} else if (_http_head_get(msg->head,
	    HTTP_HEADER_CONTENT_LENGTH) == NULL) {
		_http_head_set(msg->head, 0,
		    _http_message_connection_header(msg), "Close");
	}

	/* Debugging */
	if (PDEL_DEBUG_ENABLED(HTTP_HDRS)) {
		DBG(HTTP_HDRS, "dumping %s headers:",
		    msg->conn->server ? "RESPONSE" : "REQUEST");
		_http_head_write(msg->head, stdout);
	}

	/* Send headers */
	if (!msg->no_headers)
		_http_head_write(msg->head, msg->conn->fp);
	msg->hdrs_sent = 1;

	/* If unbuffering, send and release any output buffered so far */
	if (unbuffer) {
		_http_message_send_body(msg);
		FREE("http_message.output_buf", msg->output_buf);
		msg->output_buf = NULL;
		msg->output_len = 0;
	}
}

/*
 * Send message body.
 */
void
_http_message_send_body(struct http_message *msg)
{
	struct http_connection *const conn = msg->conn;

	/* Flush output stream data */
	if (msg->output != NULL)
		fflush(msg->output);

	/* Send buffered output (if it was buffered) */
	if (msg->output_buf != NULL) {
		if (!msg->skip_body)
			fwrite(msg->output_buf, 1, msg->output_len, conn->fp);
		fflush(conn->fp);
	}
}

/*
 * Set a message header.
 */
int
_http_message_vset_header(struct http_message *msg, int append,
	const char *name, const char *valfmt, va_list args)
{

	/* Check if we already sent the headers */
	if (msg->hdrs_sent) {
		errno = EALREADY;
		return (-1);
	}

	/* Set header */
	return (_http_head_vset(msg->head, append, name, valfmt, args));
}

/*
 * Remove a message header.
 */
int
_http_message_remove_header(struct http_message *msg, const char *name)
{
	return (_http_head_remove(msg->head, name));
}

/*
 * Get the "Connection" header name, which will be either "Connection"
 * or "Proxy-Connection".
 */
const char *
_http_message_connection_header(struct http_message *msg)
{
	struct http_connection *const conn = msg->conn;
	const char *hval;

	if (!conn->proxy
	    || (hval = _http_head_get(msg->head, conn->server ?
	      HDR_REQUEST_VERSION : HDR_REPLY_VERSION)) == NULL
	    || strcmp(hval, HTTP_PROTO_1_1) >= 0)
		return (HTTP_HEADER_CONNECTION);
	return (HTTP_HEADER_PROXY_CONNECTION);
}

/*
 * Get remote IP address.
 */
struct in_addr
_http_message_get_remote_ip(struct http_message *msg)
{
	return (msg->conn->remote_ip);
}

/*
 * Get remote port.
 */
u_int16_t
_http_message_get_remote_port(struct http_message *msg)
{
	return (msg->conn->remote_port);
}

/*
 * Figure out whether anything is in the message at all.
 */
int
_http_message_has_anything(struct http_message *msg)
{
	return (_http_head_has_anything(msg->head));
}

/*********************************************************************
			INPUT STREAM METHODS
*********************************************************************/

/*
 * Read message input stream.
 */
static int
http_message_input_read(void *cookie, char *buf, int len)
{
	struct http_message *const msg = cookie;
	int ret;

	if (msg->input_read == msg->input_len || len < 0)
		return (0);
	if (len > msg->input_len - msg->input_read)
		len = msg->input_len - msg->input_read;
	if ((ret = fread(buf, 1, len, msg->conn->fp)) != len) {
		if (ferror(msg->conn->fp))
			return (-1);
	}
	msg->input_read += ret;
	return (ret);
}

/*
 * Close message input stream.
 */
static int
http_message_input_close(void *cookie)
{
	struct http_message *const msg = cookie;

	msg->input = NULL;
	return (0);
}

/*********************************************************************
			OUTPUT STREAM METHODS
*********************************************************************/

/*
 * Write to message output stream.
 */
static int
http_message_output_write(void *cookie, const char *buf, int len)
{
	struct http_message *const msg = cookie;
	struct http_connection *const conn = msg->conn;
	int totlen;
	int ret;
	void *mem;

	/* Ignore zero length writes */
	if (len == 0)
		return (0);

	/* Check whether to allow an entity body at all */
	if (msg->no_body) {
		errno = EINVAL;
		return (-1);
	}

	/* If not buffered, check if headers have been sent then write data */
	if (!msg->buffered) {
		if (!msg->hdrs_sent)
			_http_message_send_headers(msg, 0);
		if (msg->skip_body)
			return (len);
		if ((ret = fwrite(buf, 1, len, conn->fp)) != len)
			return (-1);
		return (ret);
	}

	/* Expand buffer and write data into it */
	totlen = msg->output_len + len;
	if ((mem = REALLOC("http_message.output_buf",
	    msg->output_buf, totlen)) == NULL) {
		(*conn->logger)(LOG_ERR, "%s: %s", "realloc", strerror(errno));
		return (-1);
	}
	msg->output_buf = mem;
	memcpy(msg->output_buf + msg->output_len, buf, len);
	msg->output_len += len;
	return (len);
}

/*
 * Close message output stream.
 */
static int
http_message_output_close(void *cookie)
{
	struct http_message *const msg = cookie;

	msg->output = NULL;
	return (0);
}

