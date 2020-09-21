/** @file
  AML Code Generation.

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>

#include <AcpiTableGenerator.h>

#include <AmlCoreInterface.h>
#include <AmlEncoding/Aml.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>
#include <String/AmlString.h>
#include <Utils/AmlUtility.h>

/** Utility function to link a node when returning from a CodeGen function.

  @param [in]  Node           Newly created node.
  @param [in]  ParentNode     If provided, set ParentNode as the parent
                              of the node created.
  @param [out] NewObjectNode  If success, contains the created object node.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
LinkNode (
  IN  AML_OBJECT_NODE    * Node,
  IN  AML_NODE_HEADER    * ParentNode,
  OUT AML_OBJECT_NODE   ** NewObjectNode
  )
{
  EFI_STATUS    Status;

  if (NewObjectNode != NULL) {
    *NewObjectNode = Node;
  }

  // Add RdNode as the last element.
  if (ParentNode != NULL) {
    Status = AmlVarListAddTail (ParentNode, (AML_NODE_HEADER*)Node);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
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
  IN  CONST CHAR8             * TableSignature,
  IN  CONST CHAR8             * OemId,
  IN  CONST CHAR8             * OemTableId,
  IN        UINT32              OemRevision,
  OUT       AML_ROOT_NODE    ** NewRootNode
  )
{
  EFI_STATUS                      Status;
  EFI_ACPI_DESCRIPTION_HEADER     AcpiHeader;

  if ((TableSignature == NULL)  ||
      (OemId == NULL)           ||
      (OemTableId == NULL)      ||
      (NewRootNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&AcpiHeader.Signature, TableSignature, 4);
  AcpiHeader.Length = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
  AcpiHeader.Revision = 2;
  CopyMem (&AcpiHeader.OemId, OemId, 6);
  CopyMem (&AcpiHeader.OemTableId, OemTableId, 8);
  AcpiHeader.OemRevision = OemRevision;
  AcpiHeader.CreatorId = TABLE_GENERATOR_CREATOR_ID_ARM;
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
  IN  CHAR8               * String,
  OUT AML_OBJECT_NODE    ** NewObjectNode
  )
{
  EFI_STATUS          Status;
  AML_OBJECT_NODE   * ObjectNode;
  AML_DATA_NODE     * DataNode;

  if ((String == NULL)  ||
      (NewObjectNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode = NULL;
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
             (UINT8*)String,
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
             (AML_NODE_HEADER*)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER*)DataNode);
    goto error_handler;
  }

  *NewObjectNode = ObjectNode;
  return Status;

error_handler:
  if (ObjectNode != NULL) {
    AmlDeleteTree ((AML_NODE_HEADER*)ObjectNode);
  }

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
  IN  UINT64                Integer,
  OUT AML_OBJECT_NODE    ** NewObjectNode
  )
{
  EFI_STATUS          Status;
  INT8                ValueWidthDiff;

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
    AmlDeleteTree ((AML_NODE_HEADER*)*NewObjectNode);
  }

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
  IN  CONST CHAR8              * NameString,
  IN        AML_OBJECT_NODE    * Object,
  IN        AML_NODE_HEADER    * ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE   ** NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS          Status;
  AML_OBJECT_NODE   * ObjectNode;
  AML_DATA_NODE     * DataNode;
  CHAR8             * AmlNameString;
  UINT32              AmlNameStringSize;

  if ((NameString == NULL)    ||
      (Object == NULL)        ||
      ((ParentNode == NULL) && (NewObjectNode == NULL))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode = NULL;
  DataNode = NULL;
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
             (UINT8*)AmlNameString,
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
             (AML_NODE_HEADER*)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER*)DataNode);
    goto error_handler2;
  }

  Status = AmlSetFixedArgument (
             ObjectNode,
             EAmlParseIndexTerm1,
             (AML_NODE_HEADER*)Object
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
    AmlDeleteTree ((AML_NODE_HEADER*)ObjectNode);
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
  IN  CONST CHAR8              * NameString,
  IN        CHAR8              * String,
  IN        AML_NODE_HEADER    * ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE   ** NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS          Status;
  AML_OBJECT_NODE   * ObjectNode;

  if ((NameString == NULL)  ||
      (String == NULL)      ||
      ((ParentNode == NULL) && (NewObjectNode == NULL))) {
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
    AmlDeleteTree ((AML_NODE_HEADER*)ObjectNode);
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
  IN  CONST CHAR8              * NameString,
  IN        UINT64               Integer,
  IN        AML_NODE_HEADER    * ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE   ** NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS          Status;
  AML_OBJECT_NODE   * ObjectNode;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL))) {
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
    AmlDeleteTree ((AML_NODE_HEADER*)ObjectNode);
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
  IN  CONST CHAR8              * NameString,
  IN        AML_NODE_HEADER    * ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE   ** NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS          Status;
  AML_OBJECT_NODE   * ObjectNode;
  AML_DATA_NODE     * DataNode;
  CHAR8             * AmlNameString;
  UINT32              AmlNameStringSize;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode = NULL;
  DataNode = NULL;
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
             (UINT8*)AmlNameString,
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
             (AML_NODE_HEADER*)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER*)DataNode);
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
    AmlDeleteTree ((AML_NODE_HEADER*)ObjectNode);
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
  IN  CONST CHAR8              * NameString,
  IN        AML_NODE_HEADER    * ParentNode,     OPTIONAL
  OUT       AML_OBJECT_NODE   ** NewObjectNode   OPTIONAL
  )
{
  EFI_STATUS          Status;
  AML_OBJECT_NODE   * ObjectNode;
  AML_DATA_NODE     * DataNode;
  CHAR8             * AmlNameString;
  UINT32              AmlNameStringSize;

  if ((NameString == NULL)  ||
      ((ParentNode == NULL) && (NewObjectNode == NULL))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode = NULL;
  DataNode = NULL;
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
             (UINT8*)AmlNameString,
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
             (AML_NODE_HEADER*)DataNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    AmlDeleteTree ((AML_NODE_HEADER*)DataNode);
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
    AmlDeleteTree ((AML_NODE_HEADER*)ObjectNode);
  }

error_handler1:
  if (AmlNameString != NULL) {
    FreePool (AmlNameString);
  }

  return Status;
}
