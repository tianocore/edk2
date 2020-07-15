/** @file
  ACPI memory mapped configuration space access table definition, defined at
  in the PCI Firmware Specification, version 3.0 draft version 0.5.
  Specification is available at http://www.pcisig.com.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_H_
#define _MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_H_

//
// Ensure proper structure formats
//
#pragma pack(1)
//
// Memory Mapped Configuration Space Access Table (MCFG)
// This table is a basic description table header followed by
// a number of base address allocation structures.
//
typedef struct {
  UINT64  BaseAddress;
  UINT16  PciSegmentGroupNumber;
  UINT8   StartBusNumber;
  UINT8   EndBusNumber;
  UINT32  Reserved;
} EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE;

//
// MCFG Revision (defined in spec)
//
#define EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION  0x01

#pragma pack()

#endif
