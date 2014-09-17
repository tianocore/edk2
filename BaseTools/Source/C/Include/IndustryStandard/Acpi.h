/** @file
  This file contains some basic ACPI definitions that are consumed by drivers
  that do not care about ACPI versions.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ACPI_H_
#define _ACPI_H_

//
// Common table header, this prefaces all ACPI tables, including FACS, but
// excluding the RSD PTR structure
//
typedef struct {
  UINT32  Signature;
  UINT32  Length;
} EFI_ACPI_COMMON_HEADER;

//
// Common ACPI description table header.  This structure prefaces most ACPI tables.
//
#pragma pack(1)

typedef struct {
  UINT32  Signature;
  UINT32  Length;
  UINT8   Revision;
  UINT8   Checksum;
  UINT8   OemId[6];
  UINT64  OemTableId;
  UINT32  OemRevision;
  UINT32  CreatorId;
  UINT32  CreatorRevision;
} EFI_ACPI_DESCRIPTION_HEADER;

#pragma pack()
//
// Define for Pci Host Bridge Resource Allocation
//
#define ACPI_ADDRESS_SPACE_DESCRIPTOR 0x8A
#define ACPI_END_TAG_DESCRIPTOR       0x79

#define ACPI_ADDRESS_SPACE_TYPE_MEM   0x00
#define ACPI_ADDRESS_SPACE_TYPE_IO    0x01
#define ACPI_ADDRESS_SPACE_TYPE_BUS   0x02

//
// Power Management Timer frequency is fixed at 3.579545MHz
//
#define ACPI_TIMER_FREQUENCY       3579545

//
// Make sure structures match spec
//
#pragma pack(1)

typedef struct {
  UINT8   Desc;
  UINT16  Len;
  UINT8   ResType;
  UINT8   GenFlag;
  UINT8   SpecificFlag;
  UINT64  AddrSpaceGranularity;
  UINT64  AddrRangeMin;
  UINT64  AddrRangeMax;
  UINT64  AddrTranslationOffset;
  UINT64  AddrLen;
} EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR;

typedef struct {
  UINT8 Desc;
  UINT8 Checksum;
} EFI_ACPI_END_TAG_DESCRIPTOR;

//
// General use definitions
//
#define EFI_ACPI_RESERVED_BYTE  0x00
#define EFI_ACPI_RESERVED_WORD  0x0000
#define EFI_ACPI_RESERVED_DWORD 0x00000000
#define EFI_ACPI_RESERVED_QWORD 0x0000000000000000

//
// Resource Type Specific Flags
// Ref ACPI specification 6.4.3.5.5
//
// Bit [0]    : Write Status, _RW
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_READ_WRITE                (1 << 0)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_READ_ONLY                 (0 << 0)
//
// Bit [2:1]  : Memory Attributes, _MEM
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_NON_CACHEABLE             (0 << 1)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE                 (1 << 1)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_WRITE_COMBINING (2 << 1)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE    (3 << 1)
//
// Bit [4:3]  : Memory Attributes, _MTP
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_MEMORY      (0 << 3)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_RESERVED    (1 << 3)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_ACPI        (2 << 3)
#define EFI_APCI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_NVS         (3 << 3)
//
// Bit [5]    : Memory to I/O Translation, _TTP
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_TYPE_TRANSLATION          (1 << 5)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_TYPE_STATIC               (0 << 5)

#pragma pack()

#endif
