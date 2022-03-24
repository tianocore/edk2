/** @file
  AML Code Generation.

  Copyright (c) 2020 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>

#include <AcpiTableGenerator.h>

#include <AmlCoreInterface.h>
#include <AmlEncoding/Aml.h>
#include <Api/AmlApiHelper.h>
#include <CodeGen/AmlResourceDataCodeGen.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>
#include <String/AmlString.h>
#include <Utils/AmlUtility.h>

/** Utility function to link a node when returning from a CodeGen function.

  @param [in]  Node           Newly created node.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If not NULL:
                               - and Success, contains the created Node.
                               - and Error, reset to NULL.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
LinkNode (
  IN  AML_OBJECT_NODE  *Node,
  IN  AML_NODE_HEADER  *ParentNode,
  OUT AML_OBJECT_NODE  **NewObjectNode
  )
{
  EFI_STATUS  Status;

  if (NewObjectNode != NULL) {
    *NewObjectNode = NULL;
  }

  // Add RdNode as the last element.
  if (ParentNode != NULL) {
    Status = AmlVarListAddTail (ParentNode, (AML_NODE_HEADER *)Node);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  if (NewObjectNode != NULL) {
    *NewObjectNode = Node;
  }

  return EFI_SUCCESS;
}

/** AML code generation for DefinitionBlock.

  Create a Root Node handle.
  It is the caller's responsibility to free the allocated memory
  with the AmlDeleteTree function.

  AmlCodeGenDefinitionBlock (TableSignature, OemID, TableID, OEMRevision) is
  equivalent to the following ASL code:
    DefinitionBlock (AMLFileName, TableSignature, ComplianceRevision,
      OemID, TableID, OEMRevision) {}
  with the ComplianceRevision set to 2 and the AMLFileName is ignored.

  @param[in]  TableSignature       4-character ACPI signature.
                                   Must be 'DSDT' or 'SSDT'.
  @param[in]  OemId                6-character string OEM identifier.
  @param[in]  OemTableId           8-character string OEM table identifier.
  @param[in]  OemRevision          OEM revision number.
  @param[out] NewRootNode          Pointer to the root node representing a
                                   Definition Block.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCodeGenDefinitionBlock (
  IN  CONST CHAR8          *TableSignature,
  IN  CONST CHAR8          *OemId,
  IN  CONST CHAR8          *OemTableId,
  IN        UINT32         OemRevision,
  OUT       AML_ROOT_NODE  **NewRootNode
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  AcpiHeader;

  if ((TableSignature == NULL)  ||
      (OemId == NULL)           ||
      (OemTableId == NULL)      ||
      (NewRootNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&AcpiHeader.Signature, TableSignature, 4);
  AcpiHeader.Length   = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
  AcpiHeader.Revision = 2;
  CopyMem (&AcpiHeader.OemId, OemId, 6);
  CopyMem (&AcpiHeader.OemTableId, OemTableId, 8);
  AcpiHeader.OemRevision     = OemRevision;
  AcpiHeader.CreatorId       = TABLE_GENERATOR_CREATOR_ID_ARM;
  AcpiHeader.CreatorRevision = CREATE_REVISION (1, 0);

  Status = AmlCreateRootNode (&AcpiHeader, NewRootNode);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** AML code generation for a String object node.

  @param [in]  String          Pointer to a NULL terminated string.
  @param [out] NewObjectNode   If success, contains the created
                               String object node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenString (
  IN  CHAR8            *String,
  OUT AML_OBJECT_NODE  **NewObjectNode
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;
  AML_DATA_NODE    *DataNode;

  if ((String == NULL)  ||
      (NewObjectNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DataNode = NULL;

  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_STRING_PREFIX, 0),
             0,
             &ObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeString,
             (UINT8 *)String,
             (UINT32)AsciiStrLen (String) + 1,
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
    goto error_handler;
  }

  *NewObjectNode = ObjectNode;
  return Status;

error_handler:
  AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  return Status;
}

/** AML code generation for an Integer object node.

  @param [in]  Integer         Integer of the Integer object node.
  @param [out] NewObjectNode   If success, contains the created
                               Integer object node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenInteger (
  IN  UINT64           Integer,
  OUT AML_OBJECT_NODE  **NewObjectNode
  )
{
  EFI_STATUS  Status;
  INT8        ValueWidthDiff;

  if (NewObjectNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Create an object node containing Zero.
  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_ZERO_OP, 0),
             0,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Update the object node with integer value.
  Status = AmlNodeSetIntegerValue (*NewObjectNode, Integer, &ValueWidthDiff);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)*NewObjectNode);
  }

  return Status;
}

/** AML code generation for a Package object node.

  The package generated is empty. New elements can be added via its
  list of variable arguments.

  @param [out] NewObjectNode   If success, contains the created
                               Package object node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenPackage (
  OUT AML_OBJECT_NODE  **NewObjectNode
  )
{
  EFI_STATUS     Status;
  AML_DATA_NODE  *DataNode;
  UINT8          NodeCount;

  if (NewObjectNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NodeCount = 0;

  // Create an object node.
  // PkgLen is 2:
  //  - one byte to store the PkgLength
  //  - one byte for the NumElements.
  // Cf ACPI6.3, s20.2.5 "Term Objects Encoding"
  // DefPackage  := PackageOp PkgLength NumElements PackageElementList
  // NumElements := ByteData
  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_PACKAGE_OP, 0),
             2,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // NumElements is a ByteData.
  Status = AmlCreateDataNode (
             EAmlNodeDataTypeUInt,
             &NodeCount,
             sizeof (NodeCount),
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = AmlSetFixedArgument (
             *NewObjectNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  AmlDeleteTree ((AML_NODE_HEADER *)*NewObjectNode);
  if (DataNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
  }

  return Status;
}

/** AML code generation for a Buffer object node.

  To create a Buffer object node with an empty buffer,
  call the function with (Buffer=NULL, BufferSize=0).

  @param [in]  Buffer          Buffer to set for the created Buffer
                               object node. The Buffer's content is copied.
                               NULL if there is no buffer to set for
                               the Buffer node.
  @param [in]  BufferSize      Size of the Buffer.
                               0 if there is no buffer to set for
                               the Buffer node.
  @param [out] NewObjectNode   If success, contains the created
                               Buffer object node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenBuffer (
  IN  CONST UINT8            *Buffer        OPTIONAL,
  IN        UINT32           BufferSize    OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *BufferNode;
  AML_OBJECT_NODE  *BufferSizeNode;
  UINT32           BufferSizeNodeSize;
  AML_DATA_NODE    *DataNode;
  UINT32           PkgLen;

  // Buffer and BufferSize must be either both set, or both clear.
  if ((NewObjectNode == NULL)                 ||
      ((Buffer == NULL) != (BufferSize == 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  BufferNode = NULL;
  DataNode   = NULL;

  // Cf ACPI 6.3 specification, s20.2.5.4 "Type 2 Opcodes Encoding"
  // DefBuffer := BufferOp PkgLength BufferSize ByteList
  // BufferOp  := 0x11
  // BufferSize := TermArg => Integer

  Status = AmlCodeGenInteger (BufferSize, &BufferSizeNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the number of bytes required to encode the BufferSizeNode.
  Status = AmlComputeSize (
             (AML_NODE_HEADER *)BufferSizeNode,
             &BufferSizeNodeSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Compute the size to write in the PkgLen.
  Status = AmlComputePkgLength (BufferSizeNodeSize + BufferSize, &PkgLen);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Create an object node for the buffer.
  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_BUFFER_OP, 0),
             PkgLen,
             &BufferNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Set the BufferSizeNode as a fixed argument of the BufferNode.
  Status = AmlSetFixedArgument (
             BufferNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)BufferSizeNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // BufferSizeNode is now attached.
  BufferSizeNode = NULL;

  // If there is a buffer, create a DataNode and attach it to the BufferNode.
  if (Buffer != NULL) {
    Status = AmlCreateDataNode (
               EAmlNodeDataTypeRaw,
               Buffer,
               BufferSize,
               &DataNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    Status = AmlVarListAddTail (
               (AML_NODE_HEADER *)BufferNode,
               (AML_NODE_HEADER *)DataNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }
  }

  *NewObjectNode = BufferNode;
  return Status;

error_handler:
  if (BufferSizeNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)BufferSizeNode);
  }

  if (BufferNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)BufferNode);
  }

  if (DataNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
  }

  return Status;
}

/** AML code generation for a ResourceTemplate.

  "ResourceTemplate" is a macro defined in ACPI 6.3, s19.3.3
  "ASL Resource Templates". It allows to store resource data elements.

  In AML, a ResourceTemplate is implemented as a Buffer storing resource
  data elements. An EndTag resource data descriptor must be at the end
  of the list of resource data elements.
  This function generates a Buffer node with an EndTag resource data
  descriptor. It can be seen as an empty list of resource data elements.

  @param [out] NewObjectNode   If success, contains the created
                               ResourceTemplate object node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenResourceTemplate (
  OUT AML_OBJECT_NODE  **NewObjectNode
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *BufferNode;

  if (NewObjectNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Create a BufferNode with an empty buffer.
  Status = AmlCodeGenBuffer (NULL, 0, &BufferNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Create an EndTag resource data element and attach it to the Buffer.
  Status = AmlCodeGenEndTag (0, BufferNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)BufferNode);
    return Status;
  }

  *NewObjectNode = BufferNode;
  return Status;
}

/** AML code generation for a Name object node.

  @param  [in] NameString     The new variable name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "DEV0", "DV15.DEV0", etc.
                              This input string is copied.
  @param [in]  Object         Object associated to the NameString.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenName (
  IN  CONST CHAR8            *NameString,
  IN        AML_OBJECT_NODE  *Object,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;
  AML_DATA_NODE    *DataNode;
  CHAR8            *AmlNameString;
  UINT32           AmlNameStringSize;

  if ((NameString == NULL)    ||
      (Object == NULL)        ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode    = NULL;
  DataNode      = NULL;
  AmlNameString = NULL;

  Status = ConvertAslNameToAmlName (NameString, &AmlNameString);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlGetNameStringSize (AmlNameString, &AmlNameStringSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_NAME_OP, 0),
             0,
             &ObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeNameString,
             (UINT8 *)AmlNameString,
             AmlNameStringSize,
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
    goto error_handler2;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm1,
             (AML_NODE_HEADER *)Object
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  Status = LinkNode (
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  // Free AmlNameString before returning as it is copied
  // in the call to AmlCreateDataNode().
  goto error_handler1;

error_handler2:
  if (ObjectNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

error_handler1:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}

/** AML code generation for a Name object node, containing a String.

  AmlCodeGenNameString ("_HID", "HID0000", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(_HID, "HID0000")

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
  IN  CONST CHAR8            *NameString,
  IN        CHAR8            *String,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;

  if ((NameString == NULL)  ||
      (String == NULL)      ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenString (String, &ObjectNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenName (
             NameString,
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

  return Status;
}

/** AML code generation for a Name object node, containing an Integer.

  AmlCodeGenNameInteger ("_UID", 1, ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(_UID, One)

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
  IN  CONST CHAR8            *NameString,
  IN        UINT64           Integer,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenInteger (Integer, &ObjectNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenName (
             NameString,
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

  return Status;
}

/** AML code generation for a Name object node, containing a Package.

  AmlCodeGenNamePackage ("PKG0", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(PKG0, Package () {})

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
  IN        AML_NODE_HEADER *ParentNode, OPTIONAL
  OUT       AML_OBJECT_NODE   **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *PackageNode;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenPackage (&PackageNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenName (
             NameString,
             PackageNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)PackageNode);
  }

  return Status;
}

/** AML code generation for a Name object node, containing a ResourceTemplate.

  AmlCodeGenNameResourceTemplate ("PRS0", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Name(PRS0, ResourceTemplate () {})

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
  IN        AML_NODE_HEADER *ParentNode, OPTIONAL
  OUT       AML_OBJECT_NODE   **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ResourceTemplateNode;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenResourceTemplate (&ResourceTemplateNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenName (
             NameString,
             ResourceTemplateNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)ResourceTemplateNode);
  }

  return Status;
}

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

  The 2 models described in ACPI 6.4, s6.2.13 "_PRT (PCI Routing Table)" can
  be generated by this function. The example above matches the first model.

  The package is added at the tail of the list of the input _PRT node
  name:
    Name (_PRT, Package () {
      [Pre-existing _PRT entries],
      [Newly created _PRT entry]
    })

  Cf. ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"

  @param [in]  Address        Address. Cf ACPI 6.4 specification, Table 6.2:
                              "ADR Object Address Encodings":
                              High word-Device #, Low word-Function #. (for
                              example, device 3, function 2 is 0x00030002).
                              To refer to all the functions on a device #,
                              use a function number of FFFF).
  @param [in]  Pin            PCI pin number of the device (0-INTA ... 3-INTD).
                              Must be between 0-3.
  @param [in]  LinkName       Link Name, i.e. device in the AML NameSpace
                              describing the interrupt used. The input string
                              is copied.
                              If NULL, generate 0 in the 'Source' field (cf.
                              second model, using GSIV).
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
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *PrtEntryList;
  AML_OBJECT_NODE  *PackageNode;
  AML_OBJECT_NODE  *NewElementNode;

  CHAR8          *AmlNameString;
  UINT32         AmlNameStringSize;
  AML_DATA_NODE  *DataNode;

  if ((Pin > 3)                 ||
      (PrtNameNode == NULL)     ||
      (AmlGetNodeType ((AML_NODE_HANDLE)PrtNameNode) != EAmlNodeObject) ||
      (!AmlNodeHasOpCode (PrtNameNode, AML_NAME_OP, 0))                 ||
      !AmlNameOpCompareName (PrtNameNode, "_PRT"))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NewElementNode = NULL;
  AmlNameString  = NULL;
  DataNode       = NULL;

  // Get the Package object node of the _PRT node,
  // which is the 2nd fixed argument (i.e. index 1).
  PrtEntryList = (AML_OBJECT_NODE_HANDLE)AmlGetFixedArgument (
                                           PrtNameNode,
                                           EAmlParseIndexTerm1
                                           );
  if ((PrtEntryList == NULL)                                              ||
      (AmlGetNodeType ((AML_NODE_HANDLE)PrtEntryList) != EAmlNodeObject)  ||
      (!AmlNodeHasOpCode (PrtEntryList, AML_PACKAGE_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The new _PRT entry.
  Status = AmlCodeGenPackage (&PackageNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenInteger (Address, &NewElementNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)NewElementNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  NewElementNode = NULL;

  Status = AmlCodeGenInteger (Pin, &NewElementNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)NewElementNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  NewElementNode = NULL;

  if (LinkName != NULL) {
    Status = ConvertAslNameToAmlName (LinkName, &AmlNameString);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    Status = AmlGetNameStringSize (AmlNameString, &AmlNameStringSize);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    Status = AmlCreateDataNode (
               EAmlNodeDataTypeNameString,
               (UINT8 *)AmlNameString,
               AmlNameStringSize,
               &DataNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    // AmlNameString will be freed be fore returning.

    Status = AmlVarListAddTail (
               (AML_NODE_HANDLE)PackageNode,
               (AML_NODE_HANDLE)DataNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    DataNode = NULL;
  } else {
    Status = AmlCodeGenInteger (0, &NewElementNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    Status = AmlVarListAddTail (
               (AML_NODE_HANDLE)PackageNode,
               (AML_NODE_HANDLE)NewElementNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }
  }

  Status = AmlCodeGenInteger (SourceIndex, &NewElementNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)NewElementNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Append to the the list of _PRT entries.
  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PrtEntryList,
             (AML_NODE_HANDLE)PackageNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Free AmlNameString before returning as it is copied
  // in the call to AmlCreateDataNode().
  goto exit_handler;

error_handler:
  AmlDeleteTree ((AML_NODE_HANDLE)PackageNode);
  if (NewElementNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)NewElementNode);
  }

  if (DataNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)DataNode);
  }

exit_handler:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}

/** AML code generation for a Device object node.

  AmlCodeGenDevice ("COM0", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Device(COM0) {}

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
  IN  CONST CHAR8            *NameString,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;
  AML_DATA_NODE    *DataNode;
  CHAR8            *AmlNameString;
  UINT32           AmlNameStringSize;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode    = NULL;
  DataNode      = NULL;
  AmlNameString = NULL;

  Status = ConvertAslNameToAmlName (NameString, &AmlNameString);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlGetNameStringSize (AmlNameString, &AmlNameStringSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_EXT_OP, AML_EXT_DEVICE_OP),
             AmlNameStringSize + AmlComputePkgLengthWidth (AmlNameStringSize),
             &ObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeNameString,
             (UINT8 *)AmlNameString,
             AmlNameStringSize,
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
    goto error_handler2;
  }

  Status = LinkNode (
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  // Free AmlNameString before returning as it is copied
  // in the call to AmlCreateDataNode().
  goto error_handler1;

error_handler2:
  if (ObjectNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

error_handler1:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}

/** AML code generation for a Scope object node.

  AmlCodeGenScope ("_SB", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Scope(_SB) {}

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
  IN  CONST CHAR8            *NameString,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;
  AML_DATA_NODE    *DataNode;
  CHAR8            *AmlNameString;
  UINT32           AmlNameStringSize;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode    = NULL;
  DataNode      = NULL;
  AmlNameString = NULL;

  Status = ConvertAslNameToAmlName (NameString, &AmlNameString);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlGetNameStringSize (AmlNameString, &AmlNameStringSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_SCOPE_OP, 0),
             AmlNameStringSize + AmlComputePkgLengthWidth (AmlNameStringSize),
             &ObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeNameString,
             (UINT8 *)AmlNameString,
             AmlNameStringSize,
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
    goto error_handler2;
  }

  Status = LinkNode (
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  // Free AmlNameString before returning as it is copied
  // in the call to AmlCreateDataNode().
  goto error_handler1;

error_handler2:
  if (ObjectNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

error_handler1:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}

/** AML code generation for a Method object node.

  AmlCodeGenMethod ("MET0", 1, TRUE, 3, ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Method(MET0, 1, Serialized, 3) {}

  ACPI 6.4, s20.2.5.2 "Named Objects Encoding":
    DefMethod := MethodOp PkgLength NameString MethodFlags TermList
    MethodOp := 0x14

  The ASL parameters "ReturnType" and "ParameterTypes" are not asked
  in this function. They are optional parameters in ASL.

  @param [in]  NameString     The new Method's name.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "MET0", "_SB.MET0", etc.
                              The input string is copied.
  @param [in]  NumArgs        Number of arguments.
                              Must be 0 <= NumArgs <= 6.
  @param [in]  IsSerialized   TRUE is equivalent to Serialized.
                              FALSE is equivalent to NotSerialized.
                              Default is NotSerialized in ASL spec.
  @param [in]  SyncLevel      Synchronization level for the method.
                              Must be 0 <= SyncLevel <= 15.
                              Default is 0 in ASL.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenMethod (
  IN  CONST CHAR8            *NameString,
  IN        UINT8            NumArgs,
  IN        BOOLEAN          IsSerialized,
  IN        UINT8            SyncLevel,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  UINT32           PkgLen;
  UINT8            Flags;
  AML_OBJECT_NODE  *ObjectNode;
  AML_DATA_NODE    *DataNode;
  CHAR8            *AmlNameString;
  UINT32           AmlNameStringSize;

  if ((NameString == NULL)    ||
      (NumArgs > 6)           ||
      (SyncLevel > 15)        ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode = NULL;
  DataNode   = NULL;

  // ACPI 6.4, s20.2.5.2 "Named Objects Encoding":
  //   DefMethod := MethodOp PkgLength NameString MethodFlags TermList
  //   MethodOp := 0x14
  // So:
  //  1- Create the NameString
  //  2- Compute the size to write in the PkgLen
  //  3- Create nodes for the NameString and Method object node
  //  4- Set the NameString DataNode as a fixed argument
  //  5- Create and link the MethodFlags node

  // 1- Create the NameString
  Status = ConvertAslNameToAmlName (NameString, &AmlNameString);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlGetNameStringSize (AmlNameString, &AmlNameStringSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  // 2- Compute the size to write in the PkgLen
  //    Add 1 byte (ByteData) for MethodFlags.
  Status = AmlComputePkgLength (AmlNameStringSize + 1, &PkgLen);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  //  3- Create nodes for the NameString and Method object node
  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_METHOD_OP, 0),
             PkgLen,
             &ObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler1;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeNameString,
             (UINT8 *)AmlNameString,
             AmlNameStringSize,
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  //  4- Set the NameString DataNode as a fixed argument
  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  DataNode = NULL;

  //  5- Create and link the MethodFlags node
  Flags = NumArgs                   |
          (IsSerialized ? BIT3 : 0) |
          (SyncLevel << 4);

  Status = AmlCreateDataNode (EAmlNodeDataTypeUInt, &Flags, 1, &DataNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm1,
             (AML_NODE_HEADER *)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  // Data node is attached so set the pointer to
  // NULL to ensure correct error handling.
  DataNode = NULL;

  Status = LinkNode (
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler2;
  }

  // Free AmlNameString before returning as it is copied
  // in the call to AmlCreateDataNode().
  goto error_handler1;

error_handler2:
  if (ObjectNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

  if (DataNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)DataNode);
  }

error_handler1:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}

/** AML code generation for a Return object node.

  AmlCodeGenReturn (ReturnNode, ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Return([Content of the ReturnNode])

  The ACPI 6.3 specification, s20.2.5.3 "Type 1 Opcodes Encoding" states:
    DefReturn := ReturnOp ArgObject
    ReturnOp := 0xA4
    ArgObject := TermArg => DataRefObject

  Thus, the ReturnNode must be evaluated as a DataRefObject. It can
  be a NameString referencing an object. As this CodeGen Api doesn't
  do semantic checking, it is strongly advised to check the AML bytecode
  generated by this function against an ASL compiler.

  The ReturnNode must be generated inside a Method body scope.

  @param [in]  ReturnNode     The object returned by the Return ASL statement.
                              This node is deleted if an error occurs.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
                              Must be a MethodOp node.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenReturn (
  IN  AML_NODE_HEADER  *ReturnNode,
  IN  AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *ObjectNode;

  if ((ReturnNode == NULL)                              ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)) ||
      ((ParentNode != NULL) &&
       !AmlNodeCompareOpCode (
          (AML_OBJECT_NODE *)ParentNode,
          AML_METHOD_OP,
          0
          )))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_RETURN_OP, 0),
             0,
             &ObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm0,
             ReturnNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  ReturnNode = NULL;

  Status = LinkNode (
             ObjectNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  if (ReturnNode != NULL) {
    AmlDeleteTree (ReturnNode);
  }

  if (ObjectNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER *)ObjectNode);
  }

  return Status;
}

/** AML code generation for a Return object node,
    returning the object as an input NameString.

  AmlCodeGenReturn ("NAM1", ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Return(NAM1)

  The ACPI 6.3 specification, s20.2.5.3 "Type 1 Opcodes Encoding" states:
    DefReturn := ReturnOp ArgObject
    ReturnOp := 0xA4
    ArgObject := TermArg => DataRefObject

  Thus, the ReturnNode must be evaluated as a DataRefObject. It can
  be a NameString referencing an object. As this CodeGen Api doesn't
  do semantic checking, it is strongly advised to check the AML bytecode
  generated by this function against an ASL compiler.

  The ReturnNode must be generated inside a Method body scope.

  @param [in]  NameString     The object referenced by this NameString
                              is returned by the Return ASL statement.
                              Must be a NULL-terminated ASL NameString
                              e.g.: "NAM1", "_SB.NAM1", etc.
                              The input string is copied.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
                              Must be a MethodOp node.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenReturnNameString (
  IN  CONST CHAR8            *NameString,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS     Status;
  AML_DATA_NODE  *DataNode;
  CHAR8          *AmlNameString;
  UINT32         AmlNameStringSize;

  DataNode = NULL;

  Status = ConvertAslNameToAmlName (NameString, &AmlNameString);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlGetNameStringSize (AmlNameString, &AmlNameStringSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  Status = AmlCreateDataNode (
             EAmlNodeDataTypeNameString,
             (UINT8 *)AmlNameString,
             AmlNameStringSize,
             &DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // AmlCodeGenReturn() deletes DataNode if error.
  Status = AmlCodeGenReturn (
             (AML_NODE_HEADER *)DataNode,
             ParentNode,
             NewObjectNode
             );
  ASSERT_EFI_ERROR (Status);

exit_handler:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}

/** AML code generation for a Return object node,
    returning an Integer.

  AmlCodeGenReturn (0), ParentNode, NewObjectNode) is
  equivalent of the following ASL code:
    Return (0)

  The ACPI 6.3 specification, 20.2.8 "Statement Opcodes Encoding" states:
    DefReturn := ReturnOp ArgObject
    ReturnOp := 0xA4
    ArgObject := TermArg => DataRefObject

  Thus, the ReturnNode must be evaluated as a DataRefObject.

  The ReturnNode must be generated inside a Method body scope.

  @param [in]  Integer        The integer is returned by the Return
                              ASL statement.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
                              Must be a MethodOp node.
  @param [out] NewObjectNode  If success, contains the created node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCodeGenReturnInteger (
  IN        UINT64           Integer,
  IN        AML_NODE_HEADER  *ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE  **NewObjectNode  OPTIONAL
  )
{
  EFI_STATUS       Status;
  AML_OBJECT_NODE  *IntNode;

  IntNode = NULL;

  Status = AmlCodeGenInteger (Integer, &IntNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // AmlCodeGenReturn() deletes DataNode if error.
  Status = AmlCodeGenReturn (
             (AML_NODE_HEADER *)IntNode,
             ParentNode,
             NewObjectNode
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  MethodNode;

  if ((MethodNameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Create a Method named MethodNameString.
  Status = AmlCodeGenMethod (
             MethodNameString,
             NumArgs,
             IsSerialized,
             SyncLevel,
             NULL,
             &MethodNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Return ReturnedNameString if provided.
  if (ReturnedNameString != NULL) {
    Status = AmlCodeGenReturnNameString (
               ReturnedNameString,
               (AML_NODE_HANDLE)MethodNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }
  }

  Status = LinkNode (
             MethodNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  if (MethodNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)MethodNode);
  }

  return Status;
}

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
  IN        AML_NODE_HANDLE         ParentNode      OPTIONAL,
  OUT       AML_OBJECT_NODE_HANDLE  *NewObjectNode  OPTIONAL
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  MethodNode;

  if ((MethodNameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Create a Method named MethodNameString.
  Status = AmlCodeGenMethod (
             MethodNameString,
             NumArgs,
             IsSerialized,
             SyncLevel,
             NULL,
             &MethodNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenReturnInteger (
             ReturnedInteger,
             (AML_NODE_HANDLE)MethodNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  Status = LinkNode (
             MethodNode,
             ParentNode,
             NewObjectNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  if (MethodNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)MethodNode);
  }

  return Status;
}

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
  The AmlAddLpiState () function allows to add LPI states.

  Cf ACPI 6.3 specification, s8.4.4 "Lower Power Idle States".

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
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  PackageNode;
  AML_OBJECT_NODE_HANDLE  IntegerNode;

  if ((LpiNameString == NULL)                           ||
      ((ParentNode == NULL) && (NewLpiNode == NULL)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  IntegerNode = NULL;

  Status = AmlCodeGenPackage (&PackageNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Create and attach Revision
  Status = AmlCodeGenInteger (Revision, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // Create and attach LevelId
  Status = AmlCodeGenInteger (LevelId, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // Create and attach Count. No LPI state is added, so 0.
  Status = AmlCodeGenInteger (0, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  Status = AmlCodeGenName (LpiNameString, PackageNode, ParentNode, NewLpiNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  AmlDeleteTree ((AML_NODE_HANDLE)PackageNode);
  if (IntegerNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)IntegerNode);
  }

  return Status;
}

/** Add an _LPI state to a LPI node created using AmlCreateLpiNode.

  AmlAddLpiState increments the Count of LPI states in the LPI node by one,
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
  IN  CHAR8                                   *StateName                   OPTIONAL,
  IN  AML_OBJECT_NODE_HANDLE                  LpiNode
  )
{
  EFI_STATUS              Status;
  AML_DATA_NODE_HANDLE    RdNode;
  AML_OBJECT_NODE_HANDLE  PackageNode;
  AML_OBJECT_NODE_HANDLE  IntegerNode;
  AML_OBJECT_NODE_HANDLE  StringNode;
  AML_OBJECT_NODE_HANDLE  NewLpiPackageNode;
  AML_OBJECT_NODE_HANDLE  ResourceTemplateNode;

  UINT32                  Index;
  AML_OBJECT_NODE_HANDLE  CountNode;
  UINT64                  Count;

  if ((LpiNode == NULL)                                              ||
      (AmlGetNodeType ((AML_NODE_HANDLE)LpiNode) != EAmlNodeObject)  ||
      (!AmlNodeHasOpCode (LpiNode, AML_NAME_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  RdNode               = 0;
  StringNode           = NULL;
  IntegerNode          = NULL;
  ResourceTemplateNode = NULL;

  // AmlCreateLpiNode () created a LPI container such as:
  //  Name (_LPI, Package (
  //                0,  // Revision
  //                1,  // LevelId
  //                0   // Count
  //                ))
  // Get the LPI container, a PackageOp object node stored as the 2nd fixed
  // argument (i.e. index 1) of LpiNode.
  PackageNode = (AML_OBJECT_NODE_HANDLE)AmlGetFixedArgument (
                                          LpiNode,
                                          EAmlParseIndexTerm1
                                          );
  if ((PackageNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)PackageNode) != EAmlNodeObject) ||
      (!AmlNodeHasOpCode (PackageNode, AML_PACKAGE_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CountNode = NULL;
  // The third variable argument is the LPI Count node.
  for (Index = 0; Index < 3; Index++) {
    CountNode = (AML_OBJECT_NODE_HANDLE)AmlGetNextVariableArgument (
                                          (AML_NODE_HANDLE)PackageNode,
                                          (AML_NODE_HANDLE)CountNode
                                          );
    if (CountNode == NULL) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  }

  Status = AmlNodeGetIntegerValue (CountNode, &Count);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlUpdateInteger (CountNode, Count + 1);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenPackage (&NewLpiPackageNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // MinResidency
  Status = AmlCodeGenInteger (MinResidency, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // WorstCaseWakeLatency
  Status = AmlCodeGenInteger (WorstCaseWakeLatency, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // Flags
  Status = AmlCodeGenInteger (Flags, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // ArchFlags
  Status = AmlCodeGenInteger (ArchFlags, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // ResCntFreq
  Status = AmlCodeGenInteger (ResCntFreq, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // EnableParentState
  Status = AmlCodeGenInteger (EnableParentState, &IntegerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    IntegerNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)IntegerNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  IntegerNode = NULL;

  // Entry Method
  if (GenericRegisterDescriptor != NULL) {
    // Entry Method: As a Register resource data
    Status = AmlCodeGenResourceTemplate (&ResourceTemplateNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      ResourceTemplateNode = NULL;
      goto error_handler;
    }

    Status = AmlCodeGenRdRegister (
               GenericRegisterDescriptor->AddressSpaceId,
               GenericRegisterDescriptor->RegisterBitWidth,
               GenericRegisterDescriptor->RegisterBitOffset,
               GenericRegisterDescriptor->Address,
               GenericRegisterDescriptor->AccessSize,
               NULL,
               &RdNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      RdNode = NULL;
      goto error_handler;
    }

    Status = AmlAppendRdNode (ResourceTemplateNode, RdNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    RdNode = NULL;

    Status = AmlVarListAddTail (
               (AML_NODE_HANDLE)NewLpiPackageNode,
               (AML_NODE_HANDLE)ResourceTemplateNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    ResourceTemplateNode = NULL;
  } else {
    // Entry Method: As an integer
    Status = AmlCodeGenInteger (Integer, &IntegerNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      IntegerNode = NULL;
      goto error_handler;
    }

    Status = AmlVarListAddTail (
               (AML_NODE_HANDLE)NewLpiPackageNode,
               (AML_NODE_HANDLE)IntegerNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    IntegerNode = NULL;
  }

  // Residency Counter Register.
  Status = AmlCodeGenResourceTemplate (&ResourceTemplateNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    ResourceTemplateNode = NULL;
    goto error_handler;
  }

  if (ResidencyCounterRegister != NULL) {
    Status = AmlCodeGenRdRegister (
               ResidencyCounterRegister->AddressSpaceId,
               ResidencyCounterRegister->RegisterBitWidth,
               ResidencyCounterRegister->RegisterBitOffset,
               ResidencyCounterRegister->Address,
               ResidencyCounterRegister->AccessSize,
               NULL,
               &RdNode
               );
  } else {
    Status = AmlCodeGenRdRegister (
               EFI_ACPI_6_4_SYSTEM_MEMORY,
               0,
               0,
               0,
               0,
               NULL,
               &RdNode
               );
  }

  if (EFI_ERROR (Status)) {
    ASSERT (0);
    RdNode = NULL;
    goto error_handler;
  }

  Status = AmlAppendRdNode (ResourceTemplateNode, RdNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  RdNode = NULL;

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)ResourceTemplateNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  ResourceTemplateNode = NULL;

  // Usage Counter Register.
  Status = AmlCodeGenResourceTemplate (&ResourceTemplateNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    ResourceTemplateNode = NULL;
    goto error_handler;
  }

  if (UsageCounterRegister != NULL) {
    Status = AmlCodeGenRdRegister (
               UsageCounterRegister->AddressSpaceId,
               UsageCounterRegister->RegisterBitWidth,
               UsageCounterRegister->RegisterBitOffset,
               UsageCounterRegister->Address,
               UsageCounterRegister->AccessSize,
               NULL,
               &RdNode
               );
  } else {
    Status = AmlCodeGenRdRegister (
               EFI_ACPI_6_4_SYSTEM_MEMORY,
               0,
               0,
               0,
               0,
               NULL,
               &RdNode
               );
  }

  if (EFI_ERROR (Status)) {
    ASSERT (0);
    RdNode = NULL;
    goto error_handler;
  }

  Status = AmlAppendRdNode (ResourceTemplateNode, RdNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  RdNode = NULL;

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)ResourceTemplateNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  ResourceTemplateNode = NULL;

  // State name.
  if (UsageCounterRegister != NULL) {
    Status = AmlCodeGenString (StateName, &StringNode);
  } else {
    Status = AmlCodeGenString ("", &StringNode);
  }

  if (EFI_ERROR (Status)) {
    ASSERT (0);
    StringNode = NULL;
    goto error_handler;
  }

  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)NewLpiPackageNode,
             (AML_NODE_HANDLE)StringNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  StringNode = NULL;

  // Add the new LPI state to the LpiNode.
  Status = AmlVarListAddTail (
             (AML_NODE_HANDLE)PackageNode,
             (AML_NODE_HANDLE)NewLpiPackageNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  return Status;

error_handler:
  if (RdNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)RdNode);
  }

  if (NewLpiPackageNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)NewLpiPackageNode);
  }

  if (StringNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)StringNode);
  }

  if (IntegerNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)IntegerNode);
  }

  if (ResourceTemplateNode != NULL) {
    AmlDeleteTree ((AML_NODE_HANDLE)ResourceTemplateNode);
  }

  return Status;
}
