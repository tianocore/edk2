/** @file

Copyright (c) 2013, Citrix Systems UK Ltd.
Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __E820_H__
#define __E820_H__

#pragma pack(1)

typedef enum {
  EfiAcpiAddressRangeMemory   = 1,
  EfiAcpiAddressRangeReserved = 2,
  EfiAcpiAddressRangeACPI     = 3,
  EfiAcpiAddressRangeNVS      = 4
} EFI_ACPI_MEMORY_TYPE;

typedef struct {
  UINT64                BaseAddr;
  UINT64                Length;
  EFI_ACPI_MEMORY_TYPE  Type;
} EFI_E820_ENTRY64;

typedef struct {
  UINT32                BassAddrLow;
  UINT32                BaseAddrHigh;
  UINT32                LengthLow;
  UINT32                LengthHigh;
  EFI_ACPI_MEMORY_TYPE  Type;
} EFI_E820_ENTRY;

#pragma pack()

#endif /* __E820_H__ */
