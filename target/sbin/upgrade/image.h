/*-
 * Copyright (c) 2010 Rybalko Aleksandr.
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

#ifndef _IMAGE_H_
#define _IMAGE_H_

struct image_header {
	char     image_name[32];	/* 0x00 */
	uint32_t magic_kernel;		/* 0x20 */
	uint32_t magic_rootfs;		/* 0x24 */
	uint32_t image_size;		/* 0x28 */
	uint32_t image_offset;		/* 0x2c */
	char     image_device[32];	/* 0x30 */
	uint8_t  image_digest[16];	/* 0x50 */
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
