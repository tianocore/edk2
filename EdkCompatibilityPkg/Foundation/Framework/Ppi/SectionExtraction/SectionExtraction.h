/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SectionExtraction.h

Abstract:

  Section Extraction PPI as defined in Tiano

--*/

#ifndef _SECTION_EXTRACTION_PPI_H_
#define _SECTION_EXTRACTION_PPI_H_

#define EFI_PEI_SECTION_EXTRACTION_PPI_GUID \
  { \
    0x4F89E208, 0xE144, 0x4804, {0x9E, 0xC8, 0x0F, 0x89, 0x4F, 0x7E, 0x36, 0xD7} \
  }

EFI_FORWARD_DECLARATION (EFI_PEI_SECTION_EXTRACTION_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_SECTION) (
  IN EFI_PEI_SERVICES                         **PeiServices,
  IN EFI_PEI_SECTION_EXTRACTION_PPI           * This,
  IN EFI_SECTION_TYPE                         * SectionType,
  IN EFI_GUID                                 * SectionDefinitionGuid, OPTIONAL
  IN UINTN                                    SectionInstance,
  IN VOID                                     **Buffer,
  IN OUT UINT32                               *BufferSize,
  OUT UINT32                                  *AuthenticationStatus
  );

//
// Bit values for AuthenticationStatus
//
#define EFI_PEI_AUTH_STATUS_PLATFORM_OVERRIDE 0x01
#define EFI_PEI_AUTH_STATUS_IMAGE_SIGNED      0x02
#define EFI_PEI_AUTH_STATUS_NOT_TESTED        0x04
#define EFI_PEI_AUTH_STATUS_TEST_FAILED       0x08

struct _EFI_PEI_SECTION_EXTRACTION_PPI {
  EFI_PEI_GET_SECTION PeiGetSection;
};

extern EFI_GUID gPeiSectionExtractionPpiGuid;

#endif
