/** @file
  LZMA Decompress GUIDed Section Extraction Library.
  It wraps Lzma decompress interfaces to GUIDed Section Extraction interfaces
  and registers them into GUIDed handler table.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/HobLib.h>
#include <Library/ExtractGuidedSectionLib.h>

#include <Guid/ExtractSection.h>
#include <Guid/LzmaDecompress.h>


/**
  Register LzmaDecompress and LzmaDecompressGetInfo handlers with LzmaCustomerDecompressGuid.

  @retval  RETURN_SUCCESS            Register successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to store this handler.
**/
EFI_STATUS
EFIAPI
LzmaDecompressLibConstructor (
  )
{
  EXTRACT_SECTION_HOB   *Hob;

  Hob = GetFirstGuidHob (&gLzmaCustomDecompressGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  // Locate Guided Hob

  return ExtractGuidedSectionRegisterHandlers (
          &gLzmaCustomDecompressGuid,
          Hob->Data.SectionGetInfo,
          Hob->Data.SectionExtraction
          );
}
