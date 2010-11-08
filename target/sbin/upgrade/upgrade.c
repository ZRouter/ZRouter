/********************************************************************
* Description:
* Author: Alex RAY <>
* Created at: Sat Mar 20 01:23:01 EET 2010
* Computer: terran.dlink.ua 
* System: FreeBSD 8.0-RELEASE-p2 on i386
*    
* Copyright (c) 2010 Alex RAY  All rights reserved.
*
********************************************************************/

/*
 * What ./upgrade do
 * ./upgrade -S 0x10000 new_firmware.img /dev/map/upgrade
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
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "image.h"

#define SYSCTL "kern.geom.debugflags"

#define DEFAULT_DEVICE "/dev/map/upgrade"

void usage()
{
	printf(
	    "./upgrade [-s 0x10000] [-D] -f new.img "
		"[-d /dev/device]\n"
	    "\t-f img     - New system image filename\n"
	    "\t-d dev     - Device name, default " DEFAULT_DEVICE "\n"
	    "\t-q         - Be silent\n"
	    "\t-R         - Don`t reboot when done\n"
	    "\t-s 0x10000 - use blocksize, default 64k\n"
	    "\t-V         - Don`t verify image\n"
	    );
	exit(1);
}

static int check_md5(FILE * fh, int blocksize)
{
	struct image_header header;
	unsigned char digest[16];
	MD5_CTX context;
	int i, size;
	char *buf = malloc(blocksize);

	if (!buf)
	{
		printf("Error allocating buffer[%d]\n", blocksize);
		return 1;
	}

	rewind(fh);

	if (fread((char *)&header, 1, sizeof(struct image_header), fh) != 
		sizeof(struct image_header))
	{
		printf("Error reading file header\n");
		free(buf);
		return 1;
	}

	MD5Init(&context);
	MD5Update(&context, &header.image_offset, sizeof(header.image_offset));
	MD5Update(&context,  header.image_device, sizeof(header.image_device));
	size = header.image_size;

	for ( ; (i = fread(buf, 1, (size > blocksize)?blocksize:size, fh));
		size -= i)
	{
		MD5Update(&context, buf, i);
	}
	MD5Final(digest, &context);

	free(buf);

	if ( memcmp(digest, header.image_digest, 16)) return 1;
	return 0;
}

static void
check_geom_debugflags16_enabled()
{
	int val = 0;
	size_t len;

	len = sizeof(val);
	if (sysctlbyname(SYSCTL, &val, &len, NULL, 0) != 0) {
		printf(SYSCTL " sysctl missing\n");
		exit(1);
	}
	if (!(val & 16)) {
		val |= 16;
		sysctlbyname(SYSCTL, NULL, NULL, &val, sizeof(val));
	}
}



int main(int argc, char **argv)
{
	int ch, i, c, verify = 1, check = 1, rebt = 1, silent = 0, blocksize = 0x10000, exitcode = 0;
	char *file = 0, *device = DEFAULT_DEVICE, *buf;
	FILE *fh, *ofh;

	while ((ch = getopt(argc, argv, "f:d:qRs:V")) != -1)
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'd':
			device = optarg;
			break;
		case 'q':
			silent = 1;
			break;
		case 'R':
			rebt = 0;
			break;
		case 's':
			blocksize = strtoul(optarg, 0, 0);
			break;
		case 'V':
			verify = 0;
			break;
		default:
			usage();
		}
	argv += optind;

	if (!file) usage();
	check_geom_debugflags16_enabled();

	if (!silent) printf("Use file %s\n", file);
	if (!silent) printf("Will write to %s\n", device);
	if (!silent) printf("Use blocksize %#x\n", blocksize);

	fh = fopen(file, "r");
	if ( !fh )
	{
		printf("Error opening file %s\n", file);
		exitcode = 1;
		goto just_exit;
	}

	buf = malloc(blocksize);
	if (!buf)
	{
		printf("Error allocating blocksize=%#x\n", blocksize);
		exitcode = 2;
		goto free_exit;
	}

	if (verify) check = check_md5(fh, blocksize);
	if (!silent && !check ) printf("Image check - %s\n", check?"FAIL":"ok" );

	ofh = fopen(device, "r+");
	if ( !ofh )
	{
		printf("Error opening device %s\n", device);
		exitcode = 1;
		goto close2_exit;
	}

	rewind(fh);

	/* 0xff - mean erased flash  */
	//memset(buf, 0xff, blocksize);
	bzero(buf, blocksize);
	setvbuf(stdout, NULL, _IONBF, 0);

	for ( c = 0; (i = fread(buf, 1, blocksize, fh)); c++ )
	{
		if (c % 10) printf(".");
		else      printf("%d", c);
		/* always write blocksize, not "i", like `dd conv=sync` */
		if (fwrite(buf, blocksize, 1, ofh) != 1)
		{
		    printf("Error when writing to %s, continue trying to make it done\n", device);
		    exitcode = 7;
		}
		//memset(buf, 0xff, blocksize);
		bzero(buf, blocksize);
	}
	printf("\n");
	/* sync */
	sync();

	if (verify) check = check_md5(ofh, blocksize);
	else 	sleep(3); /* For make shure, what flash write done */

	if (check)
	{
		printf("Verification fail\n");
		exitcode = 7;
	}

	if (rebt && !exitcode)
	{
		/*
		 * reboot only if we done without error
		 * if write return error, let user to fix it
		 * maybe filesystem still alive
		 */
		printf("Write done, now rebooting\n");
		reboot(RB_AUTOBOOT);
	}

close2_exit:
	fclose(ofh);
free_exit:
	free(buf);
	fclose(fh);
just_exit:
	exit(exitcode);

}



