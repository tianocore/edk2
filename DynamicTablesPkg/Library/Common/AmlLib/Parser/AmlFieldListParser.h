/** @file
  AML Field List.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_FIELD_LIST_PARSER_H_
#define AML_FIELD_LIST_PARSER_H_

#include <AmlNodeDefines.h>
#include <Stream/AmlStream.h>

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
  IN  AML_OBJECT_NODE   * FieldNode,
  IN  AML_STREAM        * FStream,
  IN  LIST_ENTRY        * NameSpaceRefList
  );

#endif // AML_FIELD_LIST_PARSER_H_
