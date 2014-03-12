/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __BOOTMON_FS_HW_H__
#define __BOOTMON_FS_HW_H__

#define MAX_NAME_LENGTH 32

#define HW_IMAGE_FOOTER_SIGNATURE_1 0x464C5348
#define HW_IMAGE_FOOTER_SIGNATURE_2 0x464F4F54

#define HW_IMAGE_FOOTER_VERSION     1
#define HW_IMAGE_FOOTER_OFFSET      92

#define HW_IMAGE_FOOTER_VERSION2    2
#define HW_IMAGE_FOOTER_OFFSET2     96

typedef struct {
  CHAR8  Filename[MAX_NAME_LENGTH];
  UINT32 Offset;
  UINT32 Version;
  UINT32 FooterSignature1;
  UINT32 FooterSignature2;
} HW_IMAGE_FOOTER;

#define HW_IMAGE_DESCRIPTION_REGION_MAX 4

// This structure is located at the end of a block when a file is present
typedef struct {
  UINT32  EntryPoint;
  UINT32  Attributes;
  UINT32  RegionCount;
  struct {
    UINT32 LoadAddress;
    UINT32 Size;
    UINT32 Offset;
    UINT32 Checksum;
  } Region[HW_IMAGE_DESCRIPTION_REGION_MAX];
  UINT32  BlockStart;
  UINT32  BlockEnd;
  UINT32  FooterChecksum;

  HW_IMAGE_FOOTER Footer;
} HW_IMAGE_DESCRIPTION;

#endif
