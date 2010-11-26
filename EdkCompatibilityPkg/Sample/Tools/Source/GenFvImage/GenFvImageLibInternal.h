/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  GenFvImageLibInternal.h

Abstract:

  This file contains describes the private declarations for the GenFvImage Library.
  The basic purpose of the library is to create Firmware Volume images.

--*/

#ifndef _EFI_GEN_FV_IMAGE_LIB_INTERNAL_H
#define _EFI_GEN_FV_IMAGE_LIB_INTERNAL_H

//
// Include files
//
#include "GenFvImageLib.h"
#include <stdlib.h>
#include "EfiFirmwareVolumeHeader.h"

//
// Private data declarations
//
//
// The maximum number of block map entries supported by the library
//
#define MAX_NUMBER_OF_FV_BLOCKS 100

//
// The maximum number of files in the FV supported by the library
//
#define MAX_NUMBER_OF_FILES_IN_FV       1000
#define MAX_NUMBER_OF_COMPONENTS_IN_FV  10

#define FV_CAPACITY_INCREASE_UNIT       0x100000

//
// INF file strings
//
#define OPTIONS_SECTION_STRING            "[options]"
#define ATTRIBUTES_SECTION_STRING         "[attributes]"
#define FILES_SECTION_STRING              "[files]"
#define COMPONENT_SECTION_STRING          "[components]"

#define EFI_FV_BASE_ADDRESS_STRING        "EFI_BASE_ADDRESS"
#define EFI_FV_FILE_NAME_STRING           "EFI_FILE_NAME"
#define EFI_SYM_FILE_NAME_STRING          "EFI_SYM_FILE_NAME"
#define EFI_NUM_BLOCKS_STRING             "EFI_NUM_BLOCKS"
#define EFI_BLOCK_SIZE_STRING             "EFI_BLOCK_SIZE"
#define EFI_FV_GUID_STRING                "EFI_FV_GUID"

#define EFI_FVB_READ_DISABLED_CAP_STRING  "EFI_READ_DISABLED_CAP"
#define EFI_FVB_READ_ENABLED_CAP_STRING   "EFI_READ_ENABLED_CAP"
#define EFI_FVB_READ_STATUS_STRING        "EFI_READ_STATUS"

#define EFI_FVB_WRITE_DISABLED_CAP_STRING "EFI_WRITE_DISABLED_CAP"
#define EFI_FVB_WRITE_ENABLED_CAP_STRING  "EFI_WRITE_ENABLED_CAP"
#define EFI_FVB_WRITE_STATUS_STRING       "EFI_WRITE_STATUS"

#define EFI_FVB_LOCK_CAP_STRING           "EFI_LOCK_CAP"
#define EFI_FVB_LOCK_STATUS_STRING        "EFI_LOCK_STATUS"

#define EFI_FVB_STICKY_WRITE_STRING       "EFI_STICKY_WRITE"
#define EFI_FVB_MEMORY_MAPPED_STRING      "EFI_MEMORY_MAPPED"
#define EFI_FVB_ERASE_POLARITY_STRING     "EFI_ERASE_POLARITY"

#define EFI_FVB_ALIGNMENT_CAP_STRING      "EFI_ALIGNMENT_CAP"
#define EFI_FVB_ALIGNMENT_2_STRING        "EFI_ALIGNMENT_2"
#define EFI_FVB_ALIGNMENT_4_STRING        "EFI_ALIGNMENT_4"
#define EFI_FVB_ALIGNMENT_8_STRING        "EFI_ALIGNMENT_8"
#define EFI_FVB_ALIGNMENT_16_STRING       "EFI_ALIGNMENT_16"
#define EFI_FVB_ALIGNMENT_32_STRING       "EFI_ALIGNMENT_32"
#define EFI_FVB_ALIGNMENT_64_STRING       "EFI_ALIGNMENT_64"
#define EFI_FVB_ALIGNMENT_128_STRING      "EFI_ALIGNMENT_128"
#define EFI_FVB_ALIGNMENT_256_STRING      "EFI_ALIGNMENT_256"
#define EFI_FVB_ALIGNMENT_512_STRING      "EFI_ALIGNMENT_512"
#define EFI_FVB_ALIGNMENT_1K_STRING       "EFI_ALIGNMENT_1K"
#define EFI_FVB_ALIGNMENT_2K_STRING       "EFI_ALIGNMENT_2K"
#define EFI_FVB_ALIGNMENT_4K_STRING       "EFI_ALIGNMENT_4K"
#define EFI_FVB_ALIGNMENT_8K_STRING       "EFI_ALIGNMENT_8K"
#define EFI_FVB_ALIGNMENT_16K_STRING      "EFI_ALIGNMENT_16K"
#define EFI_FVB_ALIGNMENT_32K_STRING      "EFI_ALIGNMENT_32K"
#define EFI_FVB_ALIGNMENT_64K_STRING      "EFI_ALIGNMENT_64K"

//
// Add these for PI1.0 new Attributes.
//
#define EFI_FVB_READ_LOCK_CAP_STRING            "EFI_READ_LOCK_CAP"
#define EFI_FVB_READ_LOCK_STATUS_STRING         "EFI_READ_LOCK_STATUS"
#define EFI_FVB_WRITE_LOCK_CAP_STRING           "EFI_WRITE_LOCK_CAP"
#define EFI_FVB_WRITE_LOCK_STATUS_STRING        "EFI_WRITE_LOCK_STATUS"
#define EFI_FVB2_ALIGNMENT_STRING               "EFI_FVB2_ALIGNMENT"

#define EFI_FVB2_ALIGNMENT_1_STRING             "1"
#define EFI_FVB2_ALIGNMENT_2_STRING             "2"
#define EFI_FVB2_ALIGNMENT_4_STRING             "4"
#define EFI_FVB2_ALIGNMENT_8_STRING             "8"
#define EFI_FVB2_ALIGNMENT_16_STRING            "16"
#define EFI_FVB2_ALIGNMENT_32_STRING            "32"
#define EFI_FVB2_ALIGNMENT_64_STRING            "64"
#define EFI_FVB2_ALIGNMENT_128_STRING           "128"
#define EFI_FVB2_ALIGNMENT_256_STRING           "256"
#define EFI_FVB2_ALIGNMENT_512_STRING           "512"
#define EFI_FVB2_ALIGNMENT_1K_STRING            "1K"
#define EFI_FVB2_ALIGNMENT_2K_STRING            "2K"
#define EFI_FVB2_ALIGNMENT_4K_STRING            "4K"
#define EFI_FVB2_ALIGNMENT_8K_STRING            "8K"
#define EFI_FVB2_ALIGNMENT_16K_STRING           "16K"
#define EFI_FVB2_ALIGNMENT_32K_STRING           "32K"
#define EFI_FVB2_ALIGNMENT_64K_STRING           "64K"
#define EFI_FVB2_ALIGNMENT_128K_STRING          "128K"
#define EFI_FVB2_ALIGNMENT_256K_STRING          "256K"
#define EFI_FVB2_ALIGNMENT_512K_STRING          "512K"
#define EFI_FVB2_ALIGNMENT_1M_STRING            "1M"
#define EFI_FVB2_ALIGNMENT_2M_STRING            "2M"
#define EFI_FVB2_ALIGNMENT_4M_STRING            "4M"
#define EFI_FVB2_ALIGNMENT_8M_STRING            "8M"
#define EFI_FVB2_ALIGNMENT_16M_STRING           "16M"
#define EFI_FVB2_ALIGNMENT_32M_STRING           "32M"
#define EFI_FVB2_ALIGNMENT_64M_STRING           "64M"
#define EFI_FVB2_ALIGNMENT_128M_STRING          "128M"
#define EFI_FVB2_ALIGNMENT_256M_STRING          "256M"
#define EFI_FVB2_ALIGNMENT_512M_STRING          "512M"
#define EFI_FVB2_ALIGNMENT_1G_STRING            "1G"
#define EFI_FVB2_ALIGNMENT_2G_STRING            "2G"


//
// Component sections
//
#define EFI_NV_VARIABLE_STRING    "EFI_NV_VARIABLE"
#define EFI_NV_EVENT_LOG_STRING   "EFI_NV_EVENT_LOG"
#define EFI_NV_FTW_WORKING_STRING "EFI_NV_FTW_WORKING"
#define EFI_NV_FTW_SPARE_STRING   "EFI_NV_FTW_SPARE"

#define EFI_FILE_NAME_STRING      "EFI_FILE_NAME"

#define ONE_STRING                "1"
#define ZERO_STRING               "0"
#define TRUE_STRING               "TRUE"
#define FALSE_STRING              "FALSE"
#define NULL_STRING               "NULL"
#define AUTO_STRING               "AUTO"

//
// Defines to calculate the offset for PEI CORE entry points
//
#define IA32_PEI_CORE_ENTRY_OFFSET  0x20

//
// Defines to calculate the FIT table
//
#define IPF_FIT_ADDRESS_OFFSET  0x20

//
// Defines to calculate the offset for SALE_ENTRY
//
#define IPF_SALE_ENTRY_ADDRESS_OFFSET 0x18

//
// Symbol file definitions, current max size if 512K
//
#define SYMBOL_FILE_SIZE  0x80000

#define FV_IMAGES_TOP_ADDRESS             0x100000000

//
// Private data types
//
//
// Component information
//
typedef struct {
  UINTN Size;
  CHAR8 ComponentName[_MAX_PATH];
} COMPONENT_INFO;

//
// FV information holder
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  EFI_GUID                FvGuid;
  UINTN                   Size;
  CHAR8                   FvName[_MAX_PATH];
  CHAR8                   SymName[_MAX_PATH];
  EFI_FV_BLOCK_MAP_ENTRY  FvBlocks[MAX_NUMBER_OF_FV_BLOCKS];
  EFI_FVB_ATTRIBUTES      FvAttributes;
  CHAR8                   FvFiles[MAX_NUMBER_OF_FILES_IN_FV][_MAX_PATH];
  COMPONENT_INFO          FvComponents[MAX_NUMBER_OF_COMPONENTS_IN_FV];
} FV_INFO;

//
// Private function prototypes
//
EFI_STATUS
ParseFvInf (
  IN MEMORY_FILE  *InfFile,
  IN FV_INFO      *FvInfo
  )
;

#endif
