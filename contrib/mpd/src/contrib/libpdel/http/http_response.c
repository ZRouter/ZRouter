
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
#include <sys/queue.h>

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
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

struct suffix_map {
	const char	*suffix;
	const char	*mime;
};

/* error responses */
#define MALFORMED_RESPONSE_MSG	"Malformed response"
#define TIMEOUT_RESPONSE_MSG	"Server timed out"

/*
 * Internal functions
 */
static int	suffix_cmp(const void *v1, const void *v2);

/* Filename suffix -> MIME encoding (list is sorted case insensitively) */
static const struct suffix_map mime_encodings[] = {
	{ "gz",		"gzip" },
	{ "uu",		"x-uuencode" },
	{ "Z",		"compress" },
};
static const int num_mime_encodings
		    = sizeof(mime_encodings) / sizeof(*mime_encodings);

/* Filename suffix -> MIME type (list is sorted case insensitively) */
static const struct suffix_map mime_types[] = {
	{ "a",		"application/octet-stream" },
	{ "aab",	"application/x-authorware-bin" },
	{ "aam",	"application/x-authorware-map" },
	{ "aas",	"application/x-authorware-seg" },
	{ "ai",		"application/postscript" },
	{ "aif",	"audio/x-aiff" },
	{ "aifc",	"audio/x-aiff" },
	{ "aiff",	"audio/x-aiff" },
	{ "au",		"audio/basic" },
	{ "avi",	"video/x-msvideo" },
	{ "bcpio",	"application/x-bcpio" },
	{ "bin",	"application/octet-stream" },
	{ "cdf",	"application/x-netcdf" },
	{ "class",	"application/java" },
	{ "cpio",	"application/x-cpio" },
	{ "css",	"text/css" },
	{ "dcr",	"application/x-director" },
	{ "dir",	"application/x-director" },
	{ "doc",	"application/msword" },
	{ "dtd",	"text/xml" },
	{ "dump",	"application/octet-stream" },
	{ "dvi",	"application/x-dvi" },
	{ "dxr",	"application/x-director" },
	{ "eps",	"application/postscript" },
	{ "etx",	"text/x-setext" },
	{ "exe",	"application/octet-stream" },
	{ "fgd",	"application/x-director" },
	{ "fh",		"image/x-freehand" },
	{ "fh4",	"image/x-freehand" },
	{ "fh5",	"image/x-freehand" },
	{ "fh7",	"image/x-freehand" },
	{ "fhc",	"image/x-freehand" },
	{ "gif",	"image/gif" },
	{ "gtar",	"application/x-gtar" },
	{ "hdf",	"application/x-hdf" },
	{ "hqx",	"application/mac-binhex40" },
	{ "htm",	"text/html" },
	{ "html",	"text/html" },
	{ "ief",	"image/ief" },
	{ "iv",		"application/x-inventor" },
	{ "jfif",	"image/jpeg" },
	{ "jpe",	"image/jpeg" },
	{ "jpeg",	"image/jpeg" },
	{ "jpg",	"image/jpeg" },
	{ "js",		"application/x-javascript" },
	{ "kar",	"audio/midi" },
	{ "latex",	"application/x-latex" },
	{ "man",	"application/x-troff-man" },
	{ "man",	"application/x-troff-man" },
	{ "me",		"application/x-troff-me" },
	{ "me",		"application/x-troff-me" },
	{ "mid",	"audio/midi" },
	{ "midi",	"audio/midi" },
	{ "mif",	"application/x-mif" },
	{ "mime",	"message/rfc822" },
	{ "mmf",	"application/x-www-urlformencoded" },
	{ "mov",	"video/quicktime" },
	{ "movie",	"video/x-sgi-movie" },
	{ "mp2",	"audio/mpeg" },
	{ "mp3",	"audio/mpeg" },
	{ "mpe",	"video/mpeg" },
	{ "mpeg",	"video/mpeg" },
	{ "mpg",	"video/mpeg" },
	{ "mpga",	"audio/mpeg" },
	{ "ms",		"application/x-troff-ms" },
	{ "ms",		"application/x-troff-ms" },
	{ "mv",		"video/x-sgi-movie" },
	{ "nc",		"application/x-netcdf" },
	{ "o",		"application/octet-stream" },
	{ "oda",	"application/oda" },
	{ "pac",	"application/x-ns-proxy-autoconfig" },
	{ "pbm",	"image/x-portable-bitmap" },
	{ "pdf",	"application/pdf" },
	{ "pgm",	"image/x-portable-graymap" },
	{ "png",	"image/png" },
	{ "pnm",	"image/x-portable-anymap" },
	{ "ppm",	"image/x-portable-pixmap" },
	{ "ppt",	"application/powerpoint" },
	{ "ps",		"application/postscript" },
	{ "qt",		"video/quicktime" },
	{ "ra",		"audio/x-pn-realaudio" },
	{ "ram",	"audio/x-pn-realaudio" },
	{ "rm",		"audio/x-pn-realaudio" },
	{ "roff",	"application/x-troff" },
	{ "rpm",	"audio/x-pn-realaudio-plugin" },
	{ "rtf",	"application/rtf" },
	{ "rtx",	"text/richtext" },
	{ "sh",		"application/x-shar" },
	{ "shar",	"application/x-shar" },
	{ "sit",	"application/x-stuffit" },
	{ "snd",	"audio/basic" },
	{ "spl",	"application/futuresplash" },
	{ "sv4cpio",	"application/x-sv4cpio" },
	{ "sv4crc",	"application/x-sv4crc" },
	{ "swf",	"application/x-shockwave-flash" },
	{ "tar",	"application/x-tar" },
	{ "tex",	"application/x-tex" },
	{ "texi",	"application/x-texinfo" },
	{ "texinfo",	"application/x-texinfo" },
	{ "tif",	"image/tiff" },
	{ "tiff",	"image/tiff" },
	{ "tmpl",	"text/plain" },		/* template file input */
	{ "tr",		"application/x-troff" },
	{ "tsp",	"application/dsptype" },
	{ "tsv",	"text/tab-separated-values" },
	{ "txt",	"text/plain" },
	{ "ustar",	"application/x-ustar" },
	{ "vrml",	"model/vrml" },
	{ "vx",		"video/x-rad-screenplay" },
	{ "wav",	"audio/wav" },
	{ "wbmp",	"image/vnd.wap.wbmp" },
	{ "wml",	"text/vnd.wap.wml" },
	{ "wmlc",	"application/vnd.wap.wmlc" },
	{ "wmls",	"text/vnd.wap.wmlscript" },
	{ "wmlsc",	"application/vnd.wap.wmlscriptc" },
	{ "wrl",	"model/vrml" },
	{ "wsrc",	"application/x-wais-source" },
	{ "xbm",	"image/x-xbitmap" },
	{ "xml",	"text/xml" },
	{ "xpm",	"image/x-xpixmap" },
	{ "xwd",	"image/x-xwindowdump" },
	{ "zip",	"application/x-zip-compressed" },
};
static const int num_mime_types = sizeof(mime_types) / sizeof(*mime_types);

/*********************************************************************
			MAIN ROUTINES
*********************************************************************/

/*
 * Create a new response structure.
 */
int
_http_response_new(struct http_connection *conn)
{
	struct http_response *resp;

	/* Create response structure */
	assert(conn->resp == NULL);
	if ((resp = MALLOC("http_response", sizeof(*resp))) == NULL) {
		(*conn->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		return (-1);
	}
	memset(resp, 0, sizeof(*resp));

	/* Attach message structure */
	if ((resp->msg = _http_message_new()) == NULL) {
		(*conn->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		FREE("http_response", resp);
		return (-1);
	}

	/* Link it up */
	resp->msg->conn = conn;
	conn->resp = resp;
	return (0);
}

/*
 * Read in, validate, and prep an HTTP response from a connection.
 *
 * In all cases, fill in the error message buffer.
 */
int
_http_response_read(struct http_connection *conn, char *errbuf, int size)
{
	struct http_response *const resp = conn->resp;
	const char *hval;

	/* Load in message from HTTP connection */
	if (_http_message_read(resp->msg, 0) == -1) {
		switch (errno) {
		case EINVAL:
			strlcpy(errbuf, MALFORMED_RESPONSE_MSG, size);
			break;

		case ETIMEDOUT:
			strlcpy(errbuf, TIMEOUT_RESPONSE_MSG, size);
			break;

		default:
			strlcpy(errbuf, strerror(errno), size);
			break;
		}

		/* fix errno now that we've used it */
		conn->keep_alive = 0;
		errno = EINVAL;
		return (-1);
	}

	/* Set code */
	hval = http_response_get_header(resp, HDR_REPLY_STATUS);
	if (sscanf(hval, "%u", &resp->code) != 1) {
		snprintf(errbuf, size, "Invalid response code ``%s''", hval);
		conn->keep_alive = 0;
		errno = EINVAL;
		return (-1);
	}

	/* Expect no body from certain responses */
	if (http_response_no_body(resp->code)) {
		resp->msg->input_len = 0;
		resp->msg->no_body = 1;
	} else if (resp->msg->input_len == UINT_MAX
	    && _http_head_want_keepalive(resp->msg->head)) {
		snprintf(errbuf, size,
		    "Keep-Alive requres %s", HTTP_HEADER_CONTENT_LENGTH);
		conn->keep_alive = 0;
		errno = EINVAL;
		return (-1);
	}

	/* Turn off keep alive if not specified by remote side */
	if (!_http_head_want_keepalive(resp->msg->head))
		conn->keep_alive = 0;

	/* Done */
	strlcpy(errbuf, http_response_get_header(resp, HDR_REPLY_REASON), size);
	return (0);
}

/*
 * Free a response
 */
void
_http_response_free(struct http_response **respp)
{
	struct http_response *const resp = *respp;

	if (resp == NULL)
		return;
	_http_message_free(&resp->msg);
	FREE("http_response", resp);
	*respp = NULL;
}

/*
 * Get response code.
 */
int
http_response_get_code(struct http_response *resp)
{
	return (resp->code);
}

/*********************************************************************
			MESSAGE WRAPPERS
*********************************************************************/

/*
 * Set a response header.
 */
int
http_response_set_header(struct http_response *resp, int append,
	const char *name, const char *valfmt, ...)
{
	va_list args;
	int ret;

	/* Set header */
	va_start(args, valfmt);
	ret = _http_message_vset_header(resp->msg, append, name, valfmt, args);
	va_end(args);
	if (ret == -1)
		return (-1);

	/* Remember response code */
	if (name == HDR_REPLY_STATUS) {
		resp->code = atoi(http_response_get_header(resp, name));
		resp->msg->no_body = http_response_no_body(resp->code);
	}

	/* Done */
	return (0);
}

/*
 * Get a response header.
 *
 * For headers listed multiple times, this only gets the first instance.
 */
const char *
http_response_get_header(struct http_response *resp, const char *name)
{
	return (_http_head_get(resp->msg->head, name));
}

/*
 * Get the number of headers
 */
int
http_response_num_headers(struct http_response *resp)
{
	return (_http_head_num_headers(resp->msg->head));
}

/*
 * Get header by index.
 */
int
http_response_get_header_by_index(struct http_response *resp,
	u_int index, const char **namep, const char **valuep)
{
	return (_http_head_get_by_index(resp->msg->head, index, namep, valuep));
}

/*
 * Remove a header.
 */
int
http_response_remove_header(struct http_response *resp, const char *name)
{
	return (_http_message_remove_header(resp->msg, name));
}

/*
 * Send response headers, if not sent already.
 */
int
http_response_send_headers(struct http_response *resp, int unbuffer)
{
	_http_message_send_headers(resp->msg, unbuffer);
	return (0);
}

/*
 * Get response input stream.
 */
FILE *
http_response_get_input(struct http_response *resp)
{
	if (resp->msg->conn->server) {
		errno = EINVAL;
		return (NULL);
	}
	return (resp->msg->input);
}

/*
 * Get response output stream.
 */
FILE *
http_response_get_output(struct http_response *resp, int buffer)
{
	if (!resp->msg->conn->server) {
		errno = EINVAL;
		return (NULL);
	}
	return (_http_message_get_output(resp->msg, buffer));
}

/*
 * Get raw i/o stream as a file descriptor.
 */
int
http_response_get_raw_socket(struct http_response *resp)
{
	return (_http_message_get_raw_socket(resp->msg));
}

/*
 * Get remote IP address.
 */
struct in_addr
http_response_get_remote_ip(struct http_response *resp)
{
	return (_http_message_get_remote_ip(resp->msg));
}

/*
 * Get remote port.
 */
u_int16_t
http_response_get_remote_port(struct http_response *resp)
{
	return (_http_message_get_remote_port(resp->msg));
}

/*
 * Get SSL context.
 */
SSL_CTX *
http_response_get_ssl(struct http_response *resp)
{
	return (resp->msg->conn->ssl);
}

/*********************************************************************
			SERVLET HELPERS
*********************************************************************/

/*
 * Send an HTTP redirect.
 */
void
http_response_send_redirect(struct http_response *resp, const char *url)
{
	const char *name;
	const char *value;

	while (_http_head_get_by_index(resp->msg->head, 0, &name, &value) == 0)
		_http_head_remove(resp->msg->head, name);
	http_response_set_header(resp, 0, HDR_REPLY_STATUS,
	      "%d", HTTP_STATUS_MOVED_PERMANENTLY);
	http_response_set_header(resp, 0, HDR_REPLY_REASON,
	      "%s", http_response_status_msg(HTTP_STATUS_MOVED_PERMANENTLY));
	http_response_set_header(resp, 0, "Location", "%s", url);
}

/*
 * Send an HTTP authorization failed response.
 */
void
http_response_send_basic_auth(struct http_response *resp, const char *realm)
{
	http_response_set_header(resp, 0, HTTP_HEADER_WWW_AUTHENTICATE,
	    "Basic realm=\"%s\"", realm);
	_http_head_remove(resp->msg->head, HTTP_HEADER_LAST_MODIFIED);
	http_response_send_error(resp, HTTP_STATUS_UNAUTHORIZED, NULL);
}

/*
 * Convert 'errno' into an HTTP error response.
 */
void
http_response_send_errno_error(struct http_response *resp)
{
	int code;

	switch (errno) {
	case ENOENT:
	case ENOTDIR:
		code = HTTP_STATUS_NOT_FOUND;
		break;
	case EPERM:
	case EACCES:
		code = HTTP_STATUS_FORBIDDEN;
		break;
	default:
		code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
		break;
	}
	http_response_send_error(resp, code, "%s", strerror(errno));
}

/*
 * Send back a simple error page
 */
void
http_response_send_error(struct http_response *resp,
	int code, const char *fmt, ...)
{
	struct http_request *const req = resp->msg->conn->req;
	const char *ua;
	FILE *fp;
	int i;

	/* Check headers already sent */
	if (resp->msg->hdrs_sent)
		return;

	/* Set response line info */
	http_response_set_header(resp, 0, HDR_REPLY_STATUS, "%d", code);
	http_response_set_header(resp, 0, HDR_REPLY_REASON,
	      "%s", http_response_status_msg(code));

	/* Set additional headers */
	http_response_set_header(resp, 0, HTTP_HEADER_CONTENT_TYPE,
		"text/html; charset=iso-8859-1");

	/* Close connection for real errors */
	if (code >= 400) {
		http_response_set_header(resp,
		    0, _http_message_connection_header(resp->msg), "close");
	}

	/* Send error page body */
	if ((fp = http_response_get_output(resp, 1)) == NULL)
		return;
	fprintf(fp, "<HTML>\n<HEAD>\n<TITLE>%d %s</TITLE></HEAD>\n",
	    code, http_response_status_msg(code));
	fprintf(fp, "<BODY BGCOLOR=\"#FFFFFF\">\n<H3>%d %s</H3>\n",
	    code, http_response_status_msg(code));
	if (fmt != NULL) {
		va_list args;

		fprintf(fp, "<B>");
		va_start(args, fmt);
		vfprintf(fp, fmt, args);
		va_end(args);
		fprintf(fp, "</B>\n");
	}
#if 0
	fprintf(fp, "<P></P>\n<HR>\n");
	fprintf(fp, "<FONT SIZE=\"-1\"><EM>%s</EM></FONT>\n",
	    serv->server_name);
#endif

	/* Add fillter for IE */
	if ((ua = http_request_get_header(req, HTTP_HEADER_USER_AGENT)) != NULL
	    && strstr(ua, "IE") != NULL) {
		for (i = 0; i < 20; i++) {
			fprintf(fp, "<!-- FILLER TO MAKE INTERNET EXPLORER SHOW"
			    " THIS PAGE INSTEAD OF ITS OWN PAGE -->\n");
		}
	}
	fprintf(fp, "</BODY>\n</HTML>\n");
}

/*
 * Try to guess MIME type and encoding from filename suffix.
 */
void
http_response_guess_mime(const char *path,
	const char **ctype, const char **cencs, int maxencs)
{
	static char buf[MAXPATHLEN];
	struct suffix_map *mime;
	int matched_ctype = -1;
	struct suffix_map key;
	const char *slash;
	char *tokctx;
	char *s;
	int i;

	/* Set defaults */
	*ctype = "text/plain; charset=iso-8859-1";
	memset(cencs, 0, maxencs * sizeof(*cencs));

	/* Strip directories */
	if ((slash = strrchr(path, '/')) != NULL)
		path = slash + 1;
	strlcpy(buf, path, sizeof(buf));

	/* Search for suffixes matching MIME types table entry */
	for (i = 0, s = strtok_r(buf, ".", &tokctx);
	    s != NULL; i++, s = strtok_r(NULL, ".", &tokctx)) {
		key.suffix = s;
		if ((mime = bsearch(&key, mime_types, num_mime_types,
		    sizeof(*mime_types), suffix_cmp)) != NULL) {
			*ctype = mime->mime;
			matched_ctype = i;
		}
	}
	if (matched_ctype == -1)
		return;

	/* Skip to the suffix after the last MIME type suffix */
	for (i = 0, s = strtok_r(buf, ".", &tokctx);
	    i <= matched_ctype; i++, s = strtok_r(NULL, ".", &tokctx));

	/* Get encoding(s) */
	for (i = 0; i < maxencs - 1
	    && (s = strtok_r(NULL, ".", &tokctx)) != NULL; i++) {
		key.suffix = s;
		if ((mime = bsearch(&key, mime_encodings, num_mime_encodings,
		    sizeof(*mime_encodings), suffix_cmp)) == NULL)
			break;
		cencs[i] = mime->mime;
	}
}

/*
 * Compare two entries in MIME type/encoding array
 */
static int
suffix_cmp(const void *v1, const void *v2)
{
	const struct suffix_map *const map1 = v1;
	const struct suffix_map *const map2 = v2;

	return (strcasecmp(map1->suffix, map2->suffix));
}

/*********************************************************************
			MISC ROUTINES
*********************************************************************/

/*
 * Certain response codes are required to not have any body.
 */
int
http_response_no_body(int code)
{
	switch (code) {
	case HTTP_STATUS_NO_CONTENT:
	case HTTP_STATUS_NOT_MODIFIED:
		return (1);
	}
	return (0);
}

