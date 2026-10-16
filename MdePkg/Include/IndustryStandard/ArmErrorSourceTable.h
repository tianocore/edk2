/** @file
  Arm Error Source Table definitions from DEN0085 1.1 and 2.0 EAC2.

  Copyright (c) 2020 Arm Limited.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI for the Armv8 RAS Extensions 1.1 Platform Design Document,
      dated 28 September 2020.
      (https://developer.arm.com/documentation/den0085/0101/)
    - ACPI for Arm RAS, DEN0085 2.0 EAC2.
      (https://developer.arm.com/documentation/den0085/0202/)

  @par Glossary
    - Ref : Reference
    - Id  : Identifier
**/

#pragma once

#include <IndustryStandard/Acpi.h>

///
/// "AEST" Arm Error Source Table
///
#define EFI_ACPI_6_3_ARM_ERROR_SOURCE_TABLE_SIGNATURE  SIGNATURE_32('A', 'E', 'S', 'T')

#define EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION  1

#pragma pack(1)

///
/// Arm Error Source Table definition.
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
} EFI_ACPI_ARM_ERROR_SOURCE_TABLE;

///
/// AEST Node structure.
///
typedef struct {
  /// Node type:
  ///   0x00 - Processor error node
  ///   0x01 - Memory error node
  ///   0x02 - SMMU error node
  ///   0x03 - Vendor-defined error node
  ///   0x04 - GIC error node
  UINT8     Type;

  /// Length of structure in bytes.
  UINT16    Length;

  /// Reserved - Must be zero.
  UINT8     Reserved;

  /// Offset from the start of the node to node-specific data.
  UINT32    DataOffset;

  /// Offset from the start of the node to the node interface structure.
  UINT32    InterfaceOffset;

  /// Offset from the start of the node to node interrupt array.
  UINT32    InterruptArrayOffset;

  /// Number of entries in the interrupt array.
  UINT32    InterruptArrayCount;

  // Generic node data

  /// The timestamp frequency of the counter in Hz.
  UINT64    TimestampRate;

  /// Reserved - Must be zero.
  UINT64    Reserved1;

  /// The rate in Hz at which the Error Generation Counter decrements.
  UINT64    ErrorInjectionCountdownRate;
} EFI_ACPI_AEST_NODE_STRUCT;

// AEST Node type definitions
#define EFI_ACPI_AEST_NODE_TYPE_PROCESSOR       0x0
#define EFI_ACPI_AEST_NODE_TYPE_MEMORY          0x1
#define EFI_ACPI_AEST_NODE_TYPE_SMMU            0x2
#define EFI_ACPI_AEST_NODE_TYPE_VENDOR_DEFINED  0x3
#define EFI_ACPI_AEST_NODE_TYPE_GIC             0x4

///
/// AEST Node Interface structure.
///
typedef struct {
  /// Interface type:
  ///   0x0 - System register (SR)
  ///   0x1 - Memory mapped (MMIO)
  UINT8     Type;

  /// Reserved - Must be zero.
  UINT8     Reserved[3];

  /// AEST node interface flags.
  UINT32    Flags;

  /// Base address of error group that contains the error node.
  UINT64    BaseAddress;

  /// Zero-based index of the first standard error record that
  /// belongs to this node.
  UINT32    StartErrorRecordIndex;

  /// Number of error records in this node including both
  /// implemented and unimplemented records.
  UINT32    NumberErrorRecords;

  /// A bitmap indicating the error records within this
  /// node that are implemented in the current system.
  UINT64    ErrorRecordImplemented;

  /// A bitmap indicating the error records within this node that
  /// support error status reporting through the ERRGSR register.
  UINT64    ErrorRecordStatusReportingSupported;

  /// A bitmap indicating the addressing mode used by each error
  /// record within this node to populate the ERR<n>_ADDR register.
  UINT64    AddressingMode;
} EFI_ACPI_AEST_INTERFACE_STRUCT;

// AEST Interface node type definitions.
#define EFI_ACPI_AEST_INTERFACE_TYPE_SR    0x0
#define EFI_ACPI_AEST_INTERFACE_TYPE_MMIO  0x1

// AEST node interface flag definitions.
#define EFI_ACPI_AEST_INTERFACE_FLAG_PRIVATE      0
#define EFI_ACPI_AEST_INTERFACE_FLAG_SHARED       BIT0
#define EFI_ACPI_AEST_INTERFACE_FLAG_CLEAR_MISCX  BIT1

///
/// AEST Node Interrupt structure.
///
typedef struct {
  /// Interrupt type:
  ///   0x0 - Fault Handling Interrupt
  ///   0x1 - Error Recovery Interrupt
  UINT8     InterruptType;

  /// Reserved - Must be zero.
  UINT8     Reserved[2];

  /// Interrupt flags
  /// Bits [31:1]: Must be zero.
  /// Bit 0:
  ///   0b - Interrupt is edge-triggered
  ///   1b - Interrupt is level-triggered
  UINT8     InterruptFlags;

  /// GSIV of interrupt, if interrupt is an SPI or a PPI.
  UINT32    InterruptGsiv;

  /// If MSI is supported, then this field must be set to the
  /// Identifier field of the IORT ITS Group node.
  UINT8     ItsGroupRefId;

  /// Reserved - must be zero.
  UINT8     Reserved1[3];
} EFI_ACPI_AEST_INTERRUPT_STRUCT;

// AEST Interrupt node - interrupt type definitions.
#define EFI_ACPI_AEST_INTERRUPT_TYPE_FAULT_HANDLING  0x0
#define EFI_ACPI_AEST_INTERRUPT_TYPE_ERROR_RECOVERY  0x1

// AEST Interrupt node - interrupt flag definitions.
#define EFI_ACPI_AEST_INTERRUPT_FLAG_TRIGGER_TYPE_EDGE   0
#define EFI_ACPI_AEST_INTERRUPT_FLAG_TRIGGER_TYPE_LEVEL  BIT0

///
/// Cache Processor Resource structure.
///
typedef struct {
  /// Reference to the cache structure in the PPTT table.
  UINT32    CacheRefId;

  /// Reserved
  UINT32    Reserved;
} EFI_ACPI_AEST_PROCESSOR_CACHE_RESOURCE_STRUCT;

///
/// TLB Processor Resource structure.
///
typedef struct {
  /// TLB level from perspective of current processor.
  UINT32    TlbRefId;

  /// Reserved
  UINT32    Reserved;
} EFI_ACPI_AEST_PROCESSOR_TLB_RESOURCE_STRUCT;

///
/// Processor Generic Resource structure.
///
typedef struct {
  /// Vendor-defined supplementary data.
  UINT32    Data;
} EFI_ACPI_AEST_PROCESSOR_GENERIC_RESOURCE_STRUCT;

///
/// AEST Processor Resource union.
///
typedef union {
  /// Processor Cache resource.
  EFI_ACPI_AEST_PROCESSOR_CACHE_RESOURCE_STRUCT      Cache;

  /// Processor TLB resource.
  EFI_ACPI_AEST_PROCESSOR_TLB_RESOURCE_STRUCT        Tlb;

  /// Processor Generic resource.
  EFI_ACPI_AEST_PROCESSOR_GENERIC_RESOURCE_STRUCT    Generic;
} EFI_ACPI_AEST_PROCESSOR_RESOURCE;

///
/// AEST Processor structure.
///
typedef struct {
  /// AEST Node header
  EFI_ACPI_AEST_NODE_STRUCT           NodeHeader;

  /// Processor ID of node.
  UINT32                              AcpiProcessorId;

  /// Resource type of the processor node.
  ///   0x0 - Cache
  ///   0x1 - TLB
  ///   0x2 - Generic
  UINT8                               ResourceType;

  /// Reserved - must be zero.
  UINT8                               Reserved;

  /// Processor structure flags.
  UINT8                               Flags;

  /// Processor structure revision.
  UINT8                               Revision;

  /// Processor affinity descriptor for the resource that this
  /// error node pertains to.
  UINT64                              ProcessorAffinityLevelIndicator;

  /// Processor resource
  EFI_ACPI_AEST_PROCESSOR_RESOURCE    Resource;

  // Node Interface
  // EFI_ACPI_AEST_INTERFACE_STRUCT   NodeInterface;

  // Node Interrupt Array
  // EFI_ACPI_AEST_INTERRUPT_STRUCT   NodeInterruptArray[n];
} EFI_ACPI_AEST_PROCESSOR_STRUCT;

// AEST Processor resource type definitions.
#define EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_CACHE    0x0
#define EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_TLB      0x1
#define EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_GENERIC  0x2

// AEST Processor flag definitions.
#define EFI_ACPI_AEST_PROCESSOR_FLAG_GLOBAL  BIT0
#define EFI_ACPI_AEST_PROCESSOR_FLAG_SHARED  BIT1

///
/// Memory Controller structure.
///
typedef struct {
  /// AEST Node header
  EFI_ACPI_AEST_NODE_STRUCT    NodeHeader;

  /// SRAT proximity domain.
  UINT32                       ProximityDomain;

  // Node Interface
  // EFI_ACPI_AEST_INTERFACE_STRUCT   NodeInterface;

  // Node Interrupt Array
  // EFI_ACPI_AEST_INTERRUPT_STRUCT   NodeInterruptArray[n];
} EFI_ACPI_AEST_MEMORY_CONTROLLER_STRUCT;

///
/// SMMU structure.
///
typedef struct {
  /// AEST Node header
  EFI_ACPI_AEST_NODE_STRUCT    NodeHeader;

  /// Reference to the IORT table node that describes this SMMU.
  UINT32                       SmmuRefId;

  /// Reference to the IORT table node that is associated with the
  /// sub-component within this SMMU.
  UINT32                       SubComponentRefId;

  // Node Interface
  // EFI_ACPI_AEST_INTERFACE_STRUCT   NodeInterface;

  // Node Interrupt Array
  // EFI_ACPI_AEST_INTERRUPT_STRUCT   NodeInterruptArray[n];
} EFI_ACPI_AEST_SMMU_STRUCT;

///
/// Vendor-Defined structure.
///
typedef struct {
  /// AEST Node header
  EFI_ACPI_AEST_NODE_STRUCT    NodeHeader;

  /// ACPI HID of the component.
  UINT32                       HardwareId;

  /// The ACPI Unique identifier of the component.
  UINT32                       UniqueId;

  /// Vendor-specific data, for example to identify this error source.
  UINT8                        VendorData[16];

  // Node Interface
  // EFI_ACPI_AEST_INTERFACE_STRUCT   NodeInterface;

  // Node Interrupt Array
  // EFI_ACPI_AEST_INTERRUPT_STRUCT   NodeInterruptArray[n];
} EFI_ACPI_AEST_VENDOR_DEFINED_STRUCT;

///
/// GIC structure.
///
typedef struct {
  /// AEST Node header
  EFI_ACPI_AEST_NODE_STRUCT    NodeHeader;

  /// Type of GIC interface that is associated with this error node.
  ///   0x0 - GIC CPU (GICC)
  ///   0x1 - GIC Distributor (GICD)
  ///   0x2 - GIC Redistributor (GICR)
  ///   0x3 - GIC ITS (GITS)
  UINT32                       InterfaceType;

  /// Identifier for the interface instance.
  UINT32                       GicInterfaceRefId;

  // Node Interface
  // EFI_ACPI_AEST_INTERFACE_STRUCT   NodeInterface;

  // Node Interrupt Array
  // EFI_ACPI_AEST_INTERRUPT_STRUCT   NodeInterruptArray[n];
} EFI_ACPI_AEST_GIC_STRUCT;

// AEST GIC interface type definitions.
#define EFI_ACPI_AEST_GIC_INTERFACE_TYPE_GICC  0x0
#define EFI_ACPI_AEST_GIC_INTERFACE_TYPE_GICD  0x1
#define EFI_ACPI_AEST_GIC_INTERFACE_TYPE_GICR  0x2
#define EFI_ACPI_AEST_GIC_INTERFACE_TYPE_GITS  0x3

// AEST revision 2 definitions from DEN0085 2.0 EAC2.

#define EFI_ACPI_AEST_2_0_TABLE_SIGNATURE  SIGNATURE_32 ('A', 'E', 'S', 'T')
#define EFI_ACPI_AEST_2_0_TABLE_REVISION   2

#define EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR  0x00
#define EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY     0x01
#define EFI_ACPI_AEST_2_0_NODE_TYPE_SMMU       0x02
#define EFI_ACPI_AEST_2_0_NODE_TYPE_VENDOR     0x03
#define EFI_ACPI_AEST_2_0_NODE_TYPE_GIC        0x04
#define EFI_ACPI_AEST_2_0_NODE_TYPE_PCIE       0x05
#define EFI_ACPI_AEST_2_0_NODE_TYPE_PROXY      0x06

#define EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_CACHE    0
#define EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_TLB      1
#define EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_GENERIC  2

#define EFI_ACPI_AEST_2_0_PROCESSOR_FLAG_GLOBAL  BIT0
#define EFI_ACPI_AEST_2_0_PROCESSOR_FLAG_SHARED  BIT1

/// Mask of defined AEST 2.0 processor flag bits.
#define EFI_ACPI_AEST_2_0_PROCESSOR_FLAGS_MASK \
  (EFI_ACPI_AEST_2_0_PROCESSOR_FLAG_GLOBAL | \
   EFI_ACPI_AEST_2_0_PROCESSOR_FLAG_SHARED)

#define EFI_ACPI_AEST_2_0_GIC_INTERFACE_TYPE_GICC  0x0
#define EFI_ACPI_AEST_2_0_GIC_INTERFACE_TYPE_GICD  0x1
#define EFI_ACPI_AEST_2_0_GIC_INTERFACE_TYPE_GICR  0x2
#define EFI_ACPI_AEST_2_0_GIC_INTERFACE_TYPE_GITS  0x3

#define EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SR           0
#define EFI_ACPI_AEST_2_0_INTERFACE_TYPE_MMIO         1
#define EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO  2

#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB   0
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB  1
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB  2

/// Number of 64-bit words in each AEST 2.0 node interface bitmap.
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB_BITMAP_WORD_COUNT   1
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB_BITMAP_WORD_COUNT  4
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB_BITMAP_WORD_COUNT  14

/// Maximum number of error records for each error group format.
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB_MAX_ERROR_RECORDS   56
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB_MAX_ERROR_RECORDS  224
#define EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB_MAX_ERROR_RECORDS  896

#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_SHARED               BIT0
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_CLEAR_MISCX          BIT1
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_DEVICE_VALID         BIT2
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_AFFINITY_TYPE        BIT3
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_GROUP_ADDRESS_VALID  BIT4
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INJECTION_VALID      BIT5
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INTERRUPT_CFG_VALID  BIT6

/// Mask of defined AEST node interface flag bits.
#define EFI_ACPI_AEST_2_0_INTERFACE_FLAGS_MASK \
  (EFI_ACPI_AEST_2_0_INTERFACE_FLAG_SHARED | \
   EFI_ACPI_AEST_2_0_INTERFACE_FLAG_CLEAR_MISCX | \
   EFI_ACPI_AEST_2_0_INTERFACE_FLAG_DEVICE_VALID | \
   EFI_ACPI_AEST_2_0_INTERFACE_FLAG_AFFINITY_TYPE | \
   EFI_ACPI_AEST_2_0_INTERFACE_FLAG_GROUP_ADDRESS_VALID | \
   EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INJECTION_VALID | \
   EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INTERRUPT_CFG_VALID)

#define EFI_ACPI_AEST_2_0_INTERRUPT_TYPE_FHI  0
#define EFI_ACPI_AEST_2_0_INTERRUPT_TYPE_ERI  1

#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_TRIGGER_TYPE_MASK  BIT0
#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_FHI_ON_UE_MASK     BIT1

#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_TRIGGER_TYPE_EDGE    0
#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_TRIGGER_TYPE_LEVEL   BIT0
#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_FHI_ON_UE_SUPPORTED  0
#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_NO_FHI_ON_UE         BIT1

/// Mask of defined AEST node interrupt flag bits.
#define EFI_ACPI_AEST_2_0_INTERRUPT_FLAGS_MASK \
  (EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_TRIGGER_TYPE_MASK | \
   EFI_ACPI_AEST_2_0_INTERRUPT_FLAG_FHI_ON_UE_MASK)

///
/// AEST 2.0 table header. See DEN0085 2.0 EAC2 Table 3.
///
typedef struct {
  /// Standard ACPI table header.
  EFI_ACPI_DESCRIPTION_HEADER    Header;
} EFI_ACPI_AEST_2_0_TABLE;

///
/// Fixed prefix of every AEST 2.0 node.
///
/// The three offsets are relative to the start of the node. See
/// DEN0085 2.0 EAC2 Table 4.
///
typedef struct {
  /// Node type:
  ///   0x00 - Processor error node
  ///   0x01 - Memory error node
  ///   0x02 - SMMU error node
  ///   0x03 - Vendor-defined error node
  ///   0x04 - GIC error node
  ///   0x05 - PCIe error node
  ///   0x06 - Proxy error node
  UINT8     Type;
  /// Total node length in bytes.
  UINT16    Length;
  /// Reserved; must be zero.
  UINT8     Reserved;
  /// Offset to node-specific data.
  UINT32    DataOffset;
  /// Offset to the node interface.
  UINT32    InterfaceOffset;
  /// Offset to the interrupt array.
  UINT32    InterruptArrayOffset;
  /// Number of interrupt array entries.
  UINT32    InterruptArrayCount;
  /// Timestamp counter rate in hertz.
  UINT64    TimestampRate;
  /// Signed timebase offset in nanoseconds.
  INT64     TimebaseOffset;
  /// Error injection countdown rate in hertz.
  UINT64    ErrorInjectionCountdownRate;
} EFI_ACPI_AEST_2_0_NODE;

///
/// Processor node data. See DEN0085 2.0 EAC2 Table 5.
///
typedef struct {
  /// Processor _UID, or zero for a Global or Shared node.
  UINT32    AcpiProcessorId;
  /// Processor resource type:
  ///   0x0 - Cache
  ///   0x1 - TLB
  ///   0x2 - Generic
  UINT8     ResourceType;
  /// Reserved; must be zero.
  UINT8     Reserved;
  /// Global and Shared processor node flags.
  UINT8     Flags;
  /// Processor structure revision; must be zero.
  UINT8     Revision;
  /// Deprecated processor affinity level indicator.
  UINT64    ProcessorAffinityLevelIndicator;
} EFI_ACPI_AEST_2_0_PROCESSOR_DATA;

///
/// Cache resource of a processor node. See DEN0085 2.0 EAC2 Table 7.
///
typedef struct {
  /// Cache ID from the PPTT cache structure.
  UINT32    CacheReference;
  /// Reserved; must be zero.
  UINT32    Reserved;
} EFI_ACPI_AEST_2_0_PROCESSOR_CACHE_RESOURCE;

///
/// TLB resource of a processor node. See DEN0085 2.0 EAC2 Table 8.
///
typedef struct {
  /// TLB level from the perspective of the associated processor.
  UINT32    TlbLevel;
  /// Reserved; must be zero.
  UINT32    Reserved;
} EFI_ACPI_AEST_2_0_PROCESSOR_TLB_RESOURCE;

///
/// Generic resource of a processor node. See DEN0085 2.0 EAC2 Table 9.
///
typedef struct {
  /// Vendor-defined supplementary data.
  UINT32    Data;
} EFI_ACPI_AEST_2_0_PROCESSOR_GENERIC_RESOURCE;

///
/// Memory controller node data. See DEN0085 2.0 EAC2 Table 10.
///
typedef struct {
  /// SRAT proximity domain.
  UINT32    ProximityDomain;
} EFI_ACPI_AEST_2_0_MEMORY_DATA;

///
/// SMMU node data. See DEN0085 2.0 EAC2 Table 11.
///
typedef struct {
  /// Identifier of the IORT SMMU node.
  UINT32    IortNodeReference;
  /// Identifier of the associated IORT subcomponent.
  UINT32    SubcomponentReference;
} EFI_ACPI_AEST_2_0_SMMU_DATA;

///
/// Vendor-defined node data. See DEN0085 2.0 EAC2 Table 12.
///
typedef struct {
  /// Eight-byte hardware ID.
  UINT8     HardwareId[8];
  /// Four-byte unique ID.
  UINT32    UniqueId;
  /// Vendor-defined data.
  UINT8     VendorData[16];
} EFI_ACPI_AEST_2_0_VENDOR_DATA;

///
/// GIC node data. See DEN0085 2.0 EAC2 Table 13.
///
typedef struct {
  /// GIC interface type:
  ///   0x0 - GIC CPU (GICC)
  ///   0x1 - GIC Distributor (GICD)
  ///   0x2 - GIC Redistributor (GICR)
  ///   0x3 - GIC ITS (GITS)
  UINT32    InterfaceType;
  /// Identifier of the selected GIC interface instance.
  UINT32    InstanceIdentifier;
} EFI_ACPI_AEST_2_0_GIC_DATA;

///
/// PCIe Root Complex node data. See DEN0085 2.0 EAC2 Table 14.
///
typedef struct {
  /// IORT Root Complex identifier or offset, depending on IORT revision.
  UINT32    IortNodeReference;
} EFI_ACPI_AEST_2_0_PCIE_DATA;

///
/// Proxy node data. See DEN0085 2.0 EAC2 Table 15.
///
typedef struct {
  /// Address of the proxied error-record group's first record.
  UINT64    NodeAddress;
} EFI_ACPI_AEST_2_0_PROXY_DATA;

///
/// AEST 2.0 interface prefix.
///
/// The complete interface contains this prefix, three variable-length
/// UINT64 bitmap arrays, and EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX, in that
/// order. Each bitmap has 1, 4, or 14 words according to GroupFormat.
/// See DEN0085 2.0 EAC2 Table 17.
///
typedef struct {
  /// Interface type:
  ///   0x0 - System register (SR) view
  ///   0x1 - Memory-mapped (MMIO) view
  ///   0x2 - Single-record memory-mapped view
  UINT8     Type;
  /// Error-record group format:
  ///   0x0 - 4 KiB (one UINT64 word per bitmap)
  ///   0x1 - 16 KiB (four UINT64 words per bitmap)
  ///   0x2 - 64 KiB (fourteen UINT64 words per bitmap)
  UINT8     GroupFormat;
  /// Reserved; must be zero.
  UINT16    Reserved;
  /// Node interface flags.
  UINT32    Flags;
  /// Base address of the error-record group or single record.
  UINT64    BaseAddress;
  /// Zero-based index of this node's first error record.
  UINT32    StartErrorRecordIndex;
  /// Number of error records belonging to this node.
  UINT32    NumberErrorRecords;
} EFI_ACPI_AEST_2_0_INTERFACE_PREFIX;

///
/// Fields following the three variable-length interface bitmaps.
///
/// See DEN0085 2.0 EAC2 Table 17.
///
typedef struct {
  /// UID of the optional Arm error-node device.
  UINT32    ArmErrorNodeDeviceUid;
  /// UID of the associated processor or processor container.
  UINT32    ProcessorAffinity;
  /// Base address of the error-group level register set.
  UINT64    ErrorGroupRegistersBaseAddress;
  /// Base address of the fault-injection register set.
  UINT64    FaultInjectionRegistersBaseAddress;
  /// Base address of the interrupt-configuration register set.
  UINT64    InterruptConfigurationRegistersBaseAddress;
} EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX;

///
/// AEST 2.0 interrupt entry. See DEN0085 2.0 EAC2 Table 19.
///
typedef struct {
  /// Interrupt type:
  ///   0x0 - Fault Handling Interrupt (FHI)
  ///   0x1 - Error Recovery Interrupt (ERI)
  UINT8     Type;
  /// Reserved; must be zero.
  UINT8     Reserved[2];
  /// Bit 0: edge (0) or level (1).
  /// Bit 1: FHI on UE (0) or no FHI (1).
  UINT8     Flags;
  /// Interrupt GSIV; zero selects MSI.
  UINT32    Gsiv;
  /// Reserved; must be zero.
  UINT32    Reserved2;
} EFI_ACPI_AEST_2_0_INTERRUPT;

#pragma pack()
