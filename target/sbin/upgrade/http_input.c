/*-
 * Copyright (c) 2012 Rybalko Aleksandr.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

#define _WITH_DPRINTF
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <unistd.h>

#include "http_input.h"

#define	CRLF	"\r\n"
#define	RING_BUFFER_SIZE	256

#ifdef	TARGET_VENDOR
#define	PAGE_TITLE	TARGET_VENDOR TARGET_DEVICE "Firmware Update"
#else
#define	PAGE_TITLE	"D-Link DIR-632 Firmware Update"
#endif

#define	WRB_SIZE	1024
int wrb_p = 0;
char wrbuf[WRB_SIZE];

struct rbuffer {
	unsigned int 	curr;
	unsigned int	total;
	int		outfd;
	int		matched;
	int 		boundary_len;
	unsigned int	filesize;		/* Collected file size */
	char 		buf[RING_BUFFER_SIZE];
	char 		*boundary;
};

static void
http_templ(char *status_code, char *status, char *hdrs, char *title,
    char *html)
{

	printf(
	    "HTTP/1.1 %s %s" CRLF
	    "Date: Thu, 19 Feb 2009 12:27:04 GMT" CRLF
	    "Server: upgrade utility (ZRouter.org)" CRLF
	    "Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT" CRLF
	    "Content-Type: text/html" CRLF
	    "%s"
	    "Connection: close" CRLF
	    "" CRLF
	    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
		"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
	    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" "
		"lang=\"en\">" CRLF
	    "  <head>" CRLF
	    "    <title>%s</title>" CRLF
	    "    <meta http-equiv=\"Content-Type\" content=\"text/html; "
		"charset=utf-8\" />" CRLF
	    "    <link rel=\"Help\" href=\"http://www.zrouter.org/\" />" CRLF
	    "    <style type=\"text/css\">" CRLF
	    "/*<![CDATA[*/" CRLF
	    "BODY {" CRLF
	    "    color: black;" CRLF
	    "    background: white;" CRLF
	    "}" CRLF
	    "" CRLF
	    "P.error {" CRLF
	    "    color: red;" CRLF
	    "}" CRLF
	    "" CRLF
	    "/*]]>*/" CRLF
	    "    </style>" CRLF
	    "  </head>" CRLF
	    "  <body>" CRLF
	    "    <center>" CRLF
	    "%s" CRLF
	    "    </center>" CRLF
	    "  </body>" CRLF
	    "</html>" CRLF,
	    status_code, status, hdrs, title, html);
}

static void
http_index()
{

	http_templ("200", "OK", "", PAGE_TITLE,
	    "<h1>" PAGE_TITLE "</h1>" CRLF
	    "<p>Select file with Firmware image.</p>" CRLF
	    "<form method=\"POST\" enctype=\"multipart/form-data\">" CRLF
	    "  <input type=\"file\" name=\"firmware\">" CRLF
	    "  <input type=\"submit\">" CRLF
	    "</form>" CRLF);
}

static void
http_ok()
{

	http_templ("200", "OK", "", PAGE_TITLE,
	    "<h1>" PAGE_TITLE "</h1>" CRLF
	    "<p>Firmware image transferred successfully.</p>" CRLF
	    "<p>Update procedure started.</p>" CRLF);
}

static void
http_unauth(struct args *args)
{
	char WWW_Auth[256];

	snprintf(WWW_Auth, 255,
	    "WWW-Authenticate: Basic realm=\"%s\"" CRLF, args->realm);
	WWW_Auth[255] = '\0';

	http_templ("401", "Unauthorized",
	    WWW_Auth,
	    PAGE_TITLE,
	    "<h1>" PAGE_TITLE "</h1>" CRLF
	    "<p>Authorization required.</p>" CRLF
	    "<p>Specify correct username and password.</p>" CRLF);
}

static void
http_fail()
{

	http_templ("200", "OK", "", PAGE_TITLE,
	    "<h1>" PAGE_TITLE "</h1>" CRLF
	    "<p>Firmware image transferr failed.</p>" CRLF
	    "<p>Please try again.</p>" CRLF);
}

static void
http_error(char *msg)
{

	http_templ("200", "OK", "", PAGE_TITLE, msg);
}

static int 
local_write(int fd, char *buf, int count)
{

	if (count > 1) {
		if (wrb_p > 0) {
			/* Send buffered data */
			write(fd, wrbuf, wrb_p);
			wrb_p = 0;
		}
		return (write(fd, buf, count));
	}

	if (wrb_p == WRB_SIZE) {
		/* Buffer full, so write data */
		write(fd, wrbuf, wrb_p);
		wrb_p = 0;
	}

	wrbuf[wrb_p++] = buf[0];
	return (1);

}

static int 
local_close(int fd)
{

	write(fd, wrbuf, wrb_p);
	wrb_p = 0;

	return (close(fd));
}

static struct rbuffer *
ring_buffer_init(char *boundary, int outfd)
{
	struct rbuffer *rb;

	rb = (struct rbuffer *)malloc(sizeof(struct rbuffer));
	if (!rb)
		return (NULL);

	rb->curr = rb->total = rb->matched = rb->filesize = 0;
	bzero(rb->buf, RING_BUFFER_SIZE);
	rb->outfd = outfd;

	rb->boundary_len = strlen(boundary) + 2;
	rb->boundary = calloc(rb->boundary_len + 1, 1);
	if (!rb->boundary) {
		free(rb);
		return (NULL);
	}
	rb->boundary[0] = '-';
	rb->boundary[1] = '-';
	memcpy(rb->boundary + 2, boundary, rb->boundary_len - 2);

	return (rb);
}

static void
ring_buffer_free(struct rbuffer *rb)
{

	free(rb->boundary);
	free(rb);
}

static int
ring_buffer_append(struct rbuffer *rb, char ch)
{

	rb->curr = (rb->curr + 1) % RING_BUFFER_SIZE;
	if (rb->total >= RING_BUFFER_SIZE) {
		rb->filesize ++;
		local_write(rb->outfd, &rb->buf[rb->curr], 1);
	}
	rb->buf[rb->curr] = ch;
	rb->total++;

	if (ch == rb->boundary[rb->matched])
		rb->matched++;
	else
		rb->matched = 0;

	if (rb->matched == rb->boundary_len)
		return (1);

	return (0);
}

static int
ring_buffer_get_filesize(struct rbuffer *rb)
{

	return (rb->filesize);
}

static void
ring_buffer_drain(struct rbuffer *rb)
{
	int i, last;

	last = (rb->curr + RING_BUFFER_SIZE - rb->boundary_len) %
	    RING_BUFFER_SIZE;
	if (rb->buf[(last + RING_BUFFER_SIZE - 1) % RING_BUFFER_SIZE] == '\r')
		last = (last + RING_BUFFER_SIZE - 1) % RING_BUFFER_SIZE;

	if (rb->total < RING_BUFFER_SIZE) {
		rb->filesize += rb->total;
		local_write(rb->outfd, rb->buf+1, rb->total);
	} else {
		for (i = ((rb->curr + 1) % RING_BUFFER_SIZE); i != last;
		    i = ((i + 1) % RING_BUFFER_SIZE)) {
			rb->filesize ++;
			local_write(rb->outfd, &rb->buf[i], 1);
		}
	}
}

static void
skip_spaces(char **s)
{

	while ((**s != '\0') && isspace(**s))
		(*s)++;
}

static int 
local_read(struct args *args, char *buf, int count)
{
	int ret, m;

	if (!args->enable_counter || (count > 1)) {
		ret = read(args->fd, buf, count);
	} else {
		/* XXX: Can operate only if read always 1 byte size */
		args->inbuf_p %= INPUT_BUFFER_SIZE;
		if (args->inbuf_p == 0) {
			m = MIN(INPUT_BUFFER_SIZE,
			    (args->len - args->counter_value));
			ret = read(args->fd, args->inbuf, m);
			args->inbuf_l = ret;
		}
		if (args->inbuf_p < args->inbuf_l) {
			buf[0] = args->inbuf[args->inbuf_p++];
			ret = 1;
		} else {
			args->inbuf_p = 0;
			ret = 0;
		}
	}
	if (args->enable_counter) {
		args->counter_value += ret;
	}

	return (ret);
}

static int
read_line(struct args *args, char *line, int maxlen)
{
	int i;

	for (i = 0; i < maxlen; i ++) {
		if (local_read(args, line + i, 1) != 1) {
			return (1);
		}

		if (line[i] == '\r')
			line[i] = '\0';
		else if (line[i] == '\n') {
			line[i] = '\0';
			break;
		}
	}

	if (i == maxlen)
		line[maxlen - 1] = '\0';

	return (0);
}


static void
parse_boundary(char *buf, char *boundary)
{
	char *boundaryp;

	/* Content-Type: multipart/form-data; boundary=Asrf456BGe4h*/
	boundaryp = strnstr(buf + strlen("Content-Type:"), "boundary=",
	    LINE_BUFFER_LEN - strlen("Content-Type:"));
	if (boundaryp) {
		/* Check if we have multipart/form-data */
		if (!strstr(buf, "multipart/form-data"))
			return;
		/* skip field name */
		boundaryp += strlen("boundary=");
		/* stop on any whitespace */
		boundaryp = strsep(&boundaryp, " \t\r\n");
		if (strlen(boundaryp) < 1)
			return;
		strcpy(boundary, boundaryp);
		return;
	}
	/* TODO: warning here */
	return;
}

static void
parse_len(char *buf, unsigned int *len)
{
	char *lenp, *elenp;
	int slen;

	/* Content-Length: 6268237 */
	lenp = buf += strlen("Content-Length:");
	skip_spaces(&lenp);
	slen = strlen(lenp);
	if (slen < 1)
		return;
	*len = strtoul(lenp, &elenp, 10);
	if ((elenp - lenp) < slen) {
		*len = 0;
		/* TODO: warning here */
	}
	return;
}

static void
parse_hash(char *buf, char *hash)
{
	char *hashp, *p;

	/* Authorization: Basic YWRtaW46YWRtaW4= */
	hashp = strnstr(buf + strlen("Authorization:"), "Basic",
	    LINE_BUFFER_LEN - strlen("Authorization:"));
	if (hashp) {
		/* skip field name */
		hashp += strlen("Basic");
		/* skip spaces and tabs */
		skip_spaces(&hashp);
		/* stop on any whitespace */
		p = hashp;
		p = strsep(&p, " \t\r\n");
		if (strlen(hashp) < 2)
			return;
		strcpy(hash, hashp);
		return;
	}
	/* TODO: warning here */
	return;
}

static int
test_part_header(struct args *args, char *boundary, char *buf)
{
	char *p, filename[256];
	int i, match, err;

	args->filename[0] = args->filename[255] = '\0';
	match = err = 0;

	if (read_line(args, buf, LINE_BUFFER_LEN))
		return (1);

	if (strncmp(buf, "Content-Disposition:",
	    strlen("Content-Disposition:")) == 0) {
		p = buf + strlen("Content-Disposition:");
		skip_spaces(&p);
		if (strncmp(p, "form-data", strlen("form-data")) != 0)
			return (0);
		p += strlen("form-data");

		for (i = 0; i < 8; i++) {
			strsep(&p, ";");
			if (p == 0 || *p == '\0')
				break;
			p += strspn(p, " \t");
			if (strncmp(p, "name=\"firmware\"",
			    strlen("name=\"firmware\"")) == 0)
				match ++;
			if (strncmp(p, "filename=\"",
			    strlen("filename=\"")) == 0) {
				p += strlen("filename=\"");
				bzero(filename, sizeof(filename));
				strncpy(filename, p, index(p, '\"') - p);
				if (filename[0] == '\0')
					return (1);
				match ++;
			}
			if (match == 2)
				break;
		}
	}
	/* Skip part header lines until empty line */
	while ((err = read_line(args, buf, LINE_BUFFER_LEN)) == 0) {
		if (err != 0)
			return (1);
		if (strlen(buf) == 0)
			break;
	}
	if (match == 2) {
		return (2); /* ready for recive firmware */
	}
	char c;
	while (local_read(args, &c, 1) == 1)
		if (c == '\n' && 
		    (local_read(args, &c, 1) == 1) && c == '-' &&
		    (local_read(args, &c, 1) == 1) && c == '-') {
			if (read_line(args, buf, LINE_BUFFER_LEN))
				return (1);
			if (strncmp(buf, boundary, strlen(boundary)) == 0) {
				p = buf + strlen(boundary);
				if (p[0] == '-' && p[1] == '-') {
					/* return if form data over */
					return (1);
				} else {
					break;
				}
			}
		}
	/* Boundary marker skipped, start new part */
	return (test_part_header(args, boundary, buf));
}

enum http_input_status
http_input(struct args *args, int console)
{
	char buf[LINE_BUFFER_LEN], hash[256], boundary[71], ch;
	int err, post, i, out;
	struct rbuffer *rb;

	err = 1;
	boundary[0] = hash[0] = post = 0;
	args->console = console;

	/* POST /send-message.html HTTP/1.1 */
	if (read_line(args, buf, LINE_BUFFER_LEN)) {
		http_error("Can't accept methods other than GET or POST");
		return (HTTP_INPUT_READ_ERR);
	}

	if (strncmp(buf, "GET", 3) == 0)
		post = 0;
	else if (strncmp(buf, "POST", 4) == 0)
		post = 1;
	else {
		http_error("Can't accept methods other than GET or POST");
		return (HTTP_INPUT_METHOD_ERR);
	}

	for (;;) {
		if (read_line(args, buf, LINE_BUFFER_LEN)) {
			dprintf(console, "Can't get input line\n");
			return (HTTP_INPUT_READ_ERR);
		}
		if (!boundary[0] && (strncmp(buf, "Content-Type:",
		    strlen("Content-Type:")) == 0))
			parse_boundary(buf, boundary);
		if (!args->len && (strncmp(buf, "Content-Length:",
		    strlen("Content-Length:")) == 0))
			parse_len(buf, &args->len);
		if (!hash[0] && (strncmp(buf, "Authorization:",
		    strlen("Authorization:")) == 0))
			parse_hash(buf, hash);
		if (buf[0] == '\0') /* Empty line */
			break;
	}

	/* Header parsing done, run counter */
	args->enable_counter = 1;

	/* Check authentication if hash supplied */
	if (args->key && args->realm) {
		if (strcmp(hash, args->key) != 0) {
			http_unauth(args);
			dprintf(console,
			    "Sent Authorization required message\n");
			return (HTTP_INPUT_AUTH_FAIL);
		}
	}

	/*
	 * We have two non error answers:
	 * 1. Page with form for GET requiest.
	 */
	if (post == 0) {
		http_index();
		dprintf(console, "Sent index page to user\n");
		return (HTTP_INPUT_SHOW_INDEX);
	}
	if (!boundary[0] || !args->len) {
		dprintf(console, "\n");
		http_error("<font class=\"red\">" CRLF
		    "  Multipart form must have both boundary marker and "
			"Content-Length defined." CRLF
		    "</font>" CRLF);
		return (HTTP_INPUT_BAD_MULTIPART);
	}

	dprintf(console, "boundary=\"%s\", body length=%d\n",
	    boundary, args->len);

	/*
	 * 2. Parse/check image and post success notification for POST
	 * requiest.
	 */
	if (read_line(args, buf, LINE_BUFFER_LEN)) {
		http_error("<font class=\"red\">" CRLF
		    "Failed to parse form data." CRLF
		    "</font>" CRLF);
		return (HTTP_INPUT_READ_ERR);
	}

	/* Multipart for body must start with boundary marker */
	if ((buf[0] == '-') &&
	    (buf[1] == '-') &&
	    (strncmp(buf+2, boundary, strlen(boundary)) == 0)) {
		/* Find firmware part. */
		if (test_part_header(args, boundary, buf) != 2) {
			http_error("<font class=\"red\">" CRLF
			    "No Firmware image in posted data." CRLF
			    "</font>" CRLF);
			return (HTTP_INPUT_FW_PART_REQUIRED);
		}
	}

	out = open(args->outfile, O_WRONLY, 0644);
	if (out < 0) {
		dprintf(console, "Can't open \"%s\" file\n", args->outfile);
		err = HTTP_INPUT_CANT_OPEN_OFILE;
		goto return_err;
	}

	rb = ring_buffer_init(boundary, out);
	if (!rb) {
		dprintf(console, "Can't init rb\n");
		err = HTTP_INPUT_RB_INIT_ERR;
		goto close_out;
	}

	for (i = args->counter_value; i < args->len; i++) {
		if (local_read(args, &ch, 1) != 1) {
			/* Error on read */
			dprintf(args->console, "Error on read @ count=%d\n",
			    args->counter_value);
			break;
		}
		if (ring_buffer_append(rb, ch)) {
			/*
			 * Found next or last boundary marker, nothing to do
			 * anymore.
			 */
			ring_buffer_drain(rb);
			args->filesize = ring_buffer_get_filesize(rb);

			/* Drain everything left in socket */
			fcntl(args->fd, F_SETFL, O_NONBLOCK);

			for (; args->counter_value != args->len; )
				local_read(args, buf, 1);

			dprintf(console,
			    "POST parsing done, we get new FW\n");
			http_ok();
			err = HTTP_INPUT_READY;

			break;
		}
	}

	if (err) {
		dprintf(console,
		    "POST parsing failed, last boundary marker not found\n");
		http_fail();
	}

	ring_buffer_free(rb);
close_out:
	local_close(out);
return_err:

	return (err);
}

#ifdef MODULE_TEST
int 
main(int argc, char **argv)
{
	struct args args;
	int console;

	bzero(&args, sizeof(args));

	if (argc < 2)
		exit(1);
	console = open("/dev/stdout", O_WRONLY);

	args.fd = open(argv[1], O_RDONLY);
	args.key = "YWRtaW46YWRtaW4=";
	args.outfile = "./test.out";
	http_input(&args, console);
	close(args.fd);
	close(console);
}
#endif

