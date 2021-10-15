/** @file
  This file defines the hob structure for system tables like ACPI, SMBIOS tables.

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SYSTEM_TABLE_INFO_GUID_H_
#define SYSTEM_TABLE_INFO_GUID_H_

///
/// System Table Information GUID
///
extern EFI_GUID gUefiSystemTableInfoGuid;

typedef struct {
  UINT8     Revision;
  UINT8     Reserved0[3];
  UINT64    AcpiTableBase;
  UINT32    AcpiTableSize;
  UINT64    SmbiosTableBase;
  UINT32    SmbiosTableSize;
} SYSTEM_TABLE_INFO;

#endif
