/** @file
  AML Resource Data Code Generation.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_RESOURCE_DATA_CODE_GEN_H_
#define AML_RESOURCE_DATA_CODE_GEN_H_

/** Code generation for the "Interrupt ()" ASL function.

  This function creates a Resource Data element corresponding to the
  "Interrupt ()" ASL function and stores it in an AML Data Node.

  The Resource Data effectively created is an Extended Interrupt Resource
  Data. See ACPI 6.3 specification, s6.4.3.6 "Extended Interrupt Descriptor"
  for more information about Extended Interrupt Resource Data.

  This function allocates memory to create a data node. It is the caller's
  responsibility to either:
   - attach this node to an AML tree;
   - delete this node.

  @param [in]  ResourceConsumer    The device consumes the specified interrupt
                                   or produces it for use by a child device.
  @param [in]  EdgeTriggered       The interrupt is edge triggered or
                                   level triggered.
  @param [in]  ActiveLow           The interrupt is active-high or active-low.
  @param [in]  Shared              The interrupt can be shared with other
                                   devices or not (Exclusive).
  @param [in]  IrqList             Interrupt list. Must be non-NULL.
  @param [in]  IrqCount            Interrupt count. Must be non-zero.
  @param [in]  ParentNode          If not NULL, add the generated node
                                   to the end of the variable list of
                                   argument of the ParentNode, but
                                   before the "End Tag" Resource Data.
                                   Must be a BufferOpNode.
  @param  [out] NewRdNode          If success, contains the generated node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenInterrupt (
  IN  BOOLEAN             ResourceConsumer,
  IN  BOOLEAN             EdgeTriggered,
  IN  BOOLEAN             ActiveLow,
  IN  BOOLEAN             Shared,
  IN  UINT32            * IrqList,
  IN  UINT8               IrqCount,
  IN  AML_OBJECT_NODE   * ParentNode,   OPTIONAL
  OUT AML_DATA_NODE    ** NewRdNode     OPTIONAL
  );

#endif // AML_RESOURCE_DATA_CODE_GEN_H_
