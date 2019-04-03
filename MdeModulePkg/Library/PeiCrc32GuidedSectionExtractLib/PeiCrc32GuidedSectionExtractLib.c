/** @file

  This library registers CRC32 guided section handler
  to parse CRC32 encapsulation section and extract raw data.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Guid/Crc32GuidedSectionExtraction.h>
#include <Library/BaseLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

///
/// CRC32 Guided Section header
///
typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidedSectionHeader; ///< EFI guided section header
  UINT32                    CRC32Checksum;       ///< 32bit CRC check sum
} CRC32_SECTION_HEADER;

typedef struct {
  EFI_GUID_DEFINED_SECTION2 GuidedSectionHeader; ///< EFI guided section header
  UINT32                    CRC32Checksum;       ///< 32bit CRC check sum
} CRC32_SECTION2_HEADER;

/**

  GetInfo gets raw data size and attribute of the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param InputSection       Buffer containing the input GUIDed section to be processed.
  @param OutputBufferSize   The size of OutputBuffer.
  @param ScratchBufferSize  The size of ScratchBuffer.
  @param SectionAttribute   The attribute of the input guided section.

  @retval EFI_SUCCESS            The size of destination buffer, the size of scratch buffer and
                                 the attribute of the input section are successfully retrieved.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Crc32GuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
{
  if (IS_SECTION2 (InputSection)) {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionGuid,
        &(((EFI_GUID_DEFINED_SECTION2 *) InputSection)->SectionDefinitionGuid))) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Retrieve the size and attribute of the input section data.
    //
    *SectionAttribute  = ((EFI_GUID_DEFINED_SECTION2 *) InputSection)->Attributes;
    *ScratchBufferSize = 0;
    *OutputBufferSize  = SECTION2_SIZE (InputSection) - ((EFI_GUID_DEFINED_SECTION2 *) InputSection)->DataOffset;
  } else {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionGuid,
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Retrieve the size and attribute of the input section data.
    //
    *SectionAttribute  = ((EFI_GUID_DEFINED_SECTION *) InputSection)->Attributes;
    *ScratchBufferSize = 0;
    *OutputBufferSize  = SECTION_SIZE (InputSection) - ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset;
  }

  return EFI_SUCCESS;
}

/**

  Extraction handler tries to extract raw data from the input guided section.
  It also does authentication check for 32bit CRC value in the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param InputSection    Buffer containing the input GUIDed section to be processed.
  @param OutputBuffer    Buffer to contain the output raw data allocated by the caller.
  @param ScratchBuffer   A pointer to a caller-allocated buffer for function internal use.
  @param AuthenticationStatus A pointer to a caller-allocated UINT32 that indicates the
                              authentication status of the output buffer.

  @retval EFI_SUCCESS            Section Data and Auth Status is extracted successfully.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Crc32GuidedSectionHandler (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus
  )
{
  UINT32      SectionCrc32Checksum;
  UINT32      Crc32Checksum;
  UINT32      OutputBufferSize;

  if (IS_SECTION2 (InputSection)) {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionGuid,
        &(((EFI_GUID_DEFINED_SECTION2 *) InputSection)->SectionDefinitionGuid))) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Get section Crc32 checksum.
    //
    SectionCrc32Checksum = ((CRC32_SECTION2_HEADER *) InputSection)->CRC32Checksum;
    *OutputBuffer      = (UINT8 *) InputSection + ((EFI_GUID_DEFINED_SECTION2 *) InputSection)->DataOffset;
    OutputBufferSize   = SECTION2_SIZE (InputSection) - ((EFI_GUID_DEFINED_SECTION2 *) InputSection)->DataOffset;

    //
    // Implicitly CRC32 GUIDed section should have STATUS_VALID bit set
    //
    ASSERT (((EFI_GUID_DEFINED_SECTION2 *) InputSection)->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID);
    *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;
  } else {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionGuid,
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Get section Crc32 checksum.
    //
    SectionCrc32Checksum = ((CRC32_SECTION_HEADER *) InputSection)->CRC32Checksum;
    *OutputBuffer      = (UINT8 *) InputSection + ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset;
    OutputBufferSize   = SECTION_SIZE (InputSection) - ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset;

    //
    // Implicitly CRC32 GUIDed section should have STATUS_VALID bit set
    //
    ASSERT (((EFI_GUID_DEFINED_SECTION *) InputSection)->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID);
    *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;
  }

  //
  // Calculate CRC32 Checksum of Image
  //
  Crc32Checksum = CalculateCrc32 (*OutputBuffer, OutputBufferSize);
  if (Crc32Checksum != SectionCrc32Checksum) {
    //
    // If Crc32 checksum is not matched, AUTH tested failed bit is set.
    //
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
  }

  //
  // Temp solution until PeiCore checks AUTH Status.
  //
  if ((*AuthenticationStatus & (EFI_AUTH_STATUS_TEST_FAILED | EFI_AUTH_STATUS_NOT_TESTED)) != 0) {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Register the handler to extract CRC32 guided section.

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval  EFI_SUCCESS           Register successfully.
  @retval  EFI_OUT_OF_RESOURCES  Not enough memory to register this handler.

**/
EFI_STATUS
EFIAPI
PeiCrc32GuidedSectionExtractLibConstructor (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  return ExtractGuidedSectionRegisterHandlers (
          &gEfiCrc32GuidedSectionExtractionGuid,
          Crc32GuidedSectionGetInfo,
          Crc32GuidedSectionHandler
          );
}
