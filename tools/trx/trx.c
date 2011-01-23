#include <sys/types.h>
#include <sys/endian.h>
#include <getopt.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>


struct trx_header {
    uint32_t magic;		/* "HDR0" */
    uint32_t len;		/* Length of file including header */
    uint32_t crc32;		/* 32-bit CRC from flag_version to end of file */
    uint16_t flag;		/* flags */
    uint16_t version;		/* version */
    uint32_t offsets[3];	/* Offsets of partitions from start of header */
};

//48 44 52 30
//00 60 17 00
//6C B2 BE D8
//00 00
//01 00
//1C 00 00 00
//D8 08 00 00
//00 C4 07 00
//1F 8B 08 00
//-    fprintf(stderr, "Usage: trx [-o outfile] [-m maxlen] [-a align] [-b offset] [-f file] [-f file [-f file]]\n");
//+    fprintf(stderr, "Usage: trx [-o outfile] [-m maxlen] [-a align] [-b absolute offset] [-x relative offset] [-f file] [-f file [-f file]]\n");
///home/oconnor/Desktop/kamikaze_7.09/staging_dir_mipsel/bin/trx 
//	-o /home/oconnor/Desktop/kamikaze_7.09/bin/openwrt-brcm47xx-2.6-squashfs.trx 
//	-f /home/oconnor/Desktop/kamikaze_7.09/build_mipsel/linux-2.6-brcm47xx/loader.gz 
//	-f /home/oconnor/Desktop/kamikaze_7.09/build_mipsel/linux-2.6-brcm47xx/vmlinux.lzma -a 1024 
//	-f /home/oconnor/Desktop/kamikaze_7.09/build_mipsel/linux-2.6-brcm47xx/root.squashfs -a 0x10000 
//	-A /home/oconnor/Desktop/kamikaze_7.09/build_mipsel/linux-2.6-brcm47xx/fs_mark
void
usage()
{
    printf("Usage: trx [-o outfile] [-m maxlen] [-a align] [-b offset] [-f file] [-f file [-f file]]\n");
}

long addfile(FILE *out, char *filename, uint32_t align, uint32_t *offset, uLong *crc);

int
main(int argc, char **argv)
{
	FILE *out;
	char ch, *check;
	char *filenames[3], *outfilename = "image.trx";
	u_int32_t files = 0, align[3], offset[3], maxlen = 0x5A0000;
	int i;
	struct trx_header header;
	long len;

	for (i = 0; i < 3; i ++) {
		filenames[i] = 0;
		align[i]     = 4;
		offset[i]    = 0;
	}

	while ((ch = getopt(argc, argv, "a:b:f:m:o:")) != -1)
		switch (ch) {
		case 'a':
			align[files] = strtoul(optarg, &check, 0);
			if (check != (optarg + strlen(optarg)))
				errx(1, "%s - wrong align value", optarg);
			break;
		case 'b':
			offset[files] = strtoul(optarg, &check, 0);
			if (check != (optarg + strlen(optarg)))
				errx(1, "%s - wrong offset value", optarg);
			break;
		case 'f':
			filenames[files++] = optarg;
			break;
		case 'm':
			maxlen = strtoul(optarg, &check, 0);
			if (check != (optarg + strlen(optarg)))
				errx(1, "%s - wrong maxlen value", optarg);
			break;
		case 'o':
			outfilename = optarg;
			break;
		default:
			usage();
		}
	argv += optind;

	printf("maxlen = %u, out filename = %s files => [", maxlen, outfilename);
	for (i = 0; i < files; i ++) {
		printf("\"%s\"(align = %u, offset = %u)%s", filenames[i], align[i], offset[i], (i != (files-1))?", ":"");
	}
	printf("]\n");

	out = fopen(outfilename, "w");
	if (!out)
		errx(2, "Can`t open file %s", outfilename);

	uLong crc = crc32(0L, Z_NULL, 0);

	len = sizeof(struct trx_header); /* Host endian yet */

	header.magic = htole32(0x30524448); /* HDR0 */
	header.flag = htole16(0);
	header.version = htole16(1);

	fwrite(&header, sizeof(struct trx_header), 1, out);

	for (i = 0; i < files; i ++) {
		int size = addfile(out, filenames[i], align[i], &offset[i], &crc);
		if (size <= 0)
			errx(2, "");
		len += (uint32_t)size;
		header.offsets[i] = htole32(offset[i]);
	}
	rewind(out);

	header.crc32 = htole32((uint32_t)crc);
	header.len = htole32(len); /* Now LE */

	fwrite(&header, sizeof(struct trx_header), 1, out);

	fclose(out);
}

long
addfile(FILE *out, char *filename, uint32_t align, uint32_t *offset, uLong *crc)
{
	FILE * in;
	uint8_t z = 0;
	long cpos, i, s, prep = 0;

	in = fopen(filename, "r");
	if (!in) {
		printf("Can`t open source file %s\n", filename);
		return (-1);
	}

	// fstat(fileno(in), &st);

	cpos = ftell(out);
	printf("cpos = %d\n", cpos);
	if (*offset > 0) {
		if (*offset < cpos) {
			printf("Can`t overwrite existing content\n");
			return (-1);
		}

		prep = (*offset - cpos);
		fwrite(&z, 1, prep, out);
	}

	*offset = ftell(out);
	printf("cpos = %d\n", *offset);
	uint8_t *buf = malloc(1024);

	for ( i = 0; (s = fread(buf, 1, 1024, in)); i += s) {
		*crc = crc32(*crc, buf, s);
		fwrite(buf, 1, s, out);
	}

	free(buf);

	fclose(in);
	return (i+prep);
}
