/** @file
  Fsp related definitions

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FSP_H__
#define __FSP_H__

//
// Fv Header
//
#define FVH_SIGINATURE_OFFSET         0x28
#define FVH_SIGINATURE_VALID_VALUE    0x4856465F  // valid signature:_FVH
#define FVH_HEADER_LENGTH_OFFSET      0x30
#define FVH_EXTHEADER_OFFSET_OFFSET   0x34
#define FVH_EXTHEADER_SIZE_OFFSET     0x10

//
// Ffs Header
//
#define FSP_HEADER_GUID_DWORD1        0x912740BE
#define FSP_HEADER_GUID_DWORD2        0x47342284
#define FSP_HEADER_GUID_DWORD3        0xB08471B9
#define FSP_HEADER_GUID_DWORD4        0x0C3F3527
#define FFS_HEADER_SIZE_VALUE         0x18

//
// Section Header
//
#define SECTION_HEADER_TYPE_OFFSET    0x03
#define RAW_SECTION_HEADER_SIZE_VALUE 0x04

//
// Fsp Header
//
#define FSP_HEADER_IMAGEBASE_OFFSET     0x1C
#define FSP_HEADER_TEMPRAMINIT_OFFSET   0x30

#endif
