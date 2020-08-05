/** @file
  AML Lib.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_LIB_H_
#define AML_LIB_H_

/**
  @mainpage Dynamic AML Generation
  @{
    @par Summary
    @{
    ACPI tables are categorized as data tables and definition block
    tables. Dynamic Tables Framework currently supports generation of ACPI
    data tables. Generation of definition block tables is difficult as these
    tables are encoded in ACPI Machine Language (AML), which has a complex
    grammar.

    Dynamic AML Generation is an extension to the Dynamic tables Framework.
    One of the techniques used to simplify definition block generation is to
    fixup a template SSDT table.

    Dynamic AML aims to provide a framework that allows fixing up of an ACPI
    SSDT template with appropriate information about the hardware.

    This framework consists of an:
    - AMLLib core that implements a rich set of interfaces to parse, traverse
      and update AML data.
    - AMLLib library APIs that provides interfaces to search and updates nodes
      in the AML namespace.
    @}
  @}
*/

#include <IndustryStandard/Acpi.h>

#ifndef AML_HANDLE

/** Node handle.
*/
typedef void* AML_NODE_HANDLE;

/** Root Node handle.
*/
typedef void* AML_ROOT_NODE_HANDLE;

/** Object Node handle.
*/
typedef void* AML_OBJECT_NODE_HANDLE;

/** Data Node handle.
*/
typedef void* AML_DATA_NODE_HANDLE;

#endif // AML_HANDLE

/** Parse the definition block.

  The function parses the whole AML blob. It starts with the ACPI DSDT/SSDT
  header and then parses the AML bytestream.
  A tree structure is returned via the RootPtr.
  The tree must be deleted with the AmlDeleteTree function.

  @ingroup UserApis

  @param  [in]  DefinitionBlock   Pointer to the definition block.
  @param  [out] RootPtr           Pointer to the root node of the AML tree.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseDefinitionBlock (
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER   * DefinitionBlock,
  OUT       AML_ROOT_NODE_HANDLE          * RootPtr
  );

/** Serialize an AML definition block.

  This functions allocates memory with the "AllocateZeroPool ()"
  function. This memory is used to serialize the AML tree and is
  returned in the Table.

  @ingroup UserApis

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
  IN  AML_ROOT_NODE_HANDLE              RootNode,
  OUT EFI_ACPI_DESCRIPTION_HEADER    ** Table
  );

/** Clone a node and its children (clone a tree branch).

  The cloned branch returned is not attached to any tree.

  @ingroup UserApis

  @param  [in]  Node        Pointer to a node.
                            Node is the head of the branch to clone.
  @param  [out] ClonedNode  Pointer holding the head of the created cloned
                            branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCloneTree (
  IN  AML_NODE_HANDLE   Node,
  OUT AML_NODE_HANDLE * ClonedNode
  );

/** Delete a Node and its children.

  The Node must be removed from the tree first,
  or must be the root node.

  @ingroup UserApis

  @param  [in]  Node  Pointer to the node to delete.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteTree (
  IN  AML_NODE_HANDLE   Node
  );

/** Detach the Node from the tree.

  The function will fail if the Node is in its parent's fixed
  argument list.
  The Node is not deleted. The deletion is done separately
  from the removal.

  @ingroup UserApis

  @param  [in]  Node  Pointer to a Node.
                      Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDetachNode (
  IN  AML_NODE_HANDLE   Node
  );

/** Find a node in the AML namespace, given an ASL path and a reference Node.

   - The AslPath can be an absolute path, or a relative path from the
     reference Node;
   - Node must be a root node or a namespace node;
   - A root node is expected to be at the top of the tree.

  E.g.:
  For the following AML namespace, with the ReferenceNode being the node with
  the name "AAAA":
   - the node with the name "BBBB" can be found by looking for the ASL
     path "BBBB";
   - the root node can be found by looking for the ASL relative path "^",
      or the absolute path "\\".

  AML namespace:
  \
  \-AAAA      <- ReferenceNode
    \-BBBB

  @ingroup NameSpaceApis

  @param  [in]  ReferenceNode   Reference node.
                                If a relative path is given, the
                                search is done from this node. If
                                an absolute path is given, the
                                search is done from the root node.
                                Must be a root node or an object
                                node which is part of the
                                namespace.
  @param  [in]  AslPath         ASL path to the searched node in
                                the namespace. An ASL path name is
                                NULL terminated. Can be a relative
                                or absolute path.
                                E.g.: "\\_SB.CLU0.CPU0" or "^CPU0"
  @param  [out] OutNode         Pointer to the found node.
                                Contains NULL if not found.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
EFI_STATUS
EFIAPI
AmlFindNode (
  IN  AML_NODE_HANDLE       ReferenceNode,
  IN  CHAR8               * AslPath,
  OUT AML_NODE_HANDLE     * OutNode
  );

/**
  @defgroup UserApis User APIs
  @{
    User APIs are implemented to ease most common actions that might be done
    using the AmlLib. They allow to find specific objects like "_UID" or
    "_CRS" and to update their value. It also shows what can be done using
    AmlLib functions.
  @}
*/

/** Update the name of a DeviceOp object node.

  @ingroup UserApis

  @param  [in] DeviceOpNode   Object node representing a Device.
                              Must have an OpCode=AML_NAME_OP, SubOpCode=0.
                              OpCode/SubOpCode.
                              DeviceOp object nodes are defined in ASL
                              using the "Device ()" function.
  @param  [in] NewNameString  The new Device's name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              The input string is copied.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeviceOpUpdateName (
  IN  AML_OBJECT_NODE_HANDLE    DeviceOpNode,
  IN  CHAR8                   * NewNameString
  );

/** Update an integer value defined by a NameOp object node.

  For compatibility reasons, the NameOpNode must initially
  contain an integer.

  @ingroup UserApis

  @param  [in] NameOpNode   NameOp object node.
                            Must have an OpCode=AML_NAME_OP, SubOpCode=0.
                            NameOp object nodes are defined in ASL
                            using the "Name ()" function.
  @param  [in] NewInt       New Integer value to assign.
                            Must be a UINT64.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNameOpUpdateInteger (
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode,
  IN  UINT64                  NewInt
  );

/** Update a string value defined by a NameOp object node.

  The NameOpNode must initially contain a string.
  The EISAID ASL macro converts a string to an integer. This, it is
  not accepted.

  @ingroup UserApis

  @param  [in] NameOpNode   NameOp object node.
                            Must have an OpCode=AML_NAME_OP, SubOpCode=0.
                            NameOp object nodes are defined in ASL
                            using the "Name ()" function.
  @param  [in] NewName      New NULL terminated string to assign to
                            the NameOpNode.
                            The input string is copied.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNameOpUpdateString (
  IN        AML_OBJECT_NODE_HANDLE    NameOpNode,
  IN  CONST CHAR8                   * NewName
  );

/** Get the first Resource Data element contained in a "_CRS" object.

  In the following ASL code, the function will return the Resource Data
  node corresponding to the "QWordMemory ()" ASL macro.
  Name (_CRS, ResourceTemplate() {
      QWordMemory (...) {...},
      Interrupt (...) {...}
    }
  )

  Note:
   - The "_CRS" object must be declared using ASL "Name (Declare Named Object)".
   - "_CRS" declared using ASL "Method (Declare Control Method)" is not
     supported.

  @ingroup UserApis

  @param  [in] NameOpCrsNode  NameOp object node defining a "_CRS" object.
                              Must have an OpCode=AML_NAME_OP, SubOpCode=0.
                              NameOp object nodes are defined in ASL
                              using the "Name ()" function.
  @param  [out] OutRdNode     Pointer to the first Resource Data element of
                              the "_CRS" object. A Resource Data element
                              is stored in a data node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNameOpCrsGetFirstRdNode (
  IN  AML_OBJECT_NODE_HANDLE   NameOpCrsNode,
  OUT AML_DATA_NODE_HANDLE   * OutRdNode
  );

/** Get the Resource Data element following the CurrRdNode Resource Data.

  In the following ASL code, if CurrRdNode corresponds to the first
  "QWordMemory ()" ASL macro, the function will return the Resource Data
  node corresponding to the "Interrupt ()" ASL macro.
  Name (_CRS, ResourceTemplate() {
      QwordMemory (...) {...},
      Interrupt (...) {...}
    }
  )

  The CurrRdNode Resource Data node must be defined in an object named "_CRS"
  and defined by a "Name ()" ASL function.

  @ingroup UserApis

  @param  [in]  CurrRdNode   Pointer to the current Resource Data element of
                             the "_CRS" variable.
  @param  [out] OutRdNode    Pointer to the Resource Data element following
                             the CurrRdNode.
                             Contain a NULL pointer if CurrRdNode is the
                             last Resource Data element in the list.
                             The "End Tag" is not considered as a resource
                             data element and is not returned.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNameOpCrsGetNextRdNode (
  IN  AML_DATA_NODE_HANDLE    CurrRdNode,
  OUT AML_DATA_NODE_HANDLE  * OutRdNode
  );

/** Update the first interrupt of an Interrupt resource data node.

  The flags of the Interrupt resource data are left unchanged.

  The InterruptRdNode corresponds to the Resource Data created by the
  "Interrupt ()" ASL macro. It is an Extended Interrupt Resource Data.
  See ACPI 6.3 specification, s6.4.3.6 "Extended Interrupt Descriptor"
  for more information about Extended Interrupt Resource Data.

  @ingroup UserApis

  @param  [in]  InterruptRdNode   Pointer to the an extended interrupt
                                  resource data node.
  @param  [in]  Irq               Interrupt value to update.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdInterrupt (
  IN  AML_DATA_NODE_HANDLE    InterruptRdNode,
  IN  UINT32                  Irq
  );

/** Update the base address and length of a QWord resource data node.

  @ingroup UserApis

  @param  [in] QWordRdNode         Pointer a QWord resource data
                                   node.
  @param  [in] BaseAddress         Base address.
  @param  [in] BaseAddressLength   Base address length.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdQWord (
  IN  AML_DATA_NODE_HANDLE  QWordRdNode,
  IN  UINT64                BaseAddress,
  IN  UINT64                BaseAddressLength
  );

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

  @ingroup UserApis

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
  );

/** AML code generation for DefinitionBlock.

  Create a Root Node handle.
  It is the caller's responsibility to free the allocated memory
  with the AmlDeleteTree function.

  AmlCodeGenDefinitionBlock (TableSignature, OemId, TableID, OEMRevision) is
  equivalent to the following ASL code:
    DefinitionBlock (AMLFileName, TableSignature, ComplianceRevision,
      OemId, TableID, OEMRevision) {}
  with the ComplianceRevision set to 2 and the AMLFileName is ignored.

  @ingroup CodeGenApis

  @param[in]  TableSignature       4-character ACPI signature.
                                   Must be 'DSDT' or 'SSDT'.
  @param[in]  OemId                6-character string OEM identifier.
  @param[in]  OemTableId           8-character string OEM table identifier.
  @param[in]  OemRevision          OEM revision number.
  @param[out] DefinitionBlockTerm  The ASL Term handle representing a
                                   Definition Block.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenDefinitionBlock (
  IN  CONST CHAR8                 * TableSignature,
  IN  CONST CHAR8                 * OemId,
  IN  CONST CHAR8                 * OemTableId,
  IN        UINT32                  OemRevision,
  OUT       AML_ROOT_NODE_HANDLE  * NewRootNode
  );

/** AML code generation for a Name object node, containing a String.

  AmlCodeGenNameString ("_HID", "HID0000", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(_HID, "HID0000")

  @ingroup CodeGenApis

  @param  [in] NameString     The new variable name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              The input string is copied.
  @param [in]  String         NULL terminated String to associate to the
                              NameString.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenNameString (
  IN  CONST CHAR8                   * NameString,
  IN        CHAR8                   * String,
  IN        AML_NODE_HANDLE           ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE_HANDLE  * NewObjectNode   OPTIONAL
  );

/** AML code generation for a Name object node, containing an Integer.

  AmlCodeGenNameInteger ("_UID", 1, ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(_UID, One)

  @ingroup CodeGenApis

  @param  [in] NameString     The new variable name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              The input string is copied.
  @param [in]  Integer        Integer to associate to the NameString.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenNameInteger (
  IN  CONST CHAR8                   * NameString,
  IN        UINT64                    Integer,
  IN        AML_NODE_HANDLE           ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE_HANDLE  * NewObjectNode   OPTIONAL
  );

/** AML code generation for a Device object node.

  AmlCodeGenDevice ("COM0", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Device(COM0) {}

  @ingroup CodeGenApis

  @param  [in] NameString     The new Device's name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              The input string is copied.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenDevice (
  IN  CONST CHAR8                   * NameString,
  IN        AML_NODE_HANDLE           ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE_HANDLE  * NewObjectNode   OPTIONAL
  );

/** AML code generation for a Scope object node.

  AmlCodeGenScope ("_SB", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Scope(_SB) {}

  @ingroup CodeGenApis

  @param  [in] NameString     The new Scope's name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              The input string is copied.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenScope (
  IN  CONST CHAR8                   * NameString,
  IN        AML_NODE_HANDLE           ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE_HANDLE  * NewObjectNode   OPTIONAL
  );

#endif // AML_LIB_H_
