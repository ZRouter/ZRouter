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
#include <sys/reboot.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "image.h"


#define DEFAULT_OUTFILE "DIR-320_FBSD.img"

void usage()
{
	printf(
	    "./upgrade [-s 0x10000] -k kernel.lzma -r rootfs.iso.ulzma [-o outfile.img]\n"
	    "\t-k img     - Kernel image filename\n"
	    "\t-o img     - output file name, default " DEFAULT_OUTFILE "\n"
	    "\t-q         - Be silent\n"
	    "\t-r img     - Rootfs image filename\n"
	    "\t-s 0x10000 - use blocksize, default 64k\n"
	    );
	exit(1);
}

int main(int argc, char **argv)
{
	int ch, i, silent = 0, blocksize = 0x10000, exitcode = 0, tmpfd;
	char *kernel = 0, *rootfs = 0, *outfile = DEFAULT_OUTFILE, *buf, tmp[64];
	struct image_header header;
	struct image_split_header split;
	unsigned char digest[16];
	MD5_CTX context;
	FILE *fh = 0, *ofh = 0;

	while ((ch = getopt(argc, argv, "k:o:qr:s:")) != -1)
		switch (ch) {
		case 'k':
			kernel = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'q':
			silent = 1;
			break;
		case 'r':
			rootfs = optarg;
			break;
		case 's':
			blocksize = strtoul(optarg, 0, 0);
			break;
		default:
			usage();
		}
	argv += optind;

	if (!kernel || !rootfs) usage();

	if (!silent) printf("Use kernel file %s\n", kernel);
	if (!silent) printf("Use rootfs file %s\n", rootfs);
	if (!silent) printf("Use blocksize %#x\n", blocksize);
	if (!silent) printf("Will write to %s\n", outfile);


	buf = malloc(blocksize);
	if (!buf)
	{
		printf("Error allocating blocksize=%#x\n", blocksize);
		exitcode = 2;
		goto error_exit;
	}


	/* Pack image */
	/* Write kernel with blocksize padding with zeros */

	strcpy(tmp, "/tmp/temp.XXXXXX");
	tmpfd = mkstemp(tmp);

	ofh = fdopen(tmpfd, "w+");
	if ( !ofh )
	{
		printf("Error opening tmp file %s\n", tmp);
		exitcode = 3;
		goto error_exit;
	}

	fh = fopen(kernel, "r");
	if ( !fh )
	{
		printf("Error opening file %s\n", kernel);
		exitcode = 3;
		goto error_exit;
	}

	bzero(buf, blocksize);

	for ( ; (i = fread(buf, 1, blocksize, fh)); )
	{
		/* always write blocksize, not "i", like `dd conv=sync` */
		if (fwrite(buf, 1, blocksize, ofh) != blocksize)
		{
		    printf("Error when writing to %s\n", outfile);
		    exitcode = 4;
		    goto error_exit;
		}
		bzero(buf, blocksize);
	}
	fclose(fh);
	fh = 0;

	/* Open rootfs */
	fh = fopen(rootfs, "r");
	if ( !fh )
	{
		printf("Error opening file %s\n", rootfs);
		exitcode = 3;
		goto error_exit;
	}

	/* Append split signature '--PaCkImGs--'[16], size[4], zero[12] */
	bzero(&split, sizeof(struct image_split_header));
	strcpy(split.split_signature, DIR_320_DEFAULT_SPLIT_SIGNATURE);
	fseek(fh, 0, SEEK_END);
	fgetpos(fh, (fpos_t *)&split.image_size);
	split.image_size = 
		((split.image_size & 0x000000ff) << 24) |
		((split.image_size & 0x0000ff00) << 8) |
		((split.image_size & 0x00ff0000) >> 8) |
		((split.image_size & 0xff000000) >> 24) ;

	if (fwrite(&split, 1, sizeof(struct image_split_header), ofh) != sizeof(struct image_split_header))
	{
		    printf("Error when writing to %s\n", outfile);
		    exitcode = 4;
		    goto error_exit;
	}


	/* return to 0 */
	rewind(fh);
	/* Append rootfs */
	for ( ; (i = fread(buf, 1, blocksize, fh)); )
	{
		if (fwrite(buf, 1, i, ofh) != i)
		{
		    printf("Error when writing to %s\n", outfile);
		    exitcode = 4;
		    goto error_exit;
		}
	}
	fclose(fh);
	fh = 0;

	bzero(&header, sizeof(struct image_header));
	strcpy(header.image_name, DIR_320_DEFAULT_IMAGE_NAME);
	header.magic_kernel = DIR_320_DEFAULT_KERNEL_MAGIC;
	header.magic_rootfs = DIR_320_DEFAULT_ROOTFS_MAGIC;
	header.image_offset = 0;
	strcpy(header.image_device, DIR_320_DEFAULT_DEVICE);



	fh = ofh;
	ofh = fopen(outfile, "w+");
	if ( !ofh )
	{
		printf("Error opening file %s\n", outfile);
		exitcode = 3;
		goto error_exit;
	}

	/* for no use stat :) */
	fseek(fh, 0, SEEK_END);
	fgetpos(fh, (fpos_t *)&header.image_size);
	rewind(fh);
	MD5Init(&context);
	MD5Update(&context, &header.image_offset, sizeof(header.image_offset));
	MD5Update(&context,  header.image_device, sizeof(header.image_device));

	for ( ; (i = fread(buf, 1, blocksize, fh)); )
	{
		MD5Update(&context, buf, i);
	}
	MD5Final(digest, &context);
	memcpy(header.image_digest, digest, 16);


	ofh = fopen(outfile, "w");
	if ( !ofh )
	{
		printf("Error opening outfile %s\n", outfile);
		exitcode = 3;
		goto error_exit;
	}


	if (fwrite(&header, 1, sizeof(struct image_header), ofh) != sizeof(struct image_header))
	{
	    printf("Error writing header to %s\n", outfile);
	    exitcode = 4;
	    goto error_exit;
	}


	rewind(fh);
	for ( ; (i = fread(buf, 1, blocksize, fh)); )
	{
		if (fwrite(buf, 1, i, ofh) != i)
		{
		    printf("Error when writing to %s\n", outfile);
		    exitcode = 4;
		}
	}

	fseek(ofh, 0, SEEK_END);
	fgetpos(ofh, (fpos_t *)&i);
	if ( i > ((64-6) * 0x10000))
	{
		printf("Warning: image size = %d Bytes,"
			" more than upgradefs size %d\n", i, ((64-6) * 0x10000));
		exitcode = 5;
	}


error_exit:
	if (ofh) fclose(ofh);
	if (fh ) fclose(fh);
	if (buf) free(buf);
	exit(exitcode);

}



