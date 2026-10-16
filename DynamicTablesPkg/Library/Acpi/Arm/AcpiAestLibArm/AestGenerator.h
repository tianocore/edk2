/** @file
  Arm Error Source Table (AEST) generator header.

  Copyright (c) 2026, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for the Armv8-A RAS Extension and RAS System Architecture,
    DEN0085 2.0 EAC2.
    (https://support.arm.com/documentation/den0085/0202/?lang=en)
**/

#pragma once

#include <IndustryStandard/ArmErrorSourceTable.h>

#include <ArmNameSpaceObjects.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <TableGenerator.h>

#define AEST_PROCESSOR_FLAGS_MASK  (EFI_ACPI_AEST_2_0_PROCESSOR_FLAG_GLOBAL | \
                                    EFI_ACPI_AEST_2_0_PROCESSOR_FLAG_SHARED)
#define AEST_INTERFACE_FLAGS_MASK  (EFI_ACPI_AEST_2_0_INTERFACE_FLAG_SHARED | \
                                    EFI_ACPI_AEST_2_0_INTERFACE_FLAG_CLEAR_MISCX | \
                                    EFI_ACPI_AEST_2_0_INTERFACE_FLAG_DEVICE_VALID | \
                                    EFI_ACPI_AEST_2_0_INTERFACE_FLAG_AFFINITY_TYPE | \
                                    EFI_ACPI_AEST_2_0_INTERFACE_FLAG_GROUP_ADDRESS_VALID | \
                                    EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INJECTION_VALID | \
                                    EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INTERRUPT_CFG_VALID)
#define AEST_INTERRUPT_FLAGS_MASK  (EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_TRIGGER_TYPE_MASK | \
                                    EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_FHI_ON_UE_MASK)

#define AEST_IMPLEMENTED_BITMAP_INDEX   0
#define AEST_GROUP_STATUS_BITMAP_INDEX  1
#define AEST_ADDRESS_MODE_BITMAP_INDEX  2
#define AEST_INTERFACE_BITMAP_COUNT     3

// The AEST Table Generator revision.
#define AEST_GENERATOR_REVISION  CREATE_REVISION (1, 0)

// Information resolved and validated for one AEST node interface.
typedef struct {
  CONST CM_ARM_AEST_INTERFACE      *Source;
  CONST CM_ARM_AEST_BITMAP_WORD    *Bitmaps[AEST_INTERFACE_BITMAP_COUNT];
  UINT32                           ProcessorAffinity;
  UINT8                            BitmapWordCount;
} AEST_INTERFACE_INFO;

// Resolved resource data for a processor node.
typedef union {
  UINT32    CacheId;
  UINT32    TlbLevel;
  UINT32    GenericData;
} AEST_PROCESSOR_RESOURCE_INFO;

// Information resolved and validated for a processor node's data.
typedef struct {
  UINT32                          AcpiProcessorId;
  UINT8                           ResourceType;
  UINT8                           Flags;
  AEST_PROCESSOR_RESOURCE_INFO    Resource;
} AEST_PROCESSOR_DATA_INFO;

// Resolved data for one of the seven AEST node types.
typedef union {
  AEST_PROCESSOR_DATA_INFO         Processor;
  EFI_ACPI_AEST_2_0_MEMORY_DATA    Memory;
  EFI_ACPI_AEST_2_0_SMMU_DATA      Smmu;
  EFI_ACPI_AEST_2_0_VENDOR_DATA    Vendor;
  EFI_ACPI_AEST_2_0_GIC_DATA       Gic;
  EFI_ACPI_AEST_2_0_PCIE_DATA      Pcie;
  EFI_ACPI_AEST_2_0_PROXY_DATA     Proxy;
} AEST_NODE_DATA_INFO;

// Information resolved and validated for one AEST node.
typedef struct {
  UINT8                          Type;
  UINT16                         Length;
  CONST CM_ARM_AEST_COMMON       *Source;
  AEST_NODE_DATA_INFO            Data;
  AEST_INTERFACE_INFO            Interface;
  CONST CM_ARM_AEST_INTERRUPT    *Interrupts;
} AEST_NODE_INFO;

// A typed AEST node group and the CM list
typedef struct {
  UINT8         NodeType;
  UINTN         ObjectSize;
  BOOLEAN       Supported;
  CONST VOID    *List;
  UINT32        Count;
} AEST_NODE_GROUP;
