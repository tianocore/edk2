/** @file
  Tpm2 device table generating Library

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - TCG ACPI specification.
    (https://trustedcomputinggroup.org/resource/tcg-acpi-specification/)
**/
#include <IndustryStandard/DebugPort2Table.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  tpm2devicetabletemplate_aml_code[];

/** Fixup the TPM2 device UID (_UID).

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  Uid             UID for the TPM2 device.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupTpm2DeviceUid (
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
             "\\_SB_.TPM0._UID",
             &NameOpIdNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AmlNameOpUpdateInteger (NameOpIdNode, (UINT64)Uid);
}

/** Fixup the Tpm2 device name.

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
FixupTpm2DeviceName (
  IN        AML_ROOT_NODE_HANDLE  RootNodeHandle,
  IN  CONST CHAR8                 *Name
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  // Get the COM0 variable defined by the "Device ()" statement.
  Status = AmlFindNode (RootNodeHandle, "\\_SB_.TPM0", &DeviceNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Update the Device's name.
  return AmlDeviceOpUpdateName (DeviceNode, Name);
}

/** Fixup the Tpm2 device _CRS values (BaseAddress, ...).

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  TpmDevInfo      Pointer to a TPM2 device Information
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
FixupTpm2DeviceCrs (
  IN        AML_ROOT_NODE_HANDLE             RootNodeHandle,
  IN  CONST CM_ARCH_COMMON_TPM2_DEVICE_INFO  *TpmDevInfo
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  NameOpCrsNode;
  AML_DATA_NODE_HANDLE    QWordRdNode;

  // Get the "_CRS" object defined by the "Name ()" statement.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.TPM0._CRS",
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

  // Update the TPM2 device's base address and length.
  Status = AmlUpdateRdQWord (
             QWordRdNode,
             TpmDevInfo->Tpm2DeviceBaseAddress,
             TpmDevInfo->Tpm2DeviceSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/** Fixup the Tpm2 Device in the AML tree.

  For each template value:
   - find the node to update;
   - update the value.

  @param  [in]  RootNodeHandle  Pointer to the root of the AML tree.
  @param  [in]  TpmDevInfo      Pointer to a TPM2 device Information
                                structure.
  @param  [in]  Name            The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param  [in]  Uid             UID for the TPM2 device.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupTpm2DeviceInfo (
  IN            AML_ROOT_NODE_HANDLE             RootNodeHandle,
  IN      CONST CM_ARCH_COMMON_TPM2_DEVICE_INFO  *TpmDevInfo,
  IN      CONST CHAR8                            *Name,
  IN      CONST UINT64                           Uid
  )
{
  EFI_STATUS  Status;

  ASSERT (RootNodeHandle != NULL);
  ASSERT (TpmDevInfo != NULL);
  ASSERT (Name != NULL);

  // Fixup the _UID value.
  Status = FixupTpm2DeviceUid (RootNodeHandle, Uid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fixup the _CRS values.
  Status = FixupTpm2DeviceCrs (RootNodeHandle, TpmDevInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fixup the Tpm2 device name.
  // This MUST be done at the end, otherwise AML paths won't be valid anymore.
  return FixupTpm2DeviceName (RootNodeHandle, Name);
}

/** Build a SSDT table describing the TPM2 device.

  The table created by this function must be freed by FreeSImpleTpm2DeviceTable.

  @param [in]  TpmDevInfo      TPM2 device info to describe in the SSDT table.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  Uid              UID for the TPM2 device
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
BuildTpm2DeviceTable (
  IN  CONST CM_ARCH_COMMON_TPM2_DEVICE_INFO  *TpmDevInfo,
  IN  CONST CHAR8                            *Name,
  IN  CONST UINT64                           Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER      **Table
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            Status1;
  AML_ROOT_NODE_HANDLE  RootNodeHandle;

  ASSERT (TpmDevInfo != NULL);
  ASSERT (Name != NULL);
  ASSERT (Table != NULL);

  // Parse the Tpm2 Device Table Template.
  Status = AmlParseDefinitionBlock (
             (EFI_ACPI_DESCRIPTION_HEADER *)tpm2devicetabletemplate_aml_code,
             &RootNodeHandle
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2-DEVICE-FIXUP:"
      " Failed to parse SSDT TPM2 device Template. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Fixup the template values.
  Status = FixupTpm2DeviceInfo (
             RootNodeHandle,
             TpmDevInfo,
             Name,
             Uid
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2-DEVICE-FIXUP: Failed to fixup SSDT TPM2 Device Table."
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
      "ERROR: TPM2-DEVICE-FIXUP: Failed to Serialize SSDT Table Data."
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
        "ERROR: TPM2-DEVICE-FIXUP: Failed to cleanup AML tree."
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

/** Free an Tpm2 device table previously created by
    the BuildTpm2DeviceTable function.

  @param [in] Table   Pointer to a Tpm2 Device table allocated by
                      the BuildTpm2DeviceTable function.

**/
VOID
EFIAPI
FreeTpm2DeviceTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  ASSERT (Table != NULL);
  FreePool (Table);
}
