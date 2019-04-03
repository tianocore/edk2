/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef ARM_NAMESPACE_OBJECTS_H_
#define ARM_NAMESPACE_OBJECTS_H_

#include <StandardNameSpaceObjects.h>

#pragma pack(1)

/** The EARM_OBJECT_ID enum describes the Object IDs
    in the ARM Namespace
*/
typedef enum ArmObjectID {
  EArmObjReserved,                    ///<  0 - Reserved
  EArmObjBootArchInfo,                ///<  1 - Boot Architecture Info
  EArmObjCpuInfo,                     ///<  2 - CPU Info
  EArmObjPowerManagementProfileInfo,  ///<  3 - Power Management Profile Info
  EArmObjGicCInfo,                    ///<  4 - GIC CPU Interface Info
  EArmObjGicDInfo,                    ///<  5 - GIC Distributor Info
  EArmObjGicMsiFrameInfo,             ///<  6 - GIC MSI Frame Info
  EArmObjGicRedistributorInfo,        ///<  7 - GIC Redistributor Info
  EArmObjGicItsInfo,                  ///<  8 - GIC ITS Info
  EArmObjSerialConsolePortInfo,       ///<  9 - Serial Console Port Info
  EArmObjSerialDebugPortInfo,         ///< 10 - Serial Debug Port Info
  EArmObjGenericTimerInfo,            ///< 11 - Generic Timer Info
  EArmObjPlatformGTBlockInfo,         ///< 12 - Platform GT Block Info
  EArmObjGTBlockTimerFrameInfo,       ///< 13 - Generic Timer Block Frame Info
  EArmObjPlatformGenericWatchdogInfo, ///< 14 - Platform Generic Watchdog
  EArmObjPciConfigSpaceInfo,          ///< 15 - PCI Configuration Space Info
  EArmObjHypervisorVendorIdentity,    ///< 16 - Hypervisor Vendor Id
  EArmObjFixedFeatureFlags,           ///< 17 - Fixed feature flags for FADT
  EArmObjItsGroup,                    ///< 18 - ITS Group
  EArmObjNamedComponent,              ///< 19 - Named Component
  EArmObjRootComplex,                 ///< 20 - Root Complex
  EArmObjSmmuV1SmmuV2,                ///< 21 - SMMUv1 or SMMUv2
  EArmObjSmmuV3,                      ///< 22 - SMMUv3
  EArmObjPmcg,                        ///< 23 - PMCG
  EArmObjGicItsIdentifierArray,       ///< 24 - GIC ITS Identifier Array
  EArmObjIdMappingArray,              ///< 25 - ID Mapping Array
  EArmObjSmmuInterruptArray,          ///< 26 - SMMU Interrupt Array
  EArmObjMax
} EARM_OBJECT_ID;

/** A structure that describes the
    ARM Boot Architecture flags.

    ID: EArmObjBootArchInfo
*/
typedef struct CmArmBootArchInfo {
  /** This is the ARM_BOOT_ARCH flags field of the FADT Table
      described in the ACPI Table Specification.
  */
  UINT32  BootArchFlags;
} CM_ARM_BOOT_ARCH_INFO;

typedef struct CmArmCpuInfo {
  // Reserved for use when SMBIOS tables are implemented
} CM_ARM_CPU_INFO;

/** A structure that describes the
    Power Management Profile Information for the Platform.

    ID: EArmObjPowerManagementProfileInfo
*/
typedef struct CmArmPowerManagementProfileInfo {
  /** This is the Preferred_PM_Profile field of the FADT Table
      described in the ACPI Specification
  */
  UINT8  PowerManagementProfile;
} CM_ARM_POWER_MANAGEMENT_PROFILE_INFO;

/** A structure that describes the
    GIC CPU Interface for the Platform.

    ID: EArmObjGicCInfo
*/
typedef struct CmArmGicCInfo {
  /// The GIC CPU Interface number.
  UINT32  CPUInterfaceNumber;

  /** The ACPI Processor UID. This must match the
      _UID of the CPU Device object information described
      in the DSDT/SSDT for the CPU.
  */
  UINT32  AcpiProcessorUid;

  /** The flags field as described by the GICC structure
      in the ACPI Specification.
  */
  UINT32  Flags;

  /** The parking protocol version field as described by
    the GICC structure in the ACPI Specification.
  */
  UINT32  ParkingProtocolVersion;

  /** The Performance Interrupt field as described by
      the GICC structure in the ACPI Specification.
  */
  UINT32  PerformanceInterruptGsiv;

  /** The CPU Parked address field as described by
      the GICC structure in the ACPI Specification.
  */
  UINT64  ParkedAddress;

  /** The base address for the GIC CPU Interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64  PhysicalBaseAddress;

  /** The base address for GICV interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64  GICV;

  /** The base address for GICH interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64  GICH;

  /** The GICV maintenance interrupt
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT32  VGICMaintenanceInterrupt;

  /** The base address for GICR interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64  GICRBaseAddress;

  /** The MPIDR for the CPU
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64  MPIDR;

  /** The Processor Power Efficiency class
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT8   ProcessorPowerEfficiencyClass;
} CM_ARM_GICC_INFO;

/** A structure that describes the
    GIC Distributor information for the Platform.

    ID: EArmObjGicDInfo
*/
typedef struct CmArmGicDInfo {
  /// The Physical Base address for the GIC Distributor.
  UINT64  PhysicalBaseAddress;

  /** The global system interrupt
      number where this GIC Distributor's
      interrupt inputs start.
  */
  UINT32  SystemVectorBase;

  /** The GIC version as described
      by the GICD structure in the
      ACPI Specification.
  */
  UINT8   GicVersion;
} CM_ARM_GICD_INFO;

/** A structure that describes the
    GIC MSI Frame information for the Platform.

    ID: EArmObjGicMsiFrameInfo
*/
typedef struct CmArmGicMsiFrameInfo {
  /// The GIC MSI Frame ID
  UINT32  GicMsiFrameId;

  /// The Physical base address for the MSI Frame
  UINT64  PhysicalBaseAddress;

  /** The GIC MSI Frame flags
      as described by the GIC MSI frame
      structure in the ACPI Specification.
  */
  UINT32  Flags;

  /// SPI Count used by this frame
  UINT16  SPICount;

  /// SPI Base used by this frame
  UINT16  SPIBase;
} CM_ARM_GIC_MSI_FRAME_INFO;

/** A structure that describes the
    GIC Redistributor information for the Platform.

    ID: EArmObjGicRedistributorInfo
*/
typedef struct CmArmGicRedistInfo {
  /** The physical address of a page range
      containing all GIC Redistributors.
  */
  UINT64  DiscoveryRangeBaseAddress;

  /// Length of the GIC Redistributor Discovery page range
  UINT32  DiscoveryRangeLength;
} CM_ARM_GIC_REDIST_INFO;

/** A structure that describes the
    GIC Interrupt Translation Service information for the Platform.

    ID: EArmObjGicItsInfo
*/
typedef struct CmArmGicItsInfo {
  /// The GIC ITS ID
  UINT32  GicItsId;

  /// The physical address for the Interrupt Translation Service
  UINT64  PhysicalBaseAddress;
} CM_ARM_GIC_ITS_INFO;

/** A structure that describes the
    Serial Port information for the Platform.

    ID: EArmObjSerialConsolePortInfo or
        EArmObjSerialDebugPortInfo
*/
typedef struct CmArmSerialPortInfo {
  /// The physical base address for the serial port
  UINT64  BaseAddress;

  /// The serial port interrupt
  UINT32  Interrupt;

  /// The serial port baud rate
  UINT64  BaudRate;

  /// The serial port clock
  UINT32  Clock;

  /// Serial Port subtype
  UINT16  PortSubtype;
} CM_ARM_SERIAL_PORT_INFO;

/** A structure that describes the
    Generic Timer information for the Platform.

    ID: EArmObjGenericTimerInfo
*/
typedef struct CmArmGenericTimerInfo {
  /// The physical base address for the counter control frame
  UINT64  CounterControlBaseAddress;

  /// The physical base address for the counter read frame
  UINT64  CounterReadBaseAddress;

  /// The secure PL1 timer interrupt
  UINT32  SecurePL1TimerGSIV;

  /// The secure PL1 timer flags
  UINT32  SecurePL1TimerFlags;

  /// The non-secure PL1 timer interrupt
  UINT32  NonSecurePL1TimerGSIV;

  /// The non-secure PL1 timer flags
  UINT32  NonSecurePL1TimerFlags;

  /// The virtual timer interrupt
  UINT32  VirtualTimerGSIV;

  /// The virtual timer flags
  UINT32  VirtualTimerFlags;

  /// The non-secure PL2 timer interrupt
  UINT32  NonSecurePL2TimerGSIV;

  /// The non-secure PL2 timer flags
  UINT32  NonSecurePL2TimerFlags;
} CM_ARM_GENERIC_TIMER_INFO;

/** A structure that describes the
    Platform Generic Block Timer Frame information for the Platform.

    ID: EArmObjGTBlockTimerFrameInfo
*/
typedef struct CmArmGTBlockTimerFrameInfo {
  /// The Generic Timer frame number
  UINT8   FrameNumber;

  /// The physical base address for the CntBase block
  UINT64  PhysicalAddressCntBase;

  /// The physical base address for the CntEL0Base block
  UINT64  PhysicalAddressCntEL0Base;

  /// The physical timer interrupt
  UINT32  PhysicalTimerGSIV;

  /** The physical timer flags as described by the GT Block
      Timer frame Structure in the ACPI Specification.
  */
  UINT32  PhysicalTimerFlags;

  /// The virtual timer interrupt
  UINT32  VirtualTimerGSIV;

  /** The virtual timer flags as described by the GT Block
      Timer frame Structure in the ACPI Specification.
  */
  UINT32  VirtualTimerFlags;

  /** The common timer flags as described by the GT Block
      Timer frame Structure in the ACPI Specification.
  */
  UINT32  CommonFlags;
} CM_ARM_GTBLOCK_TIMER_FRAME_INFO;

/** A structure that describes the
    Platform Generic Block Timer information for the Platform.

    ID: EArmObjPlatformGTBlockInfo
*/
typedef struct CmArmGTBlockInfo {
  /// The physical base address for the GT Block Timer structure
  UINT64            GTBlockPhysicalAddress;

  /// The number of timer frames implemented in the GT Block
  UINT32            GTBlockTimerFrameCount;

  /// Reference token for the GT Block timer frame list
  CM_OBJECT_TOKEN   GTBlockTimerFrameToken;
} CM_ARM_GTBLOCK_INFO;

/** A structure that describes the
    SBSA Generic Watchdog information for the Platform.

    ID: EArmObjPlatformGenericWatchdogInfo
*/
typedef struct CmArmGenericWatchdogInfo {
  /// The physical base address of the SBSA Watchdog control frame
  UINT64  ControlFrameAddress;

  /// The physical base address of the SBSA Watchdog refresh frame
  UINT64  RefreshFrameAddress;

  /// The watchdog interrupt
  UINT32  TimerGSIV;

  /** The flags for the watchdog as described by the SBSA watchdog
      structure in the ACPI specification.
  */
  UINT32  Flags;
} CM_ARM_GENERIC_WATCHDOG_INFO;

/** A structure that describes the
    PCI Configuration Space information for the Platform.

    ID: EArmObjPciConfigSpaceInfo
*/
typedef struct CmArmPciConfigSpaceInfo {
  /// The physical base address for the PCI segment
  UINT64  BaseAddress;

  /// The PCI segment group number
  UINT16  PciSegmentGroupNumber;

  /// The start bus number
  UINT8   StartBusNumber;

  /// The end bus number
  UINT8   EndBusNumber;
} CM_ARM_PCI_CONFIG_SPACE_INFO;

/** A structure that describes the
    Hypervisor Vendor ID information for the Platform.

    ID: EArmObjHypervisorVendorIdentity
*/
typedef struct CmArmHypervisorVendorId {
  /// The hypervisor Vendor ID
  UINT64  HypervisorVendorId;
} CM_ARM_HYPERVISOR_VENDOR_ID;

/** A structure that describes the
    Fixed feature flags for the Platform.

    ID: EArmObjFixedFeatureFlags
*/
typedef struct CmArmFixedFeatureFlags {
  /// The Fixed feature flags
  UINT32  Flags;
} CM_ARM_FIXED_FEATURE_FLAGS;

/** A structure that describes the
    ITS Group node for the Platform.

    ID: EArmObjItsGroup
*/
typedef struct CmArmItsGroupNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN   Token;
  /// The number of ITS identifiers in the ITS node
  UINT32            ItsIdCount;
  /// Reference token for the ITS identifier array
  CM_OBJECT_TOKEN   ItsIdToken;
} CM_ARM_ITS_GROUP_NODE;

/** A structure that describes the
    GIC ITS Identifiers for an ITS Group node.

    ID: EArmObjGicItsIdentifierArray
*/
typedef struct CmArmGicItsIdentifier {
  /// The ITS Identifier
  UINT32  ItsId;
} CM_ARM_ITS_IDENTIFIER;

/** A structure that describes the
    Named component node for the Platform.

    ID: EArmObjNamedComponent
*/
typedef struct CmArmNamedComponentNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN   Token;
  /// Number of ID mappings
  UINT32            IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN   IdMappingToken;

  /// Flags for the named component
  UINT32            Flags;

  /// Memory access properties : Cache coherent attributes
  UINT32            CacheCoherent;
  /// Memory access properties : Allocation hints
  UINT8             AllocationHints;
  /// Memory access properties : Memory access flags
  UINT8             MemoryAccessFlags;

  /// Memory access properties : Address size limit
  UINT8             AddressSizeLimit;
  /** ASCII Null terminated string with the full path to
      the entry in the namespace for this object.
  */
  CHAR8*            ObjectName;
} CM_ARM_NAMED_COMPONENT_NODE;

/** A structure that describes the
    Root complex node for the Platform.

    ID: EArmObjRootComplex
*/
typedef struct CmArmRootComplexNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN   Token;
  /// Number of ID mappings
  UINT32            IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN   IdMappingToken;

  /// Memory access properties : Cache coherent attributes
  UINT32            CacheCoherent;
  /// Memory access properties : Allocation hints
  UINT8             AllocationHints;
  /// Memory access properties : Memory access flags
  UINT8             MemoryAccessFlags;

  /// ATS attributes
  UINT32            AtsAttribute;
  /// PCI segment number
  UINT32            PciSegmentNumber;
  /// Memory address size limit
  UINT8             MemoryAddressSize;
} CM_ARM_ROOT_COMPLEX_NODE;

/** A structure that describes the
    SMMUv1 or SMMUv2 node for the Platform.

    ID: EArmObjSmmuV1SmmuV2
*/
typedef struct CmArmSmmuV1SmmuV2Node {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN   Token;
  /// Number of ID mappings
  UINT32            IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN   IdMappingToken;

  /// SMMU Base Address
  UINT64            BaseAddress;
  /// Length of the memory range covered by the SMMU
  UINT64            Span;
  /// SMMU Model
  UINT32            Model;
  /// SMMU flags
  UINT32            Flags;

  /// Number of context interrupts
  UINT32            ContextInterruptCount;
  /// Reference token for the context interrupt array
  CM_OBJECT_TOKEN   ContextInterruptToken;

  /// Number of PMU interrupts
  UINT32            PmuInterruptCount;
  /// Reference token for the PMU interrupt array
  CM_OBJECT_TOKEN   PmuInterruptToken;

  /// GSIV of the SMMU_NSgIrpt interrupt
  UINT32            SMMU_NSgIrpt;
  /// SMMU_NSgIrpt interrupt flags
  UINT32            SMMU_NSgIrptFlags;
  /// GSIV of the SMMU_NSgCfgIrpt interrupt
  UINT32            SMMU_NSgCfgIrpt;
  /// SMMU_NSgCfgIrpt interrupt flags
  UINT32            SMMU_NSgCfgIrptFlags;
} CM_ARM_SMMUV1_SMMUV2_NODE;

/** A structure that describes the
    SMMUv3 node for the Platform.

    ID: EArmObjSmmuV3
*/
typedef struct CmArmSmmuV3Node {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN   Token;
  /// Number of ID mappings
  UINT32            IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN   IdMappingToken;

  /// SMMU Base Address
  UINT64    BaseAddress;
  /// SMMU flags
  UINT32            Flags;
  /// VATOS address
  UINT64            VatosAddress;
  /// Model
  UINT32            Model;
  /// GSIV of the Event interrupt if SPI based
  UINT32            EventInterrupt;
  /// PRI Interrupt if SPI based
  UINT32            PriInterrupt;
  /// GERR interrupt if GSIV based
  UINT32            GerrInterrupt;
  /// Sync interrupt if GSIV based
  UINT32            SyncInterrupt;

  /// Proximity domain flag
  UINT32            ProximityDomain;
  /// Index into the array of ID mapping
  UINT32            DeviceIdMappingIndex;
} CM_ARM_SMMUV3_NODE;

/** A structure that describes the
    PMCG node for the Platform.

    ID: EArmObjPmcg
*/
typedef struct CmArmPmcgNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN   Token;
  /// Number of ID mappings
  UINT32            IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN   IdMappingToken;

  /// Base Address for performance monitor counter group
  UINT64            BaseAddress;
  /// GSIV for the Overflow interrupt
  UINT32            OverflowInterrupt;
  /// Page 1 Base address
  UINT64            Page1BaseAddress;

  /// Reference token for the IORT node associated with this node
  CM_OBJECT_TOKEN   ReferenceToken;
} CM_ARM_PMCG_NODE;

/** A structure that describes the
    ID Mappings for the Platform.

    ID: EArmObjIdMappingArray
*/
typedef struct CmArmIdMapping {
  /// Input base
  UINT32           InputBase;
  /// Number of input IDs
  UINT32           NumIds;
  /// Output Base
  UINT32           OutputBase;
  /// Reference token for the output node
  CM_OBJECT_TOKEN  OutputReferenceToken;
  /// Flags
  UINT32    Flags;
} CM_ARM_ID_MAPPING;

/** A structure that describes the
    SMMU interrupts for the Platform.

    ID: EArmObjSmmuInterruptArray
*/
typedef struct CmArmSmmuInterrupt {
  /// Interrupt number
  UINT32    Interrupt;

  /// Flags
  UINT32    Flags;
} CM_ARM_SMMU_INTERRUPT;

#pragma pack()

#endif // ARM_NAMESPACE_OBJECTS_H_
