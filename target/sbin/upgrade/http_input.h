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

#define	LINE_BUFFER_LEN		256
#define	INPUT_BUFFER_SIZE	1024

struct args {
	int		console;
	int		fd;			/* Input FD */
	unsigned int	len;			/* Body len from Header */
	int		counter_value;		/* Current body position */
	int		enable_counter;		/* Switch header ->> body */

	char		*key;			/* Basic auth hash */
	char		*realm;;		/* HTTP Basic auth realm */

	char		*outfile;		/* Output filename */
	char		filename[255];		/* Firmware filename from form
						 data */
	unsigned int	filesize;		/* Collected file size */
	char		boundary[71];		/* Multipart boundary marker */
	unsigned int	inbuf_p;
	unsigned int	inbuf_l;
	char		inbuf[INPUT_BUFFER_SIZE];
};

enum http_input_status {
	HTTP_INPUT_READY = 0,
	HTTP_INPUT_SHOW_INDEX,
	HTTP_INPUT_AUTH_FAIL,
	HTTP_INPUT_BAD_MULTIPART,
	HTTP_INPUT_CANT_OPEN_OFILE,
	HTTP_INPUT_FW_PART_REQUIRED,
	HTTP_INPUT_METHOD_ERR,
	HTTP_INPUT_RB_INIT_ERR,
	HTTP_INPUT_READ_ERR
};

enum http_input_status	http_input(struct args *args, int console);

