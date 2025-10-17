/** @file
  RISC-V IO Mapping Table (RIMT) definitions

  Copyright (C) 2025, plasteli.net. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - RISC-V IO Mapping Table (RIMT) Specification, Version v1.0
    (https://github.com/riscv-non-isa/riscv-acpi-rimt/releases/download/v1.0/rimt-spec.pdf)
  - Advanced Configuration and Power Interface (ACPI) specification, Version 6.6
    (https://uefi.org/specs/ACPI/6.6/)
**/

#ifndef RISCV_IO_MAPPING_TABLE_H_
#define RISCV_IO_MAPPING_TABLE_H_

#include <IndustryStandard/Acpi.h>

//
// Ensure proper structure formats
//
#pragma pack(1)

///
/// RISC-V RIMT Interrupt Wire Node
///
typedef struct {
  UINT32    InterruptNumber;
  UINT32    Flags;
} EFI_ACPI_6_6_RIMT_INTERRUPT_WIRE_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_INTERRUPT_WIRE_STRUCTURE_VERSION  1

///
/// RISC-V RIMT Header
///
typedef struct {
  UINT8     Type;
  UINT8     Revision;
  UINT16    Length;
  UINT16    Reserved;
  UINT16    Id;
} EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE_VERSION  1

///
/// RISC-V IOMMU Node (RIMT)
///
typedef struct {
  EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE    Header;
  UINT64                                     HardwareId;
  UINT64                                     BaseAddress;
  UINT32                                     Flags;
  UINT32                                     ProximityDomain;
  UINT16                                     PcieSegmentNumber;
  UINT16                                     PcieBdf;
  UINT16                                     NumberOfInterruptWires;
  UINT16                                     InterruptWireArrayOffset;
} EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE_VERSION  1

///
/// RISC-V PCIe Root Complex Node (RIMT)
///
typedef struct {
  EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE    Header;
  UINT32                                     Flags;
  UINT16                                     Reserved;
  UINT16                                     PcieSegmentNumber;
  UINT16                                     IdMappingArrayOffset;
  UINT16                                     NumberOfIdMappings;
} EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE_VERSION  1

///
/// RISC-V RIMT ID Mapping Structure
///
typedef struct {
  UINT32    SourceIdBase;
  UINT32    NumberOfIDs;
  UINT32    DestinationDeviceIdBase;
  UINT32    DestinationIommuOffset;
  UINT32    Flags;
} EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE_VERSION  1

///
/// RISC-V Platform Device Node (RIMT)
///
typedef struct {
  EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE    Header;
  UINT16                                     IdMappingArrayOffset;
  UINT16                                     NumberOfIdMappings;
  CHAR8                                      DeviceObjectName[];
} EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE_VERSION  1

///
/// RISC-V RIMT
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         NumberOfRimtNodes;
  UINT32                         OffsetToRimtNodeArray;
  UINT32                         Reserved;
} EFI_ACPI_6_6_RIMT_STRUCTURE;

#define EFI_ACPI_6_6_RIMT_STRUCTURE_VERSION  1

#pragma pack()

///
/// RIMT node structure types as defined in RIMT spec, Chapter 2.1
///
typedef enum {
  RimtNodeIommu = 0,
  RimtNodePcieRc,
  RimtNodePlatform,
  RimtNodeUnsupported
} RimtNodeTypes;

/// Minimal number of nodes allowed
/// According to RIMT spec, Chapter 2.1, system with a single IOMMU,
/// should have at least two RIMT nodes
#define RIMT_MINIMAL_NUMBER_OF_NODES_ALLOWED  2

#define RIMT_IOMMU_FLAGS_TYPE_BIT_OFFSET              0
#define RIMT_IOMMU_FLAGS_TYPE_BIT_COUNT               1
#define RIMT_IOMMU_FLAGS_PROXIMITY_DOMAIN_BIT_OFFSET  1
#define RIMT_IOMMU_FLAGS_PROXIMITY_DOMAIN_BIT_COUNT   1

#define RIMT_ID_MAPPING_FLAGS_ATS_BIT_OFFSET  0
#define RIMT_ID_MAPPING_FLAGS_ATS_BIT_COUNT   1
#define RIMT_ID_MAPPING_FLAGS_PRI_BIT_OFFSET  1
#define RIMT_ID_MAPPING_FLAGS_PRI_BIT_COUNT   1

#define RIMT_PCIERC_FLAGS_ATS_BIT_OFFSET  0
#define RIMT_PCIERC_FLAGS_ATS_BIT_COUNT   1
#define RIMT_PCIERC_FLAGS_PRI_BIT_OFFSET  1
#define RIMT_PCIERC_FLAGS_PRI_BIT_COUNT   1
#endif
