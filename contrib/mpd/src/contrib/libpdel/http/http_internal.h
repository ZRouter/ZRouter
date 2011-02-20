
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

#ifndef _PDEL_HTTP_HTTP_INTERNAL_H_
#define _PDEL_HTTP_HTTP_INTERNAL_H_

#include "debug/debug.h"

#ifndef __FreeBSD__
#define __printflike(x,y)
#endif

/* HTTP connection */
struct http_connection {
	void			*owner;		/* owner (server or client) */
	struct http_request	*req;		/* request structure */
	struct http_response	*resp;		/* response structure */
	struct in_addr		local_ip;	/* local host ip */
	u_int16_t		local_port;	/* local host port */
	struct in_addr		remote_ip;	/* remote host ip */
	u_int16_t		remote_port;	/* remote host port */
	FILE			*fp;		/* connection to peer */
	pthread_t		tid;		/* connection thread id */
	u_char			keep_alive;	/* connection keep-alive */
	u_char			server;		/* we are server (not client) */
	u_char			proxy;		/* proxy request/response */
	u_char			started;	/* conn. thread has started */
	LIST_ENTRY(http_connection) next;	/* next in connection list */
	http_logger_t		*logger;	/* error logging routine */
	SSL_CTX			*ssl;		/* ssl context, if doing ssl */
	int			sock;		/* socket cached */
};

/* HTTP request name, value pair */
struct http_nvp {
	char		*name;
	char		*value;
};

/* HTTP request/response common info */
struct http_message {
	struct http_connection	*conn;		/* associated connection */
	struct http_head	*head;		/* headers */
	char			*host;		/* decoded url host */
	char			*path;		/* decoded url pathname */
	char			*query;		/* encoded query string */
	FILE			*input;		/* entity input stream */
	u_int			input_len;	/* input length, or UINT_MAX */
	u_int			input_read;	/* input length read so far */
	FILE			*output;	/* entity output stream */
	u_char			*output_buf;	/* buffered output, if any */
	int			output_len;	/* buffered output length */
	u_char			buffered;	/* output is buffered */
	u_char			no_headers;	/* don't send any headers */
	u_char			hdrs_sent;	/* header was sent */
	u_char			no_body;	/* no entity data is allowed */
	u_char			skip_body;	/* no entity data to be sent */
};

/* HTTP request */
struct http_request {
	struct http_message	*msg;		/* common req/reply info */
	int			num_nvp;	/* number of n-v pairs */
	struct http_nvp		*nvp;		/* name, value pairs */
	char			*username;	/* authorization username */
	char			*password;	/* authorization password */
};

/* HTTP response */
struct http_response {
	struct http_message	*msg;		/* common req/reply info */
	u_int			code;		/* http response code */
};

__BEGIN_DECLS

/*
 * http_head methods
 */
extern struct	http_head *_http_head_new(void);
extern int	_http_head_read(struct http_head *head, FILE *fp, int req);
extern int	_http_head_read_headers(struct http_head *head, FILE *fp);
extern int	_http_head_write(struct http_head *head, FILE *fp);
extern struct	http_head *_http_head_copy(struct http_head *head);
extern void	_http_head_reset(struct http_head *head);
extern void	_http_head_free(struct http_head **headp);
extern const	char *_http_head_get(struct http_head *head, const char *name);
extern int	_http_head_get_headers(struct http_head *head,
			const char **names, size_t max_names);
extern int	_http_head_set(struct http_head *head, int append,
			const char *name, const char *valfmt, ...)
			__printflike(4, 5);
extern int	_http_head_vset(struct http_head *head, int append,
			const char *name, const char *valfmt, va_list args);
extern int	_http_head_num_headers(struct http_head *head);
extern int	_http_head_get_by_index(struct http_head *head, u_int index,
			const char **namep, const char **valuep);
extern int	_http_head_remove(struct http_head *head, const char *name);
extern int	_http_head_err_response(struct http_head **headp, int code);
extern int	_http_head_has_anything(struct http_head *head);
extern int	_http_head_want_keepalive(struct http_head *head);

/*
 * http_request methods
 */
extern int	_http_request_new(struct http_connection *conn);
extern int	_http_request_read(struct http_connection *conn);
extern void	_http_request_free(struct http_request **reqp);

/*
 * http_response methods
 */
extern int	_http_response_new(struct http_connection *conn);
extern int	_http_response_read(struct http_connection *conn,
			char *errbuf, int size);
extern void	_http_response_free(struct http_response **respp);

/*
 * http_message methods
 */
extern struct	http_message *_http_message_new(void);
extern void	_http_message_free(struct http_message **msgp);
extern int	_http_message_read(struct http_message *msg, int req);
extern FILE	*_http_message_get_output(struct http_message *msg, int buffer);
extern struct	in_addr _http_message_get_remote_ip(struct http_message *msg);
extern u_int16_t _http_message_get_remote_port(struct http_message *msg);
extern int	_http_message_has_anything(struct http_message *msg);
extern void	_http_message_send_headers(struct http_message *msg,
			int unbuffer);
extern void	_http_message_send_body(struct http_message *msg);
extern int	_http_message_vset_header(struct http_message *msg, int append,
			const char *name, const char *valfmt, va_list args);
extern int	_http_message_remove_header(struct http_message *msg,
			const char *name);
extern const	char *_http_message_connection_header(struct http_message *msg);
extern int	_http_message_get_raw_socket(struct http_message *msg);

/*
 * http_connection methods
 */
extern struct	http_connection *_http_connection_create(FILE *fp, int sock,
			int server, struct in_addr ip, u_int16_t port,
			SSL_CTX *ssl, http_logger_t *logger, u_int timeout);
extern void	_http_connection_free(struct http_connection **connp);

/*
 * Client connection caching functions
 */
extern struct	http_connection_cache *_http_connection_cache_create(
			struct pevent_ctx *ctx, u_int max_num, u_int max_idle);
extern void	_http_connection_cache_destroy(
			struct http_connection_cache **cachep);
extern int	_http_connection_cache_get(struct http_connection_cache *cache,
			const struct sockaddr_in *peer, const SSL_CTX *ssl,
			FILE **fpp, int *sockp);
extern int	_http_connection_cache_put(struct http_connection_cache *cache,
			const struct sockaddr_in *peer, const SSL_CTX *ssl,
			FILE *fp, int sock);

/*
 * Misc
 */
extern void	_http_ssl_init(void);

__END_DECLS

#endif	/* _PDEL_HTTP_HTTP_INTERNAL_H_ */
