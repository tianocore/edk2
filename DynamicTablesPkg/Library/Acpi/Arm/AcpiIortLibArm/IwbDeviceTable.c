/** @file
  Generate IWB SSDT table.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - IO Remapping Table, Platform Design Document, Revision E.g, Mar 2026
    (https://developer.arm.com/documentation/den0049/)
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>

#include "IortGenerator.h"

/** According to GICv5 specification chapther 10.4.1 IWB_CONFIG_FRAME,
    IWB_CONFIG_FRAME's block size is 0x10000 bytes.
*/
#define IWB_FRAME_BLOCK_SIZE  0x10000

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  iwbdevicetabletemplate_aml_code[];

/** Fixup the IWB device UID (_UID).

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  Uid             UID for the IWB device.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupIwbDeviceUid (
  IN        AML_ROOT_NODE_HANDLE  RootNodeHandle,
  IN  CONST UINT64                Uid
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  NameOpIdNode;

  // Get the _UID NameOp object defined by the "Name ()" statement,
  // and update its value.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.IWB0._UID",
             &NameOpIdNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AmlNameOpUpdateInteger (NameOpIdNode, (UINT64)Uid);
}

/** Fixup the Iwb device name.

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  Name            The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupIwbDeviceName (
  IN        AML_ROOT_NODE_HANDLE  RootNodeHandle,
  IN  CONST CHAR8                 *Name
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  // Get the COM0 variable defined by the "Device ()" statement.
  Status = AmlFindNode (RootNodeHandle, "\\_SB_.IWB0", &DeviceNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Update the Device's name.
  return AmlDeviceOpUpdateName (DeviceNode, Name);
}

/** Fixup the Iwb device _CRS values (BaseAddress, ...).

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  IwbInfo         Pointer to a IWB device Information
                                structure.
                                Get the device size Information from there.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupIwbDeviceCrs (
  IN        AML_ROOT_NODE_HANDLE  RootNodeHandle,
  IN  CONST CM_ARM_GIC_IWB_INFO   *IwbInfo
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  NameOpCrsNode;
  AML_DATA_NODE_HANDLE    QWordRdNode;

  // Get the "_CRS" object defined by the "Name ()" statement.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.IWB0._CRS",
             &NameOpCrsNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the first Rd node in the "_CRS" object.
  Status = AmlNameOpGetFirstRdNode (NameOpCrsNode, &QWordRdNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (QWordRdNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Update the IWB device's base address and length.
  Status = AmlUpdateRdQWord (
             QWordRdNode,
             IwbInfo->ConfigFrameBase,
             IWB_FRAME_BLOCK_SIZE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/** Fixup the IWB Device in the AML tree.

  For each template value:
   - find the node to update;
   - update the value.

  @param  [in]  RootNodeHandle  Pointer to the root of the AML tree.
  @param  [in]  IwbInfo         Pointer to a IWB device Information
                                structure.
  @param  [in]  Name            The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param  [in]  Uid             UID for the IWB device.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupIwbDeviceInfo (
  IN            AML_ROOT_NODE_HANDLE  RootNodeHandle,
  IN      CONST CM_ARM_GIC_IWB_INFO   *IwbInfo,
  IN      CONST CHAR8                 *Name,
  IN      CONST UINT64                Uid
  )
{
  EFI_STATUS  Status;

  ASSERT (RootNodeHandle != NULL);
  ASSERT (IwbInfo != NULL);
  ASSERT (Name != NULL);

  // Fixup the _UID value.
  Status = FixupIwbDeviceUid (RootNodeHandle, Uid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fixup the _CRS values.
  Status = FixupIwbDeviceCrs (RootNodeHandle, IwbInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fixup the Iwb device name.
  // This MUST be done at the end, otherwise AML paths won't be valid anymore.
  return FixupIwbDeviceName (RootNodeHandle, Name);
}

/** Build a SSDT table describing the IWB device.

  The table created by this function must be freed by FreeSimpleIwbDeviceTable.

  @param [in]  IwbInfo          IWB device info to describe in the SSDT table.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
BuildIwbDeviceTable (
  IN  CONST CM_ARM_GIC_IWB_INFO          *IwbInfo,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            Status1;
  AML_ROOT_NODE_HANDLE  RootNodeHandle;
  CHAR8                 IwbDevName[IWB_DEV_NAME_SIZE];

  ASSERT (IwbInfo != NULL);
  ASSERT (Table != NULL);

  IwbDevName[0] = 'I';
  IwbDevName[1] = 'W';
  IwbDevName[2] = 'B';
  IwbDevName[3] = AsciiFromHex (IwbInfo->GicIwbId);
  IwbDevName[4] = '\0';

  // Parse the IWB Device Table Template.
  Status = AmlParseDefinitionBlock (
             (EFI_ACPI_DESCRIPTION_HEADER *)iwbdevicetabletemplate_aml_code,
             &RootNodeHandle
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IWB-DEVICE-FIXUP:"
      " Failed to parse SSDT IWB device Template. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Fixup the template values.
  Status = FixupIwbDeviceInfo (
             RootNodeHandle,
             IwbInfo,
             IwbDevName,
             IwbInfo->GicIwbId
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IWB-DEVICE-FIXUP: Failed to fixup SSDT IWB Device Table."
      " Status = %r\n",
      Status
      ));
    goto ExitHandler;
  }

  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNodeHandle,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IWB-DEVICE-FIXUP: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
  }

ExitHandler:
  // Cleanup
  if (RootNodeHandle != NULL) {
    Status1 = AmlDeleteTree (RootNodeHandle);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IWB-DEVICE-FIXUP: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
      // If Status was success but we failed to delete the AML Tree
      // return Status1 else return the original error code, i.e. Status.
      if (!EFI_ERROR (Status)) {
        return Status1;
      }
    }
  }

  return Status;
}

/** Free an Iwb device table previously created by
    the BuildIwbDeviceTable function.

  @param [in] Table   Pointer to a Iwb Device table allocated by
                      the BuildIwbDeviceTable function.

**/
VOID
EFIAPI
FreeIwbDeviceTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  ASSERT (Table != NULL);
  FreePool (Table);
}
