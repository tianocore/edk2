/** @file
  AML Api.

  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/* Even though this file has access to the internal Node definition,
   i.e. AML_ROOT_NODE, AML_OBJECT_NODE, etc. Only the external node
   handle types should be used, i.e. AML_NODE_HANDLE, AML_ROOT_NODE_HANDLE,
   etc.
   Indeed, the functions in the "Api" folder should be implemented only
   using the "safe" functions available in the "Include" folder. This
   makes the functions available in the "Api" folder easy to export.
*/
#include <AmlNodeDefines.h>

#include <AmlCoreInterface.h>
#include <AmlInclude.h>
#include <Api/AmlApiHelper.h>
#include <String/AmlString.h>

/** Update the name of a DeviceOp object node.

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
  IN  CHAR8                   *NewNameString
  )
{
  EFI_STATUS  Status;

  AML_DATA_NODE_HANDLE  DeviceNameDataNode;
  CHAR8                 *NewAmlNameString;
  UINT32                NewAmlNameStringSize;

  // Check the input node is an object node.
  if ((DeviceOpNode == NULL)                                              ||
      (AmlGetNodeType ((AML_NODE_HANDLE)DeviceOpNode) != EAmlNodeObject)  ||
      (!AmlNodeHasOpCode (DeviceOpNode, AML_EXT_OP, AML_EXT_DEVICE_OP))   ||
      (NewNameString == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the Device's name, being a data node
  // which is the 1st fixed argument (i.e. index 0).
  DeviceNameDataNode = (AML_DATA_NODE_HANDLE)AmlGetFixedArgument (
                                               DeviceOpNode,
                                               EAmlParseIndexTerm0
                                               );
  if ((DeviceNameDataNode == NULL)                                            ||
      (AmlGetNodeType ((AML_NODE_HANDLE)DeviceNameDataNode) != EAmlNodeData)  ||
      (!AmlNodeHasDataType (DeviceNameDataNode, EAmlNodeDataTypeNameString)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = ConvertAslNameToAmlName (NewNameString, &NewAmlNameString);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlGetNameStringSize (NewAmlNameString, &NewAmlNameStringSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Update the Device's name node.
  Status = AmlUpdateDataNode (
             DeviceNameDataNode,
             EAmlNodeDataTypeNameString,
             (UINT8 *)NewAmlNameString,
             NewAmlNameStringSize
             );
  ASSERT_EFI_ERROR (Status);

exit_handler:
  FreePool (NewAmlNameString);
  return Status;
}

/** Update an integer value defined by a NameOp object node.

  For compatibility reasons, the NameOpNode must initially
  contain an integer.

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
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  IntegerOpNode;

  if ((NameOpNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)NameOpNode) != EAmlNodeObject) ||
      (!AmlNodeHasOpCode (NameOpNode, AML_NAME_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the Integer object node defined by the "Name ()" function:
  // it must have an Integer OpCode (Byte/Word/DWord/QWord).
  // It is the 2nd fixed argument (i.e. index 1) of the NameOp node.
  // This can also be a ZeroOp or OneOp node.
  IntegerOpNode = (AML_OBJECT_NODE_HANDLE)AmlGetFixedArgument (
                                            NameOpNode,
                                            EAmlParseIndexTerm1
                                            );
  if ((IntegerOpNode == NULL)  ||
      (AmlGetNodeType ((AML_NODE_HANDLE)IntegerOpNode) != EAmlNodeObject))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Update the Integer value.
  Status = AmlUpdateInteger (IntegerOpNode, NewInt);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Update a string value defined by a NameOp object node.

  The NameOpNode must initially contain a string.
  The EISAID ASL macro converts a string to an integer. This, it is
  not accepted.

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
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  StringOpNode;
  AML_DATA_NODE_HANDLE    StringDataNode;

  if ((NameOpNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)NameOpNode) != EAmlNodeObject) ||
      (!AmlNodeHasOpCode (NameOpNode, AML_NAME_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the String object node defined by the "Name ()" function:
  // it must have a string OpCode.
  // It is the 2nd fixed argument (i.e. index 1) of the NameOp node.
  StringOpNode = (AML_OBJECT_NODE_HANDLE)AmlGetFixedArgument (
                                           NameOpNode,
                                           EAmlParseIndexTerm1
                                           );
  if ((StringOpNode == NULL)  ||
      (AmlGetNodeType ((AML_NODE_HANDLE)StringOpNode) != EAmlNodeObject))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the string data node.
  // It is the 1st fixed argument (i.e. index 0) of the StringOpNode node.
  StringDataNode = (AML_DATA_NODE_HANDLE)AmlGetFixedArgument (
                                           StringOpNode,
                                           EAmlParseIndexTerm0
                                           );
  if ((StringDataNode == NULL)  ||
      (AmlGetNodeType ((AML_NODE_HANDLE)StringDataNode) != EAmlNodeData))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Update the string value.
  Status = AmlUpdateDataNode (
             StringDataNode,
             EAmlNodeDataTypeString,
             (UINT8 *)NewName,
             (UINT32)AsciiStrLen (NewName) + 1
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  )
{
  AML_OBJECT_NODE_HANDLE  BufferOpNode;
  AML_DATA_NODE_HANDLE    FirstRdNode;

  if ((NameOpNode == NULL)                                              ||
      (AmlGetNodeType ((AML_NODE_HANDLE)NameOpNode) != EAmlNodeObject)  ||
      (!AmlNodeHasOpCode (NameOpNode, AML_NAME_OP, 0))                  ||
      (OutRdNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutRdNode = NULL;

  // Get the value of the variable which is represented as a BufferOp object
  // node which is the 2nd fixed argument (i.e. index 1).
  BufferOpNode = (AML_OBJECT_NODE_HANDLE)AmlGetFixedArgument (
                                           NameOpNode,
                                           EAmlParseIndexTerm1
                                           );
  if ((BufferOpNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)BufferOpNode) != EAmlNodeObject) ||
      (!AmlNodeHasOpCode (BufferOpNode, AML_BUFFER_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the first Resource data node in the variable list of
  // argument of the BufferOp node.
  FirstRdNode = (AML_DATA_NODE_HANDLE)AmlGetNextVariableArgument (
                                        (AML_NODE_HANDLE)BufferOpNode,
                                        NULL
                                        );
  if ((FirstRdNode == NULL)                                            ||
      (AmlGetNodeType ((AML_NODE_HANDLE)FirstRdNode) != EAmlNodeData)  ||
      (!AmlNodeHasDataType (FirstRdNode, EAmlNodeDataTypeResourceData)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutRdNode = FirstRdNode;
  return EFI_SUCCESS;
}

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
  )
{
  AML_OBJECT_NODE_HANDLE  NameOpNode;
  AML_OBJECT_NODE_HANDLE  BufferOpNode;

  if ((CurrRdNode == NULL)                                              ||
      (AmlGetNodeType ((AML_NODE_HANDLE)CurrRdNode) != EAmlNodeData)    ||
      (!AmlNodeHasDataType (CurrRdNode, EAmlNodeDataTypeResourceData))  ||
      (OutRdNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutRdNode = NULL;

  // The parent of the CurrRdNode must be a BufferOp node.
  BufferOpNode = (AML_OBJECT_NODE_HANDLE)AmlGetParent (
                                           (AML_NODE_HANDLE)CurrRdNode
                                           );
  if ((BufferOpNode == NULL)  ||
      (!AmlNodeHasOpCode (BufferOpNode, AML_BUFFER_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The parent of the BufferOpNode must be a NameOp node.
  NameOpNode = (AML_OBJECT_NODE_HANDLE)AmlGetParent (
                                         (AML_NODE_HANDLE)BufferOpNode
                                         );
  if ((NameOpNode == NULL)  ||
      (!AmlNodeHasOpCode (NameOpNode, AML_NAME_OP, 0)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutRdNode = (AML_DATA_NODE_HANDLE)AmlGetNextVariableArgument (
                                       (AML_NODE_HANDLE)BufferOpNode,
                                       (AML_NODE_HANDLE)CurrRdNode
                                       );

  // If the Resource Data is an End Tag, return NULL.
  if (AmlNodeHasRdDataType (
        *OutRdNode,
        AML_RD_BUILD_SMALL_DESC_ID (ACPI_SMALL_END_TAG_DESCRIPTOR_NAME)
        ))
  {
    *OutRdNode = NULL;
  }

  return EFI_SUCCESS;
}

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
  )
{
  return AmlVarListAddTail (ParentNode, NewNode);
}

// DEPRECATED APIS
#ifndef DISABLE_NEW_DEPRECATED_INTERFACES

/** DEPRECATED API

  Get the first Resource Data element contained in a "_CRS" object.

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
  IN  AML_OBJECT_NODE_HANDLE  NameOpCrsNode,
  OUT AML_DATA_NODE_HANDLE    *OutRdNode
  )
{
  return AmlNameOpGetFirstRdNode (NameOpCrsNode, OutRdNode);
}

/** DEPRECATED API

  Get the Resource Data element following the CurrRdNode Resource Data.

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
  IN  AML_DATA_NODE_HANDLE  CurrRdNode,
  OUT AML_DATA_NODE_HANDLE  *OutRdNode
  )
{
  return AmlNameOpGetNextRdNode (CurrRdNode, OutRdNode);
}

#endif // DISABLE_NEW_DEPRECATED_INTERFACES
