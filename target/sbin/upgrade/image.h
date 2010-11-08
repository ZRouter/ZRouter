#ifndef _IMAGE_H_
#define _IMAGE_H_

struct image_header {
	char     image_name[32];	/* 0x00 */
	uint32_t magic_kernel;		/* 0x20  */
	uint32_t magic_rootfs;		/* 0x24  */
	uint32_t image_size;		/* 0x28  */
	uint32_t image_offset;		/* 0x2c  */
	char     image_device[32];	/* 0x30  */
	uint8_t  image_digest[16];	/* 0x50  */
};

struct image_split_header {
	char     split_signature[16];
	uint32_t image_size;
	uint32_t image_offset;
	uint32_t image_pad[2];
};

#define DIR_320_DEFAULT_IMAGE_NAME "wrgg27_dlwbr_dir320"

#define DIR_320_DEFAULT_MAGIC 	0x20040220

#define DIR_320_DEFAULT_DEVICE 	"/dev/mtdblock/2"

#define DIR_320_DEFAULT_KERNEL_MAGIC 	DIR_320_DEFAULT_MAGIC
#define DIR_320_DEFAULT_ROOTFS_MAGIC 	DIR_320_DEFAULT_MAGIC

#define DIR_320_DEFAULT_SPLIT_SIGNATURE "--PaCkImGs--"


#endif /* _IMAGE_H_ */
