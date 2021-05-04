/** @file
  SEV Secret configuration table constructor

  Copyright (C) 2020 James Bottomley, IBM Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Guid/ConfidentialComputingSecret.h>

STATIC CONFIDENTIAL_COMPUTING_SECRET_LOCATION mSecretDxeTable = {
  FixedPcdGet32 (PcdSevLaunchSecretBase),
  FixedPcdGet32 (PcdSevLaunchSecretSize),
};

STATIC CONFIDENTIAL_COMPUTING_BLOB_LOCATION mSnpBootDxeTable = {
  SIGNATURE_32('A','M','D','E'),
  1,
  0,
  (UINT64)(UINTN) FixedPcdGet32 (PcdSevLaunchSecretBase),
  FixedPcdGet32 (PcdSevLaunchSecretSize),
  (UINT64)(UINTN) FixedPcdGet32 (PcdOvmfSnpCpuidBase),
  FixedPcdGet32 (PcdOvmfSnpCpuidSize),
};

EFI_STATUS
EFIAPI
InitializeSecretDxe(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  //
  // If its SEV-SNP active guest then install the CONFIDENTIAL_COMPUTING_BLOB.
  // It contains the location for both the Secrets and CPUID page.
  //
  if (MemEncryptSevSnpIsEnabled ()) {
    return gBS->InstallConfigurationTable (
                  &gConfidentialComputingBlobGuid,
                  &mSnpBootDxeTable
                  );
  }

  return gBS->InstallConfigurationTable (
                &gConfidentialComputingSecretGuid,
                &mSecretDxeTable
                );
}
