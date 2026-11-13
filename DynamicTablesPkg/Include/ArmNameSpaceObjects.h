/** @file

  Copyright (c) 2017 - 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - AEST       - Arm Error Source Table
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

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
  EArmObjCmnInfo,                                              ///< 20 - CMN Info
  EArmObjRmr,                                                  ///< 21 - Reserved Memory Range Node
  EArmObjMemoryRangeDescriptor,                                ///< 22 - Memory Range Descriptor
  EArmObjEtInfo,                                               ///< 23 - Embedded Trace Extension/Module Info
  EArmObjDmcPmuSocketInfo,                                     ///< 24 - DMC Socket Info
  EArmObjDmcPmuRegInfo,                                        ///< 25 - DMC PMU Reg Info
  EArmObjProcessorSpecificBlockInfo,                           ///< 26 - Processor Specific Block.
  EArmObjProcessorSpecificSubDataArchInfo,                     ///< 27 - Processor Specific Sub Data (ArchData)
  EArmObjCoresightPmuInfo,                                     ///< 28 - Coresight PMU Info
  EArmObjGicIrsInfo,                                           ///< 29 - GIC IRS Info
  EArmObjGicItsV5Info,                                         ///< 30 - GIC ITS v5 Info
  EArmObjGicItsV5TranslateFrameInfo,                           ///< 31 - GIC ITS v5 Translate Frame Info
  EArmObjGicIwbInfo,                                           ///< 32 - GIC IWB Info
  EArmObjBmuRegInfo,                                           ///< 33 - BMU Reg Info
  EArmObjPl050Info,                                            ///< 34 - PL050 Info
  EArmObjScmiDeviceInfo,                                       ///< 35 - SCMI Device Information
  EArmObjScmiPccTransportInfo,                                 ///< 36 - SCMI PCC Transport Information
  EArmObjScmiProtocolInfo,                                     ///< 37 - SCMI Protocol Information
  EArmObjAestProcessorNode,                                    ///< 38 - AEST processor node
  EArmObjAestBitmapArray,                                      ///< 39 - AEST bitmap words
  EArmObjAestInterruptArray,                                   ///< 40 - AEST interrupts
  EArmObjAestMemoryNode,                                       ///< 41 - AEST memory controller node
  EArmObjAestSmmuNode,                                         ///< 42 - AEST SMMU node
  EArmObjAestVendorNode,                                       ///< 43 - AEST vendor-defined node
  EArmObjAestGicNode,                                          ///< 44 - AEST GIC node
  EArmObjAestPcieRootComplexNode,                              ///< 45 - AEST PCIe Root Complex node
  EArmObjAestProxyNode,                                        ///< 46 - AEST proxy node
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

  /** Optional field: Reference Token to the ProximityDomain this object
      belongs to. If set to CM_NULL_TOKEN, the following field is used:
        CM_ARM_GICC_INFO.ProximityDomain
  */
  CM_OBJECT_TOKEN    ProximityDomainToken;

  /** Optional field: Reference Token to the ClockDomain this object belongs to.
      belongs to. If set to CM_NULL_TOKEN, the following field is used:
        CM_ARM_GICC_INFO.ClockDomain
  */
  CM_OBJECT_TOKEN    ClockDomainToken;

  /** GICv5 Interrupt Controller processor affinity ID.
      This must be 0 for pre-v5 GIC.
  */
  UINT16             IAffId;

  /** The ID of the IRS that this processor is connected to.
      This must be CM_NULL_TOKEN for pre-v5 GIC.
  */
  CM_OBJECT_TOKEN    IrsToken;
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
  UINT32             GicItsId;

  /// The physical address for the Interrupt Translation Service
  UINT64             PhysicalBaseAddress;

  /** The proximity domain to which the logical processor belongs.
      This field is used to populate the GIC ITS affinity structure
      in the SRAT table.
  */
  UINT32             ProximityDomain;

  /** Optional field: Reference Token to the ProximityDomain this object
      belongs to. If this field is used, the following field is ignored:
        CM_ARM_GIC_ITS_INFO.ProximityDomain
  */
  CM_OBJECT_TOKEN    ProximityDomainToken;
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

  /** Optional field: Reference Token to the ProximityDomain this object
      belongs to. If set to CM_NULL_TOKEN, the following field is used:
        CM_ARM_SMMUV3_NODE.ProximityDomain
  */
  CM_OBJECT_TOKEN    ProximityDomainToken;
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
  /// The ITS Identifier or ITS Translate Id (GicV5)
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

/** A structure that describes a PL050 keyboard or mouse interface.

    ID: EArmObjPl050Info
*/
typedef struct CmArmPl050Info {
  /// FALSE for a keyboard interface; TRUE for a mouse interface.
  BOOLEAN                      IsMouse;

  /// Controller MMIO base address.
  UINT64                       BaseAddress;

  /// AML Extended Interrupt descriptor information.
  CM_ARM_EXTENDED_INTERRUPT    Interrupt;
} CM_ARM_PL050_INFO;

/** CMN implementation types. */
typedef enum ArmCmnType {
  ArmCmnType600,
  ArmCmnType650,
  ArmCmnType700,
  ArmCmnTypeS3,
  ArmCmnTypeMax
} ARM_CMN_TYPE;

/** A structure that describes CMN hardware.

    ID: EArmObjCmnInfo
*/
typedef struct CmArmCmnInfo {
  /// The PERIPHBASE address.
  /// Corresponds to the Configuration Node Region (CFGR) base address.
  UINT64                       PeriphBaseAddress;

  /// The PERIPHBASE address length.
  /// Corresponds to the CFGR base address length.
  UINT64                       PeriphBaseAddressLength;

  /// The ROOTNODEBASE address.
  /// Corresponds to the Root node (ROOT) base address.
  /// Required only for CMN-600; must be zero for other CMN types.
  UINT64                       RootNodeBaseAddress;

  /// Number of Debug and Trace Logic Controller interrupts.
  /// A maximum of four DTC interrupts can be described.
  UINT8                        DtcCount;

  /// DTC interrupt descriptors.
  /// Entries must be ordered by increasing hardware-assigned DTC Logical ID.
  /// DtcCount determines the number of valid entries.
  CM_ARM_EXTENDED_INTERRUPT    DtcInterrupt[4];

  /// CMN implementation type.
  ARM_CMN_TYPE                 CmnType;

  /// Length of the optional root-node region. Zero means no ROOT resource.
  UINT64                       RootNodeBaseAddressLength;
} CM_ARM_CMN_INFO;

/** A structure that describes the DMC PMU hardware
    registers and interrupt.

    ID: EArmObjDmcPmuRegInfo
*/
typedef struct CmArmDmcPmuRegInfo {
  /// The Base address of PMU register space in the DMC device.
  UINT64                       BaseAddress;

  /// Length of the DMC PMU registers
  UINT64                       Length;

  /// The DMC PMU interrupt descriptor
  CM_ARM_EXTENDED_INTERRUPT    PmuIntr;
} CM_ARM_DMC_PMU_REG_INFO;

/** A structure that describes the DMC PMU hardware
    on a socket.

    ID: EArmObjDmcPmuSocketInfo
*/
typedef struct CmArmDmcPmuSocketInfo {
  /// Number of devices on this socket
  UINT8              NumDevices;

  /// Array of DMC PMU devices on this socket
  CM_OBJECT_TOKEN    DmcPmuRegInfoToken;
} CM_ARM_DMC_INFO;

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

typedef enum ArmProcessorSpecifcDataSubType {
  ArmProcessorSpecificDataSubTypeArch,
  /// Define vendor type from here.
  ArmProcessorSpecificDataSubTypeMax,
} ARM_PROCESSOR_SPECIFIC_DATA_SUB_TYPE;

/** A structure that describes the processor specific data.

    ID: EArmObjProcessorSpecificBlockInfo
*/
typedef struct CmArmProcessorSpecificBlockInfo {
  /// Revision for processor specific block
  UINT16                                  Revision;

  /// Vendor ID.
  UINT16                                  VendorId;

  /// Sub type of Processor specific block.
  ARM_PROCESSOR_SPECIFIC_DATA_SUB_TYPE    SubType;

  /// Sub data token relevant to SubType.
  CM_OBJECT_TOKEN                         SubDataToken;
} CM_ARM_PROCESSOR_SPECIFIC_BLOCK_INFO;

/** A structure that describes the processor specific sub data
    (Architecture Data).

    ID: EArmObjProcessorSpecificSubDataArchInfo
*/
typedef struct CmArmProcessorSpecificSubDataArchInfo {
  /// Version for Processor Specific Sub Data (Arch Data).
  UINT16    Version;

  /// Value of ID_AA64AFR0_EL1.
  UINT64    IdAA64Afr0;

  /// Value of ID_AA64AFR1_EL1.
  UINT64    IdAA64Afr1;

  /// Value of ID_AA64DFR0_EL1.
  UINT64    IdAA64Dfr0;

  /// Value of ID_AA64DFR1_EL1.
  UINT64    IdAA64Dfr1;

  /// Value of ID_AA64DFR2_EL1.
  UINT64    IdAA64Dfr2;

  /// Value of ID_AA64FPFR0_EL1.
  UINT64    IdAA64Fpfr0;

  /// Value of ID_AA64ISAR0_EL1.
  UINT64    IdAA64Isar0;

  /// Value of ID_AA64ISAR1_EL1.
  UINT64    IdAA64Isar1;

  /// Value of ID_AA64ISAR2_EL1.
  UINT64    IdAA64Isar2;

  /// Value of ID_AA64ISAR3_EL1.
  UINT64    IdAA64Isar3;

  /// Value of ID_AA64MMFR0_EL1.
  UINT64    IdAA64Mmfr0;

  /// Value of ID_AA64MMFR1_EL1.
  UINT64    IdAA64Mmfr1;

  /// Value of ID_AA64MMFR2_EL1.
  UINT64    IdAA64Mmfr2;

  /// Value of ID_AA64MMFR3_EL1.
  UINT64    IdAA64Mmfr3;

  /// Value of ID_AA64MMFR4_EL1.
  UINT64    IdAA64Mmfr4;

  /// Value of ID_AA64PFR0_EL1.
  UINT64    IdAA64Pfr0;

  /// Value of ID_AA64PFR1_EL1.
  UINT64    IdAA64Pfr1;

  /// Value of ID_AA64PFR2_EL1.
  UINT64    IdAA64Pfr2;

  /// Value of ID_AA64SMFR0_EL1.
  UINT64    IdAA64Smfr0;

  /// Value of ID_AA64ZFR0_EL1.
  UINT64    IdAA64Zfr0;
} CM_ARM_PROCESSOR_SPECIFIC_SUB_DATA_ARCH_INFO;

/** A structure that describes Coresight PMU information.

    ID: EArmObjCoresightPmuInfo
*/
typedef struct CmArmCoresightPmuInfo {
  /// Node Flags.
  UINT8              Flags;

  /// Node Type.
  UINT16             Type;

  /// Unique identifier for this node.
  UINT32             Identifier;

  /// Base address of Page 0 of the PMU.
  UINT64             BaseAddress0;

  ///
  /// Base address of Page 1 of the PMU if the PMU
  /// implements the dual-page extension otherwise 0.
  ///
  UINT64             BaseAddress1;

  /// Overflow Interrupt.
  UINT32             OverflowInterrupt;

  /// Overflow Interrupt Flags.
  UINT32             OverflowInterruptFlags;

  /// Proc node token for processor affinity
  CM_OBJECT_TOKEN    ProcNodeToken;

  ///
  /// Token pointing to the monitored device.
  /// The Token is evaluated in regards to the node Type and might resolve
  /// to a CM Object such as CPU cache, Smmu and etc.
  ///
  CM_OBJECT_TOKEN    TypeInstanceToken;

  ///
  /// This is valid when type is EFI_ACPI_APMT_NODE_TYPE_ACPI_DEVICE only.
  /// Specify the associated acpi device's HID with this Coresight PMU
  /// including NULL characther.
  ///
  CHAR8              AcpiDeviceHid[9];

  ///
  /// This is valid when type is EFI_ACPI_APMT_NODE_TYPE_ACPI_DEVICE only.
  /// Specify the associated acpi device's UID with this Coresight PMU.
  ///
  UINT32             AcpiDeviceUid;

  ///
  /// This field is used for specifying the identity of the
  /// implementer of this PMU.
  /// This field must be set to 0 and ignored if the
  /// PMIIDR or PMPIDR register is present in this PMU implementation.
  ///
  UINT32             ImplementationId;
} CM_ARM_CORESIGHT_PMU_INFO;

/** A structure that describes GIC interrupt Routing Service (IRS).

    ID: EArmObjGicIrsInfo
*/
typedef struct CmArmGicIrsInfo {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// GIC version.
  UINT32             GicVersion;
  /// The GIC IRS ID
  UINT32             GicIrsId;
  /// Flags
  UINT32             Flags;
  /// Base address of the IRS config frame
  UINT64             ConfigFrameBase;
  /// Base address of the IRS SET_LPI frame
  UINT64             SetLpiFrameBase;
  /// Proximity domain that this IRS belongs to
  UINT32             ProximityDomain;

  /** Optional field: Reference Token to the ProximityDomain this object
      belongs to. If this field is used, the following field is ignored:
        CM_ARM_GIC_IRS_INFO.ProximityDomain
  */
  CM_OBJECT_TOKEN    ProximityDomainToken;
} CM_ARM_GIC_IRS_INFO;

/** A structure that describes the
    GICv5 Interrupt Translation Service information for the Platform.

    ID: EArmObjGicItsV5Info
*/
typedef struct CmArmGicItsV5Info {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// The GIC ITSv5 ID
  UINT32             GicItsId;
  /// Flags
  UINT32             Flags;
  /// Base address of the ITS config frame
  UINT64             PhysicalBaseAddress;

  /** The proximity domain to which the logical processor belongs.
      This field is used to populate the GIC ITS affinity structure
      in the SRAT table.
  */
  UINT32             ProximityDomain;

  /** Optional field: Reference Token to the ProximityDomain this object
      belongs to. If this field is used, the following field is ignored:
        CM_ARM_GIC_ITSV5_INFO.ProximityDomain
  */
  CM_OBJECT_TOKEN    ProximityDomainToken;
} CM_ARM_GIC_ITSV5_INFO;

/** A structure that describes the
    frame information for GICv5 Interrupt Translation Service.

    ID: EArmObjGicItsV5TranslateFrameInfo
*/
typedef struct CmArmGicItsV5TranslateFrameInfo {
  /// Relevant ITSv5 Token
  CM_OBJECT_TOKEN    ItsV5Token;
  /// The GIC ITSv5 translate frame ID
  UINT32             ItsTranslateId;
  /// Base address of the ITS translate frame
  UINT64             ItsTranslateFrameBase;
} CM_ARM_GIC_ITSV5_TRANSLATE_FRAME_INFO;

/** A structure that describes the
    Interrupt Wire Bridge (IWB) information.

    ID: EArmObjGicIwbInfo
*/
typedef struct CmArmGicIwbInfo {
  /// An unique token used to identify this object
  CM_OBJECT_TOKEN    Token;
  /// The GIC IWB ID
  UINT32             GicIwbId;
  /// Linked ITSv5 ID Token
  CM_OBJECT_TOKEN    ItsV5Token;
  /// Base address of the IWB config frame
  UINT64             ConfigFrameBase;
  /// Device ID used to signal any interrupt to the connected ITS
  UINT32             DeviceId;
  /// Base GSIV for this IWB
  UINT32             BaseGsiv;
  /// Number of wires handled by this IWB
  UINT32             NumWires;
  /// Reference token for the ID mapping array
  CM_OBJECT_TOKEN    IdMappingToken;
  /// Unique identifier for this node.
  UINT32             Identifier;
} CM_ARM_GIC_IWB_INFO;

/** A structure that describes the BMU hardware
    registers and interrupt.

    ID: EArmObjBmuRegInfo
*/
typedef struct CmArmBmuRegInfo {
  /// The Base address of BMU register space.
  UINT64                       BaseAddress;

  /// Length of the BMU registers
  UINT64                       Length;

  /// The BMU interrupt descriptor
  CM_ARM_EXTENDED_INTERRUPT    BmuIntr;
} CM_ARM_BMU_REG_INFO;

/** SCMI protocols described by an SCMI ACPI device.
*/
typedef enum ArmScmiProtocolType {
  ArmScmiProtocolTypePcap,
  ArmScmiProtocolTypeSystemTelemetry,
  ArmScmiProtocolTypeMax
} CM_ARM_SCMI_PROTOCOL_TYPE;

/** SCMI transport directions.
*/
typedef enum ArmScmiTransportDirection {
  ArmScmiTransportDirectionA2p,
  ArmScmiTransportDirectionP2a,
  ArmScmiTransportDirectionMax
} CM_ARM_SCMI_TRANSPORT_DIRECTION;

/** A structure that describes an SCMI ACPI device.

    ID: EArmObjScmiDeviceInfo
*/
typedef struct CmArmScmiDeviceInfo {
  /// A unique token identifying this SCMI device.
  CM_OBJECT_TOKEN    Token;

  /// Tokens identifying the common transports for each direction.
  /// The agent-to-platform transport is required. The platform-to-agent
  /// transport is optional and uses CM_NULL_TOKEN when absent.
  CM_OBJECT_TOKEN    CommonTransportToken[ArmScmiTransportDirectionMax];

  /// Token referencing an array of SCMI protocol information objects.
  CM_OBJECT_TOKEN    ProtocolInfoToken;
} CM_ARM_SCMI_DEVICE_INFO;

/** A structure that describes an SCMI PCC transport.

    ID: EArmObjScmiPccTransportInfo
*/
typedef struct CmArmScmiPccTransportInfo {
  /// A unique token identifying this SCMI PCC transport.
  CM_OBJECT_TOKEN    Token;

  /// Token identifying the referenced PCC subspace object.
  CM_OBJECT_TOKEN    PccSubspaceToken;
} CM_ARM_SCMI_PCC_TRANSPORT_INFO;

/** A structure that describes an SCMI protocol.

    ID: EArmObjScmiProtocolInfo
*/
typedef struct CmArmScmiProtocolInfo {
  /// SCMI protocol type.
  CM_ARM_SCMI_PROTOCOL_TYPE    ProtocolType;

  /// Tokens identifying optional exclusive transports for each direction.
  /// An absent transport uses CM_NULL_TOKEN.
  CM_OBJECT_TOKEN              ExclusiveTransportToken[ArmScmiTransportDirectionMax];

  /// Indicates whether the protocol supports notifications.
  BOOLEAN                      NotificationsSupported;
} CM_ARM_SCMI_PROTOCOL_INFO;

/** A structure that describes one AEST error-record bitmap word.

    A token selects 1, 4, or 14 words according to GroupFormat.

    ID: EArmObjAestBitmapArray
*/
typedef struct CmArmAestBitmapWord {
  /// One 64-bit word in the selected bitmap.
  UINT64    Word;
} CM_ARM_AEST_BITMAP_WORD;

/** A structure that describes AEST FHI or ERI interrupt entry.

    A token selects an array of InterruptCount entries.

    ID: EArmObjAestInterruptArray
*/
typedef struct CmArmAestInterrupt {
  /// Fault Handling or Error Recovery interrupt type.
  UINT8     Type;
  /// AEST interrupt flags.
  UINT8     Flags;
  /// Interrupt GSIV; zero selects MSI.
  UINT32    Gsiv;
} CM_ARM_AEST_INTERRUPT;

/** A structure that describes the interface shared by AEST nodes.

    The three bitmap tokens select EArmObjAestBitmapArray lists.
    Their lengths depend on GroupFormat.
    ProcessorHierarchyToken selects EArchCommonObjProcHierarchyInfo.
*/
typedef struct CmArmAestInterface {
  /// System-register, group-MMIO, or single-record-MMIO interface type.
  UINT8              Type;
  /// Error-record group format.
  UINT8              GroupFormat;
  /// Node interface flags.
  UINT32             Flags;
  /// Base address of the error-record group or single record.
  UINT64             BaseAddress;
  /// Zero-based index of this node's first error record.
  UINT32             StartErrorRecordIndex;
  /// Number of error records belonging to this node.
  UINT32             NumberErrorRecords;
  /// Token selecting the implemented-record bitmap words.
  CM_OBJECT_TOKEN    ImplementedBitmapToken;
  /// Token selecting the group-status bitmap words.
  CM_OBJECT_TOKEN    GroupStatusBitmapToken;
  /// Token selecting the addressing-mode bitmap words.
  CM_OBJECT_TOKEN    AddressModeBitmapToken;
  /// Token selecting a processor leaf or container for affinity.
  CM_OBJECT_TOKEN    ProcessorHierarchyToken;
  /// AEST device UID.
  UINT32             ArmErrorNodeDeviceUid;
  /// Base address of the group-status register set.
  UINT64             GroupStatusRegistersBaseAddress;
  /// Base address of the fault-injection register set.
  UINT64             FaultInjectionRegistersBaseAddress;
  /// Base address of the interrupt-configuration register set.
  UINT64             InterruptConfigurationRegistersBaseAddress;
} CM_ARM_AEST_INTERFACE;

/** A structure that describes fields shared by AEST node CM objects.

    Interface contains register access and affinity data.
    InterruptArrayToken selects EArmObjAestInterruptArray when
    InterruptCount is nonzero.
*/
typedef struct CmArmAestCommon {
  /// Timestamp counter rate in hertz.
  UINT64                   TimestampRate;
  /// Signed timebase offset in nanoseconds.
  INT64                    TimebaseOffset;
  /// Error injection countdown rate in hertz.
  UINT64                   ErrorInjectionCountdownRate;
  /// Interface inputs embedded in the node.
  CM_ARM_AEST_INTERFACE    Interface;
  /// Number of entries in the interrupt array.
  UINT32                   InterruptCount;
  /// Token selecting the interrupt array.
  CM_OBJECT_TOKEN          InterruptArrayToken;
} CM_ARM_AEST_COMMON;

/** A union that describes an AEST processor error resource.
*/
typedef union CmArmAestProcessorResource {
  /// Token selecting cache information for a Cache resource.
  CM_OBJECT_TOKEN    CacheToken;
  /// TLB level for a TLB resource.
  UINT32             TlbLevel;
  /// Vendor-defined data for a Generic resource.
  UINT32             GenericData;
} CM_ARM_AEST_PROCESSOR_RESOURCE;

/** A structure that describes an AEST processor error node.

    Common is the first member. For a Cache resource, CacheToken selects
    EArchCommonObjCacheInfo.

    ID: EArmObjAestProcessorNode
*/
typedef struct CmArmAestProcessorNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON                Common;
  /// Cache, TLB, or Generic resource type.
  UINT8                             ResourceType;
  /// Global and Shared processor node flags.
  UINT8                             Flags;
  /// Resource data selected by ResourceType.
  CM_ARM_AEST_PROCESSOR_RESOURCE    Resource;
} CM_ARM_AEST_PROCESSOR_NODE;

/** A structure that describes an AEST memory controller error node.

    ProximityDomainToken selects EArchCommonObjProximityDomainInfo.
    CM_NULL_TOKEN means no SRAT proximity domain is referenced.

    ID: EArmObjAestMemoryNode
*/
typedef struct CmArmAestMemoryNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON    Common;
  /// Token selecting the SRAT proximity domain identifier owner.
  CM_OBJECT_TOKEN       ProximityDomainToken;
} CM_ARM_AEST_MEMORY_NODE;

/** A structure that describes an AEST SMMU error node.

    SmmuToken selects EArmObjSmmuV1SmmuV2 or EArmObjSmmuV3.
    SubcomponentToken selects EArmObjRootComplex, EArmObjNamedComponent,
    or the SMMU selected by SmmuToken.

    ID: EArmObjAestSmmuNode
*/
typedef struct CmArmAestSmmuNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON    Common;
  /// Owner of the IORT SMMU Identifier.
  CM_OBJECT_TOKEN       SmmuToken;
  /// Token selecting the IORT subcomponent Identifier owner.
  CM_OBJECT_TOKEN       SubcomponentToken;
} CM_ARM_AEST_SMMU_NODE;

/** A structure that describes an AEST vendor-defined error node.

    The platform CM supplies the vendor-defined payload.

    ID: EArmObjAestVendorNode
*/
typedef struct CmArmAestVendorNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON    Common;
  /// Eight-byte hardware ID as carried by AEST.
  UINT8                 HardwareId[8];
  /// Four-byte unique ID as carried by AEST.
  UINT32                UniqueId;
  /// Vendor-defined supplementary data.
  UINT8                 VendorData[16];
} CM_ARM_AEST_VENDOR_NODE;

/** A structure that describes an AEST GIC error node.

    InterfaceType selects GICC, GICD, GICR, or GITS.
    InstanceToken selects EArmObjGicCInfo for GICC/GICR and
    EArmObjGicItsInfo for GITS. GICD uses CM_NULL_TOKEN.

    Cf. ACPI Specification 6.6, Section 5.2.12.15 an Arm MADT
    must contain exactly one GICD structure. Its instance
    identifier is emitted as zero to match the Arm MADT generator's GIC ID.

    ID: EArmObjAestGicNode
*/
typedef struct CmArmAestGicNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON    Common;
  /// GICC, GICD, GICR, or GITS interface type.
  UINT32                InterfaceType;
  /// Token selecting the GIC identifier owner.
  CM_OBJECT_TOKEN       InstanceToken;
} CM_ARM_AEST_GIC_NODE;

/** A structure that describes an AEST PCIe Root Complex error node.

    RootComplexToken selects EArmObjRootComplex object.

    ID: EArmObjAestPcieRootComplexNode
*/
typedef struct CmArmAestPcieRootComplexNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON    Common;
  /// Token selecting the IORT Root Complex.
  CM_OBJECT_TOKEN       RootComplexToken;
} CM_ARM_AEST_PCIE_ROOT_COMPLEX_NODE;

/** A structure that describes an AEST proxy error node.

    ID: EArmObjAestProxyNode
*/
typedef struct CmArmAestProxyNode {
  /// Common fields shared by all AEST node types.
  CM_ARM_AEST_COMMON    Common;
  /// Address of the proxied error-record group's first record.
  UINT64                NodeAddress;
} CM_ARM_AEST_PROXY_NODE;

#pragma pack()
