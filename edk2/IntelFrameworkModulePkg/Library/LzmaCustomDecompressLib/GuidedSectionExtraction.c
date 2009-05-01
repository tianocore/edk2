/** @file
  LZMA Decompress GUIDed Section Extraction Library

  Copyright (c) 2006 - 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Guid/LzmaDecompress.h>

#include "LzmaDecompress.h"


STATIC
RETURN_STATUS
EFIAPI
LzmaGuidedSectionGetCompressedLocation (
  IN  CONST VOID  *InputSection,
  OUT VOID        **LmzaCompressedData,
  OUT UINT32      *LmzaCompressedDataSize   OPTIONAL
  )
{
  if (!CompareGuid (
        &gLzmaCustomDecompressGuid, 
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Retrieve the size and attribute of the input section data.
  //
  *LmzaCompressedData =
    (VOID*) (
        (UINT8 *) InputSection +
        ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset
      );
  if (LmzaCompressedDataSize != NULL) {
    *LmzaCompressedDataSize =
      (UINT32)(
        (
          (*(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size)) &
          0x00ffffff
        ) -
        ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset
      );
  }

  return RETURN_SUCCESS;
}


/**
  The implementation of 'GetInfo' for Guided Section
  Extraction of LZMA compression.

  @param  InputSection          Buffer containing the input GUIDed section to be processed. 
  @param  OutputBufferSize      The size of OutputBuffer.
  @param  ScratchBufferSize     The size of ScratchBuffer.
  @param  SectionAttribute      The attribute of the input guided section.

  @retval RETURN_SUCCESS            The size of destination buffer and the size of scratch buffer are successull retrieved.
  @retval RETURN_INVALID_PARAMETER  The source data is corrupted, or
                                    The GUID in InputSection does not match this instance guid.

**/
RETURN_STATUS
EFIAPI
LzmaGuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
{
  RETURN_STATUS Status;
  VOID          *LzmaInput;
  UINT32        LzmaInputSize;

  Status = LzmaGuidedSectionGetCompressedLocation(
    InputSection,
    &LzmaInput,
    &LzmaInputSize
    );
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  *SectionAttribute = ((EFI_GUID_DEFINED_SECTION *) InputSection)->Attributes;

  return LzmaUefiDecompressGetInfo (
    LzmaInput,
    LzmaInputSize,
    OutputBufferSize,
    ScratchBufferSize
    );
}

/**
  The implementation of Guided Section Extraction
  for LZMA compression.

  @param  InputSection          Buffer containing the input GUIDed section to be processed. 
  @param  OutputBuffer          OutputBuffer to point to the start of the section's contents.
                                if guided data is not prcessed. Otherwise,
                                OutputBuffer to contain the output data, which is allocated by the caller.
  @param  ScratchBuffer         A pointer to a caller-allocated buffer for function internal use. 
  @param  AuthenticationStatus  A pointer to a caller-allocated UINT32 that indicates the
                                authentication status of the output buffer. 

  @retval RETURN_SUCCESS            Decompression is successfull
  @retval RETURN_INVALID_PARAMETER  The source data is corrupted, or
                                    The GUID in InputSection does not match this instance guid.

**/
RETURN_STATUS
EFIAPI
LzmaGuidedSectionExtraction (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus
  )
{
  RETURN_STATUS Status;
  VOID          *LzmaInput;

  Status = LzmaGuidedSectionGetCompressedLocation(
    InputSection,
    &LzmaInput,
    NULL
    );
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Authentication is set to Zero, which may be ignored.
  //
  *AuthenticationStatus = 0;

  return LzmaUefiDecompress (
    LzmaInput,
    *OutputBuffer,
    ScratchBuffer
    );
}


/**
  Register LzmaDecompress handler.

  @retval  RETURN_SUCCESS            Register successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to store this handler.
**/
EFI_STATUS
EFIAPI
LzmaDecompressLibConstructor (
  )
{
  return ExtractGuidedSectionRegisterHandlers (
          &gLzmaCustomDecompressGuid,
          LzmaGuidedSectionGetInfo,
          LzmaGuidedSectionExtraction
          );      
}

