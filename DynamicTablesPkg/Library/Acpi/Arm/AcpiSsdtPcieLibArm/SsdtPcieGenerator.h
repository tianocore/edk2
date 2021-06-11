/** @file
  SSDT Pcie Table Generator.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.0
  - ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"
  - linux kernel code
**/

#ifndef SSDT_PCIE_GENERATOR_H_
#define SSDT_PCIE_GENERATOR_H_

/** Pci address attributes.
*/
#define PCI_SS_CONFIG   0
#define PCI_SS_IO       1
#define PCI_SS_M32      2
#define PCI_SS_M64      3

/** Maximum Pci root complexes supported by this generator.

  Note: This is not a hard limitation and can be extended if needed.
        Corresponding changes would be needed to support the Name and
        UID fields describing the Pci root complexes.
*/
#define MAX_PCI_ROOT_COMPLEXES_SUPPORTED    16

/** Maximum number of Pci legacy interrupts.

  Currently 4 for INTA-INTB-INTC-INTD.
*/
#define MAX_PCI_LEGACY_INTERRUPT            4

// _SB scope of the AML namespace.
#define SB_SCOPE                            "\\_SB_"

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  ssdtpcieosctemplate_aml_code[];

#pragma pack(1)

/** Structure used to map integer to an index.
*/
typedef struct MappingTable {
  /// Mapping table.
  /// Contains the Index <-> integer mapping
  UINT32             * Table;

  /// Last used index of the Table.
  /// Bound by MaxIndex.
  UINT32               LastIndex;

  /// Number of entries in the Table.
  UINT32               MaxIndex;
} MAPPING_TABLE;

/** A structure holding the Pcie generator and additional private data.
*/
typedef struct AcpiPcieGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // Private fields are defined from here.

  /** A structure used to handle the Address and Interrupt Map referencing.

    A CM_ARM_PCI_CONFIG_SPACE_INFO structure references two CM_ARM_OBJ_REF:
     - one for the address mapping, referencing
       CM_ARM_PCI_ADDRESS_MAP_INFO structures.
     - one for the address mapping, referencing
       CM_ARM_PCI_INTERRUPT_MAP_INFO structures.

    Example (for the interrupt mapping):
    (Pci0)
    CM_ARM_PCI_CONFIG_SPACE_INFO
                |
                v
    (List of references to address mappings)
    CM_ARM_OBJ_REF
                |
                +----------------------------------------
                |                                       |
                v                                       v
    (A first interrupt mapping)               (A second interrupt mapping)
    CM_ARM_PCI_INTERRUPT_MAP_INFO[0]          CM_ARM_PCI_INTERRUPT_MAP_INFO[1]

    The CM_ARM_PCI_INTERRUPT_MAP_INFO objects cannot be handled individually.
    Device's Pci legacy interrupts that are mapped to the same CPU interrupt
    are grouped under a Link device.
    For instance, the following mapping:
     - [INTA of device 0] mapped on [GIC irq 168]
     - [INTB of device 1] mapped on [GIC irq 168]
    will be represented in an SSDT table as:
     - [INTA of device 0] mapped on [Link device A]
     - [INTB of device 1] mapped on [Link device A]
     - [Link device A] mapped on [GIC irq 168]

    Counting the number of Cpu interrupts used and grouping them in Link
    devices is done through this IRQ_TABLE.

    ASL code:
    Scope (_SB) {
      Device (LNKA) {
        [...]
        Name (_PRS, ResourceTemplate () {
          Interrupt (ResourceProducer, Level, ActiveHigh, Exclusive) { 168 }
        })
      }

      Device (PCI0) {
        Name (_PRT, Package () {
          Package (0x0FFFF, 0, LNKA, 0)  // INTA of device 0 <-> LNKA
          Package (0x1FFFF, 1, LNKA, 0)  // INTB of device 1 <-> LNKA
          })
        }
    }
  */
  MAPPING_TABLE           IrqTable;

  /// Table to map: Index <-> Pci device
  MAPPING_TABLE           DeviceTable;
} ACPI_PCI_GENERATOR;

#pragma pack()

#endif // SSDT_PCIE_GENERATOR_H_
