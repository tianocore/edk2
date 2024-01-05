/** @file

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef ARCH_NAMESPACE_OBJECTS_H_
#define ARCH_NAMESPACE_OBJECTS_H_

#include <AmlCpcInfo.h>
#include <StandardNameSpaceObjects.h>

#pragma pack(1)

/** The EARCH_OBJECT_ID enum describes the Object IDs
    in the ARCH Namespace
*/
typedef enum ArchObjectID {
  EArchObjReserved,                                             ///<  0 - Reserved
  EArchObjBootArchInfo,                                         ///<  1 - Boot Architecture Info
  EArchObjCpuInfo,                                              ///<  2 - CPU Info
  EArchObjPowerManagementProfileInfo,                           ///<  3 - Power Management Profile Info
  EArchObjGicCInfo,                                             ///<  4 - GIC CPU Interface Info
  EArchObjGicDInfo,                                             ///<  5 - GIC Distributor Info
  EArchObjGicMsiFrameInfo,                                      ///<  6 - GIC MSI Frame Info
  EArchObjGicRedistributorInfo,                                 ///<  7 - GIC Redistributor Info
  EArchObjGicItsInfo,                                           ///<  8 - GIC ITS Info
  EArchObjSerialConsolePortInfo,                                ///<  9 - Serial Console Port Info
  EArchObjSerialDebugPortInfo,                                  ///< 10 - Serial Debug Port Info
  EArchObjGenericTimerInfo,                                     ///< 11 - Generic Timer Info
  EArchObjPlatformGTBlockInfo,                                  ///< 12 - Platform GT Block Info
  EArchObjGTBlockTimerFrameInfo,                                ///< 13 - Generic Timer Block Frame Info
  EArchObjPlatformGenericWatchdogInfo,                          ///< 14 - Platform Generic Watchdog
  EArchObjPciConfigSpaceInfo,                                   ///< 15 - PCI Configuration Space Info
  EArchObjHypervisorVendorIdentity,                             ///< 16 - Hypervisor Vendor Id
  EArchObjFixedFeatureFlags,                                    ///< 17 - Fixed feature flags for FADT
  EArchObjItsGroup,                                             ///< 18 - ITS Group
  EArchObjNamedComponent,                                       ///< 19 - Named Component
  EArchObjRootComplex,                                          ///< 20 - Root Complex
  EArchObjSmmuV1SmmuV2,                                         ///< 21 - SMMUv1 or SMMUv2
  EArchObjSmmuV3,                                               ///< 22 - SMMUv3
  EArchObjPmcg,                                                 ///< 23 - PMCG
  EArchObjGicItsIdentifierArray,                                ///< 24 - GIC ITS Identifier Array
  EArchObjIdMappingArray,                                       ///< 25 - ID Mapping Array
  EArchObjSmmuInterruptArray,                                   ///< 26 - SMMU Interrupt Array
  EArchObjProcHierarchyInfo,                                    ///< 27 - Processor Hierarchy Info
  EArchObjCacheInfo,                                            ///< 28 - Cache Info
  EArchObjReserved29,                                           ///< 29 - Reserved
  EArchObjCmRef,                                                ///< 30 - CM Object Reference
  EArchObjMemoryAffinityInfo,                                   ///< 31 - Memory Affinity Info
  EArchObjDeviceHandleAcpi,                                     ///< 32 - Device Handle Acpi
  EArchObjDeviceHandlePci,                                      ///< 33 - Device Handle Pci
  EArchObjGenericInitiatorAffinityInfo,                         ///< 34 - Generic Initiator Affinity
  EArchObjSerialPortInfo,                                       ///< 35 - Generic Serial Port Info
  EArchObjCmn600Info,                                           ///< 36 - CMN-600 Info
  EArchObjLpiInfo,                                              ///< 37 - Lpi Info
  EArchObjPciAddressMapInfo,                                    ///< 38 - Pci Address Map Info
  EArchObjPciInterruptMapInfo,                                  ///< 39 - Pci Interrupt Map Info
  EArchObjRmr,                                                  ///< 40 - Reserved Memory Range Node
  EArchObjMemoryRangeDescriptor,                                ///< 41 - Memory Range Descriptor
  EArchObjCpcInfo,                                              ///< 42 - Continuous Performance Control Info
  EArchObjPccSubspaceType0Info,                                 ///< 43 - Pcc Subspace Type 0 Info
  EArchObjPccSubspaceType1Info,                                 ///< 44 - Pcc Subspace Type 2 Info
  EArchObjPccSubspaceType2Info,                                 ///< 45 - Pcc Subspace Type 2 Info
  EArchObjPccSubspaceType3Info,                                 ///< 46 - Pcc Subspace Type 3 Info
  EArchObjPccSubspaceType4Info,                                 ///< 47 - Pcc Subspace Type 4 Info
  EArchObjPccSubspaceType5Info,                                 ///< 48 - Pcc Subspace Type 5 Info
  EArchObjEtInfo,                                               ///< 49 - Embedded Trace Extension/Module Info
  EArchObjMax
} EARCH_OBJECT_ID;

/** A structure that describes the
    ARM Boot Architecture flags.

    ID: EArchObjBootArchInfo
*/
typedef struct CmArchBootArchInfo {
  /** This is the ARM_BOOT_ARCH flags field of the FADT Table
      described in the ACPI Table Specification.
  */
  UINT16    BootArchFlags;
} CM_ARCH_BOOT_ARCH_INFO;

/** A structure that describes the
    Power Management Profile Information for the Platform.

    ID: EArchObjPowerManagementProfileInfo
*/
typedef struct CmArchPowerManagementProfileInfo {
  /** This is the Preferred_PM_Profile field of the FADT Table
      described in the ACPI Specification
  */
  UINT8    PowerManagementProfile;
} CM_ARCH_POWER_MANAGEMENT_PROFILE_INFO;

/** A structure that describes the
    GIC CPU Interface for the Platform.

    ID: EArchObjGicCInfo
*/
typedef struct CmArchGicCInfo {
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
      i.e. a token referencing a CM_ARCH_CPC_INFO object.
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
      i.e. a token referencing a CM_ARCH_ET_INFO object.
  */
  CM_OBJECT_TOKEN    EtToken;
} CM_ARCH_GICC_INFO;

/** A structure that describes the
    GIC Distributor information for the Platform.

    ID: EArchObjGicDInfo
*/
typedef struct CmArchGicDInfo {
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
} CM_ARCH_GICD_INFO;

/** A structure that describes the
    GIC MSI Frame information for the Platform.

    ID: EArchObjGicMsiFrameInfo
*/
typedef struct CmArchGicMsiFrameInfo {
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
} CM_ARCH_GIC_MSI_FRAME_INFO;

/** A structure that describes the
    GIC Redistributor information for the Platform.

    ID: EArchObjGicRedistributorInfo
*/
typedef struct CmArchGicRedistInfo {
  /** The physical address of a page range
      containing all GIC Redistributors.
  */
  UINT64    DiscoveryRangeBaseAddress;

  /// Length of the GIC Redistributor Discovery page range
  UINT32    DiscoveryRangeLength;
} CM_ARCH_GIC_REDIST_INFO;

/** A structure that describes the
    GIC Interrupt Translation Service information for the Platform.

    ID: EArchObjGicItsInfo
*/
typedef struct CmArchGicItsInfo {
  /// The GIC ITS ID
  UINT32    GicItsId;

  /// The physical address for the Interrupt Translation Service
  UINT64    PhysicalBaseAddress;

  /** The proximity domain to which the logical processor belongs.
      This field is used to populate the GIC ITS affinity structure
      in the SRAT table.
  */
  UINT32    ProximityDomain;
} CM_ARCH_GIC_ITS_INFO;

/** A structure that describes the
    Serial Port information for the Platform.

    ID: EArchObjSerialConsolePortInfo or
        EArchObjSerialDebugPortInfo or
        EArchObjSerialPortInfo
*/
typedef struct CmArchSerialPortInfo {
  /// The physical base address for the serial port
  UINT64    BaseAddress;

  /// The serial port interrupt
  UINT32    Interrupt;

  /// The serial port baud rate
  UINT64    BaudRate;

  /// The serial port clock
  UINT32    Clock;

  /// Serial Port subtype
  UINT16    PortSubtype;

  /// The Base address length
  UINT64    BaseAddressLength;

  /// The access size
  UINT8     AccessSize;
} CM_ARCH_SERIAL_PORT_INFO;

/** A structure that describes the
    Generic Timer information for the Platform.

    ID: EArchObjGenericTimerInfo
*/
typedef struct CmArchGenericTimerInfo {
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
} CM_ARCH_GENERIC_TIMER_INFO;

/** A structure that describes the
    Platform Generic Block Timer information for the Platform.

    ID: EArchObjPlatformGTBlockInfo
*/
typedef struct CmArchGTBlockInfo {
  /// The physical base address for the GT Block Timer structure
  UINT64             GTBlockPhysicalAddress;

  /// The number of timer frames implemented in the GT Block
  UINT32             GTBlockTimerFrameCount;

  /// Reference token for the GT Block timer frame list
  CM_OBJECT_TOKEN    GTBlockTimerFrameToken;
} CM_ARCH_GTBLOCK_INFO;

/** A structure that describes the
    Platform Generic Block Timer Frame information for the Platform.

    ID: EArchObjGTBlockTimerFrameInfo
*/
typedef struct CmArchGTBlockTimerFrameInfo {
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
} CM_ARCH_GTBLOCK_TIMER_FRAME_INFO;

/** A structure that describes the
    Arm Generic Watchdog information for the Platform.

    ID: EArchObjPlatformGenericWatchdogInfo
*/
typedef struct CmArchGenericWatchdogInfo {
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
} CM_ARCH_GENERIC_WATCHDOG_INFO;

/** A structure that describes the
    PCI Configuration Space information for the Platform.

    ID: EArchObjPciConfigSpaceInfo
*/
typedef struct CmArchPciConfigSpaceInfo {
  /// The physical base address for the PCI segment
  UINT64             BaseAddress;

  /// The PCI segment group number
  UINT16             PciSegmentGroupNumber;

  /// The start bus number
  UINT8              StartBusNumber;

  /// The end bus number
  UINT8              EndBusNumber;

  /// Optional field: Reference Token for address mapping.
  /// Token identifying a CM_ARCH_OBJ_REF structure.
  CM_OBJECT_TOKEN    AddressMapToken;

  /// Optional field: Reference Token for interrupt mapping.
  /// Token identifying a CM_ARCH_OBJ_REF structure.
  CM_OBJECT_TOKEN    InterruptMapToken;
} CM_ARCH_PCI_CONFIG_SPACE_INFO;

/** A structure that describes the
    Hypervisor Vendor ID information for the Platform.

    ID: EArchObjHypervisorVendorIdentity
*/
typedef struct CmArchHypervisorVendorId {
  /// The hypervisor Vendor ID
  UINT64    HypervisorVendorId;
} CM_ARCH_HYPERVISOR_VENDOR_ID;

/** A structure that describes the
    Fixed feature flags for the Platform.

    ID: EArchObjFixedFeatureFlags
*/
typedef struct CmArchFixedFeatureFlags {
  /// The Fixed feature flags
  UINT32    Flags;
} CM_ARCH_FIXED_FEATURE_FLAGS;

/** A structure that describes the
    ITS Group node for the Platform.

    ID: EArchObjItsGroup
*/
typedef struct CmArchItsGroupNode {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// The number of ITS identifiers in the ITS node
  UINT32             ItsIdCount;
  /// Reference token for the ITS identifier array
  CM_OBJECT_TOKEN    ItsIdToken;

  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARCH_ITS_GROUP_NODE;

/** A structure that describes the
    Named component node for the Platform.

    ID: EArchObjNamedComponent
*/
typedef struct CmArchNamedComponentNode {
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
} CM_ARCH_NAMED_COMPONENT_NODE;

/** A structure that describes the
    Root complex node for the Platform.

    ID: EArchObjRootComplex
*/
typedef struct CmArchRootComplexNode {
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
} CM_ARCH_ROOT_COMPLEX_NODE;

/** A structure that describes the
    SMMUv1 or SMMUv2 node for the Platform.

    ID: EArchObjSmmuV1SmmuV2
*/
typedef struct CmArchSmmuV1SmmuV2Node {
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
} CM_ARCH_SMMUV1_SMMUV2_NODE;

/** A structure that describes the
    SMMUv3 node for the Platform.

    ID: EArchObjSmmuV3
*/
typedef struct CmArchSmmuV3Node {
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
} CM_ARCH_SMMUV3_NODE;

/** A structure that describes the
    PMCG node for the Platform.

    ID: EArchObjPmcg
*/
typedef struct CmArchPmcgNode {
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
} CM_ARCH_PMCG_NODE;

/** A structure that describes the
    GIC ITS Identifiers for an ITS Group node.

    ID: EArchObjGicItsIdentifierArray
*/
typedef struct CmArchGicItsIdentifier {
  /// The ITS Identifier
  UINT32    ItsId;
} CM_ARCH_ITS_IDENTIFIER;

/** A structure that describes the
    ID Mappings for the Platform.

    ID: EArchObjIdMappingArray
*/
typedef struct CmArchIdMapping {
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
} CM_ARCH_ID_MAPPING;

/** A structure that describes the Arm
    Generic Interrupts.
*/
typedef struct CmArchGenericInterrupt {
  /// Interrupt number
  UINT32    Interrupt;

  /// Flags
  /// BIT0: 0: Interrupt is Level triggered
  ///       1: Interrupt is Edge triggered
  /// BIT1: 0: Interrupt is Active high
  ///       1: Interrupt is Active low
  UINT32    Flags;
} CM_ARCH_GENERIC_INTERRUPT;

/** A structure that describes the SMMU interrupts for the Platform.

    Interrupt   Interrupt number.
    Flags       Interrupt flags as defined for SMMU node.

    ID: EArchObjSmmuInterruptArray
*/
typedef CM_ARCH_GENERIC_INTERRUPT CM_ARCH_SMMU_INTERRUPT;

/** A structure that describes the AML Extended Interrupts.

    Interrupt   Interrupt number.
    Flags       Interrupt flags as defined by the Interrupt
                Vector Flags (Byte 3) of the Extended Interrupt
                resource descriptor.
                See EFI_ACPI_EXTENDED_INTERRUPT_FLAG_xxx in Acpi10.h
*/
typedef CM_ARCH_GENERIC_INTERRUPT CM_ARCH_EXTENDED_INTERRUPT;

/** A structure that describes the Processor Hierarchy Node (Type 0) in PPTT

    ID: EArchObjProcHierarchyInfo
*/
typedef struct CmArchProcHierarchyInfo {
  /// A unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Processor structure flags (ACPI 6.3 - January 2019, PPTT, Table 5-155)
  UINT32             Flags;
  /// Token for the parent CM_ARCH_PROC_HIERARCHY_INFO object in the processor
  /// topology. A value of CM_NULL_TOKEN means this node has no parent.
  CM_OBJECT_TOKEN    ParentToken;
  /// Token of the associated CM_ARCH_GICC_INFO object which has the
  /// corresponding ACPI Processor ID. A value of CM_NULL_TOKEN means this
  /// node represents a group of associated processors and it does not have an
  /// associated GIC CPU interface.
  CM_OBJECT_TOKEN    GicCToken;
  /// Number of resources private to this Node
  UINT32             NoOfPrivateResources;
  /// Token of the array which contains references to the resources private to
  /// this CM_ARCH_PROC_HIERARCHY_INFO instance. This field is ignored if
  /// the NoOfPrivateResources is 0, in which case it is recommended to set
  /// this field to CM_NULL_TOKEN.
  CM_OBJECT_TOKEN    PrivateResourcesArrayToken;
  /// Optional field: Reference Token for the Lpi state of this processor.
  /// Token identifying a CM_ARCH_OBJ_REF structure, itself referencing
  /// CM_ARCH_LPI_INFO objects.
  CM_OBJECT_TOKEN    LpiToken;
  /// Set to TRUE if UID should override index for name and _UID
  /// for processor container nodes and name of processors.
  /// This should be consistently set for containers or processors to avoid
  /// duplicate values
  BOOLEAN            OverrideNameUidEnabled;
  /// If OverrideNameUidEnabled is TRUE then this value will be used for name of
  /// processors and processor containers.
  UINT16             OverrideName;
  /// If OverrideNameUidEnabled is TRUE then this value will be used for
  /// the UID of processor containers.
  UINT32             OverrideUid;
} CM_ARCH_PROC_HIERARCHY_INFO;

/** A structure that describes the Cache Type Structure (Type 1) in PPTT

    ID: EArchObjCacheInfo
*/
typedef struct CmArchCacheInfo {
  /// A unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// Reference token for the next level of cache that is private to the same
  /// CM_ARCH_PROC_HIERARCHY_INFO instance. A value of CM_NULL_TOKEN means this
  /// entry represents the last cache level appropriate to the processor
  /// hierarchy node structures using this entry.
  CM_OBJECT_TOKEN    NextLevelOfCacheToken;
  /// Size of the cache in bytes
  UINT32             Size;
  /// Number of sets in the cache
  UINT32             NumberOfSets;
  /// Integer number of ways. The maximum associativity supported by
  /// ACPI Cache type structure is limited to MAX_UINT8. However,
  /// the maximum number of ways supported by the architecture is
  /// PPTT_ARM_CCIDX_CACHE_ASSOCIATIVITY_MAX. Therfore this field
  /// is 32-bit wide.
  UINT32             Associativity;
  /// Cache attributes (ACPI 6.4 - January 2021, PPTT, Table 5.140)
  UINT8              Attributes;
  /// Line size in bytes
  UINT16             LineSize;
  /// Unique ID for the cache
  UINT32             CacheId;
} CM_ARCH_CACHE_INFO;

/** A structure that describes a reference to another Configuration Manager
    object.

    This is useful for creating an array of reference tokens. The framework
    can then query the configuration manager for these arrays using the
    object ID EArchObjCmRef.

    This can be used is to represent one-to-many relationships between objects.

    ID: EArchObjCmRef
*/
typedef struct CmArchObjRef {
  /// Token of the CM object being referenced
  CM_OBJECT_TOKEN    ReferenceToken;
} CM_ARCH_OBJ_REF;

/** A structure that describes the Memory Affinity Structure (Type 1) in SRAT

    ID: EArchObjMemoryAffinityInfo
*/
typedef struct CmArchMemoryAffinityInfo {
  /// The proximity domain to which the "range of memory" belongs.
  UINT32    ProximityDomain;

  /// Base Address
  UINT64    BaseAddress;

  /// Length
  UINT64    Length;

  /// Flags
  UINT32    Flags;
} CM_ARCH_MEMORY_AFFINITY_INFO;

/** A structure that describes the ACPI Device Handle (Type 0) in the
    Generic Initiator Affinity structure in SRAT

    ID: EArchObjDeviceHandleAcpi
*/
typedef struct CmArchDeviceHandleAcpi {
  /// Hardware ID
  UINT64    Hid;

  /// Unique Id
  UINT32    Uid;
} CM_ARCH_DEVICE_HANDLE_ACPI;

/** A structure that describes the PCI Device Handle (Type 1) in the
    Generic Initiator Affinity structure in SRAT

    ID: EArchObjDeviceHandlePci
*/
typedef struct CmArchDeviceHandlePci {
  /// PCI Segment Number
  UINT16    SegmentNumber;

  /// PCI Bus Number - Max 256 busses (Bits 15:8 of BDF)
  UINT8     BusNumber;

  /// PCI Device Number - Max 32 devices (Bits 7:3 of BDF)
  UINT8     DeviceNumber;

  /// PCI Function Number - Max 8 functions (Bits 2:0 of BDF)
  UINT8     FunctionNumber;
} CM_ARCH_DEVICE_HANDLE_PCI;

/** A structure that describes the Generic Initiator Affinity structure in SRAT

    ID: EArchObjGenericInitiatorAffinityInfo
*/
typedef struct CmArchGenericInitiatorAffinityInfo {
  /// The proximity domain to which the generic initiator belongs.
  UINT32             ProximityDomain;

  /// Flags
  UINT32             Flags;

  /// Device Handle Type
  UINT8              DeviceHandleType;

  /// Reference Token for the Device Handle
  CM_OBJECT_TOKEN    DeviceHandleToken;
} CM_ARCH_GENERIC_INITIATOR_AFFINITY_INFO;

/** A structure that describes the CMN-600 hardware.

    ID: EArchObjCmn600Info
*/
typedef struct CmArchCmn600Info {
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
  /// Note: The size of CM_ARCH_CMN_600_INFO structure remains
  /// constant and does not vary with the DTC count.
  CM_ARCH_EXTENDED_INTERRUPT    DtcInterrupt[4];
} CM_ARCH_CMN_600_INFO;

/** A structure that describes the Lpi information.

  The Low Power Idle states are described in DSDT/SSDT and associated
  to cpus/clusters in the cpu topology.

  ID: EArchObjLpiInfo
*/
typedef struct CmArchLpiInfo {
  /** Minimum Residency. Time in microseconds after which a
      state becomes more energy efficient than any shallower state.
  */
  UINT32                                    MinResidency;

  /** Worst case time in microseconds from a wake interrupt
      being asserted to the return to a running state
  */
  UINT32                                    WorstCaseWakeLatency;

  /** Flags.
  */
  UINT32                                    Flags;

  /** Architecture specific context loss flags.
  */
  UINT32                                    ArchFlags;

  /** Residency counter frequency in cycles-per-second (Hz).
  */
  UINT32                                    ResCntFreq;

  /** Every shallower power state in the parent is also enabled.
  */
  UINT32                                    EnableParentState;

  /** The EntryMethod _LPI field can be described as an integer
      or in a Register resource data descriptor.

  If IsInteger is TRUE, the IntegerEntryMethod field is used.
  If IsInteger is FALSE, the RegisterEntryMethod field is used.
  */
  BOOLEAN                                   IsInteger;

  /** EntryMethod described as an Integer.
  */
  UINT64                                    IntegerEntryMethod;

  /** EntryMethod described as a EFI_ACPI_GENERIC_REGISTER_DESCRIPTOR.
  */
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE    RegisterEntryMethod;

  /** Residency counter register.
  */
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE    ResidencyCounterRegister;

  /** Usage counter register.
  */
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE    UsageCounterRegister;

  /** String representing the Lpi state
  */
  CHAR8                                     StateName[16];
} CM_ARCH_LPI_INFO;

/** A structure that describes a PCI Address Map.

  The memory-ranges used by the PCI bus are described by this object.

  ID: EArchObjPciAddressMapInfo
*/
typedef struct CmArchPciAddressMapInfo {
  /** Pci address space code

  Available values are:
   - 0: Configuration Space
   - 1: I/O Space
   - 2: 32-bit-address Memory Space
   - 3: 64-bit-address Memory Space
  */
  UINT8     SpaceCode;

  /// PCI address
  UINT64    PciAddress;

  /// Cpu address
  UINT64    CpuAddress;

  /// Address size
  UINT64    AddressSize;
} CM_ARCH_PCI_ADDRESS_MAP_INFO;

/** A structure that describes a PCI Interrupt Map.

  The legacy PCI interrupts used by PCI devices are described by this object.

  Cf Devicetree Specification - Release v0.3
  s2.4.3 "Interrupt Nexus Properties"

  ID: EArchObjPciInterruptMapInfo
*/
typedef struct CmArchPciInterruptMapInfo {
  /// Pci Bus.
  /// Value on 8 bits (max 255).
  UINT8    PciBus;

  /// Pci Device.
  /// Value on 5 bits (max 31).
  UINT8    PciDevice;

  /** PCI interrupt

  ACPI bindings are used:
  Cf. ACPI 6.4, s6.2.13 _PRT (PCI Routing Table):
      "0-INTA, 1-INTB, 2-INTC, 3-INTD"

  Device-tree bindings are shifted by 1:
      "INTA=1, INTB=2, INTC=3, INTD=4"
  */
  UINT8                        PciInterrupt;

  /** Interrupt controller interrupt.

  Cf Devicetree Specification - Release v0.3
  s2.4.3 "Interrupt Nexus Properties": "parent interrupt specifier"
  */
  CM_ARCH_GENERIC_INTERRUPT    IntcInterrupt;
} CM_ARCH_PCI_INTERRUPT_MAP_INFO;

/** A structure that describes the
    RMR node for the Platform.

    ID: EArchObjRmr
*/
typedef struct CmArchRmrNode {
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
} CM_ARCH_RMR_NODE;

/** A structure that describes the
    Memory Range descriptor.

    ID: EArchObjMemoryRangeDescriptor
*/
typedef struct CmArchRmrDescriptor {
  /// Base address of Reserved Memory Range,
  /// aligned to a page size of 64K.
  UINT64    BaseAddress;

  /// Length of the Reserved Memory range.
  /// Must be a multiple of the page size of 64K.
  UINT64    Length;
} CM_ARCH_MEMORY_RANGE_DESCRIPTOR;

/** A structure that describes the Cpc information.

  Continuous Performance Control is described in DSDT/SSDT and associated
  to cpus/clusters in the cpu topology.

  Unsupported Optional registers should be encoded with NULL resource
  Register {(SystemMemory, 0, 0, 0, 0)}

  For values that support Integer or Buffer, integer will be used
  if buffer is NULL resource.
  If resource is not NULL then Integer must be 0

  Cf. ACPI 6.4, s8.4.7.1 _CPC (Continuous Performance Control)

  ID: EArchObjCpcInfo
*/
typedef AML_CPC_INFO CM_ARCH_CPC_INFO;

/** A structure that describes a
    PCC Mailbox Register.
*/
typedef struct PccMailboxRegisterInfo {
  /// GAS describing the Register.
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    Register;

  /** Mask of bits to preserve when writing.

    This mask is also used for registers. The Register is only read
    and there is no write mask required. E.g.:
    - Error Status mask (Cf. PCC Subspace types 3/4/5).
    - Command Complete Check mask (Cf. PCC Subspace types 3/4/5).
  */
  UINT64    PreserveMask;

  /// Mask of bits to set when writing.
  UINT64    WriteMask;
} PCC_MAILBOX_REGISTER_INFO;

/** A structure that describes the
    PCC Subspace CHannel Timings.
*/
typedef struct PccSubspaceChannelTimingInfo {
  /// Expected latency to process a command, in microseconds.
  UINT32    NominalLatency;

  /** Maximum number of periodic requests that the subspace channel can
      support, reported in commands per minute. 0 indicates no limitation.

    This field is ignored for the PCC Subspace type 5 (HW Registers based).
  */
  UINT32    MaxPeriodicAccessRate;

  /** Minimum amount of time that OSPM must wait after the completion
      of a command before issuing the next command, in microseconds.
  */
  UINT16    MinRequestTurnaroundTime;
} PCC_SUBSPACE_CHANNEL_TIMING_INFO;

/** A structure that describes a
    Generic PCC Subspace (Type 0).
*/
typedef struct CmArchPccSubspaceGenericInfo {
  /** Subspace Id.

  Cf. ACPI 6.4, s14.7 Referencing the PCC address space
  Cf. s14.1.2 Platform Communications Channel Subspace Structures
      The subspace ID of a PCC subspace is its index in the array of
      subspace structures, starting with subspace 0.

  At most 256 subspaces are supported.
  */
  UINT8                               SubspaceId;

  /// Table type (or subspace).
  UINT8                               Type;

  /// Base address of the shared memory range.
  /// This field is ignored for the PCC Subspace type 5 (HW Registers based).
  UINT64                              BaseAddress;

  /// Address length.
  UINT64                              AddressLength;

  /// Doorbell Register.
  PCC_MAILBOX_REGISTER_INFO           DoorbellReg;

  /// Mailbox Timings.
  PCC_SUBSPACE_CHANNEL_TIMING_INFO    ChannelTiming;
} PCC_SUBSPACE_GENERIC_INFO;

/** A structure that describes a
    PCC Subspace of type 0 (Generic).

    ID: EArchObjPccSubspaceType0Info
*/
typedef PCC_SUBSPACE_GENERIC_INFO CM_ARCH_PCC_SUBSPACE_TYPE0_INFO;

/** A structure that describes a
    PCC Subspace of type 1 (HW-Reduced).

    ID: EArchObjPccSubspaceType1Info
*/
typedef struct CmArchPccSubspaceType1Info {
  /** Generic Pcc information.

    The Subspace of Type0 contains information that can be re-used
    in other Subspace types.
  */
  PCC_SUBSPACE_GENERIC_INFO    GenericPccInfo;

  /// Platform Interrupt.
  CM_ARCH_GENERIC_INTERRUPT    PlatIrq;
} CM_ARCH_PCC_SUBSPACE_TYPE1_INFO;

/** A structure that describes a
    PCC Subspace of type 2 (HW-Reduced).

    ID: EArchObjPccSubspaceType2Info
*/
typedef struct CmArchPccSubspaceType2Info {
  /** Generic Pcc information.

    The Subspace of Type0 contains information that can be re-used
    in other Subspace types.
  */
  PCC_SUBSPACE_GENERIC_INFO    GenericPccInfo;

  /// Platform Interrupt.
  CM_ARCH_GENERIC_INTERRUPT    PlatIrq;

  /// Platform Interrupt Register.
  PCC_MAILBOX_REGISTER_INFO    PlatIrqAckReg;
} CM_ARCH_PCC_SUBSPACE_TYPE2_INFO;

/** A structure that describes a
    PCC Subspace of type 3 (Extended)

    ID: EArchObjPccSubspaceType3Info
*/
typedef struct CmArchPccSubspaceType3Info {
  /** Generic Pcc information.

    The Subspace of Type0 contains information that can be re-used
    in other Subspace types.
  */
  PCC_SUBSPACE_GENERIC_INFO    GenericPccInfo;

  /// Platform Interrupt.
  CM_ARCH_GENERIC_INTERRUPT    PlatIrq;

  /// Platform Interrupt Register.
  PCC_MAILBOX_REGISTER_INFO    PlatIrqAckReg;

  /// Command Complete Check Register.
  /// The WriteMask field is not used.
  PCC_MAILBOX_REGISTER_INFO    CmdCompleteCheckReg;

  /// Command Complete Update Register.
  PCC_MAILBOX_REGISTER_INFO    CmdCompleteUpdateReg;

  /// Error Status Register.
  /// The WriteMask field is not used.
  PCC_MAILBOX_REGISTER_INFO    ErrorStatusReg;
} CM_ARCH_PCC_SUBSPACE_TYPE3_INFO;

/** A structure that describes a
    PCC Subspace of type 4 (Extended)

    ID: EArchObjPccSubspaceType4Info
*/
typedef CM_ARCH_PCC_SUBSPACE_TYPE3_INFO CM_ARCH_PCC_SUBSPACE_TYPE4_INFO;

/** A structure that describes a
    PCC Subspace of type 5 (HW-Registers).

    ID: EArchObjPccSubspaceType5Info
*/
typedef struct CmArchPccSubspaceType5Info {
  /** Generic Pcc information.

    The Subspace of Type0 contains information that can be re-used
    in other Subspace types.

    MaximumPeriodicAccessRate doesn't need to be populated for
    this structure.
  */
  PCC_SUBSPACE_GENERIC_INFO    GenericPccInfo;

  /// Version.
  UINT16                       Version;

  /// Platform Interrupt.
  CM_ARCH_GENERIC_INTERRUPT    PlatIrq;

  /// Command Complete Check Register.
  /// The WriteMask field is not used.
  PCC_MAILBOX_REGISTER_INFO    CmdCompleteCheckReg;

  /// Error Status Register.
  /// The WriteMask field is not used.
  PCC_MAILBOX_REGISTER_INFO    ErrorStatusReg;
} CM_ARCH_PCC_SUBSPACE_TYPE5_INFO;

/** An enum describing the Arm Embedded Trace device type.
*/
typedef enum ArmEtType {
  ArmEtTypeEtm,   ///< Embedded Trace module.
  ArmEtTypeEte,   ///< Embedded Trace Extension.
  ArmEtTypeMax
} ARM_ET_TYPE;

/** A structure that describes the Embedded Trace Extension/Module.

    ID: EArchObjEtInfo
*/
typedef struct CmArchEtInfo {
  ARM_ET_TYPE    EtType;
} CM_ARCH_ET_INFO;

#pragma pack()

#endif // ARCH_NAMESPACE_OBJECTS_H_
