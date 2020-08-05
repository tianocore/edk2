/** @file
  AML Core Interface.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_CORE_INTERFACE_H_
#define AML_CORE_INTERFACE_H_

/* This header file does not include internal Node definition,
   i.e. AML_ROOT_NODE, AML_OBJECT_NODE, etc. The node definitions
   must be included by the caller file. The function prototypes must
   only expose AML_NODE_HANDLE, AML_ROOT_NODE_HANDLE, etc. node
   definitions.
   This allows to keep the functions defined here both internal and
   potentially external. If necessary, any function of this file can
   be exposed externally.
   The Api folder is internal to the AmlLib, but should only use these
   functions. They provide a "safe" way to interact with the AmlLib.
*/

#include <AmlDefines.h>
#include <Include/Library/AmlLib/AmlLib.h>
#include <ResourceData/AmlResourceData.h>

/**
  @defgroup CoreApis Core APIs
  @ingroup AMLLib
  @{
    Core APIs are the main APIs of the library. They allow to:
     - Create an AML tree;
     - Delete an AML tree;
     - Clone an AML tree/node;
     - Serialize an AML tree (convert the tree to a DSDT/SSDT table).
  @}
*/

/** Serialize a tree to create a DSDT/SSDT table.

  If:
   - the content of BufferSize is >= to the size needed to serialize the
     definition block;
   - Buffer is not NULL;
   first serialize the ACPI DSDT/SSDT header from the root node,
   then serialize the AML blob from the rest of the tree.

  The content of BufferSize is always updated to the size needed to
  serialize the definition block.

  @ingroup CoreApis

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
  IN      AML_ROOT_NODE_HANDLE    RootNode,
  IN      UINT8                 * Buffer,     OPTIONAL
  IN  OUT UINT32                * BufferSize
  );

/** Clone a node.

  This function does not clone the children nodes.
  The cloned node returned is not attached to any tree.

  @ingroup CoreApis

  @param  [in]  Node        Pointer to a node.
  @param  [out] ClonedNode  Pointer holding the cloned node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCloneNode (
  IN  AML_NODE_HANDLE   Node,
  OUT AML_NODE_HANDLE * ClonedNode
  );

/**
  @defgroup TreeModificationApis Tree modification APIs
  @ingroup AMLLib
  @{
    Tree modification APIs allow to add/remove/replace nodes that are in a
    variable list of arguments.

    No interface is provided to add/remove/replace nodes that are in a fixed
    list of arguments. Indeed, these nodes are the spine of the tree and a
    mismanipulation would make the tree inconsistent.

    It is however possible to modify the content of fixed argument nodes via
    @ref NodeInterfaceApis APIs.
  @}
*/

/** Remove the Node from its parent's variable list of arguments.

  The function will fail if the Node is in its parent's fixed
  argument list.
  The Node is not deleted. The deletion is done separately
  from the removal.

  @ingroup TreeModificationApis

  @param  [in]  Node  Pointer to a Node.
                      Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlRemoveNodeFromVarArgList (
  IN  AML_NODE_HANDLE   Node
  );

/** Add the NewNode to the head of the variable list of arguments
    of the ParentNode.

  @ingroup TreeModificationApis

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be a root or an object node.
  @param  [in]  NewNode     Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddHead (
  IN  AML_NODE_HANDLE   ParentNode,
  IN  AML_NODE_HANDLE   NewNode
  );

/** Add the NewNode to the tail of the variable list of arguments
    of the ParentNode.

  @ingroup TreeModificationApis

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be a root or an object node.
  @param  [in]  NewNode     Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddTail (
  IN  AML_NODE_HANDLE   ParentNode,
  IN  AML_NODE_HANDLE   NewNode
  );

/** Add the NewNode before the Node in the list of variable
    arguments of the Node's parent.

  @ingroup TreeModificationApis

  @param  [in]  Node      Pointer to a node.
                          Must be a root or an object node.
  @param  [in]  NewNode   Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddBefore (
  IN  AML_NODE_HANDLE   Node,
  IN  AML_NODE_HANDLE   NewNode
  );

/** Add the NewNode after the Node in the variable list of arguments
    of the Node's parent.

  @ingroup TreeModificationApis

  @param  [in]  Node      Pointer to a node.
                          Must be a root or an object node.
  @param  [in]  NewNode   Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddAfter (
  IN  AML_NODE_HANDLE   Node,
  IN  AML_NODE_HANDLE   NewNode
  );

/** Append a Resource Data node to the BufferOpNode.

  The Resource Data node is added at the end of the variable
  list of arguments of the BufferOpNode, but before the End Tag.
  If no End Tag is found, the function returns an error.

  @param  [in]  BufferOpNode  Buffer node containing resource data elements.
  @param  [in]  NewRdNode     The new Resource Data node to add.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlAppendRdNode (
  IN  AML_OBJECT_NODE_HANDLE   BufferOpNode,
  IN  AML_DATA_NODE_HANDLE     NewRdNode
  );

/** Replace the OldNode, which is in a variable list of arguments,
    with the NewNode.

  Note: This function unlinks the OldNode from the tree. It is the callers
        responsibility to delete the OldNode if needed.

  @ingroup TreeModificationApis

  @param  [in]  OldNode   Pointer to the node to replace.
                          Must be a data node or an object node.
  @param  [in]  NewNode   The new node to insert.
                          Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlReplaceVariableArgument (
  IN  AML_NODE_HANDLE   OldNode,
  IN  AML_NODE_HANDLE   NewNode
  );

/**
  @defgroup NodeInterfaceApis Node Interface APIs
  @ingroup AMLLib
  @{
    Node Interface APIs allow to query information from a node. Some functions
    expect a specific node type among the root/object/data node types.

    For instance, AmlGetRootNodeInfo expects to receive a root node.

    E.g.: Query the node type, the ACPI header stored in the root node,
          the OpCode/SubOpCode/PkgLen of an object node, the type of data
          stored in a data node, etc.

    These APIs also allow to update some information.

    E.g.: The ACPI header stored in the root node, the buffer of a data node.

    The information of object nodes and the data type of data nodes cannot be
    modified. This prevents the creation of an inconsistent tree.

    It is however possible to remove a node from a variable list of arguments
    and replace it. Use the @ref TreeModificationApis APIs for this.
  @}
*/

/** Returns the tree node type (Root/Object/Data).

  @ingroup NodeInterfaceApis

  @param [in] Node  Pointer to a Node.

  @return The node type.
           EAmlNodeUnknown if invalid parameter.
**/
EAML_NODE_TYPE
EFIAPI
AmlGetNodeType (
  IN  AML_NODE_HANDLE   Node
  );

/** Get the RootNode information.
    The Node must be a root node.

  @ingroup NodeInterfaceApis

  @param  [in]  RootNode          Pointer to a root node.
  @param  [out] SdtHeaderBuffer   Buffer to copy the ACPI DSDT/SSDT header to.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetRootNodeInfo (
  IN  AML_ROOT_NODE_HANDLE            RootNode,
  OUT EFI_ACPI_DESCRIPTION_HEADER   * SdtHeaderBuffer
  );

/** Get the ObjectNode information.
    The Node must be an object node.

  @ingroup NodeInterfaceApis

  @param  [in]  ObjectNode        Pointer to an object node.
  @param  [out] OpCode            Pointer holding the OpCode.
                                  Optional, can be NULL.
  @param  [out] SubOpCode         Pointer holding the SubOpCode.
                                  Optional, can be NULL.
  @param  [out] PkgLen            Pointer holding the PkgLen.
                                  The PkgLen is 0 for nodes
                                  not having the Pkglen attribute.
                                  Optional, can be NULL.
  @param  [out] IsNameSpaceNode   Pointer holding TRUE if the node is defining
                                  or changing the NameSpace scope.
                                  E.g.: The "Name ()" and "Scope ()" ASL
                                  statements add/modify the NameSpace scope.
                                  Their corresponding node are NameSpace nodes.
                                  Optional, can be NULL.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetObjectNodeInfo (
  IN  AML_OBJECT_NODE_HANDLE    ObjectNode,
  OUT UINT8                   * OpCode,           OPTIONAL
  OUT UINT8                   * SubOpCode,        OPTIONAL
  OUT UINT32                  * PkgLen,           OPTIONAL
  OUT BOOLEAN                 * IsNameSpaceNode   OPTIONAL
  );

/** Returns the count of the fixed arguments for the input Node.

  @ingroup NodeInterfaceApis

  @param  [in]  Node  Pointer to an object node.

  @return Number of fixed arguments of the object node.
          Return 0 if the node is not an object node.
**/
UINT8
AmlGetFixedArgumentCount (
  IN  AML_OBJECT_NODE_HANDLE  Node
  );

/** Get the data type of the DataNode.
    The Node must be a data node.

  @ingroup NodeInterfaceApis

  @param  [in]  DataNode  Pointer to a data node.
  @param  [out] DataType  Pointer holding the data type of the data buffer.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetNodeDataType (
  IN  AML_DATA_NODE_HANDLE    DataNode,
  OUT EAML_NODE_DATA_TYPE   * DataType
  );

/** Get the descriptor Id of the resource data element
    contained in the DataNode.

  The Node must be a data node.
  The Node must have the resource data type, i.e. have the
  EAmlNodeDataTypeResourceData data type.

  @ingroup NodeInterfaceApis

  @param  [in]  DataNode          Pointer to a data node containing a
                                  resource data element.
  @param  [out] ResourceDataType  Pointer holding the descriptor Id of
                                  the resource data.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetResourceDataType (
  IN  AML_DATA_NODE_HANDLE    DataNode,
  OUT AML_RD_HEADER         * ResourceDataType
  );

/** Get the data buffer and size of the DataNode.
    The Node must be a data node.

  BufferSize is always updated to the size of buffer of the DataNode.

  If:
   - the content of BufferSize is >= to the DataNode's buffer size;
   - Buffer is not NULL;
  then copy the content of the DataNode's buffer in Buffer.

  @ingroup NodeInterfaceApis

  @param  [in]      DataNode      Pointer to a data node.
  @param  [out]     Buffer        Buffer to write the data to.
                                  Optional, if NULL, only update BufferSize.
  @param  [in, out] BufferSize    Pointer holding:
                                   - At entry, the size of the Buffer;
                                   - At exit, the size of the DataNode's
                                     buffer size.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetDataNodeBuffer (
  IN      AML_DATA_NODE_HANDLE    DataNode,
      OUT UINT8                 * Buffer,     OPTIONAL
  IN  OUT UINT32                * BufferSize
  );

/** Update the ACPI DSDT/SSDT table header.

  The input SdtHeader information is copied to the tree RootNode.
  The table Length field is automatically updated.
  The checksum field is only updated when serializing the tree.

  @ingroup NodeInterfaceApis

  @param  [in]  RootNode    Pointer to a root node.
  @param  [in]  SdtHeader   Pointer to an ACPI DSDT/SSDT table header.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlUpdateRootNode (
  IN        AML_ROOT_NODE_HANDLE            RootNode,
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER   * SdtHeader
  );

/** Update an object node representing an integer with a new value.

  The object node must have one of the following OpCodes:
   - AML_BYTE_PREFIX
   - AML_WORD_PREFIX
   - AML_DWORD_PREFIX
   - AML_QWORD_PREFIX
   - AML_ZERO_OP
   - AML_ONE_OP

  The following OpCode is not supported:
   - AML_ONES_OP

  @param  [in] IntegerOpNode   Pointer an object node containing an integer.
                               Must not be an object node with an AML_ONES_OP
                               OpCode.
  @param  [in] NewInteger      New integer value to set.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlUpdateInteger (
  IN  AML_OBJECT_NODE_HANDLE    IntegerOpNode,
  IN  UINT64                    NewInteger
  );

/** Update the buffer of a data node.

  Note: The data type of the buffer's content must match the data type of the
        DataNode. This is a hard restriction to prevent undesired behaviour.

  @ingroup NodeInterfaceApis

  @param  [in]  DataNode  Pointer to a data node.
  @param  [in]  DataType  Data type of the Buffer's content.
  @param  [in]  Buffer    Buffer containing the new data. The content of
                          the Buffer is copied.
  @param  [in]  Size      Size of the Buffer.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Operation not supporter.
**/
EFI_STATUS
EFIAPI
AmlUpdateDataNode (
  IN  AML_DATA_NODE_HANDLE    DataNode,
  IN  EAML_NODE_DATA_TYPE     DataType,
  IN  UINT8                 * Buffer,
  IN  UINT32                  Size
  );

/**
  @defgroup NavigationApis Navigation APIs
  @ingroup AMLLib
  @{
    Navigation APIs allow to navigate in the AML tree. There are different
    ways to navigate in the tree by:
     - Direct relation (@ref CoreNavigationApis);
     - Enumeration: enumerate all the nodes and call a callback function
       (@ref EnumerationApis);
     - Iteration: instantiate an iterator and use it to navigate
       (@ref IteratorApis);
     - NameSpace path: use the AML namespace to navigate the tree
       (@ref NameSpaceApis).
  @}
*/

/**
  @defgroup CoreNavigationApis Core Navigation APIs
  @ingroup NavigationApis
  @{
    Core Navigation APIs allow to get a node by specifying a relation.

    E.g.: Get the parent, the n-th fixed argument, the next variable
          argument, etc.
  @}
*/

/** Get the parent node of the input Node.

  @ingroup CoreNavigationApis

  @param [in] Node  Pointer to a node.

  @return The parent node of the input Node.
          NULL otherwise.
**/
AML_NODE_HANDLE
EFIAPI
AmlGetParent (
  IN  AML_NODE_HANDLE   Node
  );

/** Get the node at the input Index in the fixed argument list of the input
    ObjectNode.

  @ingroup CoreNavigationApis

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  Index       The Index of the fixed argument to get.

  @return The node at the input Index in the fixed argument list
          of the input ObjectNode.
          NULL otherwise, e.g. if the node is not an object node, or no
          node is available at this Index.
**/
AML_NODE_HANDLE
EFIAPI
AmlGetFixedArgument (
  IN  AML_OBJECT_NODE_HANDLE  ObjectNode,
  IN  EAML_PARSE_INDEX        Index
  );

/** Get the sibling node among the nodes being in
    the same variable argument list.

  (ParentNode)  /-i                 # Child of fixed argument b
      \        /
       |- [a][b][c][d]              # Fixed Arguments
       |- {(VarArgNode)->(f)->(g)}  # Variable Arguments
             \
              \-h                   # Child of variable argument e

  Node must be in a variable list of arguments.
  Traversal Order: VarArgNode, f, g, NULL

  @ingroup CoreNavigationApis

  @param  [in]  VarArgNode  Pointer to a node.
                            Must be in a variable list of arguments.

  @return The next node after VarArgNode in the variable list of arguments.
          Return NULL if
          - VarArgNode is the last node of the list, or
          - VarArgNode is not part of a variable list of arguments.
**/
AML_NODE_HANDLE
EFIAPI
AmlGetSiblingVariableArgument (
  IN  AML_NODE_HANDLE   VarArgNode
  );

/** Get the next variable argument.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: e, f, g, NULL

  @ingroup CoreNavigationApis

  @param  [in]  Node        Pointer to a Root node or Object Node.
  @param  [in]  CurrVarArg  Pointer to the Current Variable Argument.

  @return The node after the CurrVarArg in the variable list of arguments.
          If CurrVarArg is NULL, return the first node of the
          variable argument list.
          Return NULL if
          - CurrVarArg is the last node of the list, or
          - Node does not have a variable list of arguments.
**/
AML_NODE_HANDLE
EFIAPI
AmlGetNextVariableArgument (
  IN  AML_NODE_HANDLE   Node,
  IN  AML_NODE_HANDLE   CurrVarArg
  );

/** Get the previous variable argument.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: g, f, e, NULL

  @ingroup CoreNavigationApis

  @param  [in]  Node        Pointer to a root node or an object node.
  @param  [in]  CurrVarArg  Pointer to the Current Variable Argument.

  @return The node before the CurrVarArg in the variable list of
          arguments.
          If CurrVarArg is NULL, return the last node of the
          variable list of arguments.
          Return NULL if:
          - CurrVarArg is the first node of the list, or
          - Node doesn't have a variable list of arguments.
**/
AML_NODE_HANDLE
EFIAPI
AmlGetPreviousVariableArgument (
  IN  AML_NODE_HANDLE   Node,
  IN  AML_NODE_HANDLE   CurrVarArg
  );

/**
  @defgroup EnumerationApis Enumeration APIs
  @ingroup NavigationApis
  @{
    Enumeration APIs are navigation APIs, allowing to call a callback function
    on each node enumerated. Nodes are enumerated in the AML bytestream order,
    i.e. in a depth first order.
  @}
*/

/**
  Callback function prototype used when iterating through the tree.

  @ingroup EnumerationApis

  @param  [in]      Node      The Node currently being processed.
  @param  [in, out] Context   A context for the callback function.
                                Can be optional.
  @param  [in, out] Status    End the enumeration if pointing to a value
                                evaluated to TRUE.
                                Can be optional.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
typedef
BOOLEAN
(EFIAPI * EDKII_AML_TREE_ENUM_CALLBACK) (
  IN       AML_NODE_HANDLE     Node,
  IN  OUT  VOID              * Context,    OPTIONAL
  IN  OUT  EFI_STATUS        * Status      OPTIONAL
  );

/** Enumerate all nodes of the subtree under the input Node in the AML
    bytestream order (i.e. in a depth first order), and call the CallBack
    function with the input Context.
    The prototype of the Callback function is EDKII_AML_TREE_ENUM_CALLBACK.

  @ingroup EnumerationApis

  @param  [in]      Node      Enumerate nodes of the subtree under this Node.
                              Must be a valid node.
  @param  [in]      CallBack  Callback function to call on each node.
  @param  [in, out] Context   Void pointer used to pass some information
                              to the Callback function.
                              Optional, can be NULL.
  @param  [out]     Status    Optional parameter that can be used to get
                              the status of the Callback function.
                              If used, need to be init to EFI_SUCCESS.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
BOOLEAN
EFIAPI
AmlEnumTree (
  IN      AML_NODE_HANDLE                 Node,
  IN      EDKII_AML_TREE_ENUM_CALLBACK    CallBack,
  IN  OUT VOID                          * Context,  OPTIONAL
      OUT EFI_STATUS                    * Status    OPTIONAL
  );

/**
  @defgroup NameSpaceApis NameSpace APIs
  @ingroup NavigationApis
  @{
    NameSpace APIs allow to find a node from an AML path, and reciprocally
    get the AML path of a node.

    These APIs only operate on "NameSpace nodes", i.e. nodes that are
    part of the AML namespace. These are the root node and object nodes
    acknowledged by AmlGetObjectNodeInfo in @ref NodeInterfaceApis.
  @}
*/

/** Build the absolute ASL pathname to Node.

  BufferSize is always updated to the size of the pathname.

  If:
   - the content of BufferSize is >= to the size of the pathname AND;
   - Buffer is not NULL;
  then copy the pathname in the Buffer. A buffer of the size
  MAX_ASL_NAMESTRING_SIZE is big enough to receive any ASL pathname.

  @ingroup NameSpaceApis

  @param  [in]      Node            Node to build the absolute path to.
                                    Must be a root node, or a namespace node.
  @param  [out]     Buffer          Buffer to write the path to.
                                    If NULL, only update *BufferSize.
  @param  [in, out] BufferSize      Pointer holding:
                                     - At entry, the size of the Buffer;
                                     - At exit, the size of the pathname.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
EFI_STATUS
EFIAPI
AmlGetAslPathName (
  IN      AML_NODE_HANDLE     Node,
      OUT CHAR8             * Buffer,
  IN  OUT UINT32            * BufferSize
  );

#endif // AML_CORE_INTERFACE_H_
