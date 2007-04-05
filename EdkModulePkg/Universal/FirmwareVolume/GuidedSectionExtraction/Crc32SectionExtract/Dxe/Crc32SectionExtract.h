/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Crc32SectionExtract.h
  
Abstract:

  Header file for Crc32SectionExtract.c
  Please refer to the Framewokr Firmware Volume Specification 0.9.

--*/

#ifndef _CRC32_GUIDED_SECTION_EXTRACTION_H
#define _CRC32_GUIDED_SECTION_EXTRACTION_H

typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidedSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

//
// Function prototype declarations
//
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

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  InputSection          - TODO: add argument description
  OutputBuffer          - TODO: add argument description
  OutputSize            - TODO: add argument description
  AuthenticationStatus  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

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
;

#endif
