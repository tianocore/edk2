/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_TDX_H_
#define ACPI_TDX_H_

#define ACPI_MADT_MPWK_STRUCT_TYPE  0x10

#pragma pack(1)

typedef struct {
  UINT8                       Type;
  UINT8                       Length;
  UINT16                      MailBoxVersion;
  UINT32                      Reserved2;
  UINT64                      MailBoxAddress;
} ACPI_MADT_MPWK_STRUCT;

#pragma pack()
#endif
