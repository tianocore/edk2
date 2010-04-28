/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenSection.h 

Abstract:

  Header file for GenSection.

--*/

//
// Module Coded to Tiano Coding Conventions
//
#ifndef _EFI_GEN_SECTION_H
#define _EFI_GEN_SECTION_H

//
// External Files Referenced
//
#include "TianoCommon.h"
#include "EfiImageFormat.h"

typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

#define EFI_SECTION_CRC32_GUID_DEFINED  0
#define CRC32_SECTION_HEADER_SIZE       (sizeof (CRC32_SECTION_HEADER))

#endif
