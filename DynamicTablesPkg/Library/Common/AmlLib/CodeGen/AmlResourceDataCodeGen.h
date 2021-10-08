/** @file
  AML Resource Data Code Generation.

  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_RESOURCE_DATA_CODE_GEN_H_
#define AML_RESOURCE_DATA_CODE_GEN_H_

/** Code generation for the "Interrupt ()" ASL function.

  The Resource Data effectively created is an Extended Interrupt Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.6 "Extended Interrupt Descriptor"
   - s19.6.64 "Interrupt (Interrupt Resource Descriptor Macro)"

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @param  [in]  ResourceConsumer The device consumes the specified interrupt
                                 or produces it for use by a child device.
  @param  [in]  EdgeTriggered    The interrupt is edge triggered or
                                 level triggered.
  @param  [in]  ActiveLow        The interrupt is active-high or active-low.
  @param  [in]  Shared           The interrupt can be shared with other
                                 devices or not (Exclusive).
  @param  [in]  IrqList          Interrupt list. Must be non-NULL.
  @param  [in]  IrqCount         Interrupt count. Must be non-zero.
  @param  [in]  NameOpNode       NameOp object node defining a named object.
                                 If provided, append the new resource data node
                                 to the list of resource data elements of this
                                 node.
  @param  [out] NewRdNode        If provided and success,
                                 contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdInterrupt (
  IN  BOOLEAN                 ResourceConsumer,
  IN  BOOLEAN                 EdgeTriggered,
  IN  BOOLEAN                 ActiveLow,
  IN  BOOLEAN                 Shared,
  IN  UINT32                  *IrqList,
  IN  UINT8                   IrqCount,
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode, OPTIONAL
  OUT AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "Register ()" ASL function.

  The Resource Data effectively created is a Generic Register Descriptor.
  Data. Cf ACPI 6.4:
   - s6.4.3.7 "Generic Register Descriptor".
   - s19.6.114 "Register".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @param [in]  AddressSpace    Address space where the register exists.
                               Can be one of I/O space, System Memory, etc.
  @param [in]  BitWidth        Number of bits in the register.
  @param [in]  BitOffset       Offset in bits from the start of the register
                               indicated by the Address.
  @param [in]  Address         Register address.
  @param [in]  AccessSize      Size of data values used when accessing the
                               address space. Can be one of:
                                 0 - Undefined, legacy (EFI_ACPI_6_4_UNDEFINED)
                                 1 - Byte access (EFI_ACPI_6_4_BYTE)
                                 2 - Word access (EFI_ACPI_6_4_WORD)
                                 3 - DWord access (EFI_ACPI_6_4_DWORD)
                                 4 - QWord access (EFI_ACPI_6_4_QWORD)
  @param  [in]  NameOpNode       NameOp object node defining a named object.
                                 If provided, append the new resource data node
                                 to the list of resource data elements of this
                                 node.
  @param  [out] NewRdNode        If provided and success,
                                 contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdRegister (
  IN  UINT8                   AddressSpace,
  IN  UINT8                   BitWidth,
  IN  UINT8                   BitOffset,
  IN  UINT64                  Address,
  IN  UINT8                   AccessSize,
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode, OPTIONAL
  OUT AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

#endif // AML_RESOURCE_DATA_CODE_GEN_H_
