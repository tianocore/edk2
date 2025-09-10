/** @file
  AML Lib.

  Copyright (c) 2019 - 2023, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2023 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

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

#include <AcpiObjects.h>
#include <IndustryStandard/Acpi.h>

#ifndef AML_HANDLE

/** Node handle.
*/
typedef void *AML_NODE_HANDLE;

/** Root Node handle.
*/
typedef void *AML_ROOT_NODE_HANDLE;

/** Object Node handle.
*/
typedef void *AML_OBJECT_NODE_HANDLE;

/** Data Node handle.
*/
typedef void *AML_DATA_NODE_HANDLE;

#endif // AML_HANDLE

/** Memory attributes, _MEM (2 bits)

  Possible values are:
    0-The memory is non-cacheable
    1-The memory is cacheable (DEPRECATED)
    2-The memory is cacheable and supports
      write combining (DEPRECATED)
    3-The memory is cacheable and prefetchable

  @par Reference(s):
  - ACPI 6.5, s6.4.3.5.5 "Resource Type Specific Flags"

**/
typedef enum {
  AmlMemoryNonCacheable          = 0,
  AmlMemoryCacheable             = 1,
  AmlMemoryCacheableWriteCombine = 2,
  AmlMemoryCacheablePrefetch     = 3,
  AmlMemoryCacheablityMax        = 4
} AML_MEMORY_ATTRIBUTES_MEM;

/** Memory attributes, _MTP (2 bits)

  Possible values are:
    0-AddressRangeMemory
    1-AddressRangeReserved
    2-AddressRangeACPI
    3-AddressRangeNVS

  @par Reference(s):
  - ACPI 6.5, s6.4.3.5.5 "Resource Type Specific Flags"

**/
typedef enum {
  AmlAddressRangeMemory   = 0,
  AmlAddressRangeReserved = 1,
  AmlAddressRangeACPI     = 2,
  AmlAddressRangeNVS      = 3,
  AmlAddressRangeMax      = 4
} AML_MEMORY_ATTRIBUTES_MTP;

/** Method parameter types

  Possible values are:
    0 - AmlMethodParamTypeInteger
    1 - AmlMethodParamTypeString
    2 - AmlMethodParamTypeArg
    3 - AmlMethodParamTypeLocal

  @par Reference(s)
  - ACPI 6.5, s20.2.5 "Term Objects Encoding"

**/
typedef enum {
  AmlMethodParamTypeInteger = 0,
  AmlMethodParamTypeString  = 1,
  AmlMethodParamTypeArg     = 2,
  AmlMethodParamTypeLocal   = 3
} AML_METHOD_PARAM_TYPE;

/** AML Method parameter data
  holds the AML method parameter data.
**/
typedef union {
  UINT8     Arg;
  UINT8     Local;
  UINT64    Integer;
  VOID      *Buffer;
} AML_METHOD_PARAM_DATA;

/** structure to hold AML method parameter types
  Type  -   Type of parameter
  Data  -   holds data of parameter
            if Type is AmlMethodParamTypeInteger
              then Data is of type Integer to hold integer value.
            if Type is AmlMethodParamTypeString
              then Data contains null terminated string.
            If Type is AmlMethodParamTypeArg
              then Data contains the Argument number,
              0 to 6 are supported value.
            If Type is AmlMethodParamTypeLocal
              then Data contains the Local variable number,
              0 to 7 are supported value.
  DataSize - for future use
**/
typedef struct {
  AML_METHOD_PARAM_TYPE    Type;
  AML_METHOD_PARAM_DATA    Data;
  UINTN                    DataSize;
} AML_METHOD_PARAM;

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
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER  *DefinitionBlock,
  OUT       AML_ROOT_NODE_HANDLE         *RootPtr
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
  IN  AML_ROOT_NODE_HANDLE         RootNode,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **Table
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
  IN  AML_NODE_HANDLE  Node,
  OUT AML_NODE_HANDLE  *ClonedNode
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
  IN  AML_NODE_HANDLE  Node
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
  IN  AML_NODE_HANDLE  Node
  );

/** Attach a node in an AML tree.

  The node will be added as the last statement of the ParentNode.
  E.g.:
  ASL code corresponding to NewNode:
  Name (_UID, 0)

  ASL code corresponding to ParentNode:
  Device (PCI0) {
    Name(_HID, EISAID("PNP0A08"))
  }

  "AmlAttachNode (ParentNode, NewNode)" will result in:
  ASL code:
  Device (PCI0) {
    Name(_HID, EISAID("PNP0A08"))
    Name (_UID, 0)
  }

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be a root or an object node.
  @param  [in]  NewNode     Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlAttachNode (
  IN  AML_NODE_HANDLE  ParentNode,
  IN  AML_NODE_HANDLE  NewNode
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
  IN  AML_NODE_HANDLE  ReferenceNode,
  IN  CONST CHAR8      *AslPath,
  OUT AML_NODE_HANDLE  *OutNode
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
  IN  AML_OBJECT_NODE_HANDLE  DeviceOpNode,
  IN  CONST CHAR8             *NewNameString
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
  IN        AML_OBJECT_NODE_HANDLE  NameOpNode,
  IN  CONST CHAR8                   *NewName
  );

/** Get the first Resource Data element contained in a named object.

  In the following ASL code, the function will return the Resource Data
  node corresponding to the "QWordMemory ()" ASL macro.
  Name (_CRS, ResourceTemplate() {
      QWordMemory (...) {...},
      Interrupt (...) {...}
    }
  )

  Note:
  "_CRS" names defined as methods are not handled by this function.
  They must be defined as names, using the "Name ()" statement.

  @ingroup UserApis

  @param  [in] NameOpNode   NameOp object node defining a named object.
                            Must have an OpCode=AML_NAME_OP, SubOpCode=0.
                            NameOp object nodes are defined in ASL
                            using the "Name ()" function.
  @param  [out] OutRdNode   Pointer to the first Resource Data element of
                            the named object. A Resource Data element
                            is stored in a data node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNameOpGetFirstRdNode (
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode,
  OUT AML_DATA_NODE_HANDLE    *OutRdNode
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

  Note:
  "_CRS" names defined as methods are not handled by this function.
  They must be defined as names, using the "Name ()" statement.

  @ingroup UserApis

  @param  [in]  CurrRdNode   Pointer to the current Resource Data element of
                             the named object.
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
AmlNameOpGetNextRdNode (
  IN  AML_DATA_NODE_HANDLE  CurrRdNode,
  OUT AML_DATA_NODE_HANDLE  *OutRdNode
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
  IN  AML_DATA_NODE_HANDLE  InterruptRdNode,
  IN  UINT32                Irq
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

/** Code generation for the "DWordIO ()" ASL function.

  The Resource Data effectively created is a DWord Address Space Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.5.2 "DWord Address Space Descriptor".
   - s19.6.34 "DWordIO".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  See ACPI 6.4 spec, s19.6.34 for more.

  @param [in]  IsResourceConsumer   ResourceUsage parameter.
  @param [in]  IsMinFixed           Minimum address is fixed.
  @param [in]  IsMaxFixed           Maximum address is fixed.
  @param [in]  IsPosDecode          Decode parameter
  @param [in]  IsaRanges            Possible values are:
                                     0-Reserved
                                     1-NonISAOnly
                                     2-ISAOnly
                                     3-EntireRange
  @param [in]  AddressGranularity   Address granularity.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  AddressTranslation   Address translation.
  @param [in]  RangeLength          Range length.
  @param [in]  ResourceSourceIndex  Resource Source index.
                                    Not supported. Must be 0.
  @param [in]  ResourceSource       Resource Source.
                                    Not supported. Must be NULL.
  @param [in]  IsDenseTranslation   TranslationDensity parameter.
  @param [in]  IsTypeStatic         TranslationType parameter.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdDWordIo (
  IN        BOOLEAN IsResourceConsumer,
  IN        BOOLEAN IsMinFixed,
  IN        BOOLEAN IsMaxFixed,
  IN        BOOLEAN IsPosDecode,
  IN        UINT8 IsaRanges,
  IN        UINT32 AddressGranularity,
  IN        UINT32 AddressMinimum,
  IN        UINT32 AddressMaximum,
  IN        UINT32 AddressTranslation,
  IN        UINT32 RangeLength,
  IN        UINT8 ResourceSourceIndex,
  IN  CONST CHAR8 *ResourceSource,
  IN        BOOLEAN IsDenseTranslation,
  IN        BOOLEAN IsTypeStatic,
  IN        AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT       AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "DWordMemory ()" ASL function.

  The Resource Data effectively created is a DWord Address Space Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.5.2 "DWord Address Space Descriptor".
   - s19.6.35 "DWordMemory".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  See ACPI 6.4 spec, s19.6.35 for more.

  @param [in]  IsResourceConsumer   ResourceUsage parameter.
  @param [in]  IsPosDecode          Decode parameter
  @param [in]  IsMinFixed           Minimum address is fixed.
  @param [in]  IsMaxFixed           Maximum address is fixed.
  @param [in]  Cacheable            Possible values are:
                                    0-The memory is non-cacheable
                                    1-The memory is cacheable
                                    2-The memory is cacheable and supports
                                      write combining
                                    3-The memory is cacheable and prefetchable
  @param [in]  IsReadWrite          ReadAndWrite parameter.
  @param [in]  AddressGranularity   Address granularity.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  AddressTranslation   Address translation.
  @param [in]  RangeLength          Range length.
  @param [in]  ResourceSourceIndex  Resource Source index.
                                    Not supported. Must be 0.
  @param [in]  ResourceSource       Resource Source.
                                    Not supported. Must be NULL.
  @param [in]  MemoryRangeType      Possible values are:
                                      0-AddressRangeMemory
                                      1-AddressRangeReserved
                                      2-AddressRangeACPI
                                      3-AddressRangeNVS
  @param [in]  IsTypeStatic         TranslationType parameter.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdDWordMemory (
  IN        BOOLEAN IsResourceConsumer,
  IN        BOOLEAN IsPosDecode,
  IN        BOOLEAN IsMinFixed,
  IN        BOOLEAN IsMaxFixed,
  IN        AML_MEMORY_ATTRIBUTES_MEM Cacheable,
  IN        BOOLEAN IsReadWrite,
  IN        UINT32 AddressGranularity,
  IN        UINT32 AddressMinimum,
  IN        UINT32 AddressMaximum,
  IN        UINT32 AddressTranslation,
  IN        UINT32 RangeLength,
  IN        UINT8 ResourceSourceIndex,
  IN  CONST CHAR8 *ResourceSource,
  IN        AML_MEMORY_ATTRIBUTES_MTP MemoryRangeType,
  IN        BOOLEAN IsTypeStatic,
  IN        AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT       AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "Memory32Fixed ()" ASL macro.

  The Resource Data effectively created is a 32-bit Memory Resource
  Data. Cf ACPI 6.4:
   - s19.6.83 "Memory Resource Descriptor Macro".
   - s19.2.8 "Memory32FixedTerm".

  See ACPI 6.4 spec, s19.2.8 for more.

  @param [in]  IsReadWrite          ReadAndWrite parameter.
  @param [in]  Address              AddressBase parameter.
  @param [in]  RangeLength          Range length.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewMemNode           If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdMemory32Fixed (
  BOOLEAN                 IsReadWrite,
  UINT32                  Address,
  UINT32                  RangeLength,
  AML_OBJECT_NODE_HANDLE  NameOpNode,
  AML_DATA_NODE_HANDLE    *NewMemNode
  );

/** Code generation for the "WordBusNumber ()" ASL function.

  The Resource Data effectively created is a Word Address Space Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.5.3 "Word Address Space Descriptor".
   - s19.6.149 "WordBusNumber".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  See ACPI 6.4 spec, s19.6.149 for more.

  @param [in]  IsResourceConsumer   ResourceUsage parameter.
  @param [in]  IsMinFixed           Minimum address is fixed.
  @param [in]  IsMaxFixed           Maximum address is fixed.
  @param [in]  IsPosDecode          Decode parameter
  @param [in]  AddressGranularity   Address granularity.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  AddressTranslation   Address translation.
  @param [in]  RangeLength          Range length.
  @param [in]  ResourceSourceIndex  Resource Source index.
                                    Not supported. Must be 0.
  @param [in]  ResourceSource       Resource Source.
                                    Not supported. Must be NULL.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdWordBusNumber (
  IN        BOOLEAN IsResourceConsumer,
  IN        BOOLEAN IsMinFixed,
  IN        BOOLEAN IsMaxFixed,
  IN        BOOLEAN IsPosDecode,
  IN        UINT16 AddressGranularity,
  IN        UINT16 AddressMinimum,
  IN        UINT16 AddressMaximum,
  IN        UINT16 AddressTranslation,
  IN        UINT16 RangeLength,
  IN        UINT8 ResourceSourceIndex,
  IN  CONST CHAR8 *ResourceSource,
  IN        AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT       AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "WordIO ()" ASL function.

  The Resource Data effectively created is a Word Address Space Resource
  Data. Cf ACPI 6.5:
   - s6.4.3.5.3 "Word Address Space Descriptor".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @param [in]  IsResourceConsumer   ResourceUsage parameter.
  @param [in]  IsMinFixed           Minimum address is fixed.
  @param [in]  IsMaxFixed           Maximum address is fixed.
  @param [in]  IsPosDecode          Decode parameter
  @param [in]  IsaRanges            Possible values are:
                                     0-Reserved
                                     1-NonISAOnly
                                     2-ISAOnly
                                     3-EntireRange
  @param [in]  AddressGranularity   Address granularity.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  AddressTranslation   Address translation.
  @param [in]  RangeLength          Range length.
  @param [in]  ResourceSourceIndex  Resource Source index.
                                    Not supported. Must be 0.
  @param [in]  ResourceSource       Resource Source.
                                    Not supported. Must be NULL.
  @param [in]  IsDenseTranslation   TranslationDensity parameter.
  @param [in]  IsTypeStatic         TranslationType parameter.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdWordIo (
  IN        BOOLEAN IsResourceConsumer,
  IN        BOOLEAN IsMinFixed,
  IN        BOOLEAN IsMaxFixed,
  IN        BOOLEAN IsPosDecode,
  IN        UINT8 IsaRanges,
  IN        UINT16 AddressGranularity,
  IN        UINT16 AddressMinimum,
  IN        UINT16 AddressMaximum,
  IN        UINT16 AddressTranslation,
  IN        UINT16 RangeLength,
  IN        UINT8 ResourceSourceIndex,
  IN  CONST CHAR8 *ResourceSource,
  IN        BOOLEAN IsDenseTranslation,
  IN        BOOLEAN IsTypeStatic,
  IN        AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT       AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "QWordIO ()" ASL function.

  The Resource Data effectively created is a QWord Address Space Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.5.1 "QWord Address Space Descriptor".
   - s19.6.109 "QWordIO".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  See ACPI 6.4 spec, s19.6.109 for more.

  @param [in]  IsResourceConsumer   ResourceUsage parameter.
  @param [in]  IsMinFixed           Minimum address is fixed.
  @param [in]  IsMaxFixed           Maximum address is fixed.
  @param [in]  IsPosDecode          Decode parameter
  @param [in]  IsaRanges            Possible values are:
                                     0-Reserved
                                     1-NonISAOnly
                                     2-ISAOnly
                                     3-EntireRange
  @param [in]  AddressGranularity   Address granularity.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  AddressTranslation   Address translation.
  @param [in]  RangeLength          Range length.
  @param [in]  ResourceSourceIndex  Resource Source index.
                                    Unused. Must be 0.
  @param [in]  ResourceSource       Resource Source.
                                    Unused. Must be NULL.
  @param [in]  IsDenseTranslation   TranslationDensity parameter.
  @param [in]  IsTypeStatic         TranslationType parameter.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdQWordIo (
  IN        BOOLEAN IsResourceConsumer,
  IN        BOOLEAN IsMinFixed,
  IN        BOOLEAN IsMaxFixed,
  IN        BOOLEAN IsPosDecode,
  IN        UINT8 IsaRanges,
  IN        UINT64 AddressGranularity,
  IN        UINT64 AddressMinimum,
  IN        UINT64 AddressMaximum,
  IN        UINT64 AddressTranslation,
  IN        UINT64 RangeLength,
  IN        UINT8 ResourceSourceIndex,
  IN  CONST CHAR8 *ResourceSource,
  IN        BOOLEAN IsDenseTranslation,
  IN        BOOLEAN IsTypeStatic,
  IN        AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT       AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "QWordMemory ()" ASL function.

  The Resource Data effectively created is a QWord Address Space Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.5.1 "QWord Address Space Descriptor".
   - s19.6.110 "QWordMemory".

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  See ACPI 6.4 spec, s19.6.110 for more.

  @param [in]  IsResourceConsumer   ResourceUsage parameter.
  @param [in]  IsPosDecode          Decode parameter.
  @param [in]  IsMinFixed           Minimum address is fixed.
  @param [in]  IsMaxFixed           Maximum address is fixed.
  @param [in]  Cacheable            Possible values are:
                                    0-The memory is non-cacheable
                                    1-The memory is cacheable
                                    2-The memory is cacheable and supports
                                      write combining
                                    3-The memory is cacheable and prefetchable
  @param [in]  IsReadWrite          ReadAndWrite parameter.
  @param [in]  AddressGranularity   Address granularity.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  AddressTranslation   Address translation.
  @param [in]  RangeLength          Range length.
  @param [in]  ResourceSourceIndex  Resource Source index.
                                    Not supported. Must be 0.
  @param [in]  ResourceSource       Resource Source.
                                    Not supported. Must be NULL.
  @param [in]  MemoryRangeType      Possible values are:
                                      0-AddressRangeMemory
                                      1-AddressRangeReserved
                                      2-AddressRangeACPI
                                      3-AddressRangeNVS
  @param [in]  IsTypeStatic         TranslationType parameter.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdQWordMemory (
  IN        BOOLEAN IsResourceConsumer,
  IN        BOOLEAN IsPosDecode,
  IN        BOOLEAN IsMinFixed,
  IN        BOOLEAN IsMaxFixed,
  IN        AML_MEMORY_ATTRIBUTES_MEM Cacheable,
  IN        BOOLEAN IsReadWrite,
  IN        UINT64 AddressGranularity,
  IN        UINT64 AddressMinimum,
  IN        UINT64 AddressMaximum,
  IN        UINT64 AddressTranslation,
  IN        UINT64 RangeLength,
  IN        UINT8 ResourceSourceIndex,
  IN  CONST CHAR8 *ResourceSource,
  IN        AML_MEMORY_ATTRIBUTES_MTP MemoryRangeType,
  IN        BOOLEAN IsTypeStatic,
  IN        AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT       AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "Interrupt ()" ASL function.

  The Resource Data effectively created is an Extended Interrupt Resource
  Data. Cf ACPI 6.4:
   - s6.4.3.6 "Extended Interrupt Descriptor"
   - s19.6.64 "Interrupt (Interrupt Resource Descriptor Macro)"

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @ingroup CodeGenApis

  @param  [in]  ResourceConsumer The device consumes the specified interrupt
                                 or produces it for use by a child device.
  @param  [in]  EdgeTriggered    The interrupt is edge triggered or
                                 level triggered.
  @param  [in]  ActiveLow        The interrupt is active-high or active-low.
  @param  [in]  Shared           The interrupt can be shared with other
                                 devices or not (Exclusive).
  @param  [in]  IrqList          Interrupt list. Must be non-NULL.
  @param  [in]  IrqCount         Interrupt count. Must be non-zero.
  @param  [in]  NameOpNode       NameOp object node defining a named object.
                                 If provided, append the new resource data node
                                 to the list of resource data elements of this
                                 node.
  @param  [out] NewRdNode        If provided and success,
                                 contain the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdInterrupt (
  IN  BOOLEAN                 ResourceConsumer,
  IN  BOOLEAN                 EdgeTriggered,
  IN  BOOLEAN                 ActiveLow,
  IN  BOOLEAN                 Shared,
  IN  UINT32                  *IrqList,
  IN  UINT8                   IrqCount,
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode  OPTIONAL,
  OUT AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the "IO ()" ASL function.

  The Resource Data effectively created is a IO Resource
  Data. Cf ACPI 6.5:
   - s19.6.65 IO (IO Resource Descriptor Macro)
   - s6.4.2.5 I/O Port Descriptor

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @param [in]  IsDecoder16          Decoder parameter.
                                    TRUE if 16-bit decoder.
                                    FALSE if 10-bit decoder.
  @param [in]  AddressMinimum       Minimum address.
  @param [in]  AddressMaximum       Maximum address.
  @param [in]  Alignment            Alignment.
  @param [in]  RangeLength          Range length.
  @param [in]  NameOpNode           NameOp object node defining a named object.
                                    If provided, append the new resource data
                                    node to the list of resource data elements
                                    of this node.
  @param [out] NewRdNode            If provided and success,
                                    contain the created node.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdIo (
  IN  BOOLEAN IsDecoder16,
  IN  UINT16 AddressMinimum,
  IN  UINT16 AddressMaximum,
  IN  UINT8 Alignment,
  IN  UINT8 RangeLength,
  IN  AML_OBJECT_NODE_HANDLE NameOpNode, OPTIONAL
  OUT AML_DATA_NODE_HANDLE  *NewRdNode  OPTIONAL
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
  IN  CONST CHAR8                 *TableSignature,
  IN  CONST CHAR8                 *OemId,
  IN  CONST CHAR8                 *OemTableId,
  IN        UINT32                OemRevision,
  OUT       AML_ROOT_NODE_HANDLE  *NewRootNode
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
  IN  CONST CHAR8                   *NameString,
  IN  CONST CHAR8                   *String,
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
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
  IN  CONST CHAR8                   *NameString,
  IN        UINT64                  Integer,
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
  );

/** AML code generation for a Name object node, containing a Package.

  AmlCodeGenNamePackage ("PKG0", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(PKG0, Package () {})

  @ingroup CodeGenApis

  @param [in]  NameString     The new variable name.
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
AmlCodeGenNamePackage (
  IN  CONST CHAR8 *NameString,
  IN        AML_NODE_HANDLE ParentNode, OPTIONAL
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
  );

/** AML code generation for a Name object node, containing a ResourceTemplate.

  AmlCodeGenNameResourceTemplate ("PRS0", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(PRS0, ResourceTemplate () {})

  @ingroup CodeGenApis

  @param [in]  NameString     The new variable name.
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
AmlCodeGenNameResourceTemplate (
  IN  CONST CHAR8 *NameString,
  IN        AML_NODE_HANDLE ParentNode, OPTIONAL
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
  );

/** AML code generation for a Name object node, containing a String.

  AmlCodeGenNameUnicodeString ("_STR", L"String", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(_STR, Unicode ("String"))

  @ingroup CodeGenApis

  @param  [in] NameString     The new variable name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              The input string is copied.
  @param [in]  String         NULL terminated Unicode String to associate to the
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
AmlCodeGenNameUnicodeString (
  IN  CONST CHAR8                   *NameString,
  IN        CHAR16                  *String,
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
  );

/** Add a _PRT entry.

  AmlCodeGenPrtEntry (0x0FFFF, 0, "LNKA", 0, PrtNameNode) is
  equivalent of the following ASL code:
    Package (4) {
      0x0FFFF, // Address: Device address (([Device Id] << 16) | 0xFFFF).
      0,       // Pin: PCI pin number of the device (0-INTA, ...).
      LNKA     // Source: Name of the device that allocates the interrupt
               // to which the above pin is connected.
      0        // Source Index: Source is assumed to only describe one
               // interrupt, so let it to index 0.
    }

  The package is added at the tail of the list of the input _PRT node
  name:
    Name (_PRT, Package () {
      [Pre-existing _PRT entries],
      [Newly created _PRT entry]
    })

  Cf. ACPI 6.4, s6.2.13 "_PRT (PCI Routing Table)"

  @ingroup CodeGenApis

  @param [in]  Address        Address. Cf ACPI 6.4 specification, Table 6.2:
                              "ADR Object Address Encodings":
                              High word-Device #, Low word-Function #. (for
                              example, device 3, function 2 is 0x00030002).
                              To refer to all the functions on a device #,
                              use a function number of FFFF).
  @param [in]  Pin            PCI pin number of the device (0-INTA ... 3-INTD).
                              Must be between 0-3.
  @param [in]  LinkName       Link Name, i.e. device in the AML NameSpace
                              describing the interrupt used.
                              The input string is copied.
  @param [in]  SourceIndex    Source index or GSIV.
  @param [in]  PrtNameNode    Prt Named node to add the object to ....

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlAddPrtEntry (
  IN        UINT32                  Address,
  IN        UINT8                   Pin,
  IN  CONST CHAR8                   *LinkName,
  IN        UINT32                  SourceIndex,
  IN        AML_OBJECT_NODE_HANDLE  PrtNameNode
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
  IN  CONST CHAR8                   *NameString,
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
  );

/** AML code generation for a ThermalZone object node.

  AmlCodeGenThermalZone ("TZ00", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    ThermalZone(TZ00) {}

  @ingroup CodeGenApis

  @param  [in] NameString     The new ThermalZone's name.
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
AmlCodeGenThermalZone (
  IN  CONST CHAR8                   *NameString,
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
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
  IN  CONST CHAR8                   *NameString,
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode   OPTIONAL
  );

/** AML code generation for a method returning a NameString.

  AmlCodeGenMethodRetNameString (
    "MET0", "_CRS", 1, TRUE, 3, ParentNode, NewObjectNode
    );
  is equivalent of the following ASL code:
    Method(MET0, 1, Serialized, 3) {
      Return (_CRS)
    }

  The ASL parameters "ReturnType" and "ParameterTypes" are not asked
  in this function. They are optional parameters in ASL.

  @ingroup CodeGenApis

  @param [in]  MethodNameString     The new Method's name.
                                    Must be a NULL-terminated ASL NameString
                                    e.g.: "MET0", "_SB.MET0", etc.
                                    The input string is copied.
  @param [in]  ReturnedNameString   The name of the object returned by the
                                    method. Optional parameter, can be:
                                     - NULL (ignored).
                                     - A NULL-terminated ASL NameString.
                                       e.g.: "MET0", "_SB.MET0", etc.
                                       The input string is copied.
  @param [in]  NumArgs              Number of arguments.
                                    Must be 0 <= NumArgs <= 6.
  @param [in]  IsSerialized         TRUE is equivalent to Serialized.
                                    FALSE is equivalent to NotSerialized.
                                    Default is NotSerialized in ASL spec.
  @param [in]  SyncLevel            Synchronization level for the method.
                                    Must be 0 <= SyncLevel <= 15.
                                    Default is 0 in ASL.
  @param [in]  ParentNode           If provided, set ParentNode as the parent
                                    of the node created.
  @param [out] NewObjectNode        If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenMethodRetNameString (
  IN  CONST CHAR8                   *MethodNameString,
  IN  CONST CHAR8                   *ReturnedNameString   OPTIONAL,
  IN        UINT8                   NumArgs,
  IN        BOOLEAN                 IsSerialized,
  IN        UINT8                   SyncLevel,
  IN        AML_NODE_HANDLE         ParentNode           OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode        OPTIONAL
  );

/** AML code generation for a method returning an Integer.

  AmlCodeGenMethodRetInteger (
    "_CBA", 0, 1, TRUE, 3, ParentNode, NewObjectNode
    );
  is equivalent of the following ASL code:
    Method(_CBA, 1, Serialized, 3) {
      Return (0)
    }

  The ASL parameters "ReturnType" and "ParameterTypes" are not asked
  in this function. They are optional parameters in ASL.

  @param [in]  MethodNameString     The new Method's name.
                                    Must be a NULL-terminated ASL NameString
                                    e.g.: "MET0", "_SB.MET0", etc.
                                    The input string is copied.
  @param [in]  ReturnedInteger      The value of the integer returned by the
                                    method.
  @param [in]  NumArgs              Number of arguments.
                                    Must be 0 <= NumArgs <= 6.
  @param [in]  IsSerialized         TRUE is equivalent to Serialized.
                                    FALSE is equivalent to NotSerialized.
                                    Default is NotSerialized in ASL spec.
  @param [in]  SyncLevel            Synchronization level for the method.
                                    Must be 0 <= SyncLevel <= 15.
                                    Default is 0 in ASL.
  @param [in]  ParentNode           If provided, set ParentNode as the parent
                                    of the node created.
  @param [out] NewObjectNode        If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenMethodRetInteger (
  IN  CONST CHAR8                   *MethodNameString,
  IN        UINT64                  ReturnedInteger,
  IN        UINT8                   NumArgs,
  IN        BOOLEAN                 IsSerialized,
  IN        UINT8                   SyncLevel,
  IN        AML_NODE_HANDLE         ParentNode           OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode        OPTIONAL
  );

/** AML code generation for a method returning a NameString that takes an
    integer argument.

  AmlCodeGenMethodRetNameStringIntegerArgument (
    "MET0", "MET1", 1, TRUE, 3, 5, ParentNode, NewObjectNode
    );
  is equivalent of the following ASL code:
    Method(MET0, 1, Serialized, 3) {
      Return (MET1 (5))
    }

  The ASL parameters "ReturnType" and "ParameterTypes" are not asked
  in this function. They are optional parameters in ASL.

  @param [in]  MethodNameString     The new Method's name.
                                    Must be a NULL-terminated ASL NameString
                                    e.g.: "MET0", "_SB.MET0", etc.
                                    The input string is copied.
  @param [in]  ReturnedNameString   The name of the object returned by the
                                    method. Optional parameter, can be:
                                     - NULL (ignored).
                                     - A NULL-terminated ASL NameString.
                                       e.g.: "MET0", "_SB.MET0", etc.
                                       The input string is copied.
  @param [in]  NumArgs              Number of arguments.
                                    Must be 0 <= NumArgs <= 6.
  @param [in]  IsSerialized         TRUE is equivalent to Serialized.
                                    FALSE is equivalent to NotSerialized.
                                    Default is NotSerialized in ASL spec.
  @param [in]  SyncLevel            Synchronization level for the method.
                                    Must be 0 <= SyncLevel <= 15.
                                    Default is 0 in ASL.
  @param [in]  IntegerArgument      Argument to pass to the NameString.
  @param [in]  ParentNode           If provided, set ParentNode as the parent
                                    of the node created.
  @param [out] NewObjectNode        If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenMethodRetNameStringIntegerArgument (
  IN  CONST CHAR8                   *MethodNameString,
  IN  CONST CHAR8                   *ReturnedNameString   OPTIONAL,
  IN        UINT8                   NumArgs,
  IN        BOOLEAN                 IsSerialized,
  IN        UINT8                   SyncLevel,
  IN        UINT64                  IntegerArgument,
  IN        AML_NODE_HANDLE         ParentNode           OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode        OPTIONAL
  );

/** Create a _LPI name.

  AmlCreateLpiNode ("_LPI", 0, 1, ParentNode, &LpiNode) is
  equivalent of the following ASL code:
    Name (_LPI, Package (
                  0,  // Revision
                  1,  // LevelId
                  0   // Count
                  ))

  This function doesn't define any LPI state. As shown above, the count
  of _LPI state is set to 0.
  The AmlAddLpiState () function must be used to add LPI states.

  Cf ACPI 6.3 specification, s8.4.4 "Lower Power Idle States".

  @ingroup CodeGenApis

  @param [in]  LpiNameString  The new LPI 's object name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "_LPI", "DEV0.PLPI", etc.
                              The input string is copied.
  @param [in]  Revision       Revision number of the _LPI states.
  @param [in]  LevelId        A platform defined number that identifies the
                              level of hierarchy of the processor node to
                              which the LPI states apply.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewLpiNode     If success, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateLpiNode (
  IN  CONST CHAR8                   *LpiNameString,
  IN        UINT16                  Revision,
  IN        UINT64                  LevelId,
  IN        AML_NODE_HANDLE         ParentNode   OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewLpiNode   OPTIONAL
  );

/** Add an _LPI state to a LPI node created using AmlCreateLpiNode ().

  AmlAddLpiState () increments the Count of LPI states in the LPI node by one,
  and adds the following package:
    Package() {
      MinResidency,
      WorstCaseWakeLatency,
      Flags,
      ArchFlags,
      ResCntFreq,
      EnableParentState,
      (GenericRegisterDescriptor != NULL) ?           // Entry method. If a
        ResourceTemplate(GenericRegisterDescriptor) : // Register is given,
        Integer,                                      // use it. Use the
                                                      // Integer otherwise.
      ResourceTemplate() {                            // NULL Residency Counter
        Register (SystemMemory, 0, 0, 0, 0)
      },
      ResourceTemplate() {                            // NULL Usage Counter
        Register (SystemMemory, 0, 0, 0, 0)
      },
      ""                                              // NULL State Name
    },

  Cf ACPI 6.3 specification, s8.4.4 "Lower Power Idle States".

  @ingroup CodeGenApis

  @param [in]  MinResidency               Minimum Residency.
  @param [in]  WorstCaseWakeLatency       Worst case wake-up latency.
  @param [in]  Flags                      Flags.
  @param [in]  ArchFlags                  Architectural flags.
  @param [in]  ResCntFreq                 Residency Counter Frequency.
  @param [in]  EnableParentState          Enabled Parent State.
  @param [in]  GenericRegisterDescriptor  Entry Method.
                                          If not NULL, use this Register to
                                          describe the entry method address.
  @param [in]  Integer                    Entry Method.
                                          If GenericRegisterDescriptor is NULL,
                                          take this value.
  @param [in]  ResidencyCounterRegister   If not NULL, use it to populate the
                                          residency counter register.
  @param [in]  UsageCounterRegister       If not NULL, use it to populate the
                                          usage counter register.
  @param [in]  StateName                  If not NULL, use it to populate the
                                          state name.
  @param [in]  LpiNode                    Lpi node created with the function
                                          AmlCreateLpiNode to which the new LPI
                                          state is appended.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlAddLpiState (
  IN  UINT32                                  MinResidency,
  IN  UINT32                                  WorstCaseWakeLatency,
  IN  UINT32                                  Flags,
  IN  UINT32                                  ArchFlags,
  IN  UINT32                                  ResCntFreq,
  IN  UINT32                                  EnableParentState,
  IN  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE  *GenericRegisterDescriptor   OPTIONAL,
  IN  UINT64                                  Integer                     OPTIONAL,
  IN  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE  *ResidencyCounterRegister    OPTIONAL,
  IN  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE  *UsageCounterRegister        OPTIONAL,
  IN  CONST CHAR8                             *StateName                   OPTIONAL,
  IN  AML_OBJECT_NODE_HANDLE                  LpiNode
  );

/** AML code generation for a _DSD device data object.

  AmlAddDeviceDataDescriptorPackage (Uuid, DsdNode, PackageNode) is
  equivalent of the following ASL code:
    ToUUID(Uuid),
    Package () {}

  Cf ACPI 6.4 specification, s6.2.5 "_DSD (Device Specific Data)".

  _DSD (Device Specific Data) Implementation Guide
  https://github.com/UEFI/DSD-Guide
  Per s3. "'Well-Known _DSD UUIDs and Data Structure Formats'"
  If creating a Device Properties data then UUID daffd814-6eba-4d8c-8a91-bc9bbf4aa301 should be used.

  @param [in]  Uuid           The Uuid of the descriptor to be created
  @param [in]  DsdNode        Node of the DSD Package.
  @param [out] PackageNode    If success, contains the created package node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlAddDeviceDataDescriptorPackage (
  IN  CONST EFI_GUID                *Uuid,
  IN        AML_OBJECT_NODE_HANDLE  DsdNode,
  OUT       AML_OBJECT_NODE_HANDLE  *PackageNode
  );

/** AML code generation to add a package with a name and value,
    to a parent package.
    This is useful to build the _DSD package but can be used in other cases.

  AmlAddNameIntegerPackage ("Name", Value, PackageNode) is
  equivalent of the following ASL code:
    Package (2) {"Name", Value}

  Cf ACPI 6.4 specification, s6.2.5 "_DSD (Device Specific Data)".

  @param [in]  Name           String to place in first entry of package
  @param [in]  Value          Integer to place in second entry of package
  @param [in]  PackageNode    Package to add new sub package to.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlAddNameIntegerPackage (
  IN CONST CHAR8             *Name,
  IN UINT64                  Value,
  IN AML_OBJECT_NODE_HANDLE  PackageNode
  );

/** Create a _CPC node.

  Creates and optionally adds the following node
   Name(_CPC, Package()
   {
    NumEntries,                              // Integer
    Revision,                                // Integer
    HighestPerformance,                      // Integer or Buffer (Resource Descriptor)
    NominalPerformance,                      // Integer or Buffer (Resource Descriptor)
    LowestNonlinearPerformance,              // Integer or Buffer (Resource Descriptor)
    LowestPerformance,                       // Integer or Buffer (Resource Descriptor)
    GuaranteedPerformanceRegister,           // Buffer (Resource Descriptor)
    DesiredPerformanceRegister ,             // Buffer (Resource Descriptor)
    MinimumPerformanceRegister ,             // Buffer (Resource Descriptor)
    MaximumPerformanceRegister ,             // Buffer (Resource Descriptor)
    PerformanceReductionToleranceRegister,   // Buffer (Resource Descriptor)
    TimeWindowRegister,                      // Buffer (Resource Descriptor)
    CounterWraparoundTime,                   // Integer or Buffer (Resource Descriptor)
    ReferencePerformanceCounterRegister,     // Buffer (Resource Descriptor)
    DeliveredPerformanceCounterRegister,     // Buffer (Resource Descriptor)
    PerformanceLimitedRegister,              // Buffer (Resource Descriptor)
    CPPCEnableRegister                       // Buffer (Resource Descriptor)
    AutonomousSelectionEnable,               // Integer or Buffer (Resource Descriptor)
    AutonomousActivityWindowRegister,        // Buffer (Resource Descriptor)
    EnergyPerformancePreferenceRegister,     // Buffer (Resource Descriptor)
    ReferencePerformance                     // Integer or Buffer (Resource Descriptor)
    LowestFrequency,                         // Integer or Buffer (Resource Descriptor)
    NominalFrequency                         // Integer or Buffer (Resource Descriptor)
  })

  If resource buffer is NULL then integer will be used.

  Cf. ACPI 6.4, s8.4.7.1 _CPC (Continuous Performance Control)

  @ingroup CodeGenApis

  @param [in]  CpcInfo               CpcInfo object
  @param [in]  ParentNode            If provided, set ParentNode as the parent
                                     of the node created.
  @param [out] NewCpcNode            If success and provided, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateCpcNode (
  IN  AML_CPC_INFO            *CpcInfo,
  IN  AML_NODE_HANDLE         ParentNode   OPTIONAL,
  OUT AML_OBJECT_NODE_HANDLE  *NewCpcNode   OPTIONAL
  );

/** AML code generation to add a NameString to the package in a named node.


  @param [in]  NameString     NameString to add
  @param [in]  NamedNode      Node to add the string to the included package.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlAddNameStringToNamedPackage (
  IN CONST CHAR8             *NameString,
  IN AML_OBJECT_NODE_HANDLE  NamedNode
  );

/** Add an integer value to the named package node.

  AmlCodeGenNamePackage ("_CID", NULL, &PackageNode);
  AmlGetEisaIdFromString ("PNP0A03", &EisaId);
  AmlAddIntegerToNamedPackage (EisaId, NameNode);
  AmlGetEisaIdFromString ("PNP0A08", &EisaId);
  AmlAddIntegerToNamedPackage (EisaId, NameNode);

  equivalent of the following ASL code:
  Name (_CID, Package (0x02)  // _CID: Compatible ID
  {
      EisaId ("PNP0A03"),
      EisaId ("PNP0A08")
  })

  The package is added at the tail of the list of the input package node
  name:
    Name ("NamePackageNode", Package () {
      [Pre-existing package entries],
      [Newly created integer entry]
    })


  @ingroup CodeGenApis

  @param [in]       Integer       Integer value that need to be added to package node.
  @param [in, out]  NameNode      Package named node to add the object to.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval Others                  Error occurred during the operation.
**/
EFI_STATUS
EFIAPI
AmlAddIntegerToNamedPackage (
  IN        UINT32                  Integer,
  IN  OUT   AML_OBJECT_NODE_HANDLE  NameNode
  );

/** AML code generation to invoke/call another method.

  This method is a subset implementation of MethodInvocation
  defined in the ACPI specification 6.5,
  section 20.2.5 "Term Objects Encoding".
  Added integer, string, ArgObj and LocalObj support.

  Example 1:
    AmlCodeGenInvokeMethod ("MET0", 0, NULL, ParentNode);
    is equivalent to the following ASL code:
      MET0 ();

  Example 2:
    AML_METHOD_PARAM  Param[4];
    Param[0].Data.Integer = 0x100;
    Param[0].Type = AmlMethodParamTypeInteger;
    Param[1].Data.Buffer = "TEST";
    Param[1].Type = AmlMethodParamTypeString;
    Param[2].Data.Arg = 0;
    Param[2].Type = AmlMethodParamTypeArg;
    Param[3].Data.Local = 2;
    Param[3].Type = AmlMethodParamTypeLocal;
    AmlCodeGenInvokeMethod ("MET0", 4, Param, ParentNode);

    is equivalent to the following ASL code:
      MET0 (0x100, "TEST", Arg0, Local2);

  Example 3:
    AML_METHOD_PARAM  Param[2];
    Param[0].Data.Arg = 0;
    Param[0].Type = AmlMethodParamTypeArg;
    Param[1].Data.Integer = 0x100;
    Param[1].Type = AmlMethodParamTypeInteger;
    AmlCodeGenMethodRetNameString ("MET2", NULL, 2, TRUE, 0, ParentNode, &MethodNode);
    AmlCodeGenInvokeMethod ("MET3", 2, Param, MethodNode);

    is equivalent to the following ASL code:
    Method (MET2, 2, Serialized)
    {
      MET3 (Arg0, 0x0100)
    }

  @param [in] MethodNameString  The method name to be called or invoked.
  @param [in] NumArgs           Number of arguments to be passed,
                                0 to 7 are permissible values.
  @param [in] Parameters        Contains the parameter data.
  @param [in] ParentNode        The parent node to which the method invocation
                                nodes are attached.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
 **/
EFI_STATUS
EFIAPI
AmlCodeGenInvokeMethod (
  IN  CONST CHAR8             *MethodNameString,
  IN        UINT8             NumArgs,
  IN        AML_METHOD_PARAM  *Parameters   OPTIONAL,
  IN        AML_NODE_HANDLE   ParentNode
  );

/** Create a _PSD node.

  Creates and optionally adds the following node
   Name(_PSD, Package()
   {
    NumEntries,  // Integer
    Revision,    // Integer
    Domain,      // Integer
    CoordType,   // Integer
    NumProc,     // Integer
  })

  Cf. ACPI 6.5, s8.4.5.5 _PSD (P-State Dependency)

  @ingroup CodeGenApis

  @param [in]  PsdInfo      PsdInfo object
  @param [in]  ParentNode   If provided, set ParentNode as the parent
                            of the node created.
  @param [out] NewPsdNode   If success and provided, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreatePsdNode (
  IN  AML_PSD_INFO            *PsdInfo,
  IN  AML_NODE_HANDLE         ParentNode    OPTIONAL,
  OUT AML_OBJECT_NODE_HANDLE  *NewPsdNode   OPTIONAL
  );

/** Create a _CST node.

  AmlCreateCstNode ("_CST", 0, 1, ParentNode, &CstNode) is
  equivalent of the following ASL code:
    Name (_CST, Package (
                  0   // Count
                  ))

  This function doesn't define any CST state. As shown above, the count
  of _CST state is set to 0.
  The AmlAddCstState () function allows to add CST states.

  Cf ACPI 6.5 specification, s8.4.1.1 _CST (C States)

  @param [in]  CstNameString  The new CST 's object name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "_CST", "DEV0.CSTP", etc.
                              The input string is copied.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewCstNode     If success, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateCstNode (
  IN  CONST CHAR8                   *CstNameString,
  IN        AML_NODE_HANDLE         ParentNode   OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewCstNode   OPTIONAL
  );

/** Add an _CST state to a CST node created using AmlCreateCstNode.

  AmlAddCstState increments the Count of CST states in the CST node by one,
  and adds the following package:
  Package {
    Register  // Buffer (Resource Descriptor)
    Type      // Integer (BYTE)
    Latency   // Integer (WORD)
    Power     // Integer (DWORD)
  }

  Cf ACPI 6.5 specification, s8.4.1.1 _CST (C States).

  @param [in]  CstInfo                    CstInfo object
  @param [in]  CstNode                    Cst node created with the function
                                          AmlCreateCstNode to which the new CST
                                          state is appended.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlAddCstState (
  IN  AML_CST_INFO            *CstInfo,
  IN  AML_OBJECT_NODE_HANDLE  CstNode
  );

/** Create a _CSD node.

  Generates and optionally appends the following node:

  Name (_CSD, Package()
  {
    Package () {
      NumEntries,    // Integer
      Revision,      // Integer (BYTE)
      Domain,        // Integer (DWORD)
      CoordType,     // Integer (DWORD)
      NumProcessors, // Integer (DWORD)
      Index          // Integer (DWORD)
    }
    Package () {
      NumEntries,    // Integer
      Revision,      // Integer (BYTE)
      Domain,        // Integer (DWORD)
      CoordType,     // Integer (DWORD)
      NumProcessors, // Integer (DWORD)
      Index          // Integer (DWORD)
    }
    ...
  })
  Cf. ACPI 6.5, s8.4.1.2 _CSD (C-State Dependency).

  @ingroup CodeGenApis

  @param [in]  CsdInfo      CsdInfo object
  @param [in]  NumEntries   Number of packages to be created.
  @param [in]  ParentNode   If provided, set ParentNode as the parent
                            of the node created.
  @param [out] NewCsdNode   If success and provided, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateCsdNode (
  IN  AML_CSD_INFO            *CsdInfo,
  IN  UINT32                  NumEntries,
  IN  AML_NODE_HANDLE         ParentNode    OPTIONAL,
  OUT AML_OBJECT_NODE_HANDLE  *NewCsdNode   OPTIONAL
  );

/** Create _PCT node

  Generates and optionally appends the following node:
  Name (_PCT, Package()
  {
    ControlRegister   // Buffer (Resource Descriptor (Register))
    StatusRegister    // Buffer (Resource Descriptor (Register))
  })

  Cf. ACPI 6.5, s8.4.5.1 _PCT (Processor Control).

  @ingroup CodeGenApis

  @param [in]  PctInfo      PctInfo object
  @param [in]  ParentNode   If provided, set ParentNode as the parent
                            of the node created.
  @param [out] NewPctNode   If success and provided, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreatePctNode (
  IN  AML_PCT_INFO            *PctInfo,
  IN  AML_NODE_HANDLE         ParentNode    OPTIONAL,
  OUT AML_OBJECT_NODE_HANDLE  *NewPctNode   OPTIONAL
  );

/** Create _PSS node

  Generates and optionally appends the following node:
  Name (_PSS, Package()
  {
    Package () {
      CoreFrequency     // Integer (DWORD)
      Power             // Integer (DWORD)
      Latency           // Integer (DWORD)
      BusMasterLatency  // Integer (DWORD)
      Control           // Integer (DWORD)
      Status            // Integer (DWORD)
    }
    Package () {
      CoreFrequency     // Integer (DWORD)
      Power             // Integer (DWORD)
      Latency           // Integer (DWORD)
      BusMasterLatency  // Integer (DWORD)
      Control           // Integer (DWORD)
      Status            // Integer (DWORD)
    }
    ...
  })

  Cf. ACPI 6.5, s8.4.5.2 _PSS (Processor Supported Performance States).

  @ingroup CodeGenApis

  @param [in]  PssInfo      Array of PssInfo object
  @param [in]  NumPackages  Number of packages to be created.
  @param [in]  ParentNode   If provided, set ParentNode as the parent
                            of the node created.
  @param [out] NewPssNode   If success and provided, contains the created node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
EFIAPI
AmlCreatePssNode (
  IN  AML_PSS_INFO            *PssInfo,
  IN  UINT32                  NumPackages,
  IN  AML_NODE_HANDLE         ParentNode    OPTIONAL,
  OUT AML_OBJECT_NODE_HANDLE  *NewPssNode   OPTIONAL
  );

/** Code generation for the IRQ Descriptor.

  The Resource Data effectively created is an IRQ Resource
  Data. Cf ACPI 6.5 specification:
   - s6.4.2.1 "IRQ Descriptor"
   - s19.6.66 "IRQ (Interrupt Resource Descriptor Macro)"


  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @param  [in]  IsEdgeTriggered The interrupt is edge triggered or
                                level triggered.
  @param  [in]  IsActiveLow     The interrupt is active-high or active-low.
  @param  [in]  IsShared        The interrupt can be shared with other
                                devices or not (Exclusive).
  @param  [in]  IrqList         List of IRQ numbers. Must be non-NULL.
  @param  [in]  IrqCount        Number of IRQs in IrqList. Must be > 0 and <= 16.
  @param  [in]  NameOpNode      NameOp object node defining a named object.
                                If provided, append the new resource data node
                                to the list of resource data elements of this node.
  @param  [out] NewRdNode       If provided and success, contain the created node.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
  @retval various               Other errors as indicated.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdIrq (
  IN  BOOLEAN                 IsEdgeTriggered,
  IN  BOOLEAN                 IsActiveLow,
  IN  BOOLEAN                 IsShared,
  IN  UINT8                   *IrqList,
  IN  UINT8                   IrqCount,
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode  OPTIONAL,
  OUT AML_DATA_NODE_HANDLE    *NewRdNode  OPTIONAL
  );

/** Code generation for the UARTSerialBusV2() ASL macro.

  The Resource Data effectively created is a UART Serial Bus Connection
  Resource Descriptor Resource Data.
  Cf ACPI 6.5:
   - s19.6.143 UARTSerialBusV2
     (UART Serial Bus Connection Resource Descriptor Version 2 Macro)
   - s6.4.3.8.2.3 UART Serial Bus Connection Resource Descriptor

  The created resource data node can be:
   - appended to the list of resource data elements of the NameOpNode.
     In such case NameOpNode must be defined by a the "Name ()" ASL statement
     and initially contain a "ResourceTemplate ()".
   - returned through the NewRdNode parameter.

  @param [in]  InitialBaudRate           Initial baud rate.
  @param [in]  BitsPerByte               Number of bits per byte.
                                         Optional, default is 8.
  @param [in]  StopBits                  Number of stop bits.
                                         Optional, default is 1.
  @param [in]  LinesInUse                Number of lines in use.
  @param [in]  IsBigEndian               Indicates whether the bit transfer is big-endian.
                                         Optional, default is FALSE (little-endian).
  @param [in]  Parity                    Parity format used.
                                          Optional, default is no parity.
  @param [in]  FlowControl               Flow control protocol used.
                                          Optional, default is no flow control.
  @param [in]  ReceiveBufferSize         Size of the receive buffer.
  @param [in]  TransmitBufferSize        Size of the transmit buffer.
  @param [in]  ResourceSource            Name of source resource used.
  @param [in]  ResourceSourceLength      Length of the Resource Source.
  @param [in]  ResourceSourceIndex       Resource Source index.
                                         Optional, default is 0.
  @param [in]  ResourceUsage             Resource usage, TRUE for consumer,
                                         FALSE for producer.
                                         Optional, default is TRUE (consumer).
  @param [in]  IsShared                  Indicates whether the resource is shared.
                                         Optional, default is FALSE (exclusive).
  @param [in]  VendorDefinedData         Vendor defined data.
                                         Optional, can be NULL.
  @param [in]  VendorDefinedDataLength   Length of the vendor defined data.
  @param [in]  NameOpNode                NameOp object node defining a named object.
                                         If provided, append the new resource data
                                         node to the list of resource data elements
                                         of this node.
  @param [out] NewRdNode                 If provided and success,
                                         contain the created node.

  @retval EFI_SUCCESS                   The function completed successfully.
  @retval EFI_INVALID_PARAMETER         Invalid parameter.
  @retval various                       Various failure values of called functions.
**/
EFI_STATUS
EFIAPI
AmlCodeGenRdUartSerialBusV2 (
  IN  UINT32                  InitialBaudRate,
  IN  UINT8                   *BitsPerByte OPTIONAL,
  IN  UINT8                   *StopBits OPTIONAL,
  IN  UINT8                   LinesInUse,
  IN  BOOLEAN                 *IsBigEndian OPTIONAL,
  IN  UINT8                   *Parity OPTIONAL,
  IN  UINT8                   *FlowControl OPTIONAL,
  IN  UINT16                  ReceiveBufferSize,
  IN  UINT16                  TransmitBufferSize,
  IN  CHAR8                   *ResourceSource,
  IN  UINT16                  ResourceSourceLength,
  IN  UINT8                   *ResourceSourceIndex OPTIONAL,
  IN  BOOLEAN                 *ResourceUsage OPTIONAL,
  IN  BOOLEAN                 *IsShared OPTIONAL,
  IN  UINT8                   *VendorDefinedData OPTIONAL,
  IN  UINT16                  VendorDefinedDataLength,
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode OPTIONAL,
  OUT AML_DATA_NODE_HANDLE    *NewRdNode OPTIONAL
  );

#endif // AML_LIB_H_
