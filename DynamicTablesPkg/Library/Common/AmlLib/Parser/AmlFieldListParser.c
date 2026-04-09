/** @file
  AML Field List Parser.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Parser/AmlFieldListParser.h>

#include <AmlCoreInterface.h>
#include <AmlDbgPrint/AmlDbgPrint.h>
#include <Parser/AmlMethodParser.h>
#include <Parser/AmlParser.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>

/** Parse a field element.

  The field elements this function can parse are one of:
   - ReservedField;
   - AccessField;
   - ConnectField;
   - ExtendedAccessField.
  Indeed, the NamedField field element doesn't have an OpCode. Thus it needs
  to be parsed differently.

  @param  [in]      FieldByteEncoding       Field byte encoding to parse.
  @param  [in, out] FieldNode               Field node to attach the field
                                            element to.
                                            Must have the AML_HAS_FIELD_LIST
                                            attribute.
  @param  [in, out] FStream                 Forward stream pointing to a field
                                            element not being a named field.
                                            The stream must not be at its end.
  @param  [in, out] NameSpaceRefList        List of namespace reference nodes,
                                            allowing to associate an absolute
                                            path to a node in the tree.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseFieldElement (
  IN      CONST AML_BYTE_ENCODING  *FieldByteEncoding,
  IN  OUT       AML_OBJECT_NODE    *FieldNode,
  IN  OUT       AML_STREAM         *FStream,
  IN  OUT       LIST_ENTRY         *NameSpaceRefList
  )
{
  EFI_STATUS  Status;

  UINT8            *CurrPos;
  AML_OBJECT_NODE  *NewNode;

  UINT32  PkgLenOffset;
  UINT32  PkgLenSize;

  // Check whether the node is an Object Node and has a field list.
  // The byte encoding must be a field element.
  if ((FieldByteEncoding == NULL)                                   ||
      ((FieldByteEncoding->Attribute & AML_IS_FIELD_ELEMENT) == 0)  ||
      ((FieldByteEncoding->Attribute & AML_IS_PSEUDO_OPCODE) ==
       AML_IS_PSEUDO_OPCODE)                                     ||
      !AmlNodeHasAttribute (FieldNode, AML_HAS_FIELD_LIST)          ||
      !IS_STREAM (FStream)                                          ||
      IS_END_OF_STREAM (FStream)                                    ||
      !IS_STREAM_FORWARD (FStream)                                  ||
      (NameSpaceRefList == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurrPos = AmlStreamGetCurrPos (FStream);
  if (CurrPos == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Skip the field opcode (1 byte) as it is already in the FieldByteEncoding.
  AMLDBG_DUMP_RAW (CurrPos, 1);
  Status = AmlStreamProgress (FStream, 1);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  CurrPos = AmlStreamGetCurrPos (FStream);
  if (CurrPos == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Parse the PkgLen if available.
  PkgLenSize = 0;
  if ((FieldByteEncoding->Attribute & AML_HAS_PKG_LENGTH) ==
      AML_HAS_PKG_LENGTH)
  {
    PkgLenOffset = AmlGetPkgLength (CurrPos, &PkgLenSize);
    if (PkgLenOffset == 0) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    // Move stream forward as the PkgLen has been read.
    AMLDBG_DUMP_RAW (CurrPos, PkgLenOffset);
    Status = AmlStreamProgress (FStream, PkgLenOffset);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Update the current position as PkgLen has been parsed.
    CurrPos = AmlStreamGetCurrPos (FStream);
  }

  Status = AmlCreateObjectNode (
             FieldByteEncoding,
             PkgLenSize,
             &NewNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the FieldElement to the Variable Argument List.
  Status = AmlVarListAddTailInternal (
             (AML_NODE_HEADER *)FieldNode,
             (AML_NODE_HEADER *)NewNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    // Delete the sub-tree if the insertion failed.
    // Otherwise its reference will be lost.
    AmlDeleteTree ((AML_NODE_HEADER *)NewNode);
    return Status;
  }

  // Some field elements do not have fixed arguments.
  if (!IS_END_OF_STREAM (FStream)) {
    // Parse the fixed arguments of the field element.
    Status = AmlParseFixedArguments (
               NewNode,
               FStream,
               NameSpaceRefList
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  }

  return Status;
}

/** Parse a named field element.

  Indeed, the NamedField field element doesn't have an OpCode. Thus it needs
  to be parsed differently. NamedField field element start with a char.

  @param  [in]      NamedFieldByteEncoding  Field byte encoding to parse.
  @param  [in, out] FieldNode               Field node to attach the field
                                            element to.
                                            Must have the AML_HAS_FIELD_LIST
                                            attribute.
  @param  [in, out] FStream                 Forward stream pointing to a named
                                            field element.
                                            The stream must not be at its end.
  @param  [in, out] NameSpaceRefList        List of namespace reference nodes,
                                            allowing to associate an absolute
                                            path to a node in the tree.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseNamedFieldElement (
  IN      CONST AML_BYTE_ENCODING  *NamedFieldByteEncoding,
  IN  OUT       AML_OBJECT_NODE    *FieldNode,
  IN  OUT       AML_STREAM         *FStream,
  IN  OUT       LIST_ENTRY         *NameSpaceRefList
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *NewNode;

  // Check whether the node is an Object Node and has a field list.
  // The byte encoding must be a char.
  if ((NamedFieldByteEncoding == NULL)                              ||
      ((NamedFieldByteEncoding->Attribute & AML_IS_NAME_CHAR) == 0) ||
      !AmlNodeHasAttribute (FieldNode, AML_HAS_FIELD_LIST)          ||
      !IS_STREAM (FStream)                                          ||
      IS_END_OF_STREAM (FStream)                                    ||
      !IS_STREAM_FORWARD (FStream)                                  ||
      (NameSpaceRefList == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Create a NamedField node.
  Status = AmlCreateObjectNode (
             AmlGetFieldEncodingByOpCode (AML_FIELD_NAMED_OP, 0),
             0,
             &NewNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the NamedField node to the variable argument list.
  Status = AmlVarListAddTailInternal (
             (AML_NODE_HEADER *)FieldNode,
             (AML_NODE_HEADER *)NewNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    // Delete the sub-tree if the insertion failed.
    // Otherwise its reference will be lost.
    AmlDeleteTree ((AML_NODE_HEADER *)NewNode);
    return Status;
  }

  // Parse the fixed arguments: [0]NameSeg, [1]PkgLen.
  Status = AmlParseFixedArguments (
             NewNode,
             FStream,
             NameSpaceRefList
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the NamedField to the namespace reference list.
  Status = AmlAddNameSpaceReference (
             NewNode,
             NameSpaceRefList
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Parse the FieldList contained in the stream.

  Create an object node for each field element parsed in the field list
  available in the Stream, and add them to the variable list of arguments
  of the FieldNode.

  Nodes that can have a field list are referred as field nodes. They have the
  AML_HAS_FIELD_LIST attribute.

  According to the ACPI 6.3 specification, s20.2.5.2 "Named Objects Encoding",
  field elements can be:
   - NamedField           := NameSeg PkgLength;
   - ReservedField        := 0x00 PkgLength;
   - AccessField          := 0x01 AccessType AccessAttrib;
   - ConnectField         := <0x02 NameString> | <0x02 BufferData>;
   - ExtendedAccessField  := 0x03 AccessType ExtendedAccessAttrib AccessLength.

  A small set of opcodes describes the field elements. They are referred as
  field opcodes. An AML_BYTE_ENCODING table has been created for field OpCodes.
  Field elements:
   - don't have a SubOpCode;
   - have at most 3 fixed arguments (as opposed to 6 for standard AML objects);
   - don't have a variable list of arguments;
   - only the NamedField field element is part of the AML namespace.

  ConnectField's BufferData is a buffer node containing a single
  resource data element.
  NamedField field elements don't have an AML OpCode. NameSeg starts with a
  Char type and can thus be differentiated from the Opcodes for other fields.
  A pseudo OpCode has been created to simplify the parser.

  The branch created from parsing a field node is as:
  (FieldNode)
      \
       |- [FixedArg[0]][FixedArg[1]]                      # Fixed Arguments
       |- {(FieldElement[0])->(FieldElement[1])->...)}    # Variable Arguments

  With FieldElement[n] being one of NamedField, ReservedField, AccessField,
  ConnectField, ExtendedAccessField.

  @param  [in]  FieldNode         Field node.
                                  Must have the AML_HAS_FIELD_LIST
                                  attribute.
  @param  [in]  FStream           Forward stream pointing to a field list.
                                  The stream must not be at its end.
  @param  [in]  NameSpaceRefList  List of namespace reference nodes,
                                  allowing to associate an absolute
                                  path to a node in the tree.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseFieldList (
  IN  AML_OBJECT_NODE  *FieldNode,
  IN  AML_STREAM       *FStream,
  IN  LIST_ENTRY       *NameSpaceRefList
  )
{
  EFI_STATUS  Status;

  UINT8                    *CurrPos;
  CONST AML_BYTE_ENCODING  *FieldByteEncoding;
  CONST AML_BYTE_ENCODING  *NamedFieldByteEncoding;

  // Check whether the node is an Object Node and has a field list.
  if (!AmlNodeHasAttribute (FieldNode, AML_HAS_FIELD_LIST)  ||
      !IS_STREAM (FStream)                                  ||
      IS_END_OF_STREAM (FStream)                            ||
      !IS_STREAM_FORWARD (FStream)                          ||
      (NameSpaceRefList == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Iterate through the field elements, creating nodes
  // and adding them to the variable list of elements of Node.
  while (!IS_END_OF_STREAM (FStream)) {
    CurrPos = AmlStreamGetCurrPos (FStream);

    // Check for a field opcode.
    FieldByteEncoding = AmlGetFieldEncoding (CurrPos);
    if (FieldByteEncoding != NULL) {
      Status = AmlParseFieldElement (
                 FieldByteEncoding,
                 FieldNode,
                 FStream,
                 NameSpaceRefList
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    } else {
      // Handle the case of Pseudo OpCodes.
      // NamedField has a Pseudo OpCode and starts with a NameChar. Therefore,
      // call AmlGetByteEncoding() to check that the encoding is NameChar.
      NamedFieldByteEncoding = AmlGetByteEncoding (CurrPos);
      if ((NamedFieldByteEncoding != NULL) &&
          (NamedFieldByteEncoding->Attribute & AML_IS_NAME_CHAR))
      {
        // This is a NamedField field element since it is starting with a char.
        Status = AmlParseNamedFieldElement (
                   NamedFieldByteEncoding,
                   FieldNode,
                   FStream,
                   NameSpaceRefList
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }
      } else {
        // A field opcode or an AML byte encoding is expected.
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
    }
  } // while

  return EFI_SUCCESS;
}
