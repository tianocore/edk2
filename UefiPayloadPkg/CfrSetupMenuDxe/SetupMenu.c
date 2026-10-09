/** @file
  A Setup Menu for configuring boot options defined by bootloader CFR.

  Copyright (c) 2023, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SetupMenu.h"
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Guid/VariableFormat.h>

EDKII_VARIABLE_POLICY_PROTOCOL  *mVariablePolicy = NULL;

/**
  This function installs the HII form.

**/
EFI_STATUS
EFIAPI
CfrSetupMenuEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&mVariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "CFR: Unable to lock variables!\n"));
  }

  //
  // Install Device Path and Config Access protocols to driver handle
  //
  mSetupMenuPrivate.DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSetupMenuPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSetupMenuHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mSetupMenuPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data.
  //
  mSetupMenuPrivate.HiiHandle = HiiAddPackages (
                                  &mSetupMenuFormsetGuid,
                                  mSetupMenuPrivate.DriverHandle,
                                  SetupMenuVfrBin,
                                  CfrSetupMenuDxeStrings,
                                  NULL
                                  );
  ASSERT (mSetupMenuPrivate.HiiHandle != NULL);

  //
  // Insert runtime components from bootloader's CFR table.
  //
  CfrCreateRuntimeComponents ();

  return Status;
}

/**
  This function uninstalls the HII form.

**/
EFI_STATUS
EFIAPI
CfrSetupMenuUnload (
  IN EFI_HANDLE        ImageHandle
  )
{
  EFI_STATUS  Status;

  //
  // Uninstall Device Path and Config Access protocols
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  mSetupMenuPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSetupMenuHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mSetupMenuPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Remove our HII data
  //
  HiiRemovePackages (mSetupMenuPrivate.HiiHandle);

  return Status;
}
