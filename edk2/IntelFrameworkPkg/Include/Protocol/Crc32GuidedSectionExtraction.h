/** @file
  This file declares GUIDed section extraction protocol.

  This interface provides a means of decoding a GUID defined encapsulation
  section. There may be multiple different GUIDs associated with the GUIDed
  section extraction protocol. That is, all instances of the GUIDed section
  extraction protocol must have the same interface structure.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  GuidedSectionExtraction.h

  @par Revision Reference:
  This protocol is defined in Firmware Volume Specification.
  Version 0.9

**/

#ifndef __CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_H__
#define __CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_H__

#include <PiPei.h>

//
// Protocol GUID definition. Each GUIDed section extraction protocol has the
// same interface but with different GUID. All the GUIDs is defined here.
// May add multiple GUIDs here.
//
#define EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID \
  { \
    0xFC1BCDB0, 0x7D31, 0x49aa, {0x93, 0x6A, 0xA4, 0x60, 0x0D, 0x9D, 0xD0, 0x83 } \
  }

//
// may add other GUID here
//
extern EFI_GUID gEfiCrc32GuidedSectionExtractionProtocolGuid;

#endif
