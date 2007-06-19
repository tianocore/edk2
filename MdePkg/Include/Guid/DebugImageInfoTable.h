/** @file
  GUID and related data structures used with the Debug Image Info Table.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DebugImageInfoTable.h

  @par Revision Reference:
  GUID defined in UEFI 2.0 spec.

**/

#ifndef __DEBUG_IMAGE_INFO_GUID_H__
#define __DEBUG_IMAGE_INFO_GUID_H__

#include <Protocol/LoadedImage.h>

#define EFI_DEBUG_IMAGE_INFO_TABLE_GUID \
  { \
    0x49152e77, 0x1ada, 0x4764, {0xb7, 0xa2, 0x7a, 0xfe, 0xfe, 0xd9, 0x5e, 0x8b } \
  }

#define EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS 0x01
#define EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED     0x02
#define EFI_DEBUG_IMAGE_INFO_INITIAL_SIZE       (EFI_PAGE_SIZE / sizeof (UINTN))
#define EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL        0x01

typedef struct {
  UINT64                Signature;
  EFI_PHYSICAL_ADDRESS  EfiSystemTableBase;
  UINT32                Crc32;
} EFI_SYSTEM_TABLE_POINTER;

typedef struct {
  UINT32                     ImageInfoType;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImageProtocolInstance;
  EFI_HANDLE                 ImageHandle;
} EFI_DEBUG_IMAGE_INFO_NORMAL;

typedef union {
  UINT32                      *ImageInfoType;
  EFI_DEBUG_IMAGE_INFO_NORMAL *NormalImage;
} EFI_DEBUG_IMAGE_INFO;

typedef struct {
  volatile UINT32       UpdateStatus;
  UINT32                TableSize;
  EFI_DEBUG_IMAGE_INFO  *EfiDebugImageInfoTable;
} EFI_DEBUG_IMAGE_INFO_TABLE_HEADER;


extern EFI_GUID gEfiDebugImageInfoTableGuid;

#endif
