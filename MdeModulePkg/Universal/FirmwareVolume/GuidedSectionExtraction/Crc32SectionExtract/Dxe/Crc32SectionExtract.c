/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Crc32SectionExtract.c

Abstract:

  Implements GUIDed section extraction protocol interface with 
  a specific GUID: CRC32.

  Please refer to the Framewokr Firmware Volume Specification 0.9.

--*/


#include <GuidedSection.h>
#include <Crc32SectionExtract.h>

EFI_STATUS
EFIAPI
InitializeCrc32GuidedSectionExtractionProtocol (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
/*++

Routine Description: 

  Entry point of the CRC32 GUIDed section extraction protocol. 
  Creates and initializes an instance of the GUIDed section 
  extraction protocol with CRC32 GUID.

Arguments:  

  ImageHandle   EFI_HANDLE: A handle for the image that is initializing 
                this driver
  SystemTable   EFI_SYSTEM_TABLE: A pointer to the EFI system table        

Returns:  

  EFI_SUCCESS:          Driver initialized successfully
  EFI_LOAD_ERROR:       Failed to Initialize or has been loaded 
  EFI_OUT_OF_RESOURCES: Could not allocate needed resources

--*/
{
  EFI_STATUS                              Status;
  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL  *Crc32GuidedSep;
  EFI_HANDLE                              Handle;

  //
  // Call all constructors per produced protocols
  //
  Status = GuidedSectionExtractionProtocolConstructor (
            &Crc32GuidedSep,
            (EFI_EXTRACT_GUIDED_SECTION) Crc32ExtractSection
            );
  if (EFI_ERROR (Status)) {
    if (Crc32GuidedSep != NULL) {
      FreePool (Crc32GuidedSep);
    }

    return Status;
  }
  //
  // Pass in a NULL to install to a new handle
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiCrc32GuidedSectionExtractionProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  Crc32GuidedSep
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Crc32GuidedSep);
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
UINT32
EFIAPI
GetSectionLength (
  IN EFI_COMMON_SECTION_HEADER  *CommonHeader
  )
/*++

  Routine Description:
    Get a length of section.

  Parameters:
    CommonHeader      -   Pointer to the common section header.

  Return Value:
    The length of the section, including the section header.

--*/
// TODO: function comment is missing 'Arguments:'
// TODO: function comment is missing 'Returns:'
// TODO:    CommonHeader - add argument and description to function comment
{
  UINT32  Size;

  Size = *(UINT32 *) CommonHeader->Size & 0x00FFFFFF;

  return Size;
}

STATIC
EFI_STATUS
EFIAPI
Crc32ExtractSection (
  IN  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL  *This,
  IN  VOID                                    *InputSection,
  OUT VOID                                    **OutputBuffer,
  OUT UINTN                                   *OutputSize,
  OUT UINT32                                  *AuthenticationStatus
  )
/*++

  Routine Description:
    This function reads and extracts contents of a section from an
    encapsulating section.

  Parameters:
    This                    - Indicates the calling context.
    InputSection            - Buffer containing the input GUIDed section 
                              to be processed.
    OutputBuffer            - *OutputBuffer is allocated from boot services
                              pool memory and containing the new section
                              stream. The caller is responsible for freeing
                              this buffer.
    AuthenticationStatus    - Pointer to a caller allocated UINT32 that
                              indicates the authentication status of the
                              output buffer

  Return Value:
    EFI_SUCCESS
    EFI_OUT_OF_RESOURCES
    EFI_INVALID_PARAMETER
    EFI_NOT_AVAILABLE_YET

--*/
// TODO: function comment is missing 'Arguments:'
// TODO: function comment is missing 'Returns:'
// TODO:    This - add argument and description to function comment
// TODO:    InputSection - add argument and description to function comment
// TODO:    OutputBuffer - add argument and description to function comment
// TODO:    OutputSize - add argument and description to function comment
// TODO:    AuthenticationStatus - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                Status;
  CRC32_SECTION_HEADER      *Crc32SectionHeader;
  EFI_GUID_DEFINED_SECTION  *GuidedSectionHeader;
  UINT8                     *Image;
  UINT32                    Crc32Checksum;
  VOID                      *DummyInterface;

  if (OutputBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *OutputBuffer = NULL;

  //
  // Points to the section header
  //
  Crc32SectionHeader  = (CRC32_SECTION_HEADER *) InputSection;
  GuidedSectionHeader = (EFI_GUID_DEFINED_SECTION *) InputSection;

  //
  // Check if the GUID is a CRC32 section GUID
  //
  if (!CompareGuid (
        &(GuidedSectionHeader->SectionDefinitionGuid),
        &gEfiCrc32GuidedSectionExtractionProtocolGuid
        )) {
    return EFI_INVALID_PARAMETER;
  }

  Image = (UINT8 *) InputSection + (UINT32) (GuidedSectionHeader->DataOffset);
  *OutputSize = GetSectionLength ((EFI_COMMON_SECTION_HEADER *) InputSection) - (UINT32) GuidedSectionHeader->DataOffset;

  *OutputBuffer = AllocatePool (*OutputSize);
  if (*OutputBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Implictly CRC32 GUIDed section should have STATUS_VALID bit set
  //
  ASSERT (GuidedSectionHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID);
  *AuthenticationStatus = EFI_LOCAL_AUTH_STATUS_IMAGE_SIGNED | EFI_AGGREGATE_AUTH_STATUS_IMAGE_SIGNED;

  //
  // Check whether there exists EFI_SECURITY_POLICY_PROTOCOL_GUID.
  //
  Status = gBS->LocateProtocol (&gEfiSecurityPolicyProtocolGuid, NULL, &DummyInterface);
  if (!EFI_ERROR (Status)) {
    *AuthenticationStatus |= EFI_LOCAL_AUTH_STATUS_PLATFORM_OVERRIDE | EFI_AGGREGATE_AUTH_STATUS_PLATFORM_OVERRIDE;
  } else {
    //
    // Calculate CRC32 Checksum of Image
    //
    gBS->CalculateCrc32 (Image, *OutputSize, &Crc32Checksum);
    if (Crc32Checksum != Crc32SectionHeader->CRC32Checksum) {
      *AuthenticationStatus |= EFI_LOCAL_AUTH_STATUS_TEST_FAILED | EFI_AGGREGATE_AUTH_STATUS_TEST_FAILED;
    }
  }

  CopyMem (*OutputBuffer, Image, *OutputSize);

  return EFI_SUCCESS;
}
