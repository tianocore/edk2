/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GuidedSection.c
  
Abstract:

  GUIDed section extraction protocol implementation.  
  This contains the common constructor of GUIDed section
  extraction protocol. GUID specific implementation of each
  GUIDed section extraction protocol can be found in other
  files under the same directory.
  
  Please refer to the Tiano File Image Format Specification, 
  FV spec 0.3.6
  
  Acronyms used       Meaning

    
--*/


#include "Common/FirmwareFileSystem.h"
#include "GuidedSection.h"

EFI_STATUS
GuidedSectionExtractionProtocolConstructor (
  OUT EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL      **GuidedSep,
  IN  EFI_EXTRACT_GUIDED_SECTION                  ExtractSection
  )
/*++

Routine Description:

  Constructor for the GUIDed section extraction protocol.  Initializes
  instance data.

Arguments:

  This      Instance to construct

Returns:

  EFI_SUCCESS:  Instance initialized.

--*/
// TODO:    GuidedSep - add argument and description to function comment
// TODO:    ExtractSection - add argument and description to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
{
  EFI_STATUS  Status;

  *GuidedSep = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL),
                  (VOID **) GuidedSep
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*GuidedSep)->ExtractSection = ExtractSection;

  return EFI_SUCCESS;
}
