/*
 * ProgramStore CM image generator
 * Copyright (C) 2000 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PROGRAMSTORE_H
#define PROGRAMSTORE_H

typedef struct _BcmProgramHeader 
	{
	uint16_t usSignature; 	// the unique signature may be specified as a command
									// line option: The default is: 0x3350

	uint16_t usControl;		// Control flags: currently defined lsb=1 for compression
									// remaining bits are currently reserved

	uint16_t usMajorRevision; // Major SW Program Revision
	uint16_t usMinorRevision; // Minor SW Program Revision
									// From a command line option this is specified as xxx.xxx
									// for the Major.Minor revision (note: Minor Revision is 3 digits)

	uint32_t ulcalendarTime;	// calendar time of this build (expressed as seconds since Jan 1 1970)

	uint32_t ulTotalCompressedLength;	// length of Program portion of file

	uint32_t ulProgramLoadAddress; // Address where program should be loaded (virtual, uncached)

	char cFilename[48]; 			// NULL terminated filename only (not pathname)
    char pad[8];                    // For future use

    uint32_t ulCompressedLength1; // When doing a dual-compression for Linux,
    uint32_t ulCompressedLength2; // it's necessary to save both lengths.

	uint16_t usHcs;			// 16-bit crc Header checksum (CRC_CCITT) over the header [usSignature through cFilename]
	uint16_t reserved;		// reserved
	uint32_t ulcrc;			// CRC-32 of Program portion of file (following the header)

	} BcmProgramHeader;




#endif
