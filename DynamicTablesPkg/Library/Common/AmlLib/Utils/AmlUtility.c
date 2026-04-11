/** @file
  AML Utility.

  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Utils/AmlUtility.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>

/** This function computes and updates the ACPI table checksum.

  @param  [in]  AcpiTable   Pointer to an Acpi table.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AcpiPlatformChecksum (
  IN  EFI_ACPI_DESCRIPTION_HEADER  *AcpiTable
  )
{
  UINT8   *Ptr;
  UINT8   Sum;
  UINT32  Size;

  if (AcpiTable == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Ptr  = (UINT8 *)AcpiTable;
  Size = AcpiTable->Length;
  Sum  = 0;

  // Set the checksum field to 0 first.
  AcpiTable->Checksum = 0;

  // Compute the checksum.
  while ((Size--) != 0) {
    Sum = (UINT8)(Sum + (*Ptr++));
  }

  // Set the checksum.
  AcpiTable->Checksum = (UINT8)(0xFF - Sum + 1);

  return EFI_SUCCESS;
}

/** A callback function that computes the size of a Node and adds it to the
    Size pointer stored in the Context.
    Calling this function on the root node will compute the total size of the
    AML bytestream.

  @param  [in]      Node      Node to compute the size.
  @param  [in, out] Context   Pointer holding the computed size.
                              (UINT32 *) Context.
  @param  [in, out] Status    Pointer holding:
                               - At entry, the Status returned by the
                                 last call to this exact function during
                                 the enumeration;
                               - At exit, he returned status of the
                                 call to this function.
                              Optional, can be NULL.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
STATIC
BOOLEAN
EFIAPI
AmlComputeSizeCallback (
  IN      AML_NODE_HEADER  *Node,
  IN  OUT VOID             *Context,
  IN  OUT EFI_STATUS       *Status   OPTIONAL
  )
{
  UINT32                 Size;
  EAML_PARSE_INDEX       IndexPtr;
  CONST AML_OBJECT_NODE  *ParentNode;

  if (!IS_AML_NODE_VALID (Node) ||
      (Context == NULL))
  {
    ASSERT (0);
    if (Status != NULL) {
      *Status = EFI_INVALID_PARAMETER;
    }

    return FALSE;
  }

  // Ignore the second fixed argument of method invocation nodes
  // as the information stored there (the argument count) is not in the
  // ACPI specification.
  ParentNode = (CONST AML_OBJECT_NODE *)AmlGetParent (Node);
  if (IS_AML_OBJECT_NODE (ParentNode)                             &&
      AmlNodeCompareOpCode (ParentNode, AML_METHOD_INVOC_OP, 0)   &&
      AmlIsNodeFixedArgument (Node, &IndexPtr))
  {
    if (IndexPtr == EAmlParseIndexTerm1) {
      if (Status != NULL) {
        *Status = EFI_SUCCESS;
      }

      return TRUE;
    }
  }

  Size = *((UINT32 *)Context);

  if (IS_AML_DATA_NODE (Node)) {
    Size += ((AML_DATA_NODE *)Node)->Size;
  } else if (IS_AML_OBJECT_NODE (Node)  &&
             !AmlNodeHasAttribute (
                (CONST AML_OBJECT_NODE *)Node,
                AML_IS_PSEUDO_OPCODE
                ))
  {
    // Ignore pseudo-opcodes as they are not part of the
    // ACPI specification.

    Size += (((AML_OBJECT_NODE *)Node)->AmlByteEncoding->OpCode ==
             AML_EXT_OP) ? 2 : 1;

    // Add the size of the PkgLen.
    if (AmlNodeHasAttribute (
          (AML_OBJECT_NODE *)Node,
          AML_HAS_PKG_LENGTH
          ))
    {
      Size += AmlComputePkgLengthWidth (((AML_OBJECT_NODE *)Node)->PkgLen);
    }
  }

  // Check for overflow.
  // The root node has a null size, thus the strict comparison.
  if (*((UINT32 *)Context) > Size) {
    ASSERT (0);
    *Status = EFI_INVALID_PARAMETER;
    return FALSE;
  }

  *((UINT32 *)Context) = Size;

  if (Status != NULL) {
    *Status = EFI_SUCCESS;
  }

  return TRUE;
}

/** Compute the size of a tree/sub-tree.

  @param  [in]      Node      Node to compute the size.
  @param  [in, out] Size      Pointer holding the computed size.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlComputeSize (
  IN      CONST AML_NODE_HEADER  *Node,
  IN  OUT       UINT32           *Size
  )
{
  EFI_STATUS  Status;

  if (!IS_AML_NODE_VALID (Node) ||
      (Size == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *Size = 0;

  AmlEnumTree (
    (AML_NODE_HEADER *)Node,
    AmlComputeSizeCallback,
    (VOID *)Size,
    &Status
    );

  return Status;
}

/** Get the value contained in an integer node.

  @param  [in]  Node    Pointer to an integer node.
                        Must be an object node.
  @param  [out] Value   Value contained in the integer node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNodeGetIntegerValue (
  IN  AML_OBJECT_NODE  *Node,
  OUT UINT64           *Value
  )
{
  AML_DATA_NODE  *DataNode;

  if ((!IsIntegerNode (Node)            &&
       !IsSpecialIntegerNode (Node))    ||
      (Value == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // For ZeroOp and OneOp, there is no data node.
  if (IsSpecialIntegerNode (Node)) {
    if (AmlNodeCompareOpCode (Node, AML_ZERO_OP, 0)) {
      *Value = 0;
    } else if (AmlNodeCompareOpCode (Node, AML_ONE_OP, 0)) {
      *Value = 1;
    } else {
      // OnesOp cannot be handled: it represents a maximum value.
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    return EFI_SUCCESS;
  }

  // For integer nodes, the value is in the first fixed argument.
  DataNode = (AML_DATA_NODE *)Node->FixedArgs[EAmlParseIndexTerm0];
  if (!IS_AML_DATA_NODE (DataNode) ||
      (DataNode->DataType != EAmlNodeDataTypeUInt))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  switch (DataNode->Size) {
    case 1:
    {
      *Value = *((UINT8 *)(DataNode->Buffer));
      break;
    }
    case 2:
    {
      *Value = *((UINT16 *)(DataNode->Buffer));
      break;
    }
    case 4:
    {
      *Value = *((UINT32 *)(DataNode->Buffer));
      break;
    }
    case 8:
    {
      *Value = *((UINT64 *)(DataNode->Buffer));
      break;
    }
    default:
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  } // switch

  return EFI_SUCCESS;
}

/** Replace a Zero (AML_ZERO_OP) or One (AML_ONE_OP) object node
    with a byte integer (AML_BYTE_PREFIX) object node having the same value.

  @param  [in]  Node    Pointer to an integer node.
                        Must be an object node having ZeroOp or OneOp.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlUnwindSpecialInteger (
  IN  AML_OBJECT_NODE  *Node
  )
{
  EFI_STATUS  Status;

  AML_DATA_NODE            *NewDataNode;
  UINT8                    Value;
  CONST AML_BYTE_ENCODING  *ByteEncoding;

  if (!IsSpecialIntegerNode (Node)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Find the value.
  if (AmlNodeCompareOpCode (Node, AML_ZERO_OP, 0)) {
    Value = 0;
  } else if (AmlNodeCompareOpCode (Node, AML_ONE_OP, 0)) {
    Value = 1;
  } else {
    // OnesOp cannot be handled: it represents a maximum value.
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeUInt,
             &Value,
             sizeof (UINT8),
             (AML_DATA_NODE **)&NewDataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Change the encoding of the special node to a ByteOp encoding.
  ByteEncoding = AmlGetByteEncodingByOpCode (AML_BYTE_PREFIX, 0);
  if (ByteEncoding == NULL) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  // Update the ByteEncoding from ZERO_OP/ONE_OP to AML_BYTE_PREFIX.
  Node->AmlByteEncoding = ByteEncoding;

  // Add the data node as the first fixed argument of the ByteOp object.
  Status = AmlSetFixedArgument (
             (AML_OBJECT_NODE *)Node,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)NewDataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  AmlDeleteTree ((AML_NODE_HEADER *)NewDataNode);
  return Status;
}

/** Set the value contained in an integer node.

  The OpCode is updated accordingly to the new value
  (e.g.: If the original value was a UINT8 value, then the OpCode
         would be AML_BYTE_PREFIX. If it the new value is a UINT16
         value then the OpCode will be updated to AML_WORD_PREFIX).

  @param  [in]  Node            Pointer to an integer node.
                                Must be an object node.
  @param  [in]  NewValue        New value to write in the integer node.
  @param  [out] ValueWidthDiff  Difference in number of bytes used to store
                                the new value.
                                Can be negative.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlNodeSetIntegerValue (
  IN  AML_OBJECT_NODE  *Node,
  IN  UINT64           NewValue,
  OUT INT8             *ValueWidthDiff
  )
{
  EFI_STATUS     Status;
  AML_DATA_NODE  *DataNode;

  UINT8  NewOpCode;
  UINT8  NumberOfBytes;

  if ((!IsIntegerNode (Node)            &&
       !IsSpecialIntegerNode (Node))    ||
      (ValueWidthDiff == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *ValueWidthDiff = 0;
  // For ZeroOp and OneOp, there is no data node.
  // Thus the object node is converted to a byte object node holding 0 or 1.
  if (IsSpecialIntegerNode (Node)) {
    switch (NewValue) {
      case AML_ZERO_OP:
        Node->AmlByteEncoding = AmlGetByteEncodingByOpCode (AML_ZERO_OP, 0);
        return EFI_SUCCESS;
      case AML_ONE_OP:
        Node->AmlByteEncoding = AmlGetByteEncodingByOpCode (AML_ONE_OP, 0);
        return EFI_SUCCESS;
      default:
      {
        Status = AmlUnwindSpecialInteger (Node);
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        // The AmlUnwindSpecialInteger functions converts a special integer
        // node to a UInt8/Byte data node. Thus, the size increments by one:
        // special integer are encoded as one byte (the opcode only) while byte
        // integers are encoded as two bytes (the opcode + the value).
        *ValueWidthDiff += sizeof (UINT8);
      }
    } // switch
  } // IsSpecialIntegerNode (Node)

  // For integer nodes, the value is in the first fixed argument.
  DataNode = (AML_DATA_NODE *)Node->FixedArgs[EAmlParseIndexTerm0];
  if (!IS_AML_DATA_NODE (DataNode) ||
      (DataNode->DataType != EAmlNodeDataTypeUInt))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The value can be encoded with a special 0 or 1 OpCode.
  // The AML_ONES_OP is not handled.
  if (NewValue <= 1) {
    NewOpCode             = (NewValue == 0) ? AML_ZERO_OP : AML_ONE_OP;
    Node->AmlByteEncoding = AmlGetByteEncodingByOpCode (NewOpCode, 0);

    // The value is encoded with a AML_ZERO_OP or AML_ONE_OP.
    // This means there is no need for a DataNode containing the value.
    // The change in size is equal to the size of the DataNode's buffer.
    *ValueWidthDiff = -((INT8)DataNode->Size);

    // Detach and free the DataNode containing the integer value.
    DataNode->NodeHeader.Parent          = NULL;
    Node->FixedArgs[EAmlParseIndexTerm0] = NULL;
    Status                               = AmlDeleteNode ((AML_NODE_HEADER *)DataNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    return EFI_SUCCESS;
  }

  // Check the number of bits needed to represent the value.
  if (NewValue > MAX_UINT32) {
    // Value is 64 bits.
    NewOpCode     = AML_QWORD_PREFIX;
    NumberOfBytes = 8;
  } else if (NewValue > MAX_UINT16) {
    // Value is 32 bits.
    NewOpCode     = AML_DWORD_PREFIX;
    NumberOfBytes = 4;
  } else if (NewValue > MAX_UINT8) {
    // Value is 16 bits.
    NewOpCode     = AML_WORD_PREFIX;
    NumberOfBytes = 2;
  } else {
    // Value is 8 bits.
    NewOpCode     = AML_BYTE_PREFIX;
    NumberOfBytes = 1;
  }

  *ValueWidthDiff += (INT8)(NumberOfBytes - DataNode->Size);

  // Update the ByteEncoding as it may have changed between [8 .. 64] bits.
  Node->AmlByteEncoding = AmlGetByteEncodingByOpCode (NewOpCode, 0);
  if (Node->AmlByteEncoding == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Free the old DataNode buffer and allocate a buffer with the right size
  // to store the new data.
  if (*ValueWidthDiff != 0) {
    FreePool (DataNode->Buffer);
    DataNode->Buffer = AllocateZeroPool (NumberOfBytes);
    if (DataNode->Buffer == NULL) {
      ASSERT (0);
      return EFI_OUT_OF_RESOURCES;
    }

    DataNode->Size = NumberOfBytes;
  }

  // Write the new value.
  CopyMem (DataNode->Buffer, &NewValue, NumberOfBytes);

  return EFI_SUCCESS;
}

/** Increment/decrement the value contained in the IntegerNode.

  @param  [in]  IntegerNode     Pointer to an object node containing
                                an integer.
  @param  [in]  IsIncrement     Choose the operation to do:
                                 - TRUE:  Increment the Node's size and
                                          the Node's count;
                                 - FALSE: Decrement the Node's size and
                                          the Node's count.
  @param  [in]  Diff            Value to add/subtract to the integer.
  @param  [out] ValueWidthDiff  When modifying the integer, it can be
                                promoted/demoted, e.g. from UINT8 to UINT16.
                                Stores the change in width.
                                Can be negative.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlNodeUpdateIntegerValue (
  IN  AML_OBJECT_NODE  *IntegerNode,
  IN  BOOLEAN          IsIncrement,
  IN  UINT64           Diff,
  OUT INT8             *ValueWidthDiff
  )
{
  EFI_STATUS  Status;
  UINT64      Value;

  if (ValueWidthDiff == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the current value.
  // Checks on the IntegerNode are done in the call.
  Status = AmlNodeGetIntegerValue (IntegerNode, &Value);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Check for UINT64 over/underflow.
  if ((IsIncrement && (Value > (MAX_UINT64 - Diff))) ||
      (!IsIncrement && (Value < Diff)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Compute the new value.
  if (IsIncrement) {
    Value += Diff;
  } else {
    Value -= Diff;
  }

  Status = AmlNodeSetIntegerValue (
             IntegerNode,
             Value,
             ValueWidthDiff
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Propagate the size information up the tree.

  The length of the ACPI table is updated in the RootNode,
  but not the checksum.

  @param  [in]  Node          Pointer to a node.
                              Must be a root node or an object node.
  @param  [in]  IsIncrement   Choose the operation to do:
                               - TRUE:  Increment the Node's size and
                                        the Node's count;
                               - FALSE: Decrement the Node's size and
                                        the Node's count.
  @param  [in]  Diff          Value to add/subtract to the Node's size.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlPropagateSize (
  IN  AML_NODE_HEADER  *Node,
  IN  BOOLEAN          IsIncrement,
  IN  UINT32           *Diff
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;
  AML_NODE_HEADER  *ParentNode;

  UINT32  Value;
  UINT32  InitialPkgLenWidth;
  UINT32  NewPkgLenWidth;
  UINT32  ReComputedPkgLenWidth;
  INT8    FieldWidthChange;

  if (!IS_AML_OBJECT_NODE (Node) &&
      !IS_AML_ROOT_NODE (Node))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (IS_AML_OBJECT_NODE (Node)) {
    ObjectNode = (AML_OBJECT_NODE *)Node;

    // For BufferOp, the buffer size is stored in BufferSize. Therefore,
    // BufferOp needs special handling to update the BufferSize.
    // BufferSize must be updated before the PkgLen to accommodate any
    // increment resulting from the update of the BufferSize.
    // DefBuffer := BufferOp PkgLength BufferSize ByteList
    // BufferOp := 0x11
    // BufferSize := TermArg => Integer
    if (AmlNodeCompareOpCode (ObjectNode, AML_BUFFER_OP, 0)) {
      // First fixed argument of BufferOp is an integer (BufferSize)
      // (can be a BYTE, WORD, DWORD or QWORD).
      // BufferSize is an object node.
      Status = AmlNodeUpdateIntegerValue (
                 (AML_OBJECT_NODE *)AmlGetFixedArgument (
                                      ObjectNode,
                                      EAmlParseIndexTerm0
                                      ),
                 IsIncrement,
                 (UINT64)(*Diff),
                 &FieldWidthChange
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }

      // FieldWidthChange is an integer.
      // It must be positive if IsIncrement is TRUE, negative otherwise.
      if ((IsIncrement              &&
           (FieldWidthChange < 0))  ||
          (!IsIncrement             &&
           (FieldWidthChange > 0)))
      {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Check for UINT32 overflow.
      if (*Diff > (MAX_UINT32 - (UINT32)ABS (FieldWidthChange))) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Update Diff if the field width changed.
      *Diff = (UINT32)(*Diff + ABS (FieldWidthChange));
    } // AML_BUFFER_OP node.

    // Update the PgkLen.
    // Needs to be done at last to reflect the potential field width changes.
    if (AmlNodeHasAttribute (ObjectNode, AML_HAS_PKG_LENGTH)) {
      Value = ObjectNode->PkgLen;

      // Subtract the size of the PkgLen encoding. The size of the PkgLen
      // encoding must be computed after having updated Value.
      InitialPkgLenWidth = AmlComputePkgLengthWidth (Value);
      Value             -= InitialPkgLenWidth;

      // Check for an over/underflows.
      // PkgLen is a 28 bit value, cf 20.2.4 Package Length Encoding
      // i.e. the maximum value is (2^28 - 1) = ((BIT0 << 28) - 1).
      if ((IsIncrement && ((((BIT0 << 28) - 1) - Value) < *Diff))   ||
          (!IsIncrement && (Value < *Diff)))
      {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Update the size.
      if (IsIncrement) {
        Value += *Diff;
      } else {
        Value -= *Diff;
      }

      // Compute the new PkgLenWidth.
      NewPkgLenWidth = AmlComputePkgLengthWidth (Value);
      if (NewPkgLenWidth == 0) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Add it to the Value.
      Value += NewPkgLenWidth;

      // Check that adding the PkgLenWidth didn't trigger a domino effect,
      // increasing the encoding width of the PkgLen again.
      // The PkgLen is encoded on at most 4 bytes. It is possible to increase
      // the PkgLen width if its encoding is on less than 3 bytes.
      ReComputedPkgLenWidth = AmlComputePkgLengthWidth (Value);
      if (ReComputedPkgLenWidth != NewPkgLenWidth) {
        if ((ReComputedPkgLenWidth != 0)   &&
            (ReComputedPkgLenWidth < 4))
        {
          // No need to recompute the PkgLen since a new threshold cannot
          // be reached by incrementing the value by one.
          Value += 1;
        } else {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }
      }

      *Diff += (InitialPkgLenWidth > ReComputedPkgLenWidth) ?
               (InitialPkgLenWidth - ReComputedPkgLenWidth) :
               (ReComputedPkgLenWidth - InitialPkgLenWidth);
      ObjectNode->PkgLen = Value;
    } // PkgLen update.

    // During CodeGeneration, the tree is incomplete and
    // there is no root node at the top of the tree. Stop
    // propagating the new size when finding a root node
    // OR when a NULL parent is found.
    ParentNode = AmlGetParent ((AML_NODE_HEADER *)Node);
    if (ParentNode != NULL) {
      // Propagate the size up the tree.
      Status = AmlPropagateSize (
                 Node->Parent,
                 IsIncrement,
                 Diff
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }
  } else if (IS_AML_ROOT_NODE (Node)) {
    // Update the length field in the SDT header.
    Value = ((AML_ROOT_NODE *)Node)->SdtHeader->Length;

    // Check for an over/underflows.
    if ((IsIncrement && (Value > (MAX_UINT32 - *Diff))) ||
        (!IsIncrement && (Value < *Diff)))
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    // Update the size.
    if (IsIncrement) {
      Value += *Diff;
    } else {
      Value -= *Diff;
    }

    ((AML_ROOT_NODE *)Node)->SdtHeader->Length = Value;
  }

  return EFI_SUCCESS;
}

/** Propagate the node count information up the tree.

  @param  [in]  ObjectNode        Pointer to an object node.
  @param  [in]  IsIncrement       Choose the operation to do:
                                   - TRUE:  Increment the Node's size and
                                            the Node's count;
                                   - FALSE: Decrement the Node's size and
                                           the Node's count.
  @param  [in]  NodeCount         Number of nodes added/removed (depends on the
                                  value of Operation).
  @param  [out] FieldWidthChange  When modifying the integer, it can be
                                  promoted/demoted, e.g. from UINT8 to UINT16.
                                  Stores the change in width.
                                  Can be negative.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlPropagateNodeCount (
  IN  AML_OBJECT_NODE  *ObjectNode,
  IN  BOOLEAN          IsIncrement,
  IN  UINT8            NodeCount,
  OUT INT8             *FieldWidthChange
  )
{
  EFI_STATUS  Status;

  AML_NODE_HEADER  *NodeCountArg;
  UINT8            CurrNodeCount;

  // Currently there is no use case where (NodeCount > 1).
  if (!IS_AML_OBJECT_NODE (ObjectNode)  ||
      (FieldWidthChange == NULL)        ||
      (NodeCount > 1))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *FieldWidthChange = 0;

  // Update the number of elements stored in PackageOp and VarPackageOp.
  // The number of elements is stored as the first fixed argument.
  // DefPackage := PackageOp PkgLength NumElements PackageElementList
  // PackageOp := 0x12
  // DefVarPackage := VarPackageOp PkgLength VarNumElements PackageElementList
  // VarPackageOp := 0x13
  // NumElements := ByteData
  // VarNumElements := TermArg => Integer
  NodeCountArg = AmlGetFixedArgument (ObjectNode, EAmlParseIndexTerm0);
  if (AmlNodeCompareOpCode (ObjectNode, AML_PACKAGE_OP, 0)) {
    // First fixed argument of PackageOp stores the number of elements
    // in the package. It is an UINT8.

    // Check for over/underflow.
    CurrNodeCount = *(((AML_DATA_NODE *)NodeCountArg)->Buffer);
    if ((IsIncrement && (CurrNodeCount == MAX_UINT8)) ||
        (!IsIncrement && (CurrNodeCount == 0)))
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    // Update the node count in the DataNode.
    CurrNodeCount                              = IsIncrement ? (CurrNodeCount + 1) : (CurrNodeCount - 1);
    *(((AML_DATA_NODE *)NodeCountArg)->Buffer) =  CurrNodeCount;
  } else if (AmlNodeCompareOpCode (ObjectNode, AML_VAR_PACKAGE_OP, 0)) {
    // First fixed argument of PackageOp stores the number of elements
    // in the package. It is an integer (can be a BYTE, WORD, DWORD, QWORD).
    Status = AmlNodeUpdateIntegerValue (
               (AML_OBJECT_NODE *)NodeCountArg,
               IsIncrement,
               NodeCount,
               FieldWidthChange
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Propagate information up the tree.

  The information can be a new size, a new number of arguments.

  @param  [in]  Node          Pointer to a node.
                              Must be a root node or an object node.
  @param  [in]  IsIncrement   Choose the operation to do:
                               - TRUE:  Increment the Node's size and
                                        the Node's count;
                               - FALSE: Decrement the Node's size and
                                        the Node's count.
  @param  [in]  Diff          Value to add/subtract to the Node's size.
  @param  [in]  NodeCount     Number of nodes added/removed.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlPropagateInformation (
  IN  AML_NODE_HEADER  *Node,
  IN  BOOLEAN          IsIncrement,
  IN  UINT32           Diff,
  IN  UINT8            NodeCount
  )
{
  EFI_STATUS  Status;
  INT8        FieldWidthChange;

  // Currently there is no use case where (NodeCount > 1).
  if ((!IS_AML_ROOT_NODE (Node)     &&
       !IS_AML_OBJECT_NODE (Node))  ||
      (NodeCount > 1))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Propagate the node count first as it may change the number of bytes
  // needed to store the node count, and then impact FieldWidthChange.
  if ((NodeCount != 0) &&
      IS_AML_OBJECT_NODE (Node))
  {
    Status = AmlPropagateNodeCount (
               (AML_OBJECT_NODE *)Node,
               IsIncrement,
               NodeCount,
               &FieldWidthChange
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Propagate the potential field width change.
    // Maximum change is between UINT8/UINT64: 8 bytes.
    if ((ABS (FieldWidthChange) > 8)                          ||
        (IsIncrement                                          &&
         ((FieldWidthChange < 0)                             ||
          ((Diff + (UINT8)FieldWidthChange) > MAX_UINT32)))  ||
        (!IsIncrement                                         &&
         ((FieldWidthChange > 0)                             ||
          (Diff < (UINT32)ABS (FieldWidthChange)))))
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Diff = (UINT32)(Diff + (UINT8)ABS (FieldWidthChange));
  }

  // Diff can be zero if some data is updated without modifying the data size.
  if (Diff != 0) {
    Status = AmlPropagateSize (Node, IsIncrement, &Diff);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Find and set the EndTag's Checksum of a list of Resource Data elements.

  Lists of Resource Data elements end with an EndTag (most of the time). This
  function finds the EndTag (if present) in a list of Resource Data elements
  and sets the checksum.

  ACPI 6.4, s6.4.2.9 "End Tag":
  "This checksum is generated such that adding it to the sum of all the data
  bytes will produce a zero sum."
  "If the checksum field is zero, the resource data is treated as if the
  checksum operation succeeded. Configuration proceeds normally."

  To avoid re-computing checksums, if a new resource data elements is
  added/removed/modified in a list of resource data elements, the AmlLib
  resets the checksum to 0.

  @param [in]  BufferOpNode   Node having a list of Resource Data elements.
  @param [in]  CheckSum       CheckSum to store in the EndTag.
                              To ignore/avoid computing the checksum,
                              give 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No EndTag found.
**/
EFI_STATUS
EFIAPI
AmlSetRdListCheckSum (
  IN  AML_OBJECT_NODE  *BufferOpNode,
  IN  UINT8            CheckSum
  )
{
  EFI_STATUS     Status;
  AML_DATA_NODE  *LastRdNode;
  AML_RD_HEADER  RdDataType;

  if (!AmlNodeCompareOpCode (BufferOpNode, AML_BUFFER_OP, 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the last Resource data node in the variable list of
  // argument of the BufferOp node.
  LastRdNode = (AML_DATA_NODE *)AmlGetPreviousVariableArgument (
                                  (AML_NODE_HEADER *)BufferOpNode,
                                  NULL
                                  );
  if ((LastRdNode == NULL)             ||
      !IS_AML_DATA_NODE (LastRdNode)   ||
      (LastRdNode->DataType != EAmlNodeDataTypeResourceData))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlGetResourceDataType (LastRdNode, &RdDataType);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Check the LastRdNode is an EndTag.
  // It is possible to have only one Resource Data in a BufferOp with
  // no EndTag. Return EFI_NOT_FOUND is such case.
  if (!AmlRdCompareDescId (
         &RdDataType,
         AML_RD_BUILD_SMALL_DESC_ID (ACPI_SMALL_END_TAG_DESCRIPTOR_NAME)
         ))
  {
    ASSERT (0);
    return EFI_NOT_FOUND;
  }

  Status = AmlRdSetEndTagChecksum (LastRdNode->Buffer, CheckSum);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
