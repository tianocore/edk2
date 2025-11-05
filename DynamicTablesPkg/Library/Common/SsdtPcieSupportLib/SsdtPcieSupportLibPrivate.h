/** @file
  SSDT PCIe Support Library private data.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.0
  - ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"
  - linux kernel code
  - Arm Base Boot Requirements v1.0
**/

#ifndef SSDT_PCIE_SUPPORT_LIB_PRIVATE_H_
#define SSDT_PCIE_SUPPORT_LIB_PRIVATE_H_

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  ssdtpcieosctemplate_aml_code[];

#endif // SSDT_PCIE_SUPPORT_LIB_PRIVATE_H_
