/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EXTRACT_SECTION_GUID_H__
#define __EXTRACT_SECTION_GUID_H__

#include <Library/ExtractGuidedSectionLib.h>


//
// The GUID for this protocol mathes the Decompression scheme being used
// So for example LZMA would be gLzmaCustomDecompressGuid
//
typedef struct {
  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER   SectionGetInfo;
  EXTRACT_GUIDED_SECTION_DECODE_HANDLER     SectionExtraction;
} EXTRACT_SECTION_DATA;

typedef struct {
  EFI_HOB_GUID_TYPE     Hob;
  EXTRACT_SECTION_DATA  Data;
} EXTRACT_SECTION_HOB;

#endif

