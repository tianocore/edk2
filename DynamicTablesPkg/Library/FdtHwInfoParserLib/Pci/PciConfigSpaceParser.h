/** @file
  PCI Configuration Space Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
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

/** PCI interrupt flags for device-tree.

  Local Bus Specification Revision 3.0, s2.2.6., Interrupt Pins:
   - 'Interrupts on PCI are optional and defined as "level sensitive,"
      asserted low (negative true)'
*/
#define DT_PCI_IRQ_FLAGS(x)  (((x) & 0xF) == BIT0)

/** Indexes in the mapping table.
*/
typedef enum PciMappingTable {
  PciMappingTableAddress,           ///<  0 - Address mapping
  PciMappingTableInterrupt,         ///<  1 - Interrupt mapping
  PciMappingTableMax,               ///<  2 - Max
} PCI_MAPPING_TABLE;

#pragma pack(1)

/** PCI parser table

  Multiple address-map and interrupt map can correspond to
  one host-pci device. This structure allows to temporarily
  store the CmObjects created and generate tokens once
  the whole device tree is parsed.
*/
typedef struct PciParserTable {
  /// PCI Configuration Space Info
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO    PciConfigSpaceInfo;

  /// Store the address mapping and interrupt mapping as CmObjDesc
  /// before adding them to the Configuration Manager.
  CM_OBJ_DESCRIPTOR                       Mapping[PciMappingTableMax];
} PCI_PARSER_TABLE;

#pragma pack()

/** CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO parser function.

  The following structure is populated:
  typedef struct CmArchCommonPciConfigSpaceInfo {
    UINT64  BaseAddress;                          // {Populated}
    UINT16  PciSegmentGroupNumber;                // {Populated}
    UINT8   StartBusNumber;                       // {Populated}
    UINT8   EndBusNumber;                         // {Populated}
  } CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO;

  typedef struct CmArchCommonPciAddressMapInfo {
    UINT8                     SpaceCode;          // {Populated}
    UINT64                    PciAddress;         // {Populated}
    UINT64                    CpuAddress;         // {Populated}
    UINT64                    AddressSize;        // {Populated}
  } CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO;

  typedef struct CmArchCommonPciInterruptMapInfo {
    UINT8                               PciBus;           // {Populated}
    UINT8                               PciDevice;        // {Populated}
    UINT8                               PciInterrupt;     // {Populated}
    CM_ARCH_COMMON_GENERIC_INTERRUPT    IntcInterrupt;    // {Populated}
  } CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO;

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

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
