/** @file
  AML Print Function.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>
#include <AmlDbgPrint/AmlDbgPrint.h>

#include <AmlCoreInterface.h>
#include <String/AmlString.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTreeTraversal.h>

#if !defined (MDEPKG_NDEBUG)

/** String table representing AML Data types as defined by EAML_NODE_DATA_TYPE.
*/
CONST CHAR8 * NodeDataTypeStrTbl[] = {
  "EAmlNodeDataTypeNone",
  "EAmlNodeDataTypeReserved1",
  "EAmlNodeDataTypeReserved2",
  "EAmlNodeDataTypeReserved3",
  "EAmlNodeDataTypeReserved4",
  "EAmlNodeDataTypeReserved5",
  "EAmlNodeDataTypeNameString",
  "EAmlNodeDataTypeString",
  "EAmlNodeDataTypeUInt",
  "EAmlNodeDataTypeRaw",
  "EAmlNodeDataTypeResourceData",
  "EAmlNodeDataTypeFieldPkgLen",
  "EAmlNodeDataTypeMax"
};

/** String table representing AML Node types as defined by EAML_NODE_TYPE.
*/
CONST CHAR8 * NodeTypeStrTbl[] = {
  "EAmlNodeUnknown",
  "EAmlNodeRoot",
  "EAmlNodeObject",
  "EAmlNodeData",
  "EAmlNodeMax"
};

/** Print Size chars at Buffer address.

  @param  [in]  ErrorLevel    Error level for the DEBUG macro.
  @param  [in]  Buffer        Buffer containing the chars.
  @param  [in]  Size          Number of chars to print.
**/
VOID
EFIAPI
AmlDbgPrintChars (
  IN        UINT32      ErrorLevel,
  IN  CONST CHAR8     * Buffer,
  IN        UINT32      Size
  )
{
  UINT32  i;

  if (Buffer == NULL) {
    ASSERT (0);
    return;
  }

  for (i = 0; i < Size; i++) {
    DEBUG ((ErrorLevel, "%c", Buffer[i]));
  }
}

/** Print an AML NameSeg.
    Don't print trailing underscores ('_').

  @param  [in] Buffer   Buffer containing an AML NameSeg.
**/
VOID
EFIAPI
AmlDbgPrintNameSeg (
  IN  CONST CHAR8   * Buffer
  )
{
  if (Buffer == NULL) {
    ASSERT (0);
    return;
  }

  DEBUG ((DEBUG_INFO, "%c", Buffer[0]));
  if ((Buffer[1] == AML_NAME_CHAR__)  &&
      (Buffer[2] == AML_NAME_CHAR__)  &&
      (Buffer[3] == AML_NAME_CHAR__)) {
    return;
  }
  DEBUG ((DEBUG_INFO, "%c", Buffer[1]));
  if ((Buffer[2] == AML_NAME_CHAR__)  &&
      (Buffer[3] == AML_NAME_CHAR__)) {
    return;
  }
  DEBUG ((DEBUG_INFO, "%c", Buffer[2]));
  if (Buffer[3] == AML_NAME_CHAR__) {
    return;
  }
  DEBUG ((DEBUG_INFO, "%c", Buffer[3]));
  return;
}

/** Print an AML NameString.

  @param  [in] Buffer   Buffer containing an AML NameString.
  @param  [in] NewLine  Print a newline char at the end of the NameString.
**/
VOID
EFIAPI
AmlDbgPrintNameString (
  IN  CONST CHAR8   * Buffer,
  IN        BOOLEAN   NewLine
  )
{
  UINT8     SegCount;
  UINT8     Index;

  if (Buffer == NULL) {
    ASSERT (0);
    return;
  }

  // Handle Root and Parent(s).
  if (*Buffer == AML_ROOT_CHAR) {
    Buffer++;
    DEBUG ((DEBUG_INFO, "\\"));
  } else if (*Buffer == AML_PARENT_PREFIX_CHAR) {
    do {
      Buffer++;
      DEBUG ((DEBUG_INFO, "^"));
    } while (*Buffer == AML_PARENT_PREFIX_CHAR);
  }

  // Handle SegCount(s).
  if (*Buffer == AML_DUAL_NAME_PREFIX) {
    Buffer++;
    SegCount = 2;
  } else if (*Buffer == AML_MULTI_NAME_PREFIX) {
    Buffer++;
    // For multi name prefix the seg count is in the second byte.
    SegCount = *Buffer;
    Buffer++;
  } else if (AmlIsLeadNameChar (*Buffer)) {
    // Only check the first char first to avoid overflow.
    // Then the whole NameSeg can be checked.
    if (!AmlIsNameSeg (Buffer)) {
      ASSERT (0);
      return;
    }
    SegCount = 1;
  } else if (*Buffer == AML_ZERO_OP) {
    SegCount = 0;
  } else {
    // Should not be possible.
    ASSERT (0);
    return;
  }

  if (SegCount != 0) {
    AMLDBG_PRINT_NAMESEG (Buffer);
    Buffer += AML_NAME_SEG_SIZE;
    for (Index = 0; Index < SegCount - 1; Index++) {
      DEBUG ((DEBUG_INFO, "."));
      AMLDBG_PRINT_NAMESEG (Buffer);
      Buffer += AML_NAME_SEG_SIZE;
    }
  }

  if (NewLine) {
    DEBUG ((DEBUG_INFO, "\n"));
  }

  return;
}

/** Print the information contained in the header of the Node.

  @param  [in]  Node    Pointer to a node.
  @param  [in]  Level   Level of the indentation.
**/
STATIC
VOID
EFIAPI
AmlDbgPrintNodeHeader (
  IN  AML_NODE_HEADER  * Node,
  IN  UINT8              Level
  )
{
  if (!IS_AML_NODE_VALID (Node)) {
    ASSERT (0);
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%3d | %-15a | ",
    Level,
    NodeTypeStrTbl[Node->NodeType]
    ));
}

/** Print fields of a data node.

  @param  [in]  DataNode  Pointer to a data node.
  @param  [in]  Level     Level of the indentation.
**/
STATIC
VOID
EFIAPI
AmlDbgPrintDataNode (
  IN  AML_DATA_NODE   * DataNode,
  IN  UINT8             Level
  )
{
  UINT32  Idx;

  if (!IS_AML_DATA_NODE (DataNode)) {
    ASSERT (0);
    return;
  }

  AmlDbgPrintNodeHeader ((AML_NODE_HEADER*)DataNode, Level);

  DEBUG ((DEBUG_INFO, "%-36a | ", NodeDataTypeStrTbl[DataNode->DataType]));
  DEBUG ((DEBUG_INFO, "0x%04x | ", DataNode->Size));

  if ((DataNode->DataType == EAmlNodeDataTypeNameString) ||
      (DataNode->DataType == EAmlNodeDataTypeString)) {
    AMLDBG_PRINT_CHARS (
      DEBUG_INFO,
      (CONST CHAR8*)DataNode->Buffer,
      DataNode->Size
      );
  } else if (DataNode->DataType == EAmlNodeDataTypeUInt) {
    switch (DataNode->Size) {
      case 1:
      {
        DEBUG ((DEBUG_INFO, "0x%0x", *((UINT8*)DataNode->Buffer)));
        break;
      }
      case 2:
      {
        DEBUG ((DEBUG_INFO, "0x%0x", *((UINT16*)DataNode->Buffer)));
        break;
      }
      case 4:
      {
        DEBUG ((DEBUG_INFO, "0x%0lx", *((UINT32*)DataNode->Buffer)));
        break;
      }
      case 8:
      {
        DEBUG ((DEBUG_INFO, "0x%0llx", *((UINT64*)DataNode->Buffer)));
        break;
      }
      default:
      {
        ASSERT (0);
        return;
      }
    }
  } else {
    // No specific format.
    for (Idx = 0; Idx < DataNode->Size; Idx++) {
      DEBUG ((DEBUG_INFO, "%02x ", DataNode->Buffer[Idx]));
    }
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/** Print fields of an object node.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  Level       Level of the indentation.
**/
STATIC
VOID
EFIAPI
AmlDbgPrintObjectNode (
  IN  AML_OBJECT_NODE  * ObjectNode,
  IN  UINT8              Level
  )
{
  if (!IS_AML_OBJECT_NODE (ObjectNode)) {
    ASSERT (0);
    return;
  }

  AmlDbgPrintNodeHeader ((AML_NODE_HEADER*)ObjectNode, Level);

  DEBUG ((DEBUG_INFO, "0x%02x | ", ObjectNode->AmlByteEncoding->OpCode));
  DEBUG ((DEBUG_INFO, "0x%02x | ", ObjectNode->AmlByteEncoding->SubOpCode));

  // Print a string corresponding to the field object OpCode/SubOpCode.
  if (AmlNodeHasAttribute (ObjectNode, AML_IS_FIELD_ELEMENT)) {
    DEBUG ((DEBUG_INFO, "%-15a ", AmlGetFieldOpCodeStr (
                                    ObjectNode->AmlByteEncoding->OpCode,
                                    0
                                    )));
  } else {
    // Print a string corresponding to the object OpCode/SubOpCode.
    DEBUG ((DEBUG_INFO, "%-15a | ", AmlGetOpCodeStr (
                                      ObjectNode->AmlByteEncoding->OpCode,
                                      ObjectNode->AmlByteEncoding->SubOpCode)
                                      ));
  }

  DEBUG ((DEBUG_INFO, "%3d | ", ObjectNode->AmlByteEncoding->MaxIndex));
  DEBUG ((DEBUG_INFO, "0x%08x | ", ObjectNode->AmlByteEncoding->Attribute));
  DEBUG ((DEBUG_INFO, "0x%04x | ", ObjectNode->PkgLen));
  if (AmlNodeHasAttribute (ObjectNode, AML_IN_NAMESPACE)) {
    AMLDBG_PRINT_NAMESTR (
      AmlNodeGetName ((CONST AML_OBJECT_NODE*)ObjectNode),
      FALSE
      );
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/** Print fields of a root node.

  @param  [in]  RootNode  Pointer to a root node.
  @param  [in]  Level     Level of the indentation.
**/
STATIC
VOID
EFIAPI
AmlDbgPrintRootNode (
  IN  AML_ROOT_NODE  * RootNode,
  IN  UINT8            Level
  )
{
  if (!IS_AML_ROOT_NODE (RootNode)) {
    ASSERT (0);
    return;
  }

  AmlDbgPrintNodeHeader ((AML_NODE_HEADER*)RootNode, Level);

  DEBUG ((DEBUG_INFO, "%8x | ", RootNode->SdtHeader->Signature));
  DEBUG ((DEBUG_INFO, "0x%08x | ", RootNode->SdtHeader->Length));
  DEBUG ((DEBUG_INFO, "%3d | ", RootNode->SdtHeader->Revision));
  DEBUG ((DEBUG_INFO, "0x%02x | ", RootNode->SdtHeader->Checksum));
  DEBUG ((
    DEBUG_INFO,
    "%c%c%c%c%c%c | ",
    RootNode->SdtHeader->OemId[0],
    RootNode->SdtHeader->OemId[1],
    RootNode->SdtHeader->OemId[2],
    RootNode->SdtHeader->OemId[3],
    RootNode->SdtHeader->OemId[4],
    RootNode->SdtHeader->OemId[5]
    ));
  DEBUG ((DEBUG_INFO, "%-16llx | ", RootNode->SdtHeader->OemTableId));
  DEBUG ((DEBUG_INFO, "%8x | ", RootNode->SdtHeader->OemRevision));
  DEBUG ((DEBUG_INFO, "%8x | ", RootNode->SdtHeader->CreatorId));
  DEBUG ((DEBUG_INFO, "%8x", RootNode->SdtHeader->CreatorRevision));
  DEBUG ((DEBUG_INFO, "\n"));
}

/** Print a header to help interpreting node information.
**/
STATIC
VOID
EFIAPI
AmlDbgPrintTableHeader (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "Lvl | Node Type       |\n"));
  DEBUG ((
    DEBUG_INFO,
    "    | %-15a | Signature| Length     | Rev | CSum | OemId  | "
      "OemTableId       | OemRev   | CreatorId| CreatorRev\n",
    NodeTypeStrTbl[EAmlNodeRoot]
    ));
  DEBUG ((
    DEBUG_INFO,
    "    | %-15a | Op   | SubOp| OpName          | MaxI| Attribute  | "
      "PkgLen | NodeName (opt)\n",
    NodeTypeStrTbl[EAmlNodeObject]
    ));
  DEBUG ((
    DEBUG_INFO,
    "    | %-15a | Data Type                            | Size   | "
      "Buffer\n",
    NodeTypeStrTbl[EAmlNodeData]
    ));
  DEBUG ((
    DEBUG_INFO,
    "---------------------------------------"
      "---------------------------------------\n"
    ));
}

/** Recursively print the subtree under the Node.
    This is an internal function.

  @param  [in]  Node            Pointer to the root of the subtree to print.
                                Can be a root/object/data node.
  @param  [in]  Recurse         If TRUE, recurse.
  @param  [in]  Level           Level in the tree.
**/
STATIC
VOID
EFIAPI
AmlDbgPrintTreeInternal (
  IN  AML_NODE_HEADER   * Node,
  IN  BOOLEAN             Recurse,
  IN  UINT8               Level
  )
{
  AML_NODE_HEADER   * ChildNode;

  if (!IS_AML_NODE_VALID (Node)) {
    ASSERT (0);
    return;
  }

  if (IS_AML_DATA_NODE (Node)) {
    AmlDbgPrintDataNode ((AML_DATA_NODE*)Node, Level);
    return;
  } else if (IS_AML_OBJECT_NODE (Node)) {
    AmlDbgPrintObjectNode ((AML_OBJECT_NODE*)Node, Level);
  } else if (IS_AML_ROOT_NODE (Node)) {
    AmlDbgPrintRootNode ((AML_ROOT_NODE*)Node, Level);
  } else {
    // Should not be possible.
    ASSERT (0);
    return;
  }

  if (!Recurse) {
    return;
  }

  // Get the first child node.
  ChildNode = AmlGetNextSibling (Node, NULL);
  while (ChildNode != NULL) {
    ASSERT (Level < MAX_UINT8);
    AmlDbgPrintTreeInternal (ChildNode, Recurse, (UINT8)(Level + 1));
    ChildNode = AmlGetNextSibling (Node, ChildNode);
  }
}

/** Print Node information.

  @param  [in]  Node    Pointer to the Node to print.
                        Can be a root/object/data node.
**/
VOID
EFIAPI
AmlDbgPrintNode (
  IN  AML_NODE_HEADER   * Node
  )
{
  AmlDbgPrintTableHeader ();
  AmlDbgPrintTreeInternal (Node, FALSE, 0);
}

/** Recursively print the subtree under the Node.

  @param  [in]  Node    Pointer to the root of the subtree to print.
                        Can be a root/object/data node.
**/
VOID
EFIAPI
AmlDbgPrintTree (
  IN  AML_NODE_HEADER   * Node
  )
{
  AmlDbgPrintTableHeader ();
  AmlDbgPrintTreeInternal (Node, TRUE, 0);
}

/** This function performs a raw data dump of the ACPI table.

  @param  [in]  Ptr     Pointer to the start of the table buffer.
  @param  [in]  Length  The length of the buffer.
**/
VOID
EFIAPI
AmlDbgDumpRaw (
  IN  CONST UINT8   * Ptr,
  IN        UINT32    Length
  )
{
  UINT32  ByteCount;
  UINT32  PartLineChars;
  UINT32  AsciiBufferIndex;
  CHAR8   AsciiBuffer[17];

  ByteCount = 0;
  AsciiBufferIndex = 0;

  DEBUG ((DEBUG_VERBOSE, "Address  : 0x%p\n", Ptr));
  DEBUG ((DEBUG_VERBOSE, "Length   : %lld", Length));

  while (ByteCount < Length) {
    if ((ByteCount & 0x0F) == 0) {
      AsciiBuffer[AsciiBufferIndex] = '\0';
      DEBUG ((DEBUG_VERBOSE, "  %a\n%08X : ", AsciiBuffer, ByteCount));
      AsciiBufferIndex = 0;
    } else if ((ByteCount & 0x07) == 0) {
      DEBUG ((DEBUG_VERBOSE, "- "));
    }

    if ((*Ptr >= ' ') && (*Ptr < 0x7F)) {
      AsciiBuffer[AsciiBufferIndex++] = *Ptr;
    } else {
      AsciiBuffer[AsciiBufferIndex++] = '.';
    }

    DEBUG ((DEBUG_VERBOSE, "%02X ", *Ptr++));

    ByteCount++;
  }

  // Justify the final line using spaces before printing
  // the ASCII data.
  PartLineChars = (Length & 0x0F);
  if (PartLineChars != 0) {
    PartLineChars = 48 - (PartLineChars * 3);
    if ((Length & 0x0F) <= 8) {
      PartLineChars += 2;
    }
    while (PartLineChars > 0) {
      DEBUG ((DEBUG_VERBOSE, " "));
      PartLineChars--;
    }
  }

  // Print ASCII data for the final line.
  AsciiBuffer[AsciiBufferIndex] = '\0';
  DEBUG ((DEBUG_VERBOSE, "  %a\n\n", AsciiBuffer));
}

#endif // MDEPKG_NDEBUG
