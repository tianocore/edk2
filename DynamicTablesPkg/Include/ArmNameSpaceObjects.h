/** @file

  Copyright (c) 2017 - 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef ARM_NAMESPACE_OBJECTS_H_
#define ARM_NAMESPACE_OBJECTS_H_

#include <AcpiObjects.h>
#include <StandardNameSpaceObjects.h>

#pragma pack(1)

/** The EARM_OBJECT_ID enum describes the Object IDs
    in the ARM Namespace

  Note: Whenever an entry in this enum is updated,
        the following data structures must also be
        updated:
        - CM_OBJECT_TOKEN_FIXER TokenFixer[] in
          Library\Common\DynamicPlatRepoLib\CmObjectTokenFixer.c
*/
typedef enum ArmObjectID {
  EArmObjReserved,                                             ///<  0 - Reserved
  EArmObjBootArchInfo,                                         ///<  1 - Boot Architecture Info
  EArmObjGicCInfo,                                             ///<  2 - GIC CPU Interface Info
  EArmObjGicDInfo,                                             ///<  3 - GIC Distributor Info
  EArmObjGicMsiFrameInfo,                                      ///<  4 - GIC MSI Frame Info
  EArmObjGicRedistributorInfo,                                 ///<  5 - GIC Redistributor Info
  EArmObjGicItsInfo,                                           ///<  6 - GIC ITS Info
  EArmObjGenericTimerInfo,                                     ///<  7 - Generic Timer Info
  EArmObjPlatformGTBlockInfo,                                  ///<  8 - Platform GT Block Info
  EArmObjGTBlockTimerFrameInfo,                                ///<  9 - Generic Timer Block Frame Info
  EArmObjPlatformGenericWatchdogInfo,                          ///< 10 - Platform Generic Watchdog
  EArmObjItsGroup,                                             ///< 11 - ITS Group
  EArmObjNamedComponent,                                       ///< 12 - Named Component
  EArmObjRootComplex,                                          ///< 13 - Root Complex
  EArmObjSmmuV1SmmuV2,                                         ///< 14 - SMMUv1 or SMMUv2
  EArmObjSmmuV3,                                               ///< 15 - SMMUv3
  EArmObjPmcg,                                                 ///< 16 - PMCG
  EArmObjGicItsIdentifierArray,                                ///< 17 - GIC ITS Identifier Array
  EArmObjIdMappingArray,                                       ///< 18 - ID Mapping Array
  EArmObjSmmuInterruptArray,                                   ///< 19 - SMMU Interrupt Array
  EArmObjCmn600Info,                                           ///< 20 - CMN-600 Info
  EArmObjRmr,                                                  ///< 21 - Reserved Memory Range Node
  EArmObjMemoryRangeDescriptor,                                ///< 22 - Memory Range Descriptor
  EArmObjEtInfo,                                               ///< 23 - Embedded Trace Extension/Module Info
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
  UINT16    BootArchFlags;
} CM_ARM_BOOT_ARCH_INFO;

/** A structure that describes the
    GIC CPU Interface for the Platform.

    ID: EArmObjGicCInfo
*/
typedef struct CmArmGicCInfo {
  /// The GIC CPU Interface number.
  UINT32             CPUInterfaceNumber;

  /** The ACPI Processor UID. This must match the
      _UID of the CPU Device object information described
      in the DSDT/SSDT for the CPU.
  */
  UINT32             AcpiProcessorUid;

  /** The flags field as described by the GICC structure
      in the ACPI Specification.
  */
  UINT32             Flags;

  /** The parking protocol version field as described by
    the GICC structure in the ACPI Specification.
  */
  UINT32             ParkingProtocolVersion;

  /** The Performance Interrupt field as described by
      the GICC structure in the ACPI Specification.
  */
  UINT32             PerformanceInterruptGsiv;

  /** The CPU Parked address field as described by
      the GICC structure in the ACPI Specification.
  */
  UINT64             ParkedAddress;

  /** The base address for the GIC CPU Interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64             PhysicalBaseAddress;

  /** The base address for GICV interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64             GICV;

  /** The base address for GICH interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64             GICH;

  /** The GICV maintenance interrupt
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT32             VGICMaintenanceInterrupt;

  /** The base address for GICR interface
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64             GICRBaseAddress;

  /** The MPIDR for the CPU
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT64             MPIDR;

  /** The Processor Power Efficiency class
      as described by the GICC structure in the
      ACPI Specification.
  */
  UINT8              ProcessorPowerEfficiencyClass;

  /** Statistical Profiling Extension buffer overflow GSIV. Zero if
      unsupported by this processor. This field was introduced in
      ACPI 6.3 (MADT revision 5) and is therefore ignored when
      generating MADT revision 4 or lower.
  */
  UINT16             SpeOverflowInterrupt;

  /** The proximity domain to which the logical processor belongs.
      This field is used to populate the GICC affinity structure
      in the SRAT table.
  */
  UINT32             ProximityDomain;

  /** The clock domain to which the logical processor belongs.
      This field is used to populate the GICC affinity structure
      in the SRAT table.
  */
  UINT32             ClockDomain;

  /** The GICC Affinity flags field as described by the GICC Affinity structure
      in the SRAT table.
  */
  UINT32             AffinityFlags;

  /** Optional field: Reference Token for the Cpc info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_CPC_INFO object.
  */
  CM_OBJECT_TOKEN    CpcToken;

  /** Trace Buffer Extension interrupt GSIV. Zero if
      unsupported by this processor. This field was introduced in
      ACPI 6.5 (MADT revision 6) and is therefore ignored when
      generating MADT revision 5 or lower.
  */
  UINT16             TrbeInterrupt;

  /** Optional field: Reference Token for the Embedded Trace device info for
      this processing element.
      i.e. a token referencing a CM_ARM_ET_INFO object.
  */
  CM_OBJECT_TOKEN    EtToken;

  /** Optional field: Reference Token for the Psd info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_PSD_INFO object.
  */
  CM_OBJECT_TOKEN    PsdToken;
} CM_ARM_GICC_INFO;

/** A structure that describes the
    GIC Distributor information for the Platform.

    ID: EArmObjGicDInfo
*/
typedef struct CmArmGicDInfo {
  /// The Physical Base address for the GIC Distributor.
  UINT64    PhysicalBaseAddress;

  /** The global system interrupt
      number where this GIC Distributor's
      interrupt inputs start.
  */
  UINT32    SystemVectorBase;

  /** The GIC version as described
      by the GICD structure in the
      ACPI Specification.
  */
  UINT8     GicVersion;
} CM_ARM_GICD_INFO;

/** A structure that describes the
    GIC MSI Frame information for the Platform.

    ID: EArmObjGicMsiFrameInfo
*/
typedef struct CmArmGicMsiFrameInfo {
  /// The GIC MSI Frame ID
  UINT32    GicMsiFrameId;

  /// The Physical base address for the MSI Frame
  UINT64    PhysicalBaseAddress;

  /** The GIC MSI Frame flags
      as described by the GIC MSI frame
      structure in the ACPI Specification.
  */
  UINT32    Flags;

  /// SPI Count used by this frame
  UINT16    SPICount;

  /// SPI Base used by this frame
  UINT16    SPIBase;
} CM_ARM_GIC_MSI_FRAME_INFO;

/** A structure that describes the
    GIC Redistributor information for the Platform.

    ID: EArmObjGicRedistributorInfo
*/
typedef struct CmArmGicRedistInfo {
  /** The physical address of a page range
      containing all GIC Redistributors.
  */
  UINT64    DiscoveryRangeBaseAddress;

  /// Length of the GIC Redistributor Discovery page range
  UINT32    DiscoveryRangeLength;
} CM_ARM_GIC_REDIST_INFO;

/** A structure that describes the
    GIC Interrupt Translation Service information for the Platform.

    ID: EArmObjGicItsInfo
*/
typedef struct CmArmGicItsInfo {
  /// The GIC ITS ID
  UINT32    GicItsId;

  /// The physical address for the Interrupt Translation Service
  UINT64    PhysicalBaseAddress;

  /** The proximity domain to which the logical processor belongs.
      This field is used to populate the GIC ITS affinity structure
      in the SRAT table.
  */
  UINT32    ProximityDomain;
} CM_ARM_GIC_ITS_INFO;

/** A structure that describes the
    Generic Timer information for the Platform.

    ID: EArmObjGenericTimerInfo
*/
typedef struct CmArmGenericTimerInfo {
  /// The physical base address for the counter control frame
  UINT64    CounterControlBaseAddress;

  /// The physical base address for the counter read frame
  UINT64    CounterReadBaseAddress;

  /// The secure PL1 timer interrupt
  UINT32    SecurePL1TimerGSIV;

  /// The secure PL1 timer flags
  UINT32    SecurePL1TimerFlags;

  /// The non-secure PL1 timer interrupt
  UINT32    NonSecurePL1TimerGSIV;

  /// The non-secure PL1 timer flags
  UINT32    NonSecurePL1TimerFlags;

  /// The virtual timer interrupt
  UINT32    VirtualTimerGSIV;

  /// The virtual timer flags
  UINT32    VirtualTimerFlags;

  /// The non-secure PL2 timer interrupt
  UINT32    NonSecurePL2TimerGSIV;

  /// The non-secure PL2 timer flags
  UINT32    NonSecurePL2TimerFlags;

  /// GSIV for the virtual EL2 timer
  UINT32    VirtualPL2TimerGSIV;

  /// Flags for the virtual EL2 timer
  UINT32    VirtualPL2TimerFlags;
} CM_ARM_GENERIC_TIMER_INFO;

/** A structure that describes the
    Platform Generic Block Timer information for the Platform.

    ID: EArmObjPlatformGTBlockInfo
*/
typedef struct CmArmGTBlockInfo {
  /// The physical base address for the GT Block Timer structure
  UINT64             GTBlockPhysicalAddress;

  /// The number of timer frames implemented in the GT Block
  UINT32             GTBlockTimerFrameCount;

  /// Reference token for the GT Block timer frame list
  CM_OBJECT_TOKEN    GTBlockTimerFrameToken;
} CM_ARM_GTBLOCK_INFO;

/** A structure that describes the
    Platform Generic Block Timer Frame information for the Platform.

    ID: EArmObjGTBlockTimerFrameInfo
*/
typedef struct CmArmGTBlockTimerFrameInfo {
  /// The Generic Timer frame number
  UINT8     FrameNumber;

  /// The physical base address for the CntBase block
  UINT64    PhysicalAddressCntBase;

  /// The physical base address for the CntEL0Base block
  UINT64    PhysicalAddressCntEL0Base;

  /// The physical timer interrupt
  UINT32    PhysicalTimerGSIV;

  /** The physical timer flags as described by the GT Block
      Timer frame Structure in the ACPI Specification.
  */
  UINT32    PhysicalTimerFlags;

  /// The virtual timer interrupt
  UINT32    VirtualTimerGSIV;

  /** The virtual timer flags as described by the GT Block
      Timer frame Structure in the ACPI Specification.
  */
  UINT32    VirtualTimerFlags;

  /** The common timer flags as described by the GT Block
      Timer frame Structure in the ACPI Specification.
  */
  UINT32    CommonFlags;
} CM_ARM_GTBLOCK_TIMER_FRAME_INFO;

/** A structure that describes the
    Arm Generic Watchdog information for the Platform.

    ID: EArmObjPlatformGenericWatchdogInfo
*/
typedef struct CmArmGenericWatchdogInfo {
  /// The physical base address of the Arm Watchdog control frame
  UINT64    ControlFrameAddress;

  /// The physical base address of the Arm Watchdog refresh frame
  UINT64    RefreshFrameAddress;

  /// The watchdog interrupt
  UINT32    TimerGSIV;

  /** The flags for the watchdog as described by the Arm watchdog
      structure in the ACPI specification.
  */
  UINT32    Flags;
} CM_ARM_GENERIC_WATCHDOG_INFO;

/** A structure that describes the
    ITS Group node for the Platform.

    ID: EArmObjItsGroup
*/
typedef struct CmArmItsGroupNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// The number of ITS identifiers in the ITS node
  UINT32             ItsIdCount;
  /// Reference token for the ITS identifier array
  CM_OBJECT_TOKEN    ItsIdToken;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_ITS_GROUP_NODE;

/** A structure that describes the
    Named component node for the Platform.

    ID: EArmObjNamedComponent
*/
typedef struct CmArmNamedComponentNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Number of ID mappings
  UINT32             IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;

  /// Flags for the named component
  UINT32             Flags;

  /// Memory access properties : Cache coherent attributes
  UINT32             CacheCoherent;
  /// Memory access properties : Allocation hints
  UINT8              AllocationHints;
  /// Memory access properties : Memory access flags
  UINT8              MemoryAccessFlags;

  /// Memory access properties : Address size limit
  UINT8              AddressSizeLimit;

  /** ASCII Null terminated string with the full path to
      the entry in the namespace for this object.
  */
  CHAR8              *ObjectName;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_NAMED_COMPONENT_NODE;

/** A structure that describes the
    Root complex node for the Platform.

    ID: EArmObjRootComplex
*/
typedef struct CmArmRootComplexNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Number of ID mappings
  UINT32             IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;

  /// Memory access properties : Cache coherent attributes
  UINT32             CacheCoherent;
  /// Memory access properties : Allocation hints
  UINT8              AllocationHints;
  /// Memory access properties : Memory access flags
  UINT8              MemoryAccessFlags;

  /// ATS attributes
  UINT32             AtsAttribute;
  /// PCI segment number
  UINT32             PciSegmentNumber;
  /// Memory address size limit
  UINT8              MemoryAddressSize;
  /// PASID capabilities
  UINT16             PasidCapabilities;
  /// Flags
  UINT32             Flags;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_ROOT_COMPLEX_NODE;

/** A structure that describes the
    SMMUv1 or SMMUv2 node for the Platform.

    ID: EArmObjSmmuV1SmmuV2
*/
typedef struct CmArmSmmuV1SmmuV2Node {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Number of ID mappings
  UINT32             IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;

  /// SMMU Base Address
  UINT64             BaseAddress;
  /// Length of the memory range covered by the SMMU
  UINT64             Span;
  /// SMMU Model
  UINT32             Model;
  /// SMMU flags
  UINT32             Flags;

  /// Number of context interrupts
  UINT32             ContextInterruptCount;
  /// Reference token for the context interrupt array
  CM_OBJECT_TOKEN    ContextInterruptToken;

  /// Number of PMU interrupts
  UINT32             PmuInterruptCount;
  /// Reference token for the PMU interrupt array
  CM_OBJECT_TOKEN    PmuInterruptToken;

  /// GSIV of the SMMU_NSgIrpt interrupt
  UINT32             SMMU_NSgIrpt;
  /// SMMU_NSgIrpt interrupt flags
  UINT32             SMMU_NSgIrptFlags;
  /// GSIV of the SMMU_NSgCfgIrpt interrupt
  UINT32             SMMU_NSgCfgIrpt;
  /// SMMU_NSgCfgIrpt interrupt flags
  UINT32             SMMU_NSgCfgIrptFlags;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_SMMUV1_SMMUV2_NODE;

/** A structure that describes the
    SMMUv3 node for the Platform.

    ID: EArmObjSmmuV3
*/
typedef struct CmArmSmmuV3Node {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Number of ID mappings
  UINT32             IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;

  /// SMMU Base Address
  UINT64             BaseAddress;
  /// SMMU flags
  UINT32             Flags;
  /// VATOS address
  UINT64             VatosAddress;
  /// Model
  UINT32             Model;
  /// GSIV of the Event interrupt if SPI based
  UINT32             EventInterrupt;
  /// PRI Interrupt if SPI based
  UINT32             PriInterrupt;
  /// GERR interrupt if GSIV based
  UINT32             GerrInterrupt;
  /// Sync interrupt if GSIV based
  UINT32             SyncInterrupt;

  /// Proximity domain flag
  UINT32             ProximityDomain;
  /// Index into the array of ID mapping
  UINT32             DeviceIdMappingIndex;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_SMMUV3_NODE;

/** A structure that describes the
    PMCG node for the Platform.

    ID: EArmObjPmcg
*/
typedef struct CmArmPmcgNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Number of ID mappings
  UINT32             IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;

  /// Base Address for performance monitor counter group
  UINT64             BaseAddress;
  /// GSIV for the Overflow interrupt
  UINT32             OverflowInterrupt;
  /// Page 1 Base address
  UINT64             Page1BaseAddress;

  /// Reference token for the IORT node associated with this node
  CM_OBJECT_TOKEN    ReferenceToken;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_PMCG_NODE;

/** A structure that describes the
    GIC ITS Identifiers for an ITS Group node.

    ID: EArmObjGicItsIdentifierArray
*/
typedef struct CmArmGicItsIdentifier {
  /// The ITS Identifier
  UINT32    ItsId;
} CM_ARM_ITS_IDENTIFIER;

/** A structure that describes the
    ID Mappings for the Platform.

    ID: EArmObjIdMappingArray
*/
typedef struct CmArmIdMapping {
  /// Input base
  UINT32             InputBase;
  /// Number of input IDs
  UINT32             NumIds;
  /// Output Base
  UINT32             OutputBase;
  /// Reference token for the output node
  CM_OBJECT_TOKEN    OutputReferenceToken;
  /// Flags
  UINT32             Flags;
} CM_ARM_ID_MAPPING;

/** A structure that describes the SMMU interrupts for the Platform.

    Interrupt   Interrupt number.
    Flags       Interrupt flags as defined for SMMU node.

    ID: EArmObjSmmuInterruptArray
*/
typedef CM_ARCH_COMMON_GENERIC_INTERRUPT CM_ARM_SMMU_INTERRUPT;

/** A structure that describes the AML Extended Interrupts.

    Interrupt   Interrupt number.
    Flags       Interrupt flags as defined by the Interrupt
                Vector Flags (Byte 3) of the Extended Interrupt
                resource descriptor.
                See EFI_ACPI_EXTENDED_INTERRUPT_FLAG_xxx in Acpi10.h
*/
typedef CM_ARCH_COMMON_GENERIC_INTERRUPT CM_ARM_EXTENDED_INTERRUPT;

/** A structure that describes the CMN-600 hardware.

    ID: EArmObjCmn600Info
*/
typedef struct CmArmCmn600Info {
  /// The PERIPHBASE address.
  /// Corresponds to the Configuration Node Region (CFGR) base address.
  UINT64    PeriphBaseAddress;

  /// The PERIPHBASE address length.
  /// Corresponds to the CFGR base address length.
  UINT64    PeriphBaseAddressLength;

  /// The ROOTNODEBASE address.
  /// Corresponds to the Root node (ROOT) base address.
  UINT64    RootNodeBaseAddress;

  /// The Debug and Trace Logic Controller (DTC) count.
  /// CMN-600 can have maximum 4 DTCs.
  UINT8     DtcCount;

  /// DTC Interrupt list.
  /// The first interrupt resource descriptor pertains to
  /// DTC[0], the second to DTC[1] and so on.
  /// DtcCount determines the number of DTC Interrupts that
  /// are populated. If DTC count is 2 then DtcInterrupt[2]
  /// and DtcInterrupt[3] are ignored.
  /// Note: The size of CM_ARM_CMN_600_INFO structure remains
  /// constant and does not vary with the DTC count.
  CM_ARM_EXTENDED_INTERRUPT    DtcInterrupt[4];
} CM_ARM_CMN_600_INFO;

/** A structure that describes the
    RMR node for the Platform.

    ID: EArmObjRmr
*/
typedef struct CmArmRmrNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Number of ID mappings
  UINT32             IdMappingCount;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;

  /// Unique identifier for this node.
  UINT32             Identifier;

  /// Reserved Memory Range flags.
  UINT32             Flags;

  /// Memory range descriptor count.
  UINT32             MemRangeDescCount;
  /// Reference token for the Memory Range descriptor array
  CM_OBJECT_TOKEN    MemRangeDescToken;
} CM_ARM_RMR_NODE;

/** A structure that describes the
    Memory Range descriptor.

    ID: EArmObjMemoryRangeDescriptor
*/
typedef struct CmArmRmrDescriptor {
  /// Base address of Reserved Memory Range,
  /// aligned to a page size of 64K.
  UINT64    BaseAddress;

  /// Length of the Reserved Memory range.
  /// Must be a multiple of the page size of 64K.
  UINT64    Length;
} CM_ARM_MEMORY_RANGE_DESCRIPTOR;

/** An enum describing the Arm Embedded Trace device type.
*/
typedef enum ArmEtType {
  ArmEtTypeEtm,   ///< Embedded Trace module.
  ArmEtTypeEte,   ///< Embedded Trace Extension.
  ArmEtTypeMax
} ARM_ET_TYPE;

/** A structure that describes the Embedded Trace Extension/Module.

    ID: EArmObjEtInfo
*/
typedef struct CmArmEtInfo {
  ARM_ET_TYPE    EtType;
} CM_ARM_ET_INFO;

#pragma pack()

#endif // ARM_NAMESPACE_OBJECTS_H_
