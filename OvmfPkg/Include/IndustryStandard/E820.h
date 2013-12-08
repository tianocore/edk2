/** @file

Copyright (c) 2013, Citrix Systems UK Ltd.
Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
