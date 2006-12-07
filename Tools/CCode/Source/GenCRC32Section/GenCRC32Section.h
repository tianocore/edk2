/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenCRC32Section.h 

Abstract:

  Header file for GenFfsFile. Mainly defines the header of section
  header for CRC32 GUID defined sections. Share with GenSection.c

--*/

//
// External Files Referenced
//

/* Standard Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* MDE Headers */
#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>
#include <Common/FirmwareVolumeImageFormat.h>
#include <Common/FirmwareFileSystem.h>
#include <Common/FirmwareVolumeHeader.h>
#include <Protocol/GuidedSectionExtraction.h>

/* Tool Headers */
#include "CommonLib.h"
#include "Crc32.h"
#include "Compress.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"

//
// Module Coded to Tiano Coding Conventions
//
#ifndef _EFI_GEN_CRC32_SECTION_H
#define _EFI_GEN_CRC32_SECTION_H


typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

#define EFI_SECTION_CRC32_GUID_DEFINED  0
#define CRC32_SECTION_HEADER_SIZE       (sizeof (CRC32_SECTION_HEADER))

#endif
