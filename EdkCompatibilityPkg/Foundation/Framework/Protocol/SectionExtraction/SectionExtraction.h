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

  Section extraction protocol as defined in the Tiano File Image Format specification.

  This interface provides a means of decoding a set of sections into a linked list of
  leaf sections.  This provides for an extensible and flexible file format.

--*/

#ifndef _SECTION_EXTRACTION_PROTOCOL_H
#define _SECTION_EXTRACTION_PROTOCOL_H

#include "EfiFirmwareFileSystem.h"

//
// Protocol GUID definition
//
#define EFI_SECTION_EXTRACTION_PROTOCOL_GUID \
  { \
    0x448F5DA4, 0x6DD7, 0x4FE1, {0x93, 0x07, 0x69, 0x22, 0x41, 0x92, 0x21, 0x5D} \
  }

EFI_FORWARD_DECLARATION (EFI_SECTION_EXTRACTION_PROTOCOL);

//
// Protocol member functions
//
typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_SECTION_STREAM) (
  IN  EFI_SECTION_EXTRACTION_PROTOCOL                   * This,
  IN  UINTN                                             SectionStreamLength,
  IN  VOID                                              *SectionStream,
  OUT UINTN                                             *SectionStreamHandle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_SECTION) (
  IN EFI_SECTION_EXTRACTION_PROTOCOL                    * This,
  IN UINTN                                              SectionStreamHandle,
  IN EFI_SECTION_TYPE                                   * SectionType,
  IN EFI_GUID                                           * SectionDefinitionGuid,
  IN UINTN                                              SectionInstance,
  IN VOID                                               **Buffer,
  IN OUT UINTN                                          *BufferSize,
  OUT UINT32                                            *AuthenticationStatus
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_SECTION_STREAM) (
  IN EFI_SECTION_EXTRACTION_PROTOCOL                    * This,
  IN UINTN                                              SectionStreamHandle
  );

//
// Protocol definition
//
struct _EFI_SECTION_EXTRACTION_PROTOCOL {
  EFI_OPEN_SECTION_STREAM   OpenSectionStream;
  EFI_GET_SECTION           GetSection;
  EFI_CLOSE_SECTION_STREAM  CloseSectionStream;
};

extern EFI_GUID gEfiSectionExtractionProtocolGuid;

#endif
