/** @file
  LoongArch I/O Virtualization Table (IOVT) definitions.

  Copyright (C) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - LoongArch I/O Virtualization Table Specification, Version v0.1
    (https://www.loongson.cn/uploads/images/2024110517404135188.LoongArch-IO-Virtualization-Table-Specification.pdf)
  - Advanced Configuration and Power Interface (ACPI) specification, Version 6.6
    (https://uefi.org/specs/ACPI/6.6/)

**/

#pragma once

#include <IndustryStandard/Acpi.h>

#pragma pack (1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT16                         IommuCount;
  UINT16                         IommuOffset;
  UINT64                         Reserved;
} EFI_ACPI_IOVT_HEADER;

#define EFI_ACPI_LOONGARCH_IOVT_REVISION  0x01

typedef struct {
  UINT16    Type;
  UINT16    Length;
  UINT32    Flags;
  UINT16    PciSegmentNumber;
  UINT16    PhysicalAddressWidth;
  UINT16    VirtualAddressWidth;
  UINT16    MaxPageLevel;
  UINT64    PageSizeSupported;
  UINT32    IommuDeviceId;
  UINT64    IommuBaseAddress;
  UINT32    IommuRegisterSize;
  UINT8     InterruptType;
  UINT8     Reserved[3];
  UINT32    GlobalSystemInterrupt;
  UINT32    ProximityDomain;
  UINT32    MaxDeviceNum;
  UINT32    NumberOfDeviceEntries;
  UINT32    OffsetOfDeviceEntries;
} EFI_ACPI_IOVT_IOMMU;

#define EFI_ACPI_IOVT_TYPE_LOONGARCH_V1  0x0000

#define EFI_ACPI_IOVT_FLAG_PCI_DEVICE            BIT0
#define EFI_ACPI_IOVT_FLAG_PROXIMITY_VALID       BIT1
#define EFI_ACPI_IOVT_FLAG_DEVICE_SCOPE_SEGMENT  BIT2
#define EFI_ACPI_IOVT_FLAG_HW_CAPABILITY         BIT3
#define EFI_ACPI_IOVT_FLAG_MSI_ADDR_BYPASS       BIT4

typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Flags;
  UINT8     Reserved[3];
  UINT16    DevId;
} EFI_ACPI_IOVT_DEVICE_ENTRY;

#define EFI_ACPI_IOVT_DEVICE_ENTRY_TYPE_PCI_DEVICE   0x00
#define EFI_ACPI_IOVT_DEVICE_ENTRY_TYPE_RANGE_START  0x01
#define EFI_ACPI_IOVT_DEVICE_ENTRY_TYPE_RANGE_END    0x02

#define EFI_ACPI_IOVT_IOMMU_LENGTH         64
#define EFI_ACPI_IOVT_DEVICE_ENTRY_LENGTH  8

#pragma pack ()
