/** @file
  AML Node Definition.

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_NODE_DEFINES_H_
#define AML_NODE_DEFINES_H_

#include <AmlEncoding/Aml.h>
#include <IndustryStandard/Acpi.h>

/** AML header node.

  This abstract class represents either a root/object/data node.
  All the enumerated nodes have this same common header.
*/
typedef struct AmlNodeHeader {
  /// This must be the first field in this structure.
  LIST_ENTRY              Link;

  /// Parent of this node. NULL for the root node.
  struct AmlNodeHeader    *Parent;

  /// Node type allowing to identify a root/object/data node.
  EAML_NODE_TYPE          NodeType;
} AML_NODE_HEADER;

/** Node handle.
*/
typedef AML_NODE_HEADER *AML_NODE_HANDLE;

/** AML root node.

  The root node is unique and at the head of tree. It is a fake node used
  to maintain the list of AML statements (stored as object nodes) which are
  at the first scope level.
*/
typedef struct AmlRootNode {
  /// Header information. Must be the first field of the struct.
  AML_NODE_HEADER                NodeHeader;

  /// List of object nodes being at the first scope level.
  /// These are children and can only be object nodes.
  LIST_ENTRY                     VariableArgs;

  /// ACPI DSDT/SSDT header.
  EFI_ACPI_DESCRIPTION_HEADER    *SdtHeader;
} AML_ROOT_NODE;

/** Root Node handle.
*/
typedef AML_ROOT_NODE *AML_ROOT_NODE_HANDLE;

/** AML object node.

  Object nodes match AML statements. They are associated with an
  OpCode/SubOpCode, and can have children.
*/
typedef struct AmlObjectNode {
  /// Header information. Must be the first field of the struct.
  AML_NODE_HEADER            NodeHeader;

  /// Some object nodes have a variable list of arguments.
  /// These are children and can only be object/data nodes.
  /// Cf ACPI specification, s20.3.
  LIST_ENTRY                 VariableArgs;

  /// Fixed arguments of this object node.
  /// These are children and can be object/data nodes.
  /// Cf ACPI specification, s20.3.
  AML_NODE_HEADER            *FixedArgs[EAmlParseIndexMax];

  /// AML byte encoding. Stores the encoding information:
  /// (OpCode/SubOpCode/number of fixed arguments/ attributes).
  CONST AML_BYTE_ENCODING    *AmlByteEncoding;

  /// Some nodes have a PkgLen following their OpCode/SubOpCode in the
  /// AML bytestream. This field stores the decoded value of the PkgLen.
  UINT32                     PkgLen;
} AML_OBJECT_NODE;

/** Object Node handle.
*/
typedef AML_OBJECT_NODE *AML_OBJECT_NODE_HANDLE;

/** AML data node.

  Data nodes store the smallest pieces of information.
  E.g.: UINT8, UINT64, NULL terminated string, etc.
  Data node don't have children nodes.
*/
typedef struct AmlDataNode {
  /// Header information. Must be the first field of the struct.
  AML_NODE_HEADER        NodeHeader;

  /// Tag identifying what data is stored in this node.
  /// E.g. UINT, NULL terminated string, resource data element, etc.
  EAML_NODE_DATA_TYPE    DataType;

  /// Buffer containing the data stored by this node.
  UINT8                  *Buffer;

  /// Size of the Buffer.
  UINT32                 Size;
} AML_DATA_NODE;

/** Data Node handle.
*/
typedef AML_DATA_NODE *AML_DATA_NODE_HANDLE;

/** Check whether a Node has a valid NodeType.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node has a valid NodeType.
  @retval FALSE Otherwise.
*/
#define IS_AML_NODE_VALID(Node)                                               \
          ((Node != NULL)                                                  && \
            ((((CONST AML_NODE_HEADER*)Node)->NodeType > EAmlNodeUnknown)  || \
            (((CONST AML_NODE_HEADER*)Node)->NodeType < EAmlNodeMax)))

/** Check whether a Node is a root node.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is a root node.
  @retval FALSE Otherwise.
*/
#define IS_AML_ROOT_NODE(Node)                                                \
          ((Node != NULL)                                                  && \
           (((CONST AML_NODE_HEADER*)Node)->NodeType == EAmlNodeRoot))

/** Check whether a Node is an object node.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is an object node.
  @retval FALSE Otherwise.
*/
#define IS_AML_OBJECT_NODE(Node)                                              \
          ((Node != NULL)                                                  && \
           (((CONST AML_NODE_HEADER*)Node)->NodeType == EAmlNodeObject))

/** Check whether a Node is a data node.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is a data node.
  @retval FALSE Otherwise.
*/
#define IS_AML_DATA_NODE(Node)                                                \
          ((Node != NULL)                                                  && \
           (((CONST AML_NODE_HEADER*)Node)->NodeType == EAmlNodeData))

/** Check whether a Node has a parent.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is a data node.
  @retval FALSE Otherwise.
*/
#define AML_NODE_HAS_PARENT(Node)                                             \
          (IS_AML_NODE_VALID (Node)                                        && \
           (((CONST AML_NODE_HEADER*)Node)->Parent != NULL))

/** Check that the Node is not attached somewhere.
    This doesn't mean the node cannot have children.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node has been detached.
  @retval FALSE Otherwise.
*/
#define AML_NODE_IS_DETACHED(Node)                                            \
          (IS_AML_NODE_VALID (Node)                                        && \
           IsListEmpty ((CONST LIST_ENTRY*)Node)                           && \
           (((CONST AML_NODE_HEADER*)Node)->Parent == NULL))

#endif // AML_NODE_DEFINES_H_
