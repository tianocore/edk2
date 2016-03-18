/** @file
  This file defines CRC32 GUID to specify the CRC32 
  encapsulation scheme for the GUIDed section.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CRC32_GUIDED_SECTION_EXTRACTION_H__
#define __CRC32_GUIDED_SECTION_EXTRACTION_H__

#define EFI_CRC32_GUIDED_SECTION_EXTRACTION_GUID \
  { 0xFC1BCDB0, 0x7D31, 0x49aa, {0x93, 0x6A, 0xA4, 0x60, 0x0D, 0x9D, 0xD0, 0x83 } }

extern EFI_GUID gEfiCrc32GuidedSectionExtractionGuid;

#endif
