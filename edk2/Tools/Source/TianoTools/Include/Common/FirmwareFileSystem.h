/** @file
  This file defines the data structures that comprise the FFS file system.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  FirmwareFileSystem.h

  @par Revision Reference:
  These definitions are from Firmware File System Spec 0.9.

**/

#ifndef __EFI_FFS_FILE_SYSTEM_H__
#define __EFI_FFS_FILE_SYSTEM_H__

///
/// FFS specific file types
///
#define EFI_FV_FILETYPE_FFS_PAD 0xF0

//
// FFS File Attributes
//
#define FFS_ATTRIB_TAIL_PRESENT     0x01
#define FFS_ATTRIB_RECOVERY         0x02
#define FFS_ATTRIB_HEADER_EXTENSION 0x04
#define FFS_ATTRIB_DATA_ALIGNMENT   0x38
#define FFS_ATTRIB_CHECKSUM         0x40

///
/// FFS_FIXED_CHECKSUM is the default checksum value used when the
/// FFS_ATTRIB_CHECKSUM attribute bit is clear
/// note this is NOT an architecturally defined value, but is in this file for
/// implementation convenience
///
#define FFS_FIXED_CHECKSUM  0x5A

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

typedef UINT16  EFI_FFS_FILE_TAIL;

///
/// FFS file integrity check structure
///
typedef union {
  struct {
    UINT8 Header;
    UINT8 File;
  } Checksum;
  UINT16  TailReference;
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
