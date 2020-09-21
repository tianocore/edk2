/** @file
  AML Resource Data Code Generation.

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Rd or RD   - Resource Data
  - Rds or RDS - Resource Data Small
  - Rdl or RDL - Resource Data Large
**/

#include <AmlNodeDefines.h>
#include <CodeGen/AmlResourceDataCodeGen.h>

#include <AmlCoreInterface.h>
#include <AmlDefines.h>
#include <Api/AmlApiHelper.h>
#include <Tree/AmlNode.h>
#include <ResourceData/AmlResourceData.h>

/** If ParentNode is not NULL, append RdNode.
    If NewRdNode is not NULL, update its value to RdNode.

  @param [in]  RdNode       Newly created Resource Data node.
  @param [in]  ParentNode   If not NULL, add the generated node
                            to the end of the variable list of
                            argument of the ParentNode, but
                            before the "End Tag" Resource Data.
                            Must be a BufferOpNode.
  @param [out] NewRdNode    If not NULL, update the its value to RdNode.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
LinkRdNode (
  IN  AML_DATA_NODE      * RdNode,
  IN  AML_OBJECT_NODE    * ParentNode,
  OUT AML_DATA_NODE     ** NewRdNode
  )
{
  EFI_STATUS    Status;
  EFI_STATUS    Status1;

  if (NewRdNode != NULL) {
    *NewRdNode = RdNode;
  }

  // Add RdNode as the last element, but before the EndTag.
  if (ParentNode != NULL) {
    Status = AmlAppendRdNode (ParentNode, RdNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      Status1 = AmlDeleteTree ((AML_NODE_HEADER*)RdNode);
      ASSERT_EFI_ERROR (Status1);
      // Return original error.
      return Status;
    }
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                               Status;

  AML_DATA_NODE                          * RdNode;
  EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR   RdInterrupt;
  UINT32                                 * FirstInterrupt;

  if ((IrqList == NULL) ||
      (IrqCount == 0)   ||
      ((ParentNode == NULL) && (NewRdNode == NULL))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  RdInterrupt.Header.Header.Bits.Name =
    ACPI_LARGE_EXTENDED_IRQ_DESCRIPTOR_NAME;
  RdInterrupt.Header.Header.Bits.Type = ACPI_LARGE_ITEM_FLAG;
  RdInterrupt.Header.Length = sizeof (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR) -
                                sizeof (ACPI_LARGE_RESOURCE_HEADER);
  RdInterrupt.InterruptVectorFlags = (ResourceConsumer ? BIT0 : 0) |
                                     (EdgeTriggered ? BIT1 : 0)    |
                                     (ActiveLow ? BIT2 : 0)        |
                                     (Shared ? BIT3 : 0);
  RdInterrupt.InterruptTableLength = IrqCount;

  // Get the address of the first interrupt field.
  FirstInterrupt = RdInterrupt.InterruptNumber;

  // Copy the list of interrupts.
  CopyMem (FirstInterrupt, IrqList, (sizeof (UINT32) * IrqCount));

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeResourceData,
             (UINT8*)&RdInterrupt,
             sizeof (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR),
             &RdNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  return LinkRdNode (RdNode, ParentNode, NewRdNode);
}

/** Add an Interrupt Resource Data node.

  This function creates a Resource Data element corresponding to the
  "Interrupt ()" ASL function, stores it in an AML Data Node.

  It then adds it after the input CurrRdNode in the list of resource data
  element.

  The Resource Data effectively created is an Extended Interrupt Resource
  Data. See ACPI 6.3 specification, s6.4.3.6 "Extended Interrupt Descriptor"
  for more information about Extended Interrupt Resource Data.

  The Extended Interrupt contains one single interrupt.

  This function allocates memory to create a data node. It is the caller's
  responsibility to either:
   - attach this node to an AML tree;
   - delete this node.

  Note: The _CRS node must be defined using the ASL Name () function.
        e.g. Name (_CRS, ResourceTemplate () {
               ...
             }

  @param  [in]  NameOpCrsNode    NameOp object node defining a "_CRS" object.
                                 Must have an OpCode=AML_NAME_OP, SubOpCode=0.
                                 NameOp object nodes are defined in ASL
                                 using the "Name ()" function.
  @param  [in]  ResourceConsumer The device consumes the specified interrupt
                                 or produces it for use by a child device.
  @param  [in]  EdgeTriggered    The interrupt is edge triggered or
                                 level triggered.
  @param  [in]  ActiveLow        The interrupt is active-high or active-low.
  @param  [in]  Shared           The interrupt can be shared with other
                                 devices or not (Exclusive).
  @param  [in]  IrqList          Interrupt list. Must be non-NULL.
  @param  [in]  IrqCount         Interrupt count. Must be non-zero.


  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenCrsAddRdInterrupt (
  IN  AML_OBJECT_NODE_HANDLE  NameOpCrsNode,
  IN  BOOLEAN                 ResourceConsumer,
  IN  BOOLEAN                 EdgeTriggered,
  IN  BOOLEAN                 ActiveLow,
  IN  BOOLEAN                 Shared,
  IN  UINT32                * IrqList,
  IN  UINT8                   IrqCount
  )
{
  EFI_STATUS              Status;

  AML_OBJECT_NODE_HANDLE  BufferOpNode;

  if ((IrqList == NULL)                                                   ||
      (IrqCount == 0)                                                     ||
      (!AmlNodeHasOpCode (NameOpCrsNode, AML_NAME_OP, 0))                 ||
      (!AmlNameOpCompareName (NameOpCrsNode, "_CRS"))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the _CRS value which is represented as a BufferOp object node
  // which is the 2nd fixed argument (i.e. index 1).
  BufferOpNode = (AML_OBJECT_NODE_HANDLE)AmlGetFixedArgument (
                                           NameOpCrsNode,
                                           EAmlParseIndexTerm1
                                           );
  if ((BufferOpNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)BufferOpNode) != EAmlNodeObject) ||
      (!AmlNodeHasOpCode (BufferOpNode, AML_BUFFER_OP, 0))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Generate the Extended Interrupt Resource Data node,
  // and attach it as the last variable argument of the BufferOpNode.
  Status = AmlCodeGenInterrupt (
             ResourceConsumer,
             EdgeTriggered,
             ActiveLow,
             Shared,
             IrqList,
             IrqCount,
             BufferOpNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  return Status;
}
