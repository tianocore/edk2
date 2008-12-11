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

#define EFI_SECITON_SIZE_MASK 0x00ffffff

typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidedSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

/**

  The implementation of Crc32 guided section GetInfo() to get 
  size and attribute of the guided section.

  @param InputSection       Buffer containing the input GUIDed section to be processed.
  @param OutputBufferSize   The size of OutputBuffer.
  @param ScratchBufferSize  The size of ScratchBuffer.
  @param SectionAttribute   The attribute of the input guided section.

  @retval EFI_SUCCESS            The size of destination buffer, the size of scratch buffer and 
                                 the attribute of the input section are successull retrieved.
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
  //
  // Check whether the input guid section is recognized.
  //
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
  *OutputBufferSize  = *(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size) & EFI_SECITON_SIZE_MASK;
  *OutputBufferSize  -= ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset;

  return EFI_SUCCESS;
}

/**

  The implementation of Crc32 Guided section extraction to get the section data.

  @param InputSection    Buffer containing the input GUIDed section to be processed.
  @param OutputBuffer    to contain the output data, which is allocated by the caller.
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
  CRC32_SECTION_HEADER      *Crc32SectionHeader;
  UINT32                    Crc32Checksum;
  UINT32                    OutputBufferSize;
  VOID                      *DummyInterface;

  //
  // Check whether the input guid section is recognized.
  //
  if (!CompareGuid (
        &gEfiCrc32GuidedSectionExtractionProtocolGuid, 
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Init Checksum value to Zero.
  //
  Crc32Checksum = 0;
  //
  // Points to the Crc32 section header
  //
  Crc32SectionHeader = (CRC32_SECTION_HEADER *) InputSection;
  *OutputBuffer      = (UINT8 *) InputSection + Crc32SectionHeader->GuidedSectionHeader.DataOffset;
  OutputBufferSize   = *(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size) & EFI_SECITON_SIZE_MASK; 
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
      if (Crc32Checksum != Crc32SectionHeader->CRC32Checksum) {
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
  Register Crc32 section handler.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  RETURN_SUCCESS            Register successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to register this handler.
**/
EFI_STATUS
EFIAPI
DxeCrc32GuidedSectionExtractLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return ExtractGuidedSectionRegisterHandlers (
          &gEfiCrc32GuidedSectionExtractionProtocolGuid,
          Crc32GuidedSectionGetInfo,
          Crc32GuidedSectionHandler
          );
}

