/** @file
  SSDT Cpu Topology Table Generator.

  Copyright (c) 2021 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019 - s8.4 Declaring Processors
    - ACPI for CoreSight version 1.2 Platform Design Document
      (https://developer.arm.com/documentation/den0067/a/?lang=en)

  @par Glossary:
    - ETE - Embedded Trace Extension.
    - ETM - Embedded Trace Macrocell.
**/

#ifndef SSDT_CPU_TOPOLOGY_GENERATOR_H_
#define SSDT_CPU_TOPOLOGY_GENERATOR_H_

#pragma pack(1)

// Mask for the flags that need to be checked.
#define PPTT_PROCESSOR_MASK  (                                                \
          (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL)          |                     \
          (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1)   |                     \
          (EFI_ACPI_6_3_PPTT_NODE_IS_LEAF << 3))

// Mask for the cpu flags.
#define PPTT_CPU_PROCESSOR_MASK  (                                            \
          (EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL)      |                     \
          (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1)   |                     \
          (EFI_ACPI_6_3_PPTT_NODE_IS_LEAF << 3))

// Mask for the cluster flags.
// Even though a _UID is generated for clusters, it is simpler to use
// EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID and to not match the cluster id of
// the PPTT table (not sure the PPTT table is generated).
#define PPTT_CLUSTER_PROCESSOR_MASK  (                                        \
          (EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL)      |                     \
          (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID << 1) |                     \
          (EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF << 3))

// Leaf nodes specific mask.
#define PPTT_LEAF_MASK  ((EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1) |        \
                         (EFI_ACPI_6_3_PPTT_NODE_IS_LEAF << 3))

/** LPI states are stored in the ASL namespace at '\_SB_.Lxxx',
    with xxx being the node index of the LPI state.
*/
#define SB_SCOPE         "\\_SB_"
#define SB_SCOPE_PREFIX  SB_SCOPE "."
/// Size of the SB_SCOPE_PREFIX string.
#define SB_SCOPE_PREFIX_SIZE  sizeof (SB_SCOPE_PREFIX)

/// HID for a processor device.
#define ACPI_HID_PROCESSOR_DEVICE  "ACPI0007"

/// HID for a ETM/ETE device.
#define ACPI_HID_ET_DEVICE  "ARMHC500"

/// HID for a processor container device.
#define ACPI_HID_PROCESSOR_CONTAINER_DEVICE  "ACPI0010"

/** Node names of Cpus and Clusters are 'Cxxx', and 'Lxxx' for LPI states.
    The 'xxx' is an index on 12 bits is given to node name,
    thus the limitation in the number of nodes.
*/
#define MAX_NODE_COUNT  (1 << 12)

/** A structure used to handle the Lpi structures referencing.

  A CM_ARCH_COMMON_PROC_HIERARCHY_INFO structure references a CM_ARCH_COMMON_OBJ_REF.
  This CM_ARCH_COMMON_OBJ_REF references CM_ARCH_COMMON_LPI_INFO structures.

  Example:
  (Cpu0)                                   (Cpu1)
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO       CM_ARCH_COMMON_PROC_HIERARCHY_INFO
              |                                       |
              +----------------------------------------
              |
              v
  (List of references to Lpi states)
  CM_ARCH_COMMON_OBJ_REF
              |
              +----------------------------------------
              |                                       |
              v                                       v
  (A first Lpi state)                       (A second Lpi state)
  CM_ARCH_COMMON_LPI_INFO[0]                        CM_ARCH_COMMON_LPI_INFO[1]

  Here, Cpu0 and Cpu1 have the same Lpi states. Both CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  structures reference the same CM_ARCH_COMMON_OBJ_REF. An entry is created in the
  TokenTable such as:
  0 <-> CM_ARCH_COMMON_OBJ_REF

  This will lead to the creation of this pseudo-ASL code where Cpu0 and Cpu1
  return the same object at \_SB.L000:
  Scope (\_SB) {
    Device (C000) {
      [...]
      Method (_LPI) {
        Return (\_SB.L000)
      }
    } // C000

    Device (C001) {
      [...]
      Method (_LPI) {
        Return (\_SB.L000)
      }
    } // C001

    // Lpi states
    Name (L000, Package (0x05) {
      [...]
    }
  }
*/
typedef struct TokenTable {
  /// TokenTable, a table allowing to map:
  /// Index <-> CM_OBJECT_TOKEN (to CM_ARCH_COMMON_LPI_INFO structures).
  CM_OBJECT_TOKEN    *Table;

  /// Last used index of the TokenTable.
  /// LastIndex is bound by ProcNodeCount.
  UINT32             LastIndex;
} TOKEN_TABLE;

/** A structure holding the Cpu topology generator and additional private data.
*/
typedef struct AcpiCpuTopologyGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR                  Header;

  // Private fields are defined from here.

  /// Private object used to handle token referencing.
  TOKEN_TABLE                           TokenTable;
  /// List of CM_ARCH_COMMON_PROC_HIERARCHY_INFO CM objects.
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO    *ProcNodeList;
  /// Count of CM_ARCH_COMMON_PROC_HIERARCHY_INFO CM objects.
  UINT32                                ProcNodeCount;
} ACPI_CPU_TOPOLOGY_GENERATOR;

#pragma pack()

/** Write a string 'Xxxx\0' in AslName (5 bytes long),
  with 'X' being the leading char of the name, and
  with 'xxx' being Value in hexadecimal.

  As 'xxx' in hexadecimal represents a number on 12 bits,
  we have Value < (1 << 12).

  @param [in]       LeadChar  Leading char of the name.
  @param [in]       Value     Hex value of the name.
                              Must be lower than (2 << 12).
  @param [in, out]  AslName   Pointer to write the 'Xxxx' string to.
                              Must be at least 5 bytes long.

  @retval EFI_SUCCESS               Success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
**/
EFI_STATUS
EFIAPI
WriteAslName (
  IN      CHAR8   LeadChar,
  IN      UINT32  Value,
  IN OUT  CHAR8   *AslName
  );

/** Get generic interrupt information from arch specific CM objects.

  The AcpiProcessorUid, CpcToken, etc. are held in arch specific CM objects,
  in the CM_ARM_GICC_INFO CM object for Arm for instance.
  This wrapper allows to get this information from each arch object.

  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in]  AcpiProcessorUid       ACPI processor Uid of the local intc (gicc, other)
                                  other fields from.
  @param [out] AcpiProcessorUid   AcpiProcessorUid of the CPU identified by
                                  the AcpiIdObjectToken.
  @param [out] CpcToken           CpcToken of the CPU identified by
                                  the AcpiIdObjectToken.
  @param [out] PsdToken           PsdToken of the CPU identified by
                                  the AcpiIdObjectToken.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
**/
EFI_STATUS
EFIAPI
GetIntCInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
  OUT UINT32                                              *AcpiProcessorUid,
  OUT CM_OBJECT_TOKEN                                     *CpcToken,
  OUT CM_OBJECT_TOKEN                                     *PsdToken
  );

/** Create and add an _CPC Node to Cpu Node.

  For instance, transform an AML node from:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0007")
  }

  To:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0007")
      Name(_CPC, Package()
      {
        NumEntries,                              // Integer
        Revision,                                // Integer
        HighestPerformance,                      // Integer or Buffer (Resource Descriptor)
        NominalPerformance,                      // Integer or Buffer (Resource Descriptor)
        LowestNonlinearPerformance,              // Integer or Buffer (Resource Descriptor)
        LowestPerformance,                       // Integer or Buffer (Resource Descriptor)
        GuaranteedPerformanceRegister,           // Buffer (Resource Descriptor)
        DesiredPerformanceRegister ,             // Buffer (Resource Descriptor)
        MinimumPerformanceRegister ,             // Buffer (Resource Descriptor)
        MaximumPerformanceRegister ,             // Buffer (Resource Descriptor)
        PerformanceReductionToleranceRegister,   // Buffer (Resource Descriptor)
        TimeWindowRegister,                      // Buffer (Resource Descriptor)
        CounterWraparoundTime,                   // Integer or Buffer (Resource Descriptor)
        ReferencePerformanceCounterRegister,     // Buffer (Resource Descriptor)
        DeliveredPerformanceCounterRegister,     // Buffer (Resource Descriptor)
        PerformanceLimitedRegister,              // Buffer (Resource Descriptor)
        CPPCEnableRegister                       // Buffer (Resource Descriptor)
        AutonomousSelectionEnable,               // Integer or Buffer (Resource Descriptor)
        AutonomousActivityWindowRegister,        // Buffer (Resource Descriptor)
        EnergyPerformancePreferenceRegister,     // Buffer (Resource Descriptor)
        ReferencePerformance                     // Integer or Buffer (Resource Descriptor)
        LowestFrequency,                         // Integer or Buffer (Resource Descriptor)
        NominalFrequency                         // Integer or Buffer (Resource Descriptor)
      })
  }

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  CpcToken               CPC token of the INTC info
                                      describing the Cpu.
  @param [in]  Node                   CPU Node to which the _CPC node is
                                      attached.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
CreateAmlCpcNode (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     CpcToken,
  IN  AML_OBJECT_NODE_HANDLE                              *Node
  );

/** Create a Cpu in the AML namespace.

  This generates the following ASL code:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0007")
  }

  @param [in]  Generator         The SSDT Cpu Topology generator.
  @param [in]  ParentNode        Parent node to attach the Cpu node to.
  @param [in]  AcpiProcessorUid  ACPI processor UID of the CPU.
  @param [in]  CpuName           Value used to generate the node name.
  @param [out] CpuNodePtr        If not NULL, return the created Cpu node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
CreateAmlCpu (
  IN   ACPI_CPU_TOPOLOGY_GENERATOR  *Generator,
  IN   AML_NODE_HANDLE              ParentNode,
  IN   UINT32                       AcpiProcessorUid,
  IN   UINT32                       CpuName,
  OUT  AML_OBJECT_NODE_HANDLE       *CpuNodePtr OPTIONAL
  );

/** Create the processor hierarchy AML tree from arch specific CM objects.

  The Arm architecture will use the CM_ARM_GICC_INFO CM objects for instance.
  A processor container is by extension any non-leave device in the cpu topology.

  @param [in] Generator        The SSDT Cpu Topology generator.
  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
                               Protocol Interface.
  @param [in] ScopeNode        Scope node handle ('\_SB' scope).

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
CreateTopologyFromIntC (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  );

/** Add arch specific information to a CPU node in the asl description.

  @param [in]  Generator          The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in]  AcpiIdObjectToken  AcpiIdObjectToken identifying the CPU to fetch the
                                  other fields from.
  @param [in]  CpuName            Value used to generate the CPU node name.
  @param [out] CpuNode            CPU Node to which the ET device node is
                                  attached.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Feature Unsupported.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AddArchAmlCpuInfo (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
  IN  UINT32                                              CpuName,
  OUT  AML_OBJECT_NODE_HANDLE                             *CpuNode
  );

#endif // SSDT_CPU_TOPOLOGY_GENERATOR_H_
