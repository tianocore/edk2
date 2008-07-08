/** @file

  This file can define the different guids to specify the different 
  encapsulation scheme for the guided section.
  Now, only one guid is defined for CRC32 encapsulation scheme.
  The related data structures come from the SectionExtraction protocol definition.

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_H__
#define __CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_H__


//
// Protocol GUID definition. Each GUIDed section extraction protocol has the
// same interface but with different GUID. All the GUIDs is defined here.
// May add multiple GUIDs here.
//
#define EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID \
  { 0xFC1BCDB0, 0x7D31, 0x49aa, {0x93, 0x6A, 0xA4, 0x60, 0x0D, 0x9D, 0xD0, 0x83 } }

//
// The data structures are the same as GuidedSectionExtraction protocol only the GUID's are different
//
#include <Protocol/GuidedSectionExtraction.h>

extern EFI_GUID gEfiCrc32GuidedSectionExtractionProtocolGuid;

#endif
