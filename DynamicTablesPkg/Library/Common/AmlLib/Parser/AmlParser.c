/** @file
  AML Parser.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Parser/AmlParser.h>

#include <AmlCoreInterface.h>
#include <AmlDbgPrint/AmlDbgPrint.h>
#include <Parser/AmlFieldListParser.h>
#include <Parser/AmlMethodParser.h>
#include <Parser/AmlResourceDataParser.h>
#include <String/AmlString.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>

/*
  AML Tree
  --------

  Each ASL Statement is represented in AML as and ObjectNode.
  Each ObjectNode has an Opcode and has up to six FixedArguments
  followed by a list of VariableArguments.
  (ObjectNode)
    \
    |- [0][1][2][3][4][5]                        # Fixed Arguments
    |- {(VarArg1)->(VarArg2)->(VarArg3)->...N}   # Variable Arguments

  A RootNode is a special type of Object Node that does not have an
  Opcode or Fixed Arguments. It only has a list of VariableArguments
  (RootNode)
    \
    |- {(VarArg1)->(VarArg2)->(VarArg3)->...N}   # Variable Arguments

  A DataNode consists of a data buffer.

  A FixedArgument or VariableArgument can be either an ObjectNode or
  a DataNode.

  Example:
  ASL code sample:
  Device (DEV0) {
    Name (VAR0, 0x6)
  }

  Tree generated from the ASL code:
  (RootNode)
    \
    |- {(Device statement (ObjectNode))}                # Variable Arg of the
          \                                             #   RootNode
           |
           |- [0] - Device Name (DataNode)(="DEV0")     # Fixed Arg0 of the
           |                                            #   Device() statement
           |
           |- {(Name statement (ObjectNode))}           # Variable Arg of the
                \                                       #   Device() statement
                |
                |- [0] - Name statement(DataNode)(="VAR0")  # Fixed Arg0 of the
                |                                           #   Name() statement
                |- [1] - Value(DataNode)(=0x6)              # Fixed Arg1 of the
                                                            #   Name() statement
*/

// Forward declaration.
STATIC
EFI_STATUS
EFIAPI
AmlParseStream (
  IN      AML_NODE_HEADER   * Node,
  IN  OUT AML_STREAM        * FStream,
  IN  OUT LIST_ENTRY        * NameSpaceRefList
  );

/** Function pointer to parse an AML construct.

  The expected format of the AML construct is passed in the
  ExpectedFormat argument. The available formats are available in
  the AML_PARSE_FORMAT enum definition.

  An object node or a data node is created in the function,
  and returned through the OutNode parameter. This node should
  be attached after this function returns.

  @param  [in]      ParentNode      Parent node to which the parsed
                                    AML construct will be attached.
  @param  [in]      ExpectedFormat  Format of the AML construct to parse.
  @param  [in, out] FStream         Forward stream containing the AML bytecode
                                    to parse.
                                    The stream must not be at its end.
  @param  [out]     OutNode         Pointer holding the node created from the
                                    parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
typedef
EFI_STATUS
EFIAPI
(*AML_PARSE_FUNCTION) (
  IN      CONST AML_NODE_HEADER     * Node,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
      OUT       AML_NODE_HEADER    ** OutNode
  );

/** Parse a UInt<X> (where X=8, 16, 32 or 64).

  A data node is created and returned through the OutNode parameter.

  @param  [in]      ParentNode      Parent node to which the parsed
                                    AML construct will be attached.
  @param  [in]      ExpectedFormat  Format of the AML construct to parse.
  @param  [in, out] FStream         Forward stream containing the AML bytecode
                                    to parse.
                                    The stream must not be at its end.
  @param  [out]     OutNode         Pointer holding the node created from the
                                    parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseUIntX (
  IN      CONST AML_NODE_HEADER     * ParentNode,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
      OUT       AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS    Status;
  UINT32        UIntXSize;

  if ((!IS_AML_ROOT_NODE (ParentNode)       &&
       !IS_AML_OBJECT_NODE (ParentNode))    ||
      ((ExpectedFormat != EAmlUInt8)        &&
       (ExpectedFormat != EAmlUInt16)       &&
       (ExpectedFormat != EAmlUInt32)       &&
       (ExpectedFormat != EAmlUInt64))      ||
      !IS_STREAM (FStream)                  ||
      IS_END_OF_STREAM (FStream)            ||
      !IS_STREAM_FORWARD (FStream)          ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  switch (ExpectedFormat) {
  case EAmlUInt8:
    UIntXSize = 1;
    break;
  case EAmlUInt16:
    UIntXSize = 2;
    break;
  case EAmlUInt32:
    UIntXSize = 4;
    break;
  case EAmlUInt64:
    UIntXSize = 8;
    break;
  default:
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCreateDataNode (
             AmlTypeToNodeDataType (ExpectedFormat),
             AmlStreamGetCurrPos (FStream),
             UIntXSize,
             (AML_DATA_NODE**)OutNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  AMLDBG_DUMP_RAW (AmlStreamGetCurrPos (FStream), UIntXSize);

  // Move stream forward by the size of UIntX.
  Status = AmlStreamProgress (FStream, UIntXSize);
  if (EFI_ERROR (Status)) {
    AmlDeleteTree (*OutNode);
    ASSERT (0);
  }

  return Status;
}

/** Parse an AML NameString.

  A data node is created and returned through the OutNode parameter.

  @param  [in]      ParentNode      Parent node to which the parsed
                                    AML construct will be attached.
  @param  [in]      ExpectedFormat  Format of the AML construct to parse.
  @param  [in, out] FStream         Forward stream containing the AML bytecode
                                    to parse.
                                    The stream must not be at its end.
  @param  [out]     OutNode         Pointer holding the node created from the
                                    parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseNameString (
  IN      CONST AML_NODE_HEADER     * ParentNode,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
      OUT       AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS                  Status;

  CONST UINT8               * Buffer;
  CONST AML_BYTE_ENCODING   * ByteEncoding;
  UINT32                      StrSize;

  if ((!IS_AML_ROOT_NODE (ParentNode)     &&
       !IS_AML_OBJECT_NODE (ParentNode))  ||
      (ExpectedFormat != EAmlName)        ||
      !IS_STREAM (FStream)                ||
      IS_END_OF_STREAM (FStream)          ||
      !IS_STREAM_FORWARD (FStream)        ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (CONST UINT8*)AmlStreamGetCurrPos (FStream);
  ByteEncoding = AmlGetByteEncoding (Buffer);
  if ((ByteEncoding == NULL)    ||
      ((ByteEncoding->Attribute & AML_IS_NAME_CHAR) == 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Parse the NameString.
  Status = AmlGetNameStringSize ((CONST CHAR8*)Buffer, &StrSize);
  if ((EFI_ERROR (Status))  ||
      (StrSize > AmlStreamGetFreeSpace (FStream))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeNameString,
             Buffer,
             StrSize,
             (AML_DATA_NODE**)OutNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  AMLDBG_DUMP_RAW (AmlStreamGetCurrPos (FStream), StrSize);

  // Move the stream forward by StrSize.
  Status = AmlStreamProgress (FStream, StrSize);
  if (EFI_ERROR (Status)) {
    AmlDeleteTree (*OutNode);
    ASSERT (0);
  }

  return Status;
}

/** Parse an AML String.

  A data node is created and returned through the OutNode parameter.

  @param  [in]      ParentNode      Parent node to which the parsed
                                    AML construct will be attached.
  @param  [in]      ExpectedFormat  Format of the AML construct to parse.
  @param  [in, out] FStream         Forward stream containing the AML bytecode
                                    to parse.
                                    The stream must not be at its end.
  @param  [out]     OutNode         Pointer holding the node created from the
                                    parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseString (
  IN      CONST AML_NODE_HEADER     * ParentNode,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
      OUT       AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS      Status;
  UINT32          StrSize;
  UINT8           Byte;
  CONST UINT8   * Buffer;

  if ((!IS_AML_ROOT_NODE (ParentNode)     &&
       !IS_AML_OBJECT_NODE (ParentNode))  ||
      (ExpectedFormat != EAmlString)      ||
      !IS_STREAM (FStream)                ||
      IS_END_OF_STREAM (FStream)          ||
      !IS_STREAM_FORWARD (FStream)        ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (CONST UINT8*)AmlStreamGetCurrPos (FStream);
  StrSize = 0;
  // AML String is NULL terminated.
  do {
    // Reading the stream moves the stream forward aswell.
    Status = AmlStreamReadByte (FStream, &Byte);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
    StrSize++;
  } while (Byte != '\0');

  AMLDBG_DUMP_RAW (Buffer, StrSize);

  Status = AmlCreateDataNode (
             AmlTypeToNodeDataType (ExpectedFormat),
             Buffer,
             StrSize,
             (AML_DATA_NODE**)OutNode
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Parse an AML object.

  An object can be resolved as an AML object with an OpCode,
  or a NameString. An object node or a data node is created
  and returned through the OutNode parameter.

  @param  [in]      ParentNode      Parent node to which the parsed
                                    AML construct will be attached.
  @param  [in]      ExpectedFormat  Format of the AML construct to parse.
  @param  [in, out] FStream         Forward stream containing the AML bytecode
                                    to parse.
                                    The stream must not be at its end.
  @param  [out]     OutNode         Pointer holding the node created from the
                                    parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseObject (
  IN      CONST AML_NODE_HEADER     * ParentNode,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
      OUT       AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS                  Status;

  UINT8                       OpCodeSize;
  UINT32                      PkgLength;
  UINT32                      PkgOffset;
  UINT32                      FreeSpace;

  CONST AML_BYTE_ENCODING   * AmlByteEncoding;
  CONST UINT8               * Buffer;

  if ((!IS_AML_ROOT_NODE (ParentNode)     &&
       !IS_AML_OBJECT_NODE (ParentNode))  ||
      (ExpectedFormat != EAmlObject)      ||
      !IS_STREAM (FStream)                ||
      IS_END_OF_STREAM (FStream)          ||
      !IS_STREAM_FORWARD (FStream)        ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  PkgLength = 0;

  // 0. Get the AML Byte encoding.
  AmlByteEncoding = AmlGetByteEncoding (AmlStreamGetCurrPos (FStream));
  if (AmlByteEncoding == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // 1. Check for NameString.
  //    Indeed a NameString can be found when an AML object is expected.
  //    e.g. VAR0 = 3         // VAR0 is assigned an object which is a UINT.
  //         VAR1 = VAR2      // VAR2 is a NameString.
  //    If this is a NameString, return. A NameString can be a variable, a
  //    method invocation, etc.
  if ((AmlByteEncoding->Attribute & AML_IS_NAME_CHAR) != 0) {
    Status = AmlParseNameString (
               ParentNode,
               EAmlName,
               FStream,
               OutNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
    return Status;
  }

  // 2. Determine the OpCode size to move the stream forward.
  Buffer = (CONST UINT8*)AmlStreamGetCurrPos (FStream);
  if (*Buffer == AML_EXT_OP) {
    OpCodeSize = 2;
  } else {
    OpCodeSize = 1;
  }
  Status = AmlStreamProgress (FStream, OpCodeSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Print the opcode.
  AMLDBG_DUMP_RAW (Buffer, OpCodeSize);

  if (!IS_END_OF_STREAM (FStream)) {
    // 3. Parse the PkgLength field, if present.
    if ((AmlByteEncoding->Attribute & AML_HAS_PKG_LENGTH) != 0) {
      Buffer = (CONST UINT8*)AmlStreamGetCurrPos (FStream);
      PkgOffset = AmlGetPkgLength (Buffer, &PkgLength);
      if (PkgOffset == 0) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Print the package length.
      AMLDBG_DUMP_RAW (Buffer, PkgOffset);

      // Adjust the size of the stream if it is valid  package length.
      FreeSpace = AmlStreamGetFreeSpace (FStream);
      if (FreeSpace > PkgLength) {
        // Reduce the stream size by (FreeSpace - PkgLength) bytes.
        AmlStreamReduceMaxBufferSize (FStream, FreeSpace - PkgLength);
      } else if (FreeSpace != PkgLength) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      Status = AmlStreamProgress (FStream, PkgOffset);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }
  } else if ((AmlByteEncoding->Attribute & AML_HAS_PKG_LENGTH) != 0) {
    // The stream terminated unexpectedly. A PkgLen had to be parsed.
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // 4. Create an Object Node.
  Status = AmlCreateObjectNode (
             AmlByteEncoding,
             PkgLength,
             (AML_OBJECT_NODE**)OutNode
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Parse a FieldPkgLen.

  A FieldPkgLen can only be found in a field list, i.e. in a NamedField field
  element. The PkgLen is otherwise part of the object node structure.
  A data node is created and returned through the OutNode parameter.

  @param  [in]      ParentNode      Parent node to which the parsed
                                    AML construct will be attached.
  @param  [in]      ExpectedFormat  Format of the AML construct to parse.
  @param  [in, out] FStream         Forward stream containing the AML bytecode
                                    to parse.
                                    The stream must not be at its end.
  @param  [out]     OutNode         Pointer holding the node created from the
                                    parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseFieldPkgLen (
  IN      CONST AML_NODE_HEADER     * ParentNode,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
      OUT       AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS      Status;
  EFI_STATUS      Status1;
  CONST UINT8   * Buffer;
  UINT32          PkgOffset;
  UINT32          PkgLength;

  if (!AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)ParentNode,
         AML_IS_FIELD_ELEMENT
         )                                ||
      (ExpectedFormat != EAmlFieldPkgLen) ||
      !IS_STREAM (FStream)                ||
      IS_END_OF_STREAM (FStream)          ||
      !IS_STREAM_FORWARD (FStream)        ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (CONST UINT8*)AmlStreamGetCurrPos (FStream);

  PkgOffset = AmlGetPkgLength (Buffer, &PkgLength);
  if (PkgOffset == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Warning: Since, updating of field elements is not supported, store the
  // FieldPkgLength in a Data Node as a raw buffer.
  Status = AmlCreateDataNode (
             AmlTypeToNodeDataType (ExpectedFormat),
             Buffer,
             PkgOffset,
             (AML_DATA_NODE**)OutNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  AMLDBG_DUMP_RAW (Buffer, PkgOffset);

  Status = AmlStreamProgress (FStream, PkgOffset);
  if (EFI_ERROR (Status)) {
    Status1 = AmlDeleteNode (*OutNode);
    ASSERT_EFI_ERROR (Status1);
    ASSERT (0);
  }

  return Status;
}

/** Array of functions pointers to parse the AML constructs.

  The AML Byte encoding tables in Aml.c describe the format of the AML
  statements. The AML_PARSE_FORMAT enum definition lists these constructs
  and the corresponding parsing functions.
*/
AML_PARSE_FUNCTION mParseType[EAmlParseFormatMax] = {
  NULL,                    // EAmlNone
  AmlParseUIntX,           // EAmlUInt8
  AmlParseUIntX,           // EAmlUInt16
  AmlParseUIntX,           // EAmlUInt32
  AmlParseUIntX,           // EAmlUInt64
  AmlParseObject,          // EAmlObject
  AmlParseNameString,      // EAmlName
  AmlParseString,          // EAmlString
  AmlParseFieldPkgLen      // EAmlFieldPkgLen
};

/** Check whether the NameString stored in the data node is a method invocation.
    If so, create a method invocation node and return it.

  @param  [in]      ParentNode        Node to which the parsed AML construct
                                      will be attached.
  @param  [in]      DataNode          Data node containing a NameString,
                                      potentially being a method invocation.
  @param  [in, out] NameSpaceRefList  List of namespace reference nodes.
  @param  [out]     OutNode           Pointer holding the method invocation
                                      node if the NameString contained in the
                                      data node is a method invocation.
                                      NULL otherwise.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCheckAndParseMethodInvoc (
  IN  CONST AML_NODE_HEADER     * ParentNode,
  IN        AML_DATA_NODE       * DataNode,
  IN  OUT   LIST_ENTRY          * NameSpaceRefList,
      OUT   AML_OBJECT_NODE    ** OutNode
  )
{
  EFI_STATUS                Status;
  AML_NAMESPACE_REF_NODE  * NameSpaceRefNode;
  AML_OBJECT_NODE         * MethodInvocationNode;
  AML_STREAM                FStream;

  if ((!IS_AML_ROOT_NODE (ParentNode)                     &&
       !IS_AML_OBJECT_NODE (ParentNode))                  ||
      !IS_AML_DATA_NODE (DataNode)                        ||
      (DataNode->DataType != EAmlNodeDataTypeNameString)  ||
      (NameSpaceRefList == NULL)                          ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Initialize a stream containing the NameString which is checked.
  Status = AmlStreamInit (
             &FStream,
             DataNode->Buffer,
             DataNode->Size,
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Check whether the NameString is a method invocation.
  NameSpaceRefNode = NULL;
  Status = AmlIsMethodInvocation (
              ParentNode,
              &FStream,
              NameSpaceRefList,
              &NameSpaceRefNode
              );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  MethodInvocationNode = NULL;
  if (NameSpaceRefNode != NULL) {
    // A matching method definition has been found.
    // Create a method invocation node.
    Status = AmlCreateMethodInvocationNode (
               NameSpaceRefNode,
               (AML_DATA_NODE*)DataNode,
               &MethodInvocationNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  *OutNode = MethodInvocationNode;

  return EFI_SUCCESS;
}

/** Call the appropriate function to parse the AML construct in the stream.

  The ExpectedFormat parameter allows to choose the right parsing function.
  An object node or a data node is created according to format.

  @param  [in]      ParentNode        Node to which the parsed AML construct
                                      will be attached.
  @param  [in]      ExpectedFormat    Format of the AML construct to parse.
  @param  [in, out] FStream           Forward stream containing the AML
                                      bytecode to parse.
                                      The stream must not be at its end.
  @param  [in, out] NameSpaceRefList  List of namespace reference nodes.
  @param  [out]     OutNode           Pointer holding the node created from the
                                      parsed AML bytecode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseArgument (
  IN      CONST AML_NODE_HEADER     * ParentNode,
  IN            AML_PARSE_FORMAT      ExpectedFormat,
  IN  OUT       AML_STREAM          * FStream,
  IN  OUT       LIST_ENTRY          * NameSpaceRefList,
      OUT       AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS                Status;
  AML_PARSE_FUNCTION        ParsingFunction;
  AML_DATA_NODE           * DataNode;
  AML_OBJECT_NODE         * MethodInvocationNode;

  if ((!IS_AML_ROOT_NODE (ParentNode)         &&
       !IS_AML_OBJECT_NODE (ParentNode))      ||
      (ExpectedFormat >= EAmlParseFormatMax)  ||
      !IS_STREAM (FStream)                    ||
      IS_END_OF_STREAM (FStream)              ||
      !IS_STREAM_FORWARD (FStream)            ||
      (NameSpaceRefList == NULL)              ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParsingFunction = mParseType[ExpectedFormat];
  if (ParsingFunction == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Note: The ParsingFunction moves the stream forward as it
  // consumes the AML bytecode
  Status = ParsingFunction (
             ParentNode,
             ExpectedFormat,
             FStream,
             OutNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Check whether the parsed argument is a NameString when an object
  // is expected. In such case, it could be a method invocation.
  DataNode = (AML_DATA_NODE*)*OutNode;
  if (IS_AML_DATA_NODE (DataNode)                         &&
      (DataNode->DataType == EAmlNodeDataTypeNameString)  &&
      (ExpectedFormat == EAmlObject)) {
    Status = AmlCheckAndParseMethodInvoc (
               ParentNode,
               (AML_DATA_NODE*)*OutNode,
               NameSpaceRefList,
               &MethodInvocationNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // A method invocation node has been created and the DataNode containing
    // the NameString has been attached to the MethodInvocationNode.
    // Replace the OutNode with the MethodInvocationNode.
    if (MethodInvocationNode != NULL) {
      *OutNode = (AML_NODE_HEADER*)MethodInvocationNode;
    }
  }

  return Status;
}

/** Parse the Bytelist in the stream.
    According to the content of the stream, create data node(s)
    and add them to the variable list of arguments.
    The byte list may be a list of resource data element or a simple byte list.

  @param  [in]  BufferNode    Object node having a byte list.
  @param  [in, out] FStream   Forward stream containing the AML bytecode
                              to parse.
                              The stream must not be at its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseByteList (
  IN      AML_OBJECT_NODE   * BufferNode,
  IN  OUT AML_STREAM        * FStream
  )
{
  EFI_STATUS          Status;
  AML_NODE_HEADER   * NewNode;
  CONST UINT8       * Buffer;
  UINT32              BufferSize;

  // Check whether the node is an Object Node and has byte list.
  if (!AmlNodeHasAttribute (BufferNode, AML_HAS_BYTE_LIST)  ||
      !IS_STREAM (FStream)                                  ||
      IS_END_OF_STREAM (FStream)                            ||
      !IS_STREAM_FORWARD (FStream)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The buffer contains a list of resource data elements.
  if (AmlRdIsResourceDataBuffer (FStream)) {
    // Parse the resource data elements and add them as data nodes.
    // AmlParseResourceData() moves the stream forward.
    Status = AmlParseResourceData (BufferNode, FStream);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  } else {
    // The buffer doesn't contain a list of resource data elements.
    // Create a single node holding the whole buffer data.

    // CreateDataNode checks the Buffer and BufferSize values.
    Buffer = (CONST UINT8*)AmlStreamGetCurrPos (FStream);
    BufferSize = AmlStreamGetFreeSpace (FStream);

    Status = AmlCreateDataNode (
               EAmlNodeDataTypeRaw,
               Buffer,
               BufferSize,
               (AML_DATA_NODE**)&NewNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Status = AmlVarListAddTailInternal (
                (AML_NODE_HEADER*)BufferNode,
                NewNode
                );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      AmlDeleteTree (NewNode);
      return Status;
    }

    AMLDBG_DUMP_RAW (Buffer, BufferSize);

    // Move the stream forward as we have consumed the Buffer.
    Status = AmlStreamProgress (FStream, BufferSize);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  }

  return Status;
}

/** Parse the list of fixed arguments of the input ObjectNode.

  For each argument, create a node and add it to the fixed argument list
  of the Node.
  If a fixed argument has children, parse them.

  @param  [in]  ObjectNode        Object node to parse the fixed arguments
                                  from.
  @param  [in]  FStream           Forward stream containing the AML
                                  bytecode to parse.
                                  The stream must not be at its end.
  @param  [in]  NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseFixedArguments (
  IN  AML_OBJECT_NODE   * ObjectNode,
  IN  AML_STREAM        * FStream,
  IN  LIST_ENTRY        * NameSpaceRefList
  )
{
  EFI_STATUS                Status;

  AML_NODE_HEADER         * FixedArgNode;
  AML_STREAM                FixedArgFStream;

  EAML_PARSE_INDEX          TermIndex;
  EAML_PARSE_INDEX          MaxIndex;
  CONST AML_PARSE_FORMAT  * Format;

  // Fixed arguments of method invocations node are handled differently.
  if (!IS_AML_OBJECT_NODE (ObjectNode)                              ||
      AmlNodeCompareOpCode (ObjectNode, AML_METHOD_INVOC_OP, 0)     ||
      !IS_STREAM (FStream)                                          ||
      IS_END_OF_STREAM (FStream)                                    ||
      !IS_STREAM_FORWARD (FStream)                                  ||
      (NameSpaceRefList == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  TermIndex = EAmlParseIndexTerm0;
  MaxIndex = (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                 (AML_OBJECT_NODE*)ObjectNode
                                 );
  if ((ObjectNode->AmlByteEncoding != NULL)   &&
      (ObjectNode->AmlByteEncoding->Format != NULL)) {
    Format = ObjectNode->AmlByteEncoding->Format;
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Parse all the FixedArgs.
  while ((TermIndex < MaxIndex)       &&
         !IS_END_OF_STREAM (FStream)  &&
         (Format[TermIndex] != EAmlNone)) {
    // Initialize a FixedArgStream to parse the current fixed argument.
    Status = AmlStreamInitSubStream (FStream, &FixedArgFStream);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Parse the current fixed argument.
    Status = AmlParseArgument (
               (CONST AML_NODE_HEADER*)ObjectNode,
               Format[TermIndex],
               &FixedArgFStream,
               NameSpaceRefList,
               &FixedArgNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Add the fixed argument to the parent node's fixed argument list.
    // FixedArgNode can be an object or data node.
    Status = AmlSetFixedArgument (
               (AML_OBJECT_NODE*)ObjectNode,
               TermIndex,
               FixedArgNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      // Delete the sub-tree if the insertion failed.
      // Otherwise its reference will be lost.
      // Use DeleteTree because if the argument was a method invocation,
      // multiple nodes have been created.
      AmlDeleteTree (FixedArgNode);
      return Status;
    }

    // Parse the AML bytecode of the FixedArgNode if this is an object node.
    if (IS_AML_OBJECT_NODE (FixedArgNode) &&
        !IS_END_OF_STREAM (&FixedArgFStream)) {
      Status = AmlParseStream (
                 FixedArgNode,
                 &FixedArgFStream,
                 NameSpaceRefList
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }

    // Move the stream forward as we have consumed the sub-stream.
    Status = AmlStreamProgress (
               FStream,
               AmlStreamGetIndex (&FixedArgFStream)
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    TermIndex++;
  } // while

  return EFI_SUCCESS;
}

/** Parse the variable list of arguments of the input ObjectNode.

  For each variable argument, create a node and add it to the variable list of
  arguments of the Node.
  If a variable argument has children, parse them recursively.

  The arguments of method invocation nodes are added to the variable list of
  arguments of the method invocation node. It is necessary to first get
  the number of arguments to parse for this kind of node. A method invocation
  can have at most 7 fixed arguments.

  @param  [in]  Node              Node to parse the variable arguments
                                  from.
  @param  [in]  FStream           Forward stream containing the AML
                                  bytecode to parse.
                                  The stream must not be at its end.
  @param  [in]  NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseVariableArguments (
  IN  AML_NODE_HEADER   * Node,
  IN  AML_STREAM        * FStream,
  IN  LIST_ENTRY        * NameSpaceRefList
  )
{
  EFI_STATUS                Status;

  BOOLEAN                   IsMethodInvocation;
  UINT8                     MethodInvocationArgCount;

  AML_NODE_HEADER         * VarArgNode;
  AML_STREAM                VarArgFStream;

  if ((!AmlNodeHasAttribute (
          (CONST AML_OBJECT_NODE*)Node,
          AML_HAS_CHILD_OBJ
          ) &&
       !IS_AML_ROOT_NODE (Node))        ||
      !IS_STREAM (FStream)              ||
      IS_END_OF_STREAM (FStream)        ||
      !IS_STREAM_FORWARD (FStream)      ||
      (NameSpaceRefList == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlGetMethodInvocationArgCount (
             (CONST AML_OBJECT_NODE*)Node,
             &IsMethodInvocation,
             &MethodInvocationArgCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Parse variable arguments while the Stream is not empty.
  while (!IS_END_OF_STREAM (FStream)) {
    // If the number of variable arguments are counted, decrement the counter.
    if ((IsMethodInvocation) && (MethodInvocationArgCount-- == 0)) {
      return EFI_SUCCESS;
    }

    // Initialize a VarArgStream to parse the current variable argument.
    Status = AmlStreamInitSubStream (FStream, &VarArgFStream);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Parse the current variable argument.
    Status = AmlParseArgument (
               Node,
               EAmlObject,
               &VarArgFStream,
               NameSpaceRefList,
               &VarArgNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Add the variable argument to its parent variable list of arguments.
    // VarArgNode can be an object or data node.
    Status = AmlVarListAddTailInternal (
               (AML_NODE_HEADER*)Node,
               VarArgNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      // Delete the sub-tree if the insertion failed.
      // Otherwise its reference will be lost.
      // Use DeleteTree because if the argument was a method invocation,
      // multiple nodes have been created.
      AmlDeleteTree (VarArgNode);
      return Status;
    }

    // Parse the AML bytecode of the VarArgNode if this is an object node.
    if (IS_AML_OBJECT_NODE (VarArgNode)       &&
        (!IS_END_OF_STREAM (&VarArgFStream))) {
      Status = AmlParseStream (VarArgNode, &VarArgFStream, NameSpaceRefList);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }

    // Move the stream forward as we have consumed the sub-stream.
    Status = AmlStreamProgress (
               FStream,
               AmlStreamGetIndex (&VarArgFStream)
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // while

  // If the number of variable arguments are counted, check all the
  // MethodInvocationArgCount have been parsed.
  if (IsMethodInvocation && (MethodInvocationArgCount != 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Parse the AML stream and populate the root node.

  @param  [in]      RootNode          RootNode to which the children are
                                      added.
  @param  [in, out] FStream           Forward stream containing the AML
                                      bytecode to parse.
                                      The stream must not be at its end.
  @param  [in, out] NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlPopulateRootNode (
  IN      AML_ROOT_NODE     * RootNode,
  IN  OUT AML_STREAM        * FStream,
  IN  OUT LIST_ENTRY        * NameSpaceRefList
  )
{
  EFI_STATUS      Status;

  if (!IS_AML_ROOT_NODE (RootNode)  ||
      !IS_STREAM (FStream)          ||
      IS_END_OF_STREAM (FStream)    ||
      !IS_STREAM_FORWARD (FStream)  ||
      (NameSpaceRefList == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // A Root Node only has variable arguments.
  Status = AmlParseVariableArguments (
             (AML_NODE_HEADER*)RootNode,
             FStream,
             NameSpaceRefList
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Parse the AML stream an populate the object node.

  @param  [in]      ObjectNode        ObjectNode to which the children are
                                      added.
  @param  [in, out] FStream           Forward stream containing the AML
                                      bytecode to parse.
                                      The stream must not be at its end.
  @param  [in, out] NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlPopulateObjectNode (
  IN      AML_OBJECT_NODE   * ObjectNode,
  IN  OUT AML_STREAM        * FStream,
  IN  OUT LIST_ENTRY        * NameSpaceRefList
  )
{
  EFI_STATUS      Status;

  if (!IS_AML_OBJECT_NODE (ObjectNode)  ||
      !IS_STREAM (FStream)              ||
      IS_END_OF_STREAM (FStream)        ||
      !IS_STREAM_FORWARD (FStream)      ||
      (NameSpaceRefList == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  // Don't parse the fixed arguments of method invocation nodes.
  // The AML encoding for method invocations in the ACPI specification 6.3 is:
  // MethodInvocation := NameString TermArgList
  // Since the AML specification does not define an OpCode for method
  // invocation, this AML parser defines a pseudo opcode and redefines the
  // grammar for simplicity as:
  // MethodInvocation := MethodInvocationOp NameString ArgumentCount TermArgList
  // ArgumentCount    := ByteData
  // Due to this difference, the MethodInvocationOp and the fixed argument
  // i.e. ArgumentCount is not available in the AML stream and need to be
  // handled differently.
  if (!AmlNodeCompareOpCode (ObjectNode, AML_METHOD_INVOC_OP, 0)) {
    // Parse the fixed list of arguments.
    Status = AmlParseFixedArguments (
               ObjectNode,
               FStream,
               NameSpaceRefList
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  // Save the association [node reference/pathname] in the NameSpaceRefList.
  // This allows to identify method invocations from other namespace
  // paths. Method invocation need to be parsed differently.
  if (AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)ObjectNode,
         AML_IN_NAMESPACE)) {
    Status = AmlAddNameSpaceReference (
               (CONST AML_OBJECT_NODE*)ObjectNode,
               NameSpaceRefList
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  if (!IS_END_OF_STREAM (FStream)) {
    // Parse the variable list of arguments if present.
    if (AmlNodeHasAttribute (ObjectNode, AML_HAS_CHILD_OBJ)) {
      Status = AmlParseVariableArguments (
                (AML_NODE_HEADER*)ObjectNode,
                FStream,
                NameSpaceRefList
                );
    } else if (AmlNodeHasAttribute (ObjectNode, AML_HAS_BYTE_LIST)) {
      // Parse the byte list if present.
      Status = AmlParseByteList (
                ObjectNode,
                FStream
                );
    } else if (AmlNodeHasAttribute (ObjectNode, AML_HAS_FIELD_LIST)) {
      // Parse the field list if present.
      Status = AmlParseFieldList (
                ObjectNode,
                FStream,
                NameSpaceRefList
                );
    }

    // Check status and assert
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  }

  return Status;
}

/** Invoke the appropriate parsing functions based on the Node type.

  @param  [in]      Node              Node from which the children are parsed.
                                      Must be a root node or an object node.
  @param  [in]      FStream           Forward stream containing the AML
                                      bytecode to parse.
                                      The stream must not be at its end.
  @param  [in]      NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlParseStream (
  IN  AML_NODE_HEADER   * Node,
  IN  AML_STREAM        * FStream,
  IN  LIST_ENTRY        * NameSpaceRefList
  )
{
  EFI_STATUS    Status;

  if (IS_AML_ROOT_NODE (Node)) {
    Status = AmlPopulateRootNode (
               (AML_ROOT_NODE*)Node,
               FStream,
               NameSpaceRefList
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }

  } else if (IS_AML_OBJECT_NODE (Node)) {
    Status = AmlPopulateObjectNode (
               (AML_OBJECT_NODE*)Node,
               FStream,
               NameSpaceRefList
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }

  } else {
    // Data node or other.
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Parse the definition block.

  This function parses the whole AML blob. It starts with the ACPI DSDT/SSDT
  header and then parses the AML bytestream.
  A tree structure is returned via the RootPtr.
  The tree must be deleted with the AmlDeleteTree function.

  @param  [in]  DefinitionBlock   Pointer to the definition block.
  @param  [out] RootPtr           Pointer to the root node of the tree.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseDefinitionBlock (
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER   * DefinitionBlock,
  OUT       AML_ROOT_NODE                ** RootPtr
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  AML_STREAM              Stream;
  AML_ROOT_NODE         * Root;

  LIST_ENTRY              NameSpaceRefList;

  UINT8                 * Buffer;
  UINT32                  MaxBufferSize;

  if ((DefinitionBlock == NULL)   ||
      (RootPtr == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (UINT8*)DefinitionBlock + sizeof (EFI_ACPI_DESCRIPTION_HEADER);
  if (DefinitionBlock->Length < sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }
  MaxBufferSize = DefinitionBlock->Length -
                    (UINT32)sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  // Create a root node.
  Status = AmlCreateRootNode (
             (EFI_ACPI_DESCRIPTION_HEADER*)DefinitionBlock,
             &Root
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  *RootPtr = Root;

  if (MaxBufferSize == 0) {
    return EFI_SUCCESS;
  }

  // Initialize a stream to parse the AML bytecode.
  Status = AmlStreamInit (
             &Stream,
             Buffer,
             MaxBufferSize,
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Initialize the NameSpaceRefList, holding references to nodes declaring
  // a name in the AML namespace.
  InitializeListHead (&NameSpaceRefList);

  // Parse the whole AML blob.
  Status = AmlParseStream (
             (AML_NODE_HEADER*)Root,
             &Stream,
             &NameSpaceRefList
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Check the whole AML blob has been parsed.
  if (!IS_END_OF_STREAM (&Stream)) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  // Print the list of NameSpace reference nodes.
  // AmlDbgPrintNameSpaceRefList (&NameSpaceRefList);

  // Delete the NameSpaceRefList
  goto exit_handler;

error_handler:
  if (Root != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER*)Root);
  }

exit_handler:
  Status1 = AmlDeleteNameSpaceRefList (&NameSpaceRefList);
  if (EFI_ERROR (Status1)) {
    ASSERT (0);
    if (!EFI_ERROR (Status)) {
      return Status1;
    }
  }

  return Status;
}
