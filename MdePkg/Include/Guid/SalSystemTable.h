/** @file
  GUIDs used for SAL system table entries in the EFI system table.

  SAL System Table contains Itanium-based processor centric information about
  the system.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUIDs defined in UEFI 2.0 spec.

**/

#ifndef __SAL_SYSTEM_TABLE_GUID_H__
#define __SAL_SYSTEM_TABLE_GUID_H__

#define SAL_SYSTEM_TABLE_GUID \
  { \
    0xeb9d2d32, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

extern EFI_GUID gEfiSalSystemTableGuid;

#endif
