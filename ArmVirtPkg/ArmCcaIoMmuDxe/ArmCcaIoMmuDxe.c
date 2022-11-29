/** @file

  IoMmuArmBowDxe driver installs EDKII_IOMMU_PROTOCOL to support
  DMA operations when the execution context is a Realm.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ArmCcaIoMmu.h"

/** Pointer to the Realm Aperture Management Protocol
*/
EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL  *mRamp = NULL;

/** Entrypoint of Arm CCA IoMMU Dxe.

  @param [in] ImageHandle   Image handle of this driver.
  @param [in] SystemTable   Pointer to the EFI System Table.

  @return RETURN_SUCCESS if successful, otherwise any other error.
**/
EFI_STATUS
EFIAPI
ArmCcaIoMmuDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  // When the execution context is a Realm, install ArmCcaIoMmu protocol
  // otherwise install the placeholder protocol so that other dependent
  // module can run.
  Status = gBS->LocateProtocol (
                  &gEfiRealmApertureManagementProtocolGuid,
                  NULL,
                  (VOID **)&mRamp
                  );
  if (!EFI_ERROR (Status)) {
    // If the Realm Aperture Management Protocol is present
    // then the execution context is a Realm.
    Status = ArmCcaInstallIoMmuProtocol ();
  } else {
    DEBUG ((DEBUG_INFO, "Execution context is not a Realm.\n"));
    Handle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gIoMmuAbsentProtocolGuid,
                    NULL,
                    NULL
                    );
  }

  return Status;
}
