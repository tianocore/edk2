/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiFirmwareFileSystem.h

Abstract:

  This file defines the data structures that comprise the FFS file system.

--*/

#ifndef _EFI_FFS_FILE_SYSTEM_H_
#define _EFI_FFS_FILE_SYSTEM_H_

#include "EfiImageFormat.h"

//
// GUIDs defined by the FFS specification.
//
#define EFI_FIRMWARE_FILE_SYSTEM_GUID \
  { \
    0x7A9354D9, 0x0468, 0x444a, {0x81, 0xCE, 0x0B, 0xF6, 0x17, 0xD8, 0x90, 0xDF} \
  }

#define EFI_FFS_VOLUME_TOP_FILE_GUID \
  { \
    0x1BA0062E, 0xC779, 0x4582, {0x85, 0x66, 0x33, 0x6A, 0xE8, 0xF7, 0x8F, 0x9} \
  }

//
// FFS specific file types
//
#define EFI_FV_FILETYPE_FFS_PAD 0xF0

//
// FFS File Attributes
//
#define FFS_ATTRIB_TAIL_PRESENT     0x01
#define FFS_ATTRIB_RECOVERY         0x02
#define FFS_ATTRIB_DATA_ALIGNMENT   0x38
#define FFS_ATTRIB_CHECKSUM         0x40
#if (PI_SPECIFICATION_VERSION < 0x00010000)
#define FFS_ATTRIB_HEADER_EXTENSION 0x04
#else
//
// PI 1.0 definition.
// 
#define FFS_ATTRIB_FIXED           0x04
#endif


//
// FFS_FIXED_CHECKSUM is the default checksum value used when the
// FFS_ATTRIB_CHECKSUM attribute bit is clear
// This value is defined in PI 1.2.
//
#define FFS_FIXED_CHECKSUM  0xAA


//
// File state definitions
//
#define EFI_FILE_HEADER_CONSTRUCTION  0x01
#define EFI_FILE_HEADER_VALID         0x02
#define EFI_FILE_DATA_VALID           0x04
#define EFI_FILE_MARKED_FOR_UPDATE    0x08
#define EFI_FILE_DELETED              0x10
#define EFI_FILE_HEADER_INVALID       0x20

#define EFI_FILE_ALL_STATE_BITS       (EFI_FILE_HEADER_CONSTRUCTION | \
                                 EFI_FILE_HEADER_VALID | \
                                 EFI_FILE_DATA_VALID | \
                                 EFI_FILE_MARKED_FOR_UPDATE | \
                                 EFI_FILE_DELETED | \
                                 EFI_FILE_HEADER_INVALID \
          )

#define EFI_TEST_FFS_ATTRIBUTES_BIT(FvbAttributes, TestAttributes, Bit) \
    ( \
      (BOOLEAN) ( \
          (FvbAttributes & EFI_FVB_ERASE_POLARITY) ? (((~TestAttributes) & Bit) == Bit) : ((TestAttributes & Bit) == Bit) \
        ) \
    )

//
// FFS file integrity check structure
//
typedef UINT16  EFI_FFS_FILE_TAIL;

typedef union {
  struct {
    UINT8 Header;
    UINT8 File;
  } Checksum;
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
  UINT16  TailReference;
#else
  UINT16  Checksum16;
#endif
} EFI_FFS_INTEGRITY_CHECK;

//
// FFS file header definition
//
typedef UINT8 EFI_FFS_FILE_ATTRIBUTES;
typedef UINT8 EFI_FFS_FILE_STATE;

typedef struct {
  EFI_GUID                Name;
  EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
  EFI_FV_FILETYPE         Type;
  EFI_FFS_FILE_ATTRIBUTES Attributes;
  UINT8                   Size[3];
  EFI_FFS_FILE_STATE      State;
} EFI_FFS_FILE_HEADER;

#endif
