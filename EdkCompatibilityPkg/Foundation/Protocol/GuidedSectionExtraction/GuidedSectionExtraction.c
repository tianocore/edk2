/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GuidedSectionExtraction.c

Abstract:

  GUIDed section extraction protocol as defined in the Tiano File Image 
  Format specification.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (GuidedSectionExtraction)

//
// may add more GUIDed section extraction protocol GUID here.
//
EFI_GUID  gEfiCrc32GuidedSectionExtractionProtocolGuid = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

EFI_GUID_STRING
  (
    &gEfiSectionExtractionProtocolGuid, "CRC32 GUIDed Section Extraction Protocol",
      "Tiano CRC32 GUIDed Section Extraction Protocol"
  );
