
#include <sys/endian.h>
#include <sys/types.h>
#include <err.h>
#include <libutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

struct header {
	uint32_t sign[3];	/* 0x00000000, 0x00011d7a, 0xc3e6e6ab */
	uint32_t hdrsize;	/* 0x00000094 */
	uint32_t size;
	uint32_t sum;
	uint32_t count;		/* count or not? of what? now is 1. */
	char fw_release[0x0c];	/* "1.02B07" */
	char hw_version[0x0c];	/* "0.1" */
	char hw_name[0x20];	/* "DSR-1000N" */
	char date[0x20];	/* "Mon Jul 26 22:49:27 2010" */
	char img_name[0x8c];	/* "DSR-1000N_A1_FW1.02B07_WW" */
	char pad[0x100];
};

int
usage()
{

	printf("Usage: make_dlink_dsr_image [-o out_file_name] kernel_rootfs"
	    " DSR-1000N 9.0Z03 0.1 A1 WW\n"
	    "\t-o out_file_name - Name of output filename\n"
	    "\tkernel_rootfs - packed kernel + rootfs\n"
	    "\tDSR-1000N\t- target device name\n"
	    "\t9.0Z03\t- FW release number\n"
	    "\t0.1\t- target H/W version\n"
	    "\tA1\t- target H/W version name\n"
	    "\tWW\t- counry code (WW - for World Wide)\n");

	exit(1);
}

int
mksum(uint8_t *buf, int size)
{
	int i, sum = 0;

	for (i = 0; i < size; i++)
		sum += buf[i];

	return (sum);
}

int
main(int argc, char **argv)
{
	struct header hdr;
	char *inimage, *hw_name, *hw_version, *hw_vername, *fw_release;
	char *country, buf[512], *outimage = NULL;
	int i = 1;
	time_t tm;
	FILE *in, *out;
	int sum = 0;

	if (argc < 5)
		usage();

	bzero(&hdr, sizeof(hdr));

	if (argv[i][0] == '-' && argv[i][1] == 'o' && argv[i][2] == '\0') {
		outimage = argv[i+1];
		i += 2;
	}

	inimage = argv[i++];
	hw_name = argv[i++];
	fw_release = argv[i++];
	hw_version = argv[i++];
	hw_vername = argv[i++];
	country = argv[i++];
	if (!country)
		country = "WW";

#define	CHECK_FIELD_LENGTH(_f)					\
	if (strlen(_f) > (sizeof(hdr._f) - 1)) {		\
		printf(#_f " too long, maxlen = %d\n",		\
		    (sizeof(hdr._f) - 1));			\
		exit(1);					\
	} else {						\
		strncpy(hdr._f, _f, sizeof(hdr._f));		\
	}

	CHECK_FIELD_LENGTH(fw_release);
	CHECK_FIELD_LENGTH(hw_name);
	CHECK_FIELD_LENGTH(hw_version);

#undef	CHECK_FIELD_LENGTH

	hdr.sign[0] = 0x00000000;
	hdr.sign[1] = 0x7a1d0100;
	hdr.sign[2] = 0xabe6e6c3;
	hdr.sign[3] = htobe32(0x94);

	time(&tm);
	strncpy(hdr.date, ctime(&tm), sizeof(hdr.date));
	hdr.count = htobe32(1);

	snprintf(hdr.img_name, sizeof(hdr.img_name), "%s_%s_FW%s_%s",
	    hw_name, hw_vername, fw_release, country);

	if (!outimage)
		outimage = hdr.img_name;

	printf("Output Filename: %s\n", outimage);

	in = fopen(inimage, "r");
	if (!in) {
		errx(2, "Can't open file \"%s\" for writing", in);
	}

	/* Get input file size */
	fseek(in, 0, SEEK_END);
	hdr.size = htobe32(ftell(in) + sizeof(hdr));

	sum += mksum((uint8_t *)&hdr, sizeof(hdr));
	/* Get images sum */
	fseek(in, 0, SEEK_SET);
	while ((i = fread(buf, 1, 512, in))) {
		sum += mksum(buf, i);
		bzero(buf, 512);
	}
	hdr.sum = htobe32(sum);

	out = fopen(outimage, "w+");
	if (!out) {
		fclose(in);
		errx(2, "Can't open file \"%s\" for writing", outimage);
	}

	if (fwrite(&hdr, 1, sizeof(hdr), out) != sizeof(hdr))
		printf("Error writing image header to \"%s\" file\n", outimage);

	fseek(in, 0, SEEK_SET);
	while ((i = fread(buf, 1, 512, in))) {
		if (fwrite(buf, 1, i, out) != i)
			printf("Error writing \"%s\" file\n", outimage);
	}

	fclose(in);

	fclose(out);

	return (0);
}

