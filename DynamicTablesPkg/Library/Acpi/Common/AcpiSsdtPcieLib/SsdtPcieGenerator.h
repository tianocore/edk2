/** @file
  SSDT Pcie Table Generator.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.0
  - ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"
  - linux kernel code
  - Arm Base Boot Requirements v1.0
**/

#ifndef SSDT_PCIE_GENERATOR_H_
#define SSDT_PCIE_GENERATOR_H_

/** Pci address attributes.

  This can also be denoted as space code, address space or ss.
*/
#define PCI_SS_CONFIG   0
#define PCI_SS_IO       1
#define PCI_SS_M32      2
#define PCI_SS_M64      3
#define PCI_SS_IO_WORD  4
#define PCI_SS_M32_UC   5
#define PCI_SS_M64_UC   6

/** Maximum Pci root complexes supported by this generator.

  Note: This is not a hard limitation and can be extended if needed.
        Corresponding changes would be needed to support the Name and
        UID fields describing the Pci root complexes.
*/
#define MAX_PCI_ROOT_COMPLEXES_SUPPORTED  256

// _SB scope of the AML namespace.
#define SB_SCOPE  "\\_SB_"

#pragma pack(1)

/** A structure holding the Pcie generator and additional private data.
*/
typedef struct AcpiPcieGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // Private fields are defined from here.

  /// Table to map: Index <-> Pci device
  MAPPING_TABLE           DeviceTable;
} ACPI_PCI_GENERATOR;

#pragma pack()

#endif // SSDT_PCIE_GENERATOR_H_
