/** @file
  AML Serialize.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>

#include <AmlCoreInterface.h>
#include <Stream/AmlStream.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>
#include <Utils/AmlUtility.h>

/** Callback function to copy the AML bytecodes contained in a node
    to the Stream stored in the Context.
    The SDT header data contained in the root node is not serialized
    by this function.

  @param  [in]      Node      Pointer to the node to copy the AML bytecodes
                              from.
  @param  [in, out] Context   Contains a forward Stream to write to.
                              (AML_STREAM*)Context.
  @param  [in, out] Status    At entry, contains the status returned by the
                              last call to this exact function during the
                              enumeration.
                              As exit, contains the returned status of the
                              call to this function.
                              Optional, can be NULL.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
STATIC
BOOLEAN
EFIAPI
AmlSerializeNodeCallback (
  IN       AML_NODE_HEADER  *Node,
  IN  OUT  VOID             *Context     OPTIONAL,
  IN  OUT  EFI_STATUS       *Status      OPTIONAL
  )
{
  EFI_STATUS  Status1;

  CONST AML_DATA_NODE    *DataNode;
  CONST AML_OBJECT_NODE  *ObjectNode;
  AML_STREAM             *FStream;

  // Bytes needed to store OpCode[1] + SubOpcode[1] + MaxPkgLen[4] = 6 bytes.
  UINT8    ObjectNodeInfoArray[6];
  UINT32   Index;
  BOOLEAN  ContinueEnum;

  CONST AML_OBJECT_NODE  *ParentNode;
  EAML_PARSE_INDEX       IndexPtr;

  if (!IS_AML_NODE_VALID (Node)   ||
      (Context == NULL))
  {
    ASSERT (0);
    Status1      = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
    goto error_handler;
  }

  // Ignore the second fixed argument of method invocation nodes
  // as the information stored there (the argument count) is not in the
  // ACPI specification.
  ParentNode = (CONST AML_OBJECT_NODE *)AmlGetParent ((AML_NODE_HEADER *)Node);
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

  Status1      = EFI_SUCCESS;
  ContinueEnum = TRUE;
  FStream      = (AML_STREAM *)Context;

  if (IS_AML_DATA_NODE (Node)) {
    // Copy the content of the Buffer for a DataNode.
    DataNode = (AML_DATA_NODE *)Node;
    Status1  = AmlStreamWrite (
                 FStream,
                 DataNode->Buffer,
                 DataNode->Size
                 );
    if (EFI_ERROR (Status1)) {
      ASSERT (0);
      ContinueEnum = FALSE;
      goto error_handler;
    }
  } else if (IS_AML_OBJECT_NODE (Node)  &&
             !AmlNodeHasAttribute (
                (CONST AML_OBJECT_NODE *)Node,
                AML_IS_PSEUDO_OPCODE
                ))
  {
    // Ignore pseudo-opcodes as they are not part of the
    // ACPI specification.

    ObjectNode = (AML_OBJECT_NODE *)Node;

    Index = 0;
    // Copy the opcode(s).
    ObjectNodeInfoArray[Index++] = ObjectNode->AmlByteEncoding->OpCode;
    if (ObjectNode->AmlByteEncoding->OpCode == AML_EXT_OP) {
      ObjectNodeInfoArray[Index++] = ObjectNode->AmlByteEncoding->SubOpCode;
    }

    // Copy the PkgLen.
    if (AmlNodeHasAttribute (ObjectNode, AML_HAS_PKG_LENGTH)) {
      Index += AmlSetPkgLength (
                 ObjectNode->PkgLen,
                 &ObjectNodeInfoArray[Index]
                 );
    }

    Status1 = AmlStreamWrite (
                FStream,
                ObjectNodeInfoArray,
                Index
                );
    if (EFI_ERROR (Status1)) {
      ASSERT (0);
      ContinueEnum = FALSE;
      goto error_handler;
    }
  } // IS_AML_OBJECT_NODE (Node)

error_handler:
  if (Status != NULL) {
    *Status = Status1;
  }

  return ContinueEnum;
}

/** Serialize a tree to create an ACPI DSDT/SSDT table.

  If:
   - the content of BufferSize is >= to the size needed to serialize the
     definition block;
   - Buffer is not NULL;
  first serialize the ACPI DSDT/SSDT header from the root node,
  then serialize the AML blob from the rest of the tree.

  The content of BufferSize is always updated to the size needed to
  serialize the definition block.

  @param  [in]      RootNode    Pointer to a root node.
  @param  [in]      Buffer      Buffer to write the DSDT/SSDT table to.
                                If Buffer is NULL, the size needed to
                                serialize the DSDT/SSDT table is returned
                                in BufferSize.
  @param  [in, out] BufferSize  Pointer holding the size of the Buffer.
                                Its content is always updated to the size
                                needed to serialize the DSDT/SSDT table.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
**/
EFI_STATUS
EFIAPI
AmlSerializeTree (
  IN      AML_ROOT_NODE  *RootNode,
  IN      UINT8          *Buffer      OPTIONAL,
  IN  OUT UINT32         *BufferSize
  )
{
  EFI_STATUS  Status;
  AML_STREAM  FStream;
  UINT32      TableSize;

  if (!IS_AML_ROOT_NODE (RootNode) ||
      (BufferSize == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Compute the total size of the AML blob.
  Status = AmlComputeSize (
             (CONST AML_NODE_HEADER *)RootNode,
             &TableSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the size of the ACPI header.
  TableSize += (UINT32)sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  // Check the size against the SDT header.
  // The Length field in the SDT Header is updated if the tree has
  // been modified.
  if (TableSize != RootNode->SdtHeader->Length) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Buffer is not big enough, or NULL.
  if ((TableSize < *BufferSize) || (Buffer == NULL)) {
    *BufferSize = TableSize;
    return EFI_SUCCESS;
  }

  // Initialize the stream to the TableSize that is needed.
  Status = AmlStreamInit (
             &FStream,
             Buffer,
             TableSize,
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Serialize the header.
  Status = AmlStreamWrite (
             &FStream,
             (UINT8 *)RootNode->SdtHeader,
             sizeof (EFI_ACPI_DESCRIPTION_HEADER)
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = EFI_SUCCESS;
  AmlEnumTree (
    (AML_NODE_HEADER *)RootNode,
    AmlSerializeNodeCallback,
    (VOID *)&FStream,
    &Status
    );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Update the checksum.
  return AcpiPlatformChecksum ((EFI_ACPI_DESCRIPTION_HEADER *)Buffer);
}

/** Serialize an AML definition block.

  This functions allocates memory with the "AllocateZeroPool ()"
  function. This memory is used to serialize the AML tree and is
  returned in the Table.

  @param [in]  RootNode         Root node of the tree.
  @param [out] Table            On return, hold the serialized
                                definition block.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlSerializeDefinitionBlock (
  IN  AML_ROOT_NODE                *RootNode,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  EFI_STATUS  Status;
  UINT8       *TableBuffer;
  UINT32      TableSize;

  if (!IS_AML_ROOT_NODE (RootNode) ||
      (Table == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *Table      = NULL;
  TableBuffer = NULL;
  TableSize   = 0;

  // Get the size of the SSDT table.
  Status = AmlSerializeTree (
             RootNode,
             TableBuffer,
             &TableSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  TableBuffer = (UINT8 *)AllocateZeroPool (TableSize);
  if (TableBuffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to allocate memory for Table Buffer."
      ));
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Serialize the tree to a SSDT table.
  Status = AmlSerializeTree (
             RootNode,
             TableBuffer,
             &TableSize
             );
  if (EFI_ERROR (Status)) {
    FreePool (TableBuffer);
    ASSERT (0);
  } else {
    // Save the allocated Table buffer in the table list
    *Table = (EFI_ACPI_DESCRIPTION_HEADER *)TableBuffer;
  }

  return Status;
}
