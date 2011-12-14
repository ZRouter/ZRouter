
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

#include <sys/param.h>
#include <sys/syslog.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "structs/structs.h"
#include "structs/type/array.h"

#include "util/typed_mem.h"
#include "io/ssl_fp.h"

struct ssl_info {
	int			fd;
	SSL			*ssl;
	SSL_CTX			*ssl_ctx;
	int			error;
	u_char			server;
	ssl_logger_t		*logger;
	void			*logarg;
	const char		*mtype;
	u_int			timeout;
};

/*
 * Internal functions
 */
static int	ssl_read(void *cookie, char *buf, int len);
static int	ssl_write(void *cookie, const char *buf, int len);
static int	ssl_close(void *cookie);
static int	ssl_check(struct ssl_info *s);
static int	ssl_err(struct ssl_info *s, int ret);

static ssl_logger_t	null_logger;

/*
 * Create a FILE * from an SSL connection.
 */
FILE *
ssl_fdopen(SSL_CTX *ssl_ctx, int fd, int server, const char *mtype,
	ssl_logger_t *logger, void *logarg, u_int timeout)
{
	struct ssl_info *s;
	int flags;
	FILE *fp;

	/* Create SSL info */
	if ((s = MALLOC(mtype, sizeof(*s))) == NULL)
		return (NULL);
	memset(s, 0, sizeof(*s));
	s->fd = fd;
	s->ssl_ctx = ssl_ctx;
	s->mtype = mtype;
	s->server = !!server;
	s->logger = (logger != NULL) ? logger : null_logger;
	s->logarg = logarg;
	s->timeout = timeout;

	/* Set fd I/O to non-blocking */
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		FREE(mtype, s);
		return (NULL);
	}
#if 0
	if ((flags & O_NONBLOCK) == 0
	    && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		FREE(mtype, s);
		return (NULL);
	}
#endif

	/* Create FILE * stream */
	if ((fp = funopen(s, ssl_read, ssl_write, NULL, ssl_close)) == NULL) {
		(void)fcntl(fd, F_SETFL, flags);
		FREE(mtype, s);
		return (NULL);
	}

	/* Done */
	return (fp);
}

/*
 * Read from an SSL connection.
 */
static int
ssl_read(void *cookie, char *buf, int len)
{
	struct ssl_info *const s = cookie;
	int ret;

	/* Check connection */
	if (ssl_check(s) == -1)
		return (-1);

tryread:
	/* Try to read some more data */
	if ((ret = SSL_read(s->ssl, buf, len)) <= 0) {
		if (ssl_err(s, ret) == -1)
			return (-1);
		goto tryread;
	}
	return (ret);
}

/*
 * Write to an SSL connection.
 */
static int
ssl_write(void *cookie, const char *buf, int len)
{
	struct ssl_info *const s = cookie;
	int ret;

	/* Check connection */
	if (ssl_check(s) == -1)
		return (-1);

trywrite:
	/* Try to write some more data */
	if ((ret = SSL_write(s->ssl, buf, len)) <= 0) {
		if (ssl_err(s, ret) == -1)
			return (-1);
		goto trywrite;
	}
	return (ret);
}

/*
 * Close an SSL connection.
 */
static int
ssl_close(void *cookie)
{
	struct ssl_info *const s = cookie;
	int ret;

	while ((ret = SSL_shutdown(s->ssl)) <= 0) {
		if (ret == -1 && ssl_err(s, ret) == -1) {
			(void)close(s->fd);
			return (-1);
		}
	}
	(void)close(s->fd);
	SSL_free(s->ssl);
	FREE(s->mtype, s);
	return (0);
}

/*
 * Check SSL connection context.
 */
static int
ssl_check(struct ssl_info *s)
{
	int ret;

	/* Check for previous error */
	if (s->error != 0)
		goto fail;

	/* Already set up SSL? */
	if (s->ssl != NULL)
		return (0);

	/* Create ssl connection context */
	if ((s->ssl = SSL_new(s->ssl_ctx)) == NULL) {
		ssl_log(s->logger, s->logarg);
		s->error = ECONNABORTED;
		goto fail;
	}
	SSL_set_fd(s->ssl, s->fd);

	/* Do initial SSL handshake */
	if (s->server) {
		while ((ret = SSL_accept(s->ssl)) <= 0) {
			if (ssl_err(s, ret) == -1)
				goto fail;
		}
	} else {
		while ((ret = SSL_connect(s->ssl)) <= 0) {
			if (ssl_err(s, ret) == -1)
				goto fail;
		}
	}

	/* Ready */
	return (0);

fail:
	/* Failed */
	errno = s->error;
	return (-1);
}

/*
 * Handle an error return from an SSL I/O operation.
 *
 * It might be an error, or it might just be that more raw data
 * needs to be read or written. Return -1 in the former case,
 * otherwise return zero and wait for the readable or writable
 * event as appropriate.
 */
static int
ssl_err(struct ssl_info *s, int ret)
{
	int ret2;

	ret2 = SSL_get_error(s->ssl, ret);
	switch (ret2) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
	    {
		struct pollfd pfd;
		int timeout;
		int nfds;

		/* Set up read/write poll(2) event */
		memset(&pfd, 0, sizeof(pfd));
		pfd.fd = s->fd;
		pfd.events = (ret2 == SSL_ERROR_WANT_READ) ?
		    POLLRDNORM : POLLWRNORM;

		/* Set up timeout */
		timeout = INFTIM;
		if (s->timeout != 0)
			timeout = s->timeout * 1000;

		/* Wait for readability or writability */
		if ((nfds = poll(&pfd, 1, timeout)) == -1) {
			s->error = errno;
			(*s->logger)(s->logarg, LOG_ERR,
			    "%s: %s", "poll", strerror(errno));
			goto fail;
		}

		/* Check for timeout */
		if (nfds == 0) {
			s->error = ETIMEDOUT;
			goto fail;
		}

		/* Done */
		return (0);
	    }
	case SSL_ERROR_NONE:
		return (0);

	case SSL_ERROR_SYSCALL:
		ssl_log(s->logger, s->logarg);
		if (ret != 0) {
			s->error = errno;
			(*s->logger)(s->logarg, LOG_ERR,
			    "SSL system error: %s", strerror(errno));
			goto fail;
		}
		/* fall through */

	case SSL_ERROR_ZERO_RETURN:
		(*s->logger)(s->logarg, LOG_ERR, "SSL connection closed");
		s->error = ECONNRESET;
		goto fail;

	case SSL_ERROR_SSL:
		ssl_log(s->logger, s->logarg);
		s->error = EIO;
		goto fail;

	case SSL_ERROR_WANT_X509_LOOKUP:		/* should not happen */
	default:
		ssl_log(s->logger, s->logarg);
		(*s->logger)(s->logarg, LOG_ERR,
		    "unknown return %d from SSL_get_error", ret2);
		s->error = ECONNABORTED;
		goto fail;
	}

fail:
	/* Got an error */
	errno = s->error;
	return (-1);
}

/*
 * Log an SSL error.
 */
void
ssl_log(ssl_logger_t *logger, void *logarg)
{
	char buf[200];
	const char *file;
	const char *str;
	const char *t;
	int line, flags;
	u_long e;

	while ((e = ERR_get_error_line_data(&file, &line, &str, &flags)) != 0) {
		if (logger == NULL)
			continue;
#if 0
		(*logger)(logarg, LOG_ERR, "SSL: %s:%s:%d:%s\n",
		    ERR_error_string(e, buf), file, line,
		    (flags & ERR_TXT_STRING) ? str : "");
#else
		strlcpy(buf, "SSL: ", sizeof(buf));
#if 0
		/* Add library */
		if ((t = ERR_lib_error_string(e)) != NULL) {
			strlcat(buf, t, sizeof(buf));
			strlcat(buf, ": ", sizeof(buf));
		} else {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			    "lib=%u: ", ERR_GET_LIB(e));
		}
#endif

		/* Add function */
		if ((t = ERR_func_error_string(e)) != NULL) {
			strlcat(buf, t, sizeof(buf));
			strlcat(buf, ": ", sizeof(buf));
		} else {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			    "func=%u: ", ERR_GET_FUNC(e));
		}

		/* Add reason */
		if ((t = ERR_reason_error_string(e)) != NULL) {
			strlcat(buf, t, sizeof(buf));
		} else {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			    "reason=%u", ERR_GET_REASON(e));
		}

		/* Add error text, if any */
		if ((flags & ERR_TXT_STRING) != 0) {
			strlcat(buf, ": ", sizeof(buf));
			strlcat(buf, str, sizeof(buf));
		}
		(*logger)(logarg, LOG_ERR, "%s", buf);
#endif
	}
}

static void
null_logger(void *arg, int sev, const char *fmt, ...)
{
	return;
}

