/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DMARemappingReportingTable.h

Abstract:

  The definition for ACPI DMA-Remapping Reporting (DMAR) Table.
  It is defined in "Intel VT for Direct IO Architecture Specification".

--*/

#ifndef _EFI_DMA_REMAPPING_REPORTING_TABLE_H_
#define _EFI_DMA_REMAPPING_REPORTING_TABLE_H_

#include "AcpiCommon.h"

//
// "DMAR" DMAR Description Table Signature
//
#define EFI_ACPI_DMAR_DESCRIPTION_TABLE_SIGNATURE  0x52414d44

//
// DMAR Revision
//
#define EFI_ACPI_DMAR_DESCRIPTION_TABLE_REVISION   0x01

//
// Ensure proper structure formats
//
#pragma pack (1)

//
// Definition for DMA Remapping Structure Types
//
#define EFI_ACPI_DMA_REMAPPING_STRUCTURE_TYPE_DRHD  0
#define EFI_ACPI_DMA_REMAPPING_STRUCTURE_TYPE_RMRR  1
#define EFI_ACPI_DMA_REMAPPING_STRUCTURE_TYPE_ATSR  2
#define EFI_ACPI_DMA_REMAPPING_STRUCTURE_TYPE_RHSA  3

//
// Definition for DMA Remapping Structure Header
//
typedef struct {
  UINT16                                      Type;
  UINT16                                      Length;
} EFI_ACPI_DMAR_STRUCTURE_HEADER;

//
// Definition for DMA-Remapping PCI Path
//
typedef struct {
  UINT8         Device;
  UINT8         Function;
} EFI_ACPI_DMAR_PCI_PATH;

//
// Definition for DMA-Remapping Device Scope Entry Structure
//
#define EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ENDPOINT                 0x01
#define EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_BRIDGE                   0x02
#define EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC                   0x03
#define EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET         0x04
typedef struct {
  UINT8                   DeviceScopeEntryType;
  UINT8                   Length;
  UINT16                  Reserved_2;
  UINT8                   EnumerationID;
  UINT8                   StartingBusNumber;
} EFI_ACPI_DMAR_DEVICE_SCOPE_ENTRY_STRUCTURE;

//
// Definition for DMA-Remapping Hardware Definition (DRHD) Structure
//
#define EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_ALL_SET     0x1
#define EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_ALL_CLEAR   0x0
typedef struct {
  UINT16                                      Type;
  UINT16                                      Length;
  UINT8                                       Flags;
  UINT8                                       Reserved_5;
  UINT16                                      SegmentNumber;
  UINT64                                      RegisterBaseAddress;
} EFI_ACPI_DMAR_HARDWARE_UNIT_DEFINITION_STRUCTURE;

//
// Definition for Reserved Memory Region Reporting (RMRR) Structure
//
typedef struct {
  UINT16                                      Type;
  UINT16                                      Length;
  UINT8                                       Reserved_4[2];
  UINT16                                      SegmentNumber;
  UINT64                                      ReservedMemoryRegionBaseAddress;
  UINT64                                      ReservedMemoryRegionLimitAddress;
} EFI_ACPI_DMAR_RESERVED_MEMORY_REGION_REPORTING_STRUCTURE;

//
// Definition for Root Port ATS Capability Reporting (ATSR) Structure
//
#define EFI_ACPI_DMAR_ATSR_FLAGS_ALL_PORTS_SET     0x1
#define EFI_ACPI_DMAR_ATSR_FLAGS_ALL_PORTS_CLEAR   0x0
typedef struct {
  UINT16                                      Type;
  UINT16                                      Length;
  UINT8                                       Flags;
  UINT8                                       Reserved_5;
  UINT16                                      SegmentNumber;
} EFI_ACPI_DMAR_ROOT_PORT_ATS_CAPABILITY_REPORTING_STRUCTURE;

//
// Definition for Remapping Hardware Static Affinity(RHSA) Structure
//
typedef struct {
  UINT16                                      Type;
  UINT16                                      Length;
  UINT32                                      Reserved;
  UINT64                                      RegisterBaseAddress;
  UINT32                                      ProximityDomain;
} EFI_ACPI_DMAR_REMAPPING_HARDWARE_STATIC_AFFINITY_STRUCTURE;

//
// Definition for DMA Remapping Structure
//
typedef union {
  EFI_ACPI_DMAR_STRUCTURE_HEADER                               DMARStructureHeader;
  EFI_ACPI_DMAR_HARDWARE_UNIT_DEFINITION_STRUCTURE             DMARHardwareUnitDefinition;
  EFI_ACPI_DMAR_RESERVED_MEMORY_REGION_REPORTING_STRUCTURE     DMARReservedMemoryRegionReporting;
  EFI_ACPI_DMAR_ROOT_PORT_ATS_CAPABILITY_REPORTING_STRUCTURE   DMARRootPortATSCapabilityReporting;
} EFI_ACPI_DMA_REMAPPING_STRUCTURE;

//
// Definition for DMA-Remapping Reporting ACPI Table
//
#define EFI_ACPI_DMAR_TABLE_FLAGS_INTR_REMAP_CLEAR          0x00
#define EFI_ACPI_DMAR_TABLE_FLAGS_INTR_REMAP_SET            0x01
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER                               Header;
  UINT8                                                     HostAddressWidth;
  UINT8                                                     Flags;
  UINT8                                                     Reserved_38[10];
} EFI_ACPI_DMAR_DESCRIPTION_TABLE;

//
// The Platform specific definition can be as follows:
//   NOTE: we use /**/ as comment for user convenience to copy it.
//

/*

//
// Dmar.h
//

#define EFI_ACPI_MAX_NUM_PCI_PATH_ENTRIES                  0x01  // user need to update
typedef struct {
  EFI_ACPI_DMAR_DEVICE_SCOPE_ENTRY_STRUCTURE               Header;
  EFI_ACPI_DMAR_PCI_PATH                                   PciPath[EFI_ACPI_MAX_NUM_PCI_PATH_ENTRIES];    
} EFI_ACPI_3_0_DMAR_DEVICE_SCOPE_ENTRY_STRUCTURE;

#define EFI_ACPI_MAX_NUM_OF_DEVICE_SCOPE_PER_DHRD_ENTRY    0x01  // user need to update
typedef struct {
  EFI_ACPI_DMAR_HARDWARE_UNIT_DEFINITION_STRUCTURE           Header;
  EFI_ACPI_3_0_DMAR_DEVICE_SCOPE_ENTRY_STRUCTURE             DeviceScopeEntry[EFI_ACPI_MAX_NUM_OF_DEVICE_SCOPE_PER_DHRD_ENTRY];
} EFI_ACPI_3_0_DMAR_HARDWARE_UNIT_DEFINITION_STRUCTURE;

#define EFI_ACPI_MAX_NUM_OF_DEVICE_SCOPE_PER_RMRR_ENTRY    0x01  // user need to update
typedef struct {
  EFI_ACPI_DMAR_RESERVED_MEMORY_REGION_REPORTING_STRUCTURE   Header;
  EFI_ACPI_3_0_DMAR_DEVICE_SCOPE_ENTRY_STRUCTURE             DeviceScopeEntry[EFI_ACPI_MAX_NUM_OF_DEVICE_SCOPE_PER_RMRR_ENTRY];
} EFI_ACPI_3_0_DMAR_RESERVED_MEMORY_REGION_REPORTING_STRUCTURE;

#define EFI_ACPI_MAX_NUM_OF_DEVICE_SCOPE_PER_ATSR_ENTRY    0x01  // user need to update
typedef struct {
  EFI_ACPI_DMAR_ROOT_PORT_ATS_CAPABILITY_REPORTING_STRUCTURE   Header;
  EFI_ACPI_3_0_DMAR_DEVICE_SCOPE_ENTRY_STRUCTURE               DeviceScopeEntry[EFI_ACPI_MAX_NUM_OF_DEVICE_SCOPE_PER_ATSR_ENTRY];
} EFI_ACPI_3_0_DMAR_ROOT_PORT_ATS_CAPABILITY_REPORTING_STRUCTURE;

#define EFI_ACPI_DMAR_DHRD_ENTRY_COUNT                     0x1   // user need to update
#define EFI_ACPI_DMAR_RMRR_ENTRY_COUNT                     0x1   // user need to update
#define EFI_ACPI_DMAR_ATSR_ENTRY_COUNT                     0x1   // user need to update

typedef struct {
  EFI_ACPI_DMAR_DESCRIPTION_TABLE                                  Header;

#if EFI_ACPI_3_0_DMAR_DHRD_ENTRY_COUNT > 0
  EFI_ACPI_3_0_DMAR_HARDWARE_UNIT_DEFINITION_STRUCTURE             Dhrd[EFI_ACPI_DMAR_DHRD_ENTRY_COUNT];
#endif

#if EFI_ACPI_3_0_DMAR_RMRR_ENTRY_COUNT > 0
  EFI_ACPI_3_0_DMAR_RESERVED_MEMORY_REGION_REPORTING_STRUCTURE     Rmrr[EFI_ACPI_DMAR_RMRR_ENTRY_COUNT];
#endif

#if EFI_ACPI_3_0_DMAR_ATSR_ENTRY_COUNT > 0
  EFI_ACPI_3_0_DMAR_ROOT_PORT_ATS_CAPABILITY_REPORTING_STRUCTURE   Atsr[EFI_ACPI_DMAR_ATSR_ENTRY_COUNT];
#endif

} EFI_ACPI_3_0_DMA_REMAPPING_REPORTING_TABLE;

*/

#pragma pack()

#endif

