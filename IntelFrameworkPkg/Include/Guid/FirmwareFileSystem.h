/** @file
  Guid used to define the Firmware File System.  See the Framework Firmware
  File System Specification for more details.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  Guids defined in Firmware File System Spec 0.9.

**/

#ifndef __FIRMWARE_FILE_SYSTEM_GUID_H__
#define __FIRMWARE_FILE_SYSTEM_GUID_H__

///
/// GUIDs defined by the FFS specification.
///
#define EFI_FIRMWARE_FILE_SYSTEM_GUID \
  { 0x7A9354D9, 0x0468, 0x444a, {0x81, 0xCE, 0x0B, 0xF6, 0x17, 0xD8, 0x90, 0xDF }}

typedef UINT16                      EFI_FFS_FILE_TAIL;

#define FFS_ATTRIB_TAIL_PRESENT     0x01
#define FFS_ATTRIB_RECOVERY         0x02
#define FFS_ATTRIB_HEADER_EXTENSION 0x04

extern EFI_GUID gEfiFirmwareFileSystemGuid;

#endif
