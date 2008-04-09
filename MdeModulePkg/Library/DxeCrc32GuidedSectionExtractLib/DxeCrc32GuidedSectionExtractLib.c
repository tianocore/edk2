/** @file

  Implements CRC32 guided section handler to parse CRC32 encapsulation section, 
  extract data and authenticate 32 bit CRC value.

Copyright (c) 2007 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiDxe.h>
#include <Protocol/Crc32GuidedSectionExtraction.h>
#include <Protocol/SecurityPolicy.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidedSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

EFI_STATUS
EFIAPI
Crc32GuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
/*++

Routine Description:

  The implementation of Crc32 guided section GetInfo().

Arguments:
  InputSection          Buffer containing the input GUIDed section to be processed. 
  OutputBufferSize      The size of OutputBuffer.
  ScratchBufferSize     The size of ScratchBuffer.
  SectionAttribute      The attribute of the input guided section.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted, or
                          The GUID in InputSection does not match this instance guid.

--*/
{
  if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionProtocolGuid, 
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Retrieve the size and attribute of the input section data.
  //
  *SectionAttribute  = ((EFI_GUID_DEFINED_SECTION *) InputSection)->Attributes;
  *ScratchBufferSize = 0;
  *OutputBufferSize  = *(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size) & 0x00ffffff;
  *OutputBufferSize  -= ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Crc32GuidedSectionHandler (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus
  )
/*++

Routine Description:

  The implementation of Crc32 Guided section extraction.

Arguments:
  InputSection           Buffer containing the input GUIDed section to be processed. 
  OutputBuffer           OutputBuffer to point to the start of the section's contents.
                         if guided data is not prcessed. Otherwise,
                         OutputBuffer to contain the output data, which is allocated by the caller.
  ScratchBuffer          A pointer to a caller-allocated buffer for function internal use. 
  AuthenticationStatus   A pointer to a caller-allocated UINT32 that indicates the
                         authentication status of the output buffer. 

Returns:

  RETURN_SUCCESS           - Decompression is successfull
  RETURN_INVALID_PARAMETER - The source data is corrupted, or
                             The GUID in InputSection does not match this instance guid.

--*/
{
  EFI_STATUS                Status;
  CRC32_SECTION_HEADER      *Crc32SectionHeader;
  UINT32                    Crc32Checksum;
  UINT32                    OutputBufferSize;
  VOID                      *DummyInterface;

  if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionProtocolGuid, 
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
    return EFI_INVALID_PARAMETER;
  }

  Crc32Checksum = 0;
  //
  // Points to the Crc32 section header
  //
  Crc32SectionHeader = (CRC32_SECTION_HEADER *) InputSection;
  *OutputBuffer      = (UINT8 *) InputSection + Crc32SectionHeader->GuidedSectionHeader.DataOffset;
  OutputBufferSize   = *(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size) & 0x00ffffff; 
  OutputBufferSize   -= Crc32SectionHeader->GuidedSectionHeader.DataOffset;

  //
  // Implictly CRC32 GUIDed section should have STATUS_VALID bit set
  //
  ASSERT (Crc32SectionHeader->GuidedSectionHeader.Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID);
  *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;

  //
  // Check whether there exists EFI_SECURITY_POLICY_PROTOCOL_GUID.
  //
  Status = gBS->LocateProtocol (&gEfiSecurityPolicyProtocolGuid, NULL, &DummyInterface);
  if (!EFI_ERROR (Status)) {
    *AuthenticationStatus |= EFI_AUTH_STATUS_PLATFORM_OVERRIDE;
  } else {
    //
    // Calculate CRC32 Checksum of Image
    //
    Status = gBS->CalculateCrc32 (*OutputBuffer, OutputBufferSize, &Crc32Checksum);
    if (Status == EFI_SUCCESS) {
      if (Crc32Checksum != Crc32SectionHeader->CRC32Checksum) {
        *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
      }
    } else {
      *AuthenticationStatus |= EFI_AUTH_STATUS_NOT_TESTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Register Crc32 section handler.

  @retval  RETURN_SUCCESS            Register successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to store this handler.
**/
EFI_STATUS
EFIAPI
DxeCrc32GuidedSectionExtractLibConstructor (
  )
{
  return ExtractGuidedSectionRegisterHandlers (
          &gEfiCrc32GuidedSectionExtractionProtocolGuid,
          Crc32GuidedSectionGetInfo,
          Crc32GuidedSectionHandler
          );
}

