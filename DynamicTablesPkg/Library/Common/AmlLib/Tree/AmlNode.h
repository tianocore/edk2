/** @file
  AML Node.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_NODE_H_
#define AML_NODE_H_

#include <AmlNodeDefines.h>
#include <IndustryStandard/Acpi.h>

/** Create an AML_ROOT_NODE.
    This node will be the root of the tree.

  @param  [in]  SdtHeader       Pointer to an ACPI DSDT/SSDT header to copy
                                the data from.
  @param  [out] NewRootNodePtr  The created AML_ROOT_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateRootNode (
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER   * SdtHeader,
  OUT       AML_ROOT_NODE                ** NewRootNodePtr
  );

/** Create an AML_OBJECT_NODE.

  @param  [in]  AmlByteEncoding   Byte encoding entry.
  @param  [in]  PkgLength         PkgLength of the node if the AmlByteEncoding
                                  has the PkgLen attribute.
                                  0 otherwise.
  @param  [out] NewObjectNodePtr  The created AML_OBJECT_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateObjectNode (
  IN  CONST  AML_BYTE_ENCODING   * AmlByteEncoding,
  IN         UINT32                PkgLength,
  OUT        AML_OBJECT_NODE    ** NewObjectNodePtr
  );

/** Create an AML_DATA_NODE.

  @param  [in]  DataType        DataType of the node.
  @param  [in]  Data            Pointer to the AML bytecode corresponding to
                                this node. Data is copied from there.
  @param  [in]  DataSize        Number of bytes to consider at the address
                                pointed by Data.
  @param  [out] NewDataNodePtr  The created AML_DATA_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateDataNode (
  IN        EAML_NODE_DATA_TYPE     DataType,
  IN  CONST UINT8                 * Data,
  IN        UINT32                  DataSize,
  OUT       AML_DATA_NODE        ** NewDataNodePtr
  );

/** Delete a Node.

  @param  [in]  Node  Pointer to a Node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteNode (
  IN  AML_NODE_HEADER   * Node
  );

/** Check whether ObjectNode has the input attribute.
    This function can be used to check ObjectNode is an object node
    at the same time.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  Attribute   Attribute to check for.

  @retval TRUE    The node is an AML object and the attribute is present.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeHasAttribute (
  IN  CONST AML_OBJECT_NODE   * ObjectNode,
  IN        AML_OP_ATTRIBUTE    Attribute
  );

/** Check whether ObjectNode has the input OpCode/SubOpcode couple.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  OpCode      OpCode to check
  @param  [in]  SubOpCode   SubOpCode to check

  @retval TRUE    The node is an AML object and
                  the Opcode and the SubOpCode match.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeCompareOpCode (
  IN  CONST  AML_OBJECT_NODE  * ObjectNode,
  IN         UINT8              OpCode,
  IN         UINT8              SubOpCode
  );

/** Check whether a Node is an integer node.

  By integer node we mean an object node having one of the following opcode:
   - AML_BYTE_PREFIX;
   - AML_WORD_PREFIX;
   - AML_DWORD_PREFIX;
   - AML_QWORD_PREFIX.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is an integer node.
  @retval FALSE Otherwise.
*/
BOOLEAN
EFIAPI
IsIntegerNode (
  IN  AML_OBJECT_NODE   * Node
  );

/** Check whether a Node is a ZeroOp, a OneOp or a OnesOp.

  These two objects don't have a data node holding
  a value. This require special handling.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is a ZeroOp or OneOp.
  @retval FALSE Otherwise.
*/
BOOLEAN
EFIAPI
IsSpecialIntegerNode (
  IN  AML_OBJECT_NODE   * Node
  );

/** Check whether Node corresponds to a method definition.

  A method definition can be introduced:
   - By a method object, having an AML_METHOD_OP OpCode;
   - By an external definition of a method, having an AML_EXTERNAL_OP OpCode
     and an ObjectType byte set to the MethodObj.

  Note:
  An alias node, having an AML_ALIAS_OP, can be resolved to a method
  definition. This function doesn't handle this case.

  @param [in] Node    Node to check whether it is a method definition.

  @retval TRUE  The Node is a method definition.
  @retval FALSE Otherwise.
**/
BOOLEAN
EFIAPI
AmlIsMethodDefinitionNode (
  IN  CONST AML_OBJECT_NODE   * Node
  );

/** Get the index at which the name of the node is stored.

  @param  [in]  ObjectNode  Pointer to an object node.
                            Must have the AML_IN_NAMESPACE attribute.
  @param  [out] Index       Index of the name in the fixed list of arguments.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
AmlNodeGetNameIndex (
  IN  CONST AML_OBJECT_NODE     * ObjectNode,
  OUT       EAML_PARSE_INDEX    * Index
  );

/** Get the name of the Node.

  Node must be part of the namespace.

  @param [in] ObjectNode    Pointer to an object node,
                            which is part of the namespace.

  @return A pointer to the name.
          NULL otherwise.
          Return NULL for the root node.
**/
CHAR8 *
EFIAPI
AmlNodeGetName (
  IN  CONST AML_OBJECT_NODE   * ObjectNode
  );

#endif // AML_NODE_H_
