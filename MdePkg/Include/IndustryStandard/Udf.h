/** @file
  OSTA Universal Disk Format (UDF) definitions.

  Copyright (C) 2014-2017 Paulo Alcantara <pcacjr@zytor.com>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __UDF_H__
#define __UDF_H__

#define UDF_BEA_IDENTIFIER   "BEA01"
#define UDF_NSR2_IDENTIFIER  "NSR02"
#define UDF_NSR3_IDENTIFIER  "NSR03"
#define UDF_TEA_IDENTIFIER   "TEA01"

#define UDF_LOGICAL_SECTOR_SHIFT  11
#define UDF_LOGICAL_SECTOR_SIZE   ((UINT64)(1ULL << UDF_LOGICAL_SECTOR_SHIFT))
#define UDF_VRS_START_OFFSET      ((UINT64)(16ULL << UDF_LOGICAL_SECTOR_SHIFT))

#define _GET_TAG_ID(_Pointer) \
  (((UDF_DESCRIPTOR_TAG *)(_Pointer))->TagIdentifier)

#define IS_AVDP(_Pointer) \
  ((BOOLEAN)(_GET_TAG_ID (_Pointer) == 2))

#pragma pack(1)

typedef struct {
  UINT16  TagIdentifier;
  UINT16  DescriptorVersion;
  UINT8   TagChecksum;
  UINT8   Reserved;
  UINT16  TagSerialNumber;
  UINT16  DescriptorCRC;
  UINT16  DescriptorCRCLength;
  UINT32  TagLocation;
} UDF_DESCRIPTOR_TAG;

typedef struct {
  UINT32  ExtentLength;
  UINT32  ExtentLocation;
} UDF_EXTENT_AD;

typedef struct {
  UDF_DESCRIPTOR_TAG  DescriptorTag;
  UDF_EXTENT_AD       MainVolumeDescriptorSequenceExtent;
  UDF_EXTENT_AD       ReserveVolumeDescriptorSequenceExtent;
  UINT8               Reserved[480];
} UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER;

#pragma pack()

#endif
