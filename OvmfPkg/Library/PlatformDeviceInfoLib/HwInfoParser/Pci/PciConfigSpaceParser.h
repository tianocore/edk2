/** @file
  PCI Configuration Space Parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/pci/host-generic-pci.yaml
  - PCI Firmware Specification - Revision 3.0
  - Open Firmware Recommended Practice: Interrupt Mapping, Version 0.9
  - Devicetree Specification Release v0.3
  - linux kernel code
**/

#ifndef PCI_CONFIG_SPACE_PARSER_H_
#define PCI_CONFIG_SPACE_PARSER_H_

/** Read LEN bits at OFF offsets bits of the ADDR.

  @param [in] ADDR  Address to read the bits from.
  @param [in] OFF   Offset of the bits to read.
  @param [in] LEN   Number of bits to read.

  @return The bits read.
**/
#define READ_BITS(ADDR, OFF, LEN)  (((ADDR) >> (OFF)) & ((1<<(LEN))-1))

/* Pci address attributes.
*/
/// 0 if relocatable.
#define READ_PCI_N(ADDR)  READ_BITS((ADDR), 31, 1)
/// 1 if prefetchable.
#define READ_PCI_P(ADDR)  READ_BITS((ADDR), 30, 1)
/// 1 if aliased.
#define READ_PCI_T(ADDR)  READ_BITS((ADDR), 29, 1)

/** Space code.

  00: Configuration Space
  01: I/O Space
  10: 32-bit-address Memory Space
  11: 64-bit-address Memory Space
*/
#define READ_PCI_SS(ADDR)  READ_BITS((ADDR), 24, 2)
/// Bus number.
#define READ_PCI_BBBBBBBB(ADDR)  READ_BITS((ADDR), 16, 8)
/// Device number.
#define READ_PCI_DDDDD(ADDR)  READ_BITS((ADDR), 11, 5)

/** Number of device-tree cells used for PCI nodes properties.

  Values are well defined, except the "#interrupt-cells" which
  is assumed to be 1.
*/
#define PCI_ADDRESS_CELLS     3U
#define PCI_SIZE_CELLS        2U
#define PCI_INTERRUPTS_CELLS  1U

/** PCI information parser function.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
PciConfigInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // PCI_CONFIG_SPACE_PARSER_H_
