/** @file
  Guid used to define the Firmware File System. See PI spec volume 3 for more
  details.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at:
    http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  File Name:  PiFirmwareFileSystem.h

  @par Revision Reference:
  Guids defined in PI Spec Volume 3

**/

#ifndef __FIRMWARE_FILE_SYSTEM2_GUID_H__
#define __FIRMWARE_FILE_SYSTEM2_GUID_H__

//
// GUIDs defined by the PI specification.
//
#define EFI_FIRMWARE_FILE_SYSTEM2_GUID \
  { \
    0x8c8ce578, 0x8a3d, 0x4f1c, {0x99, 0x35, 0x89, 0x61, 0x85, 0xc3, 0x2d, 0xd3 } \
  }

#define EFI_FFS_VOLUME_TOP_FILE_GUID \
  { \
    0x1BA0062E, 0xC779, 0x4582, {0x85, 0x66, 0x33, 0x6A, 0xE8, 0xF7, 0x8F, 0x09 } \
  }

extern EFI_GUID gEfiFirmwareFileSystem2Guid;
extern EFI_GUID gEfiFirmwareVolumeTopFileGuid;

#endif
