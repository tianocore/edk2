/** @file
  This file defines the data structures that comprise the FFS file system.

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  FrameworkFirmwareFileSystem.h

  @par Revision Reference:
  These definitions are from Firmware File System Spec 0.9 but not in PI specs.

**/

#ifndef _FRAMEWORK_FIRMWARE_FILE_SYSTEM_H_
#define _FRAMEWORK_FIRMWARE_FILE_SYSTEM_H_

#include <PiFirmwareFileSystem.h>

typedef UINT16                      EFI_FFS_FILE_TAIL;

#define FFS_ATTRIB_TAIL_PRESENT     0x01
#define FFS_ATTRIB_RECOVERY         0x02
#define FFS_ATTRIB_HEADER_EXTENSION 0x04

#endif
