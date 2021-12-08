/** @file
  Confidential Computing Secret configuration table constructor

  Copyright (C) 2020 James Bottomley, IBM Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Guid/ConfidentialComputingSecret.h>

STATIC CONFIDENTIAL_COMPUTING_SECRET_LOCATION  mSecretDxeTable = {
  FixedPcdGet32 (PcdSevLaunchSecretBase),
  FixedPcdGet32 (PcdSevLaunchSecretSize),
};

EFI_STATUS
EFIAPI
InitializeSecretDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return gBS->InstallConfigurationTable (
                &gConfidentialComputingSecretGuid,
                &mSecretDxeTable
                );
}
