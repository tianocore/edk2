/** @file
  SSDT Cpu Topology Table Generator.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019 - s8.4 Declaring Processors
**/

#ifndef SSDT_CPU_TOPOLOGY_GENERATOR_H_
#define SSDT_CPU_TOPOLOGY_GENERATOR_H_

#pragma pack(1)

// Mask for the flags that need to be checked.
#define PPTT_PROCESSOR_MASK   (                                               \
          (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL)          |                     \
          (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1)   |                     \
          (EFI_ACPI_6_3_PPTT_NODE_IS_LEAF << 3))

// Mask for the cpu flags.
#define PPTT_CPU_PROCESSOR_MASK   (                                           \
          (EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL)      |                     \
          (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1)   |                     \
          (EFI_ACPI_6_3_PPTT_NODE_IS_LEAF << 3))

// Mask for the cluster flags.
// Even though a _UID is generated for clusters, it is simpler to use
// EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID and to not match the cluster id of
// the PPTT table (not sure the PPTT table is generated).
#define PPTT_CLUSTER_PROCESSOR_MASK   (                                       \
          (EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL)      |                     \
          (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID << 1) |                     \
          (EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF << 3))

/** LPI states are stored in the ASL namespace at '\_SB_.Lxxx',
    with xxx being the node index of the LPI state.
*/
#define SB_SCOPE                            "\\_SB_"
#define SB_SCOPE_PREFIX                     SB_SCOPE "."
/// Size of the SB_SCOPE_PREFIX string.
#define SB_SCOPE_PREFIX_SIZE                sizeof (SB_SCOPE_PREFIX)

/// HID for a processor device.
#define ACPI_HID_PROCESSOR_DEVICE           "ACPI0007"

/// HID for a processor container device.
#define ACPI_HID_PROCESSOR_CONTAINER_DEVICE "ACPI0010"

/** Node names of Cpus and Clusters are 'Cxxx', and 'Lxxx' for LPI states.
    The 'xxx' is an index on 12 bits is given to node name,
    thus the limitation in the number of nodes.
*/
#define MAX_NODE_COUNT                      (1 << 12)

/** A structure used to handle the Lpi structures referencing.

  A CM_ARM_PROC_HIERARCHY_INFO structure references a CM_ARM_OBJ_REF.
  This CM_ARM_OBJ_REF references CM_ARM_LPI_INFO structures.

  Example:
  (Cpu0)                                   (Cpu1)
  CM_ARM_PROC_HIERARCHY_INFO               CM_ARM_PROC_HIERARCHY_INFO
              |                                       |
              +----------------------------------------
              |
              v
  (List of references to Lpi states)
  CM_ARM_OBJ_REF
              |
              +----------------------------------------
              |                                       |
              v                                       v
  (A first Lpi state)                       (A second Lpi state)
  CM_ARM_LPI_INFO[0]                        CM_ARM_LPI_INFO[1]

  Here, Cpu0 and Cpu1 have the same Lpi states. Both CM_ARM_PROC_HIERARCHY_INFO
  structures reference the same CM_ARM_OBJ_REF. An entry is created in the
  TokenTable such as:
  0 <-> CM_ARM_OBJ_REF

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
  /// Index <-> CM_OBJECT_TOKEN (to CM_ARM_LPI_INFO structures).
  CM_OBJECT_TOKEN             * Table;

  /// Last used index of the TokenTable.
  /// LastIndex is bound by ProcNodeCount.
  UINT32                        LastIndex;
} TOKEN_TABLE;

/** A structure holding the Cpu topology generator and additional private data.
*/
typedef struct AcpiCpuTopologyGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR          Header;

  // Private fields are defined from here.

  /// Private object used to handle token referencing.
  TOKEN_TABLE                   TokenTable;
  /// List of CM_ARM_PROC_HIERARCHY_INFO CM objects.
  CM_ARM_PROC_HIERARCHY_INFO  * ProcNodeList;
  /// Count of CM_ARM_PROC_HIERARCHY_INFO CM objects.
  UINT32                        ProcNodeCount;
} ACPI_CPU_TOPOLOGY_GENERATOR;

#pragma pack()

#endif // SSDT_CPU_TOPOLOGY_GENERATOR_H_
