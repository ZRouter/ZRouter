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

/*
 * What ./upgrade do
 * ./upgrade -S 0x10000 -f new_firmware.img -d /dev/map/upgrade
 * Check new_firmware.img signature and CRC
 * open /dev/map/upgrade
 * pre shutdown most services ( first recomend turn-off Radio )
 * maybe block data on WAN
 *
 * write image to /dev/map/upgrade with bs from -S arg or 64k (default)
 * close /dev/map/upgrade
 * kill -INT 1 (reboot)
 */

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/reboot.h>
#define _WITH_DPRINTF
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_MD5
#include <md5.h>
#endif

#include "image.h"
#ifdef WITH_HTTP_MODE
#include "http_input.h"
#endif

#define SYSCTL "kern.geom.debugflags"

#ifndef	DEFAULT_DEVICE
#define DEFAULT_DEVICE "/dev/map/upgrade"
#endif

#ifdef USE_GEOM_GET_SIZE
size_t	geom_get_size(char *);
#endif

int console;

void usage()
{
	dprintf(console, 
	    "./upgrade [-s 0x10000] [-R] [-V] -f new.img "
		"[-d /dev/device]\n"
	    "\t-f img     - New system image filename\n"
	    "\t-d dev     - Device name, default " DEFAULT_DEVICE "\n"
	    "\t-q         - Be silent\n"
	    "\t-R         - Don`t reboot when done\n"
	    "\t-s 0x10000 - use blocksize, default 64k\n"
#ifdef USE_MD5
	    "\t-V         - Don`t verify image\n"
#endif
#ifdef WITH_HTTP_MODE
	    "\t-h         - Enable HTTP mode\n"
	    "\t-H hash==  - HTTP Auth hash\n"
	    "\t-M realm   - HTTP Auth realm\n"
#endif
	    );
	exit(1);
}

#ifdef USE_MD5
static int check_md5(int fh, int blocksize)
{
	struct image_header header;
	unsigned char digest[16];
	MD5_CTX context;
	int i, size;
	char *buf = malloc(blocksize);

	if (!buf)
	{
		dprintf(console, "Error allocating buffer[%d]\n", blocksize);
		return 1;
	}

	lseek(fh, 0, SEEK_SET);

	if (read(fh, (char *)&header, sizeof(struct image_header)) != 
		sizeof(struct image_header))
	{
		dprintf(console, "Error reading file header\n");
		free(buf);
		return 1;
	}

	MD5Init(&context);
	MD5Update(&context, &header.image_offset, sizeof(header.image_offset));
	MD5Update(&context,  header.image_device, sizeof(header.image_device));
	size = header.image_size;

	for ( ; (i = read(fh, buf, (size > blocksize)?blocksize:size));
		size -= i)
	{
		MD5Update(&context, buf, i);
	}
	MD5Final(digest, &context);

	free(buf);

	if ( memcmp(digest, header.image_digest, 16)) return 1;
	return 0;
}
#endif

static void
check_geom_debugflags16_enabled()
{
	int val = 0;
	size_t len;

	len = sizeof(val);
	if (sysctlbyname(SYSCTL, &val, &len, NULL, 0) != 0) {
		dprintf(console, SYSCTL " sysctl missing\n");
		exit(1);
	}
	if (!(val & 16)) {
		val |= 16;
		sysctlbyname(SYSCTL, NULL, NULL, &val, sizeof(val));
	}
}



int main(int argc, char **argv)
{
	int ch, i, c, status, bytes;
	int check = 1, do_sync = 1, rebt = 1, silent = 0;
	int blocksize = 0x10000, exitcode = 0;
	char *file = 0, *device = DEFAULT_DEVICE, *buf;
	FILE *fh;
	int ofh;
#ifdef USE_GEOM_GET_SIZE
	int target_size_check = 1;
	size_t target_size;
	size_t file_size;
#endif
#ifdef USE_MD5
	int verify = 1;
#endif
#ifdef WITH_HTTP_MODE
	int http_mode = 0;
	struct args args;
#endif

	console = STDOUT_FILENO;
	status = 0;
	while ((ch = getopt(argc, argv, "f:d:l:qRSs:"
#ifdef USE_GEOM_GET_SIZE
	    "G"
#endif
#ifdef USE_MD5
	    "V"
#endif
#ifdef WITH_HTTP_MODE
	    "hH:M:"
#endif
	    )) != -1)
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'd':
			device = optarg;
			break;
#ifdef USE_GEOM_GET_SIZE
		case 'G':
			target_size_check = 0;
			break;
#endif
		case 'l':
			console = open(optarg, O_WRONLY);
			break;
		case 'q':
			silent = 1;
			break;
		case 'R':
			rebt = 0;
			break;
		case 'S':
			do_sync = 0;
			break;
		case 's':
			blocksize = strtoul(optarg, 0, 0);
			break;
#ifdef WITH_HTTP_MODE
		case 'h':
			http_mode = 1;
			break;
		case 'H':
			args.key = optarg;
			break;
		case 'M':
			args.realm = optarg;
			break;
#endif
#ifdef USE_MD5
		case 'V':
			verify = 0;
			break;
#endif
		default:
			dprintf(console, "Wrong key %c\n", ch);
			usage();
		}
	argv += optind;


	if (!file) {
		dprintf(console, "Input filename required\n");
		usage();
	}

#ifdef WITH_HTTP_MODE
	if (http_mode == 1) {
#ifdef USE_MD5
		verify = 0;	/* XXX: just for now */
#endif
		bzero(&args, sizeof(args));
		/* inetd mode, read request from STDIN */
		args.fd = STDIN_FILENO;
		args.outfile = file;

		status = http_input(&args, console);

		/*
		 * XXX: Should parse Firmware filename from POST data to get
		 * MD5 sum here. That will allow verify file checksum.
		 * it is args.filename.
		 */
		switch (status) {
		case HTTP_INPUT_READY:
			/* We get FW image in file $file */
			break;
		case HTTP_INPUT_SHOW_INDEX:
		case HTTP_INPUT_AUTH_FAIL:
			exitcode = 0;
			goto just_exit;
		case HTTP_INPUT_BAD_MULTIPART:
			if (!silent)
				dprintf(console, "Multipart format error\n");
			exitcode = 0;
			goto just_exit;
		case HTTP_INPUT_CANT_OPEN_OFILE:
			if (!silent)
				dprintf(console,
				    "Can't open \"%s\" file for writing\n");
			exitcode = 0;
			goto just_exit;
		case HTTP_INPUT_FW_PART_REQUIRED:
			if (!silent)
				dprintf(console, "Lack of FW part\n");
			exitcode = 1;
			goto just_exit;
		case HTTP_INPUT_METHOD_ERR:
			if (!silent)
				dprintf(console, "Wrong HTTP method\n");
			exitcode = 1;
			goto just_exit;
		case HTTP_INPUT_RB_INIT_ERR:
			if (!silent)
				dprintf(console,
				    "Can't allocate ringbuffer\n");
			exitcode = 1;
			goto just_exit;
		case HTTP_INPUT_READ_ERR:
			if (!silent)
				dprintf(console, "Error on read input\n");
			exitcode = 1;
			goto just_exit;
		}
	}
#endif

#ifdef USE_GEOM_GET_SIZE
	if (target_size_check)
		target_size = geom_get_size(device);
		if (target_size == 0) {
			dprintf(console, "Can't get size of %s\n", device);
			exitcode = 1;
			goto just_exit;
		}
#ifdef WITH_HTTP_MODE
		if (http_mode && args.filesize > target_size) {
			dprintf(console, "Input file too big to write into "
			    "%s\n", device);
			exitcode = 1;
			goto just_exit;
		}
#endif
#endif
	check_geom_debugflags16_enabled();

	if (!silent) dprintf(console, "Use file %s\n", file);
	if (!silent) dprintf(console, "Will write to %s\n", device);
	if (!silent) dprintf(console, "Use blocksize %#x\n", blocksize);

	fh = fopen(file, "r");
	if ( !fh )
	{
		dprintf(console, "Error opening file %s\n", file);
		exitcode = 1;
		goto just_exit;
	}

#ifdef USE_GEOM_GET_SIZE
	fseek(fh, 0L, SEEK_END);
	file_size = ftell(fh);
	if (file_size > target_size)
	{
		dprintf(console, "Input file too big to write into "
		    "%s (%i into %i)\n", device, file_size, target_size);
		exitcode = 1;
		fclose(fh);
		goto just_exit;
	}
	rewind(fh);
#endif

	buf = malloc(blocksize);
	if (!buf)
	{
		dprintf(console, "Error allocating blocksize=%#x\n", blocksize);
		exitcode = 2;
		goto free_exit;
	}

#ifdef USE_MD5
	if (verify) check = check_md5(fh, blocksize);
#endif
	if (!silent && !check )
		dprintf(console, "Image check - %s\n", check?"FAIL":"ok" );

	ofh = open(device, O_RDWR|O_DIRECT|O_SYNC);
	if ( ofh < 0 )
	{
		dprintf(console, "Error opening device %s (errno=%d)\n",
		    device, errno);
		exitcode = 1;
		goto close2_exit;
	}

	rewind(fh);

	bzero(buf, blocksize);
	/* Non buffered io for stdout */
	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * -- Non buffered io for flash device, to avoid use of big memory
	 * chunks. --
	 */
	/*
	 * REMOVED for output, otherwise fwrite will split blocks by 1024B
	 * pieces.
	 */

	for ( c = 0, bytes = 0; (i = fread(buf, 1, blocksize, fh));
	    c++, bytes += blocksize )
	{
		if (!silent) {
			if (c % 10) dprintf(console, ".");
			else      dprintf(console, "%d", c);
		}
		/* always write blocksize, not "i", like `dd conv=sync` */
		if (write(ofh, buf, blocksize) != blocksize)
		{
		    dprintf(console, "Error when writing to %s, "
			"continue, trying to make it done (errno=%d)\n",
			device, errno);
		    exitcode = 7;
		}
#ifdef WITH_HTTP_MODE
		if ((http_mode == 1) && (bytes >= args.filesize)) {
			/* Does not copy empty blocks */
			break;
		}
#endif
		bzero(buf, blocksize);
	}
	dprintf(console, "\n");

	/* sync */
	if (do_sync) {
		if (!silent)
			dprintf(console, "Sync buffers\n");
		sync();
		sync();
		sync();
	}

#ifdef USE_MD5
	if (verify) {
		if (!silent)
			dprintf(console, "Verify md5 sum\n");
		check = check_md5(ofh, blocksize);
		if (check)
		{
			dprintf(console, "Verification fail\n");
			exitcode = 7;
		}
	}
#endif

	close(ofh);
	ofh = 0;

	if (rebt && !exitcode)
	{
		/*
		 * reboot only if we done without error
		 * if write return error, let user to fix it
		 * maybe filesystem still alive
		 */
		dprintf(console, "Write done, now rebooting\n");
		if (do_sync) {
			if (!silent)
				dprintf(console, "reboot now ...\n");
			reboot(RB_AUTOBOOT);
		} else {
			if (!silent)
				dprintf(console, "reboot w/o sync now ...\n");
			reboot(RB_AUTOBOOT|RB_NOSYNC);
		}
	}

close2_exit:
	if (ofh)
		close(ofh);
free_exit:
	free(buf);
	fclose(fh);
just_exit:
	if (console != STDOUT_FILENO)
		close(console);
	exit(exitcode);

}

