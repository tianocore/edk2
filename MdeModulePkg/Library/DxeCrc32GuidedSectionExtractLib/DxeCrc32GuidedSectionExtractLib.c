/** @file

  This library registers CRC32 guided section handler
  to parse CRC32 encapsulation section and extract raw data.
  It uses UEFI boot service CalculateCrc32 to authenticate 32 bit CRC value.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/Crc32GuidedSectionExtraction.h>
#include <Protocol/SecurityPolicy.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

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
  EFI_STATUS                Status;
  UINT32                    SectionCrc32Checksum;
  UINT32                    Crc32Checksum;
  UINT32                    OutputBufferSize;
  VOID                      *DummyInterface;

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
  // Init Checksum value to Zero.
  //
  Crc32Checksum = 0;

  //
  // Check whether there exists EFI_SECURITY_POLICY_PROTOCOL_GUID.
  //
  Status = gBS->LocateProtocol (&gEfiSecurityPolicyProtocolGuid, NULL, &DummyInterface);
  if (!EFI_ERROR (Status)) {
    //
    // If SecurityPolicy Protocol exist, AUTH platform override bit is set.
    //
    *AuthenticationStatus |= EFI_AUTH_STATUS_PLATFORM_OVERRIDE;
  } else {
    //
    // Calculate CRC32 Checksum of Image
    //
    Status = gBS->CalculateCrc32 (*OutputBuffer, OutputBufferSize, &Crc32Checksum);
    if (Status == EFI_SUCCESS) {
      if (Crc32Checksum != SectionCrc32Checksum) {
        //
        // If Crc32 checksum is not matched, AUTH tested failed bit is set.
        //
        *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
      }
    } else {
      //
      // If Crc32 checksum is not calculated, AUTH not tested bit is set.
      //
      *AuthenticationStatus |= EFI_AUTH_STATUS_NOT_TESTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Register the handler to extract CRC32 guided section.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS            Register successfully.
  @retval  EFI_OUT_OF_RESOURCES   No enough memory to register this handler.
**/
EFI_STATUS
EFIAPI
DxeCrc32GuidedSectionExtractLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return ExtractGuidedSectionRegisterHandlers (
          &gEfiCrc32GuidedSectionExtractionGuid,
          Crc32GuidedSectionGetInfo,
          Crc32GuidedSectionHandler
          );
}

