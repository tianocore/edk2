/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EXTRACT_SECTION_GUID_H__
#define __EXTRACT_SECTION_GUID_H__

#include <Library/ExtractGuidedSectionLib.h>

//
// The GUID for this protocol mathes the Decompression scheme being used
// So for example LZMA would be gLzmaCustomDecompressGuid
//
typedef struct {
  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER    SectionGetInfo;
  EXTRACT_GUIDED_SECTION_DECODE_HANDLER      SectionExtraction;
} EXTRACT_SECTION_DATA;

typedef struct {
  EFI_HOB_GUID_TYPE       Hob;
  EXTRACT_SECTION_DATA    Data;
} EXTRACT_SECTION_HOB;

#endif
