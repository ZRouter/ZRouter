
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
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/queue.h>

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <regex.h>
#include <pthread.h>
#include <netdb.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "io/ssl_fp.h"
#include "http/http_defs.h"
#include "http/http_server.h"
#include "http/http_internal.h"
#include "http/http_servlet.h"
#include "util/pevent.h"
#include "util/ghash.h"
#include "util/typed_mem.h"

/*
 * Embedded HTTP[S] web server.
 */

#define MAX_CONNECTIONS		1024
#define HTTP_SERVER_TIMEOUT	90

/* HTTP server */
struct http_server {
	struct pevent_ctx	*ctx;		/* event context */
	struct pevent		*conn_event;	/* new connection event */
	int			sock;		/* accept(2) socket */
	SSL_CTX			*ssl;		/* ssl context, if doing ssl */
	char			*pkey_pw;	/* ssl private key password */
	char			*server_name;	/* server name */
	struct ghash		*vhosts;	/* virtual hosts */
	http_proxy_t		*proxy_handler;	/* proxy handler */
	void			*proxy_arg;	/* proxy handler cookie */
	LIST_HEAD(,http_connection)
				conn_list;	/* active connections */
	int			max_conn;	/* max number of connections */
	int			num_conn;	/* number of connections */
	http_logger_t		*logger;	/* error logging routine */
	pthread_mutex_t		mutex;		/* mutex */
	u_char			stopping;	/* server being stopped */
#if PDEL_DEBUG
	int			mutex_count;	/* mutex count */
#endif
};

/* Virtual host */
struct http_virthost {
	struct http_server	*server;	/* back pointer to server */
	char			*host;		/* virtual hostname */
	LIST_HEAD(,http_servlet_hook)
				servlets;	/* registered servlets */
};

/* What a registered servlet looks like */
struct http_servlet_hook {
	struct http_servlet	*servlet;	/* servlet */
	struct http_virthost	*vhost;		/* back pointer to vhost */
#if PDEL_DEBUG
	char			*spat;		/* string pattern */
#endif
	regex_t			pat;		/* regex pattern */
	int		 	order;		/* processing order */
	pthread_rwlock_t	rwlock;		/* servlet lock */
	LIST_ENTRY(http_servlet_hook)
				next;		/* next in vhost list */
};

/*
 * Internal functions
 */
static void	*http_server_connection_main(void *arg);
static void	http_server_connection_cleanup(void *arg);
static void	http_server_dispatch(struct http_request *req,
			struct http_response *resp);
static int	http_server_ssl_pem_password_cb(char *buf, int size,
			int rwflag, void *udata);

static pevent_handler_t		http_server_accept;

static ghash_equal_t		http_server_virthost_equal;
static ghash_hash_t		http_server_virthost_hash;

static ssl_logger_t		http_server_ssl_logger;

/*********************************************************************
		    SERVER START/STOP ROUTINES
*********************************************************************/

/*
 * Create and start a new server. The server runs in a separate thread.
 */
struct http_server *
http_server_start(struct pevent_ctx *ctx, struct in_addr ip,
	u_int16_t port, const struct http_server_ssl *ssl,
	const char *server_name, http_logger_t *logger)
{
	static const int one = 1;
	struct http_server *serv;
	struct sockaddr_in sin;
	int got_mutex = 0;

	/* Initialize new server structure */
	if ((serv = MALLOC("http_server", sizeof(*serv))) == NULL) {
		(*logger)(LOG_ERR, "%s: %s", "realloc", strerror(errno));
		return (NULL);
	}
	memset(serv, 0, sizeof(*serv));
	LIST_INIT(&serv->conn_list);
	serv->ctx = ctx;
	serv->logger = logger;
	serv->sock = -1;
	serv->max_conn = MAX_CONNECTIONS;	/* XXX make configurable */

	/* Copy server name */
	if ((serv->server_name
	    = STRDUP("http_server.server_name", server_name)) == NULL) {
		(*logger)(LOG_ERR, "%s: %s", "strdup", strerror(errno));
		goto fail;
	}

	/* Create virtual host hash table */
	if ((serv->vhosts = ghash_create(serv, 0, 0, "http_server.vhosts",
	    http_server_virthost_hash, http_server_virthost_equal, NULL,
	    NULL)) == NULL) {
		(*logger)(LOG_ERR, "%s: %s", "strdup", strerror(errno));
		goto fail;
	}

	/* Initialize ssl */
	if (ssl != NULL) {

		/* Initialize SSL stuff */
		_http_ssl_init();

		/* Initialize SSL context for this server */
		if ((serv->ssl = SSL_CTX_new(SSLv2_server_method())) == NULL) {
			ssl_log(http_server_ssl_logger, serv);
			goto fail;
		}

		/* Set callback for getting private key file password */
		SSL_CTX_set_default_passwd_cb(serv->ssl,
		    http_server_ssl_pem_password_cb);
		SSL_CTX_set_default_passwd_cb_userdata(serv->ssl, serv);
		if (serv->pkey_pw != NULL
		    && (serv->pkey_pw = STRDUP("http_server.pkey_pw",
		      ssl->pkey_password)) == NULL) {
			(*serv->logger)(LOG_ERR, "%s: %s",
			    "strdup", strerror(errno));
			goto fail;
		}

		/* Read in certificate and private key files */
		if (SSL_CTX_use_certificate_file(serv->ssl,
		    ssl->cert_path, SSL_FILETYPE_PEM) <= 0) {
			ssl_log(http_server_ssl_logger, serv);
			goto fail;
		}
		if (SSL_CTX_use_PrivateKey_file(serv->ssl,
		    ssl->pkey_path, SSL_FILETYPE_PEM) <= 0) {
			ssl_log(http_server_ssl_logger, serv);
			goto fail;
		}

		/* Verify that certificate actually signs the public key */
		if (!SSL_CTX_check_private_key(serv->ssl)) {
			(*serv->logger)(LOG_ERR,
			    "certificate %s does not match private key %s",
			    ssl->cert_path, ssl->pkey_path);
			goto fail;
		}
	}

	/* Create socket */
	if ((serv->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		(*serv->logger)(LOG_ERR, "%s: %s", "socket", strerror(errno));
		goto fail;
	}
	(void)fcntl(serv->sock, F_SETFD, 1);
	if (setsockopt(serv->sock, SOL_SOCKET,
	    SO_REUSEADDR, (char *)&one, sizeof(one)) == -1) {
		(*serv->logger)(LOG_ERR,
		    "%s: %s", "setsockopt", strerror(errno));
		goto fail;
	}
	memset(&sin, 0, sizeof(sin));
#if !defined(__linux__) && !defined(__sun__)
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr = ip;
	if (bind(serv->sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		(*serv->logger)(LOG_ERR, "%s: %s", "bind", strerror(errno));
		goto fail;
	}
	if (listen(serv->sock, 1024) == -1) {
		(*serv->logger)(LOG_ERR, "%s: %s", "listen", strerror(errno));
		goto fail;
	}

	/* Initialize mutex */
	if ((errno = pthread_mutex_init(&serv->mutex, NULL)) != 0) {
		(*serv->logger)(LOG_ERR, "%s: %s",
		    "pthread_mutex_init", strerror(errno));
		goto fail;
	}
	got_mutex = 1;

	/* Start accepting connections */
	if (pevent_register(serv->ctx, &serv->conn_event, PEVENT_RECURRING,
	    &serv->mutex, http_server_accept, serv, PEVENT_READ, serv->sock)
	    == -1) {
		(*serv->logger)(LOG_ERR, "%s: %s",
		    "pevent_register", strerror(errno));
		goto fail;
	}
	DBG(HTTP, "server %p started", serv);

	/* Done */
	return (serv);

fail:
	/* Cleanup after failure */
	if (got_mutex)
		pthread_mutex_destroy(&serv->mutex);
	if (serv->pkey_pw != NULL) {
		memset(serv->pkey_pw, 0, strlen(serv->pkey_pw));
		FREE("http_server.pkey_pw", serv->pkey_pw);
	}
	if (serv->sock != -1)
		(void)close(serv->sock);
	if (serv->ssl != NULL)
		SSL_CTX_free(serv->ssl);
	ghash_destroy(&serv->vhosts);
	FREE("http_server.server_name", serv->server_name);
	FREE("http_server", serv);
	return (NULL);
}

/*
 * Stop HTTP server.
 */
void
http_server_stop(struct http_server **sp)
{
	struct http_server *const serv = *sp;
	struct http_connection *conn;

	/* Already stopped? */
	if (serv == NULL)
		return;
	*sp = NULL;

	/* Set 'stopping' flag to prevent new servlet registrations */
	if (serv->stopping)
		return;
	serv->stopping = 1;
	DBG(HTTP, "stopping server %p", serv);

	/* Acquire mutex */
	MUTEX_LOCK(&serv->mutex, serv->mutex_count);

	/* Stop accepting new connections */
	pevent_unregister(&serv->conn_event);

	/*
	 * Destroy all registered servlets. Because we must unlock the
	 * server to do this, don't rely on iteration. Instead, start at
	 * the beginning each time.
	 */
	while (ghash_size(serv->vhosts) > 0) {
		struct http_servlet_hook *hook;
		struct http_servlet *servlet;
		struct http_virthost *vhost;
		struct ghash_walk walk;

		/* Get the first virtual host */
		ghash_walk_init(serv->vhosts, &walk);
		vhost = ghash_walk_next(serv->vhosts, &walk);
		assert(vhost != NULL);

		/* Get the first servlet in this virtual host */
		assert(!LIST_EMPTY(&vhost->servlets));
		hook = LIST_FIRST(&vhost->servlets);
		servlet = hook->servlet;

		/* Unlock server and nuke the servlet */
		MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
		http_server_destroy_servlet(&servlet);
		MUTEX_LOCK(&serv->mutex, serv->mutex_count);
	}

	/* Kill any remaining connections */
	while (!LIST_EMPTY(&serv->conn_list)) {

		/* Kill active connections; they will clean up themselves */
		LIST_FOREACH(conn, &serv->conn_list, next) {
			if (conn->started && conn->tid != 0) {
				DBG(HTTP, "canceling conn %p (thread %p)",
				    conn, conn->tid);
				assert(conn->tid != pthread_self());
				pthread_cancel(conn->tid);
				conn->tid = 0;		/* don't cancel twice */
			}
		}

		/* Wait for outstanding connections to complete */
		MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
		usleep(100000);
		MUTEX_LOCK(&serv->mutex, serv->mutex_count);
	}

	/* Free SSL context */
	if (serv->ssl != NULL)
		SSL_CTX_free(serv->ssl);

	/* Close listen socket */
	(void)close(serv->sock);

	/* Free strings */
	if (serv->pkey_pw != NULL) {
		memset(serv->pkey_pw, 0, strlen(serv->pkey_pw));
		FREE("http_server.pkey_pw", serv->pkey_pw);
	}
	FREE("http_server.server_name", serv->server_name);

	/* Free virtual hosts hash table */
	assert(ghash_size(serv->vhosts) == 0);
	ghash_destroy(&serv->vhosts);

	/* Free server structure itself */
	MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
	pthread_mutex_destroy(&serv->mutex);
	DBG(HTTP, "freeing server %p", serv);
	FREE("http_server", serv);
}

/*********************************************************************
			SERVLET REGISTRATION
*********************************************************************/

/*
 * Register a servlet
 */
int
http_server_register_servlet(struct http_server *serv,
	struct http_servlet *servlet, const char *vhost,
	const char *urlpat, int order)
{
	struct http_servlet_hook *hook;
	struct http_virthost vhost_key;
	int got_rwlock = 0;
	int got_regex = 0;
	char ebuf[128];
	int r;

	/* Sanity */
	if (servlet == NULL) {
		errno = EINVAL;
		return (-1);
	}
	if (servlet->hook != NULL) {
		errno = EALREADY;
		return (-1);
	}
	if (serv->stopping) {
		errno = ESRCH;
		return (-1);
	}
	if (vhost == NULL)
		vhost = "";

	/* Create new servlet hook */
	if ((hook = MALLOC("http_servlet_hook", sizeof(*hook))) == NULL) {
		(*serv->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		return (-1);
	}
	memset(hook, 0, sizeof(*hook));
	hook->servlet = servlet;
	hook->order = order;
#if PDEL_DEBUG
	if ((hook->spat = STRDUP("http_servlet_hook.spat", urlpat)) == NULL) {
		(*serv->logger)(LOG_ERR, "%s: %s", "malloc", strerror(errno));
		goto fail;
	}
#endif

	/* Compile URL regular expression */
	if ((r = regcomp(&hook->pat, urlpat, REG_EXTENDED | REG_NOSUB)) != 0) {
		regerror(r, &hook->pat, ebuf, sizeof(ebuf));
		(*serv->logger)(LOG_ERR,
		    "invalid URL pattern \"%s\": %s", urlpat, ebuf);
		goto fail;
	}
	got_regex = 1;

	/* Initialize read/write lock */
	if ((errno = pthread_rwlock_init(&hook->rwlock, NULL)) != 0) {
		(*serv->logger)(LOG_ERR, "%s: %s",
		    "pthread_rwlock_init", strerror(errno));
		goto fail;
	}
	got_rwlock = 1;

	/* Lock server */
	MUTEX_LOCK(&serv->mutex, serv->mutex_count);

	/* Find virtual host; create a new one if necessary */
	vhost_key.host = (char *)vhost;
	if ((hook->vhost = ghash_get(serv->vhosts, &vhost_key)) == NULL) {

		/* Create a new virtual host */
		if ((hook->vhost = MALLOC("http_virthost",
		    sizeof(*hook->vhost))) == NULL) {
			MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
			(*serv->logger)(LOG_ERR, "%s: %s",
			    "malloc", strerror(errno));
			goto fail;
		}
		memset(hook->vhost, 0, sizeof(*hook->vhost));
		if ((hook->vhost->host
		    = STRDUP("http_virthost.host", vhost)) == NULL) {
			MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
			(*serv->logger)(LOG_ERR, "%s: %s",
			    "malloc", strerror(errno));
			goto fail;
		}
		LIST_INIT(&hook->vhost->servlets);

		/* Add virtual host to server's list */
		if ((r = ghash_put(serv->vhosts, hook->vhost)) == -1) {
			MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
			goto fail;
		}
		assert(r == 0);
		hook->vhost->server = serv;
	}

	/* Register servlet with virtual host */
	LIST_INSERT_HEAD(&hook->vhost->servlets, hook, next);
	servlet->hook = hook;

	/* Unlock server */
	MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);

	/* Done */
	return (0);

fail:
	/* Clean up after failure */
	if (hook->vhost != NULL) {
		FREE("http_virthost.host", hook->vhost->host);
		FREE("http_virthost", hook->vhost);
	}
	if (got_rwlock)
		pthread_rwlock_destroy(&hook->rwlock);
	if (got_regex)
		regfree(&hook->pat);
#if PDEL_DEBUG
	FREE("http_servlet_hook.spat", hook->spat);
#endif
	FREE("http_servlet_hook", hook);
	return (-1);
}

/*
 * Destroy a servlet, unregistering it if necessary.
 *
 * If it's the last servlet in the virtual host, remove the virtual host too.
 */
void
http_server_destroy_servlet(struct http_servlet **servletp)
{
	struct http_servlet *const servlet = *servletp;
	struct http_servlet_hook *hook;
	struct http_virthost *vhost;
	struct http_server *serv;
	int r;

	/* Sanity */
	if (servlet == NULL)
		return;
	*servletp = NULL;
	hook = servlet->hook;

	/* If servlet is not registered, just destroy it */
	if (hook == NULL)
		goto done;
	vhost = hook->vhost;
	serv = vhost->server;

	/* Lock server */
	MUTEX_LOCK(&serv->mutex, serv->mutex_count);

	/* Unregister servlet from its virtual host */
	LIST_REMOVE(hook, next);
	hook->vhost = NULL;				/* for good measure */

	/* Remove the virtual host if it has no more servlets */
	if (LIST_EMPTY(&vhost->servlets)) {
		r = ghash_remove(serv->vhosts, vhost);
		assert(r == 1);
		vhost->server = NULL;			/* for good measure */
		FREE("http_virthost.host", vhost->host);
		FREE("http_virthost", vhost);
	}

	/* Unlock server */
	MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);

	/* Wait for any threads running this servlet to finish */
	r = pthread_rwlock_wrlock(&hook->rwlock);
	assert(r == 0);
	r = pthread_rwlock_unlock(&hook->rwlock);
	assert(r == 0);

	/* Destroy servlet hook */
	regfree(&hook->pat);
	pthread_rwlock_destroy(&hook->rwlock);
#if PDEL_DEBUG
	FREE("http_servlet_hook.spat", hook->spat);
#endif
	FREE("http_servlet_hook", hook);
	servlet->hook = NULL;				/* for good measure */

done:
	/* Destroy servlet itself */
	(*servlet->destroy)(servlet);
}

/*
 * Set proxy servlet
 */
extern void
http_server_set_proxy_handler(struct http_server *serv,
	http_proxy_t *handler, void *arg)
{
	MUTEX_LOCK(&serv->mutex, serv->mutex_count);
	serv->proxy_handler = handler;
	serv->proxy_arg = arg;
	MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
}

/*********************************************************************
			SERVER MAIN THREAD
*********************************************************************/

/*
 * Accept a new connection.
 *
 * The mutex will be locked when this is called.
 */
static void
http_server_accept(void *arg)
{
	struct http_server *const serv = arg;
	struct http_connection *conn;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	int sock;

	/* Accept next connection */
	if ((sock = accept(serv->sock, (struct sockaddr *)&sin, &slen)) == -1) {
		if (errno != ECONNABORTED && errno != ENOTCONN) {
			(*serv->logger)(LOG_ERR, "%s: %s",
			    "accept", strerror(errno));
		}
		return;
	}
	(void)fcntl(sock, F_SETFD, 1);

	/* Get new connection object */
	if ((conn = _http_connection_create(NULL, sock, 1, sin.sin_addr,
	    ntohs(sin.sin_port), serv->ssl, serv->logger,
	    HTTP_SERVER_TIMEOUT)) == NULL) {
		(*serv->logger)(LOG_ERR, "%s: %s",
		    "_http_connection_create", strerror(errno));
		(void)close(sock);
		return;
	}
	conn->owner = serv;

	/* Add to server's list of active connections */
	LIST_INSERT_HEAD(&serv->conn_list, conn, next);
	serv->num_conn++;

	/* Spawn a new thread to handle request */
	if ((errno = pthread_create(&conn->tid, NULL,
	    http_server_connection_main, conn)) != 0) {
		(*serv->logger)(LOG_ERR, "%s: %s",
		    "pthread_create", strerror(errno));
		conn->tid = 0;
		LIST_REMOVE(conn, next);
		serv->num_conn--;
		_http_connection_free(&conn);
	}

	/* Detach thread */
	pthread_detach(conn->tid);
	DBG(HTTP, "spawning %p for connection %p from %s:%u",
	    conn->tid, conn, inet_ntoa(conn->remote_ip), conn->remote_port);

	/* If maximum number of connections reached, stop accepting new ones */
	if (serv->num_conn >= serv->max_conn)
		pevent_unregister(&serv->conn_event);
}

/*********************************************************************
		    SERVER CONNECTION MAIN THREAD
*********************************************************************/

/*
 * HTTP connection thread main routine.
 */
static void *
http_server_connection_main(void *arg)
{
	struct http_connection *const conn = arg;
	struct http_server *const serv = conn->owner;

	/* Add shutdown cleanup hook */
	pthread_cleanup_push(http_server_connection_cleanup, conn);
	conn->started = 1;		/* avoids pthread_cancel() race */
	DBG(HTTP, "connection %p started", conn);

	/* Read requests as long as connection is kept alive */
	conn->keep_alive = 1;
	while (1) {
		struct http_request *const req = conn->req;
		struct http_response *const resp = conn->resp;
		const char *hval;
		char dbuf[64];
		struct tm tm;
		time_t now;

		/* Read in request */
		if (_http_request_read(conn) == -1) {
			if (errno == ENOTCONN)	/* remote side disconnected */
				break;
			conn->keep_alive = 0;
			goto send_response;
		}

		/* Set default response headers */
		now = time(NULL);
		strftime(dbuf, sizeof(dbuf),
		    HTTP_TIME_FMT_RFC1123, gmtime_r(&now, &tm));
		if (http_response_set_header(resp, 0,
		      HDR_REPLY_VERSION, HTTP_PROTO_1_1) == -1
		    || http_response_set_header(resp, 0, HDR_REPLY_STATUS,
		      "%d", HTTP_STATUS_OK) == -1
		    || http_response_set_header(resp, 0, HDR_REPLY_REASON,
		      "%s", http_response_status_msg(HTTP_STATUS_OK)) == -1
		    || http_response_set_header(resp, 0, HTTP_HEADER_SERVER,
		      "%s", serv->server_name) == -1
		    || http_response_set_header(resp, 0, HTTP_HEADER_DATE,
		      "%s", dbuf) == -1
		    || http_response_set_header(resp, 0,
		      HTTP_HEADER_LAST_MODIFIED, "%s", dbuf) == -1) {
			conn->keep_alive = 0;
			goto send_response;
		}

		/* Turn off keep-alive if client doesn't want it */
		if (!_http_head_want_keepalive(req->msg->head))
			conn->keep_alive = 0;

		/* Handle request */
		http_server_dispatch(req, resp);

		/* Set Connection: header if not already set */
		if (((hval = http_request_get_method(req)) == NULL
		      || strcmp(hval, HTTP_METHOD_CONNECT) != 0)
		    && http_response_get_header(resp,
		      _http_message_connection_header(resp->msg)) == NULL) {
			http_response_set_header(resp, 0,
			    _http_message_connection_header(resp->msg),
			    conn->keep_alive ? "Keep-Alive" : "Close");
		}

		/* Slurp up any remaining entity data if keep-alive */
		if (conn->keep_alive && req != NULL && !feof(req->msg->input)) {
			static char buf[1024];

			while (fgets(buf, sizeof(buf), req->msg->input) != NULL)
				;
		}

send_response:
		/* Send back headers (if not already sent) */
		http_response_send_headers(resp, 0);

		/* Send back body (if it was buffered) */
		_http_message_send_body(resp->msg);

		/* Determine if we can still keep this connection alive */
		if (!resp->msg->no_body
		    && (http_response_get_header(resp,
		       HTTP_HEADER_CONTENT_LENGTH) == NULL
		      || ferror(conn->fp)
		      || feof(conn->fp)
		      || !_http_head_want_keepalive(resp->msg->head)))
			conn->keep_alive = 0;

		/* Close connection unless keeping it alive */
		if (!conn->keep_alive)
			break;

		/* Reset request & response for next time */
		_http_request_free(&conn->req);
		_http_response_free(&conn->resp);
		if (_http_request_new(conn) == -1
		    || _http_response_new(conn) == -1)
			break;
	}

	/* Done with this connection */
	DBG(HTTP, "done with connection %p", conn);
	pthread_cleanup_pop(1);
	return (NULL);
}

/*
 * Cleanup when shutting down an HTTP server connection.
 */
static void
http_server_connection_cleanup(void *arg)
{
	struct http_connection *conn = arg;
	struct http_server *const serv = conn->owner;

	/* Lock server */
	DBG(HTTP, "connection %p cleaning up", conn);
	MUTEX_LOCK(&serv->mutex, serv->mutex_count);

	/* Restart accepting new connections if needed */
	if (serv->conn_event == NULL && !serv->stopping) {
		if (pevent_register(serv->ctx, &serv->conn_event,
		    PEVENT_RECURRING, &serv->mutex, http_server_accept,
		    serv, PEVENT_READ, serv->sock) == -1) {
			(*serv->logger)(LOG_ERR, "%s: %s",
			    "pevent_register", strerror(errno));
		}
	}

	/* Remove this connection from the list of connections */
	LIST_REMOVE(conn, next);
	serv->num_conn--;

	/* Unlock server */
	MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);

	/* Free connection */
	_http_connection_free(&conn);
}

/*
 * Dispatch a request
 */
static void
http_server_dispatch(struct http_request *req, struct http_response *resp)
{
	struct http_connection *conn = req->msg->conn;
	struct http_server *const serv = conn->owner;
	const char *url = http_request_get_path(req);
	struct http_virthost vhost_key;
	char buf[MAXHOSTNAMELEN];
	const char *hval;
	int order;
	int done;

	/* Special handling for proxy requests */
	if (conn->proxy) {
		http_proxy_t *proxy_handler;
		void *proxy_arg;

		/* Get proxy handler from server */
		MUTEX_LOCK(&serv->mutex, serv->mutex_count);
		proxy_handler = serv->proxy_handler;
		proxy_arg = serv->proxy_arg;
		MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);

		/* Handle request */
		if (proxy_handler == NULL) {
			http_response_send_error(resp,
			    (strcmp(http_request_get_method(req),
			      HTTP_METHOD_CONNECT) == 0) ?
				HTTP_STATUS_METHOD_NOT_ALLOWED :
				HTTP_STATUS_NOT_FOUND, NULL);
			return;
		}
		(*proxy_handler)(proxy_arg, req, resp);
		return;
	}

	/* Extract the desired virtual host from the request */
	if ((hval = http_request_get_header(req, HTTP_HEADER_HOST)) != NULL) {
		char *s;

		strlcpy(buf, hval, sizeof(buf));
		if ((s = strchr(buf, ':')) != NULL)		/* trim port */
			*s = '\0';
	} else
		*buf = '\0';			/* fall back to default vhost */
	vhost_key.host = buf;

	/* Lock server */
	MUTEX_LOCK(&serv->mutex, serv->mutex_count);

	/* Choose and fix desired virtual host before running any servlets */
	if (ghash_get(serv->vhosts, &vhost_key) == NULL)
		*buf = '\0';			/* fall back to default vhost */

	/* Unlock server */
	MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);

	/* Execute matching vhost servlets, in order, until there is output */
	for (done = 0, order = INT_MIN; !done; ) {
		struct http_servlet_hook *best = NULL;
		struct http_servlet_hook *hook;
		struct http_virthost *vhost;
		int r;

		/* Lock server */
		MUTEX_LOCK(&serv->mutex, serv->mutex_count);

		/* Find virtual host (do this each time because we unlock) */
		if ((vhost = ghash_get(serv->vhosts, &vhost_key)) == NULL) {
			MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
			break;
		}

		/* Find lowest order virtual host servlet that matches */
		LIST_FOREACH(hook, &vhost->servlets, next) {

			/* Skip if we've already tried it */
			if (hook->order <= order)
				continue;

			/* Compare URL path, choosing best match as we go */
			if ((r = regexec(&hook->pat, url, 0, NULL, 0)) == 0) {
				if (best == NULL || hook->order < best->order)
					best = hook;
				continue;
			}

			/* If it didn't match, try the next servlet */
			if (r == REG_NOMATCH)
				continue;

			/* We got some weird error from regexec() */
			MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
			regerror(r, &hook->pat, buf, sizeof(buf));
			(*serv->logger)(LOG_ERR, "%s: %s", "regexec", buf);
			http_response_send_error(resp,
			    HTTP_STATUS_INTERNAL_SERVER_ERROR,
			    "regexec: %s", buf);
			return;
		}

		/* If no matching servlet was found, we're done */
		if ((hook = best) == NULL) {
			MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);
			break;
		}

		/* Update lowest order for next iteration */
		order = hook->order;

		/* Lock servlet to prevent it from going away */
		r = pthread_rwlock_rdlock(&hook->rwlock);
		assert(r == 0);

		/* Add cleanup hook that unlocks servlet */
		pthread_cleanup_push((void (*)(void *))pthread_rwlock_unlock,
		    &hook->rwlock);

		/* Unlock server */
		MUTEX_UNLOCK(&serv->mutex, serv->mutex_count);

		/* Execute servlet */
		done = (*hook->servlet->run)(hook->servlet, req, resp);

		/* Unlock servlet */
		pthread_cleanup_pop(1);

		/* If servlet returned an error, bail out */
		if (done == -1) {
			http_response_send_errno_error(resp);
			break;
		}
	}

	/* No matching servlet found? */
	if (!done)
		http_response_send_error(resp, HTTP_STATUS_NOT_FOUND, NULL);
}

/*********************************************************************
			MISC ROUTINES
*********************************************************************/

/*
 * SSL callback to get the password for encrypted private key files.
 */
static int
http_server_ssl_pem_password_cb(char *buf, int size, int rwflag, void *udata)
{
	struct http_server *const serv = udata;

	if (serv->pkey_pw == NULL) {
		(*serv->logger)(LOG_ERR,
		    "SSL private key is encrypted but no key was supplied");
		return (-1);
	}
	strlcpy(buf, serv->pkey_pw, size);
	return (strlen(buf));
}

/*
 * SSL callback for logging.
 */
static void
http_server_ssl_logger(void *arg, int sev, const char *fmt, ...)
{
	struct http_server *const serv = arg;
	va_list args;
	char *s;

	/* Prepend "startup" message to error message */
	va_start(args, fmt);
	VASPRINTF(TYPED_MEM_TEMP, &s, fmt, args);
	va_end(args);
	if (s == NULL)
		return;
	(*serv->logger)(sev, "%s: %s", "startup error", s);
	FREE(TYPED_MEM_TEMP, s);
}

/*
 * Virtual host hashing routines
 */
static int
http_server_virthost_equal(struct ghash *g,
	const void *item1, const void *item2)
{
	const struct http_virthost *const vhost1 = item1;
	const struct http_virthost *const vhost2 = item2;

	return (strcasecmp(vhost1->host, vhost2->host) == 0);
}

u_int32_t
http_server_virthost_hash(struct ghash *g, const void *item)
{
	const struct http_virthost *const vhost = item;
	u_int32_t hash;
	const char *s;

	for (hash = 0, s = vhost->host; *s != '\0'; s++)
		hash = (hash * 31) + (u_char)tolower(*s);
	return (hash);
}


