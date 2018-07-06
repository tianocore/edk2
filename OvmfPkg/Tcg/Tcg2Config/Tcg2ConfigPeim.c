/** @file
  Set TPM device type

  In SecurityPkg, this module initializes the TPM device type based on a UEFI
  variable and/or hardware detection. In OvmfPkg, the module only performs TPM2
  hardware detection.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2018, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/


#include <PiPei.h>

#include <Guid/TpmInstance.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Ppi/TpmInitialized.h>

STATIC CONST EFI_PEI_PPI_DESCRIPTOR mTpmSelectedPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTpmDeviceSelectedGuid,
  NULL
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpmInitializationDonePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

/**
  The entry point for Tcg2 configuration driver.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.
**/
EFI_STATUS
EFIAPI
Tcg2ConfigPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINTN                           Size;
  EFI_STATUS                      Status;

  DEBUG ((DEBUG_INFO, "%a\n", __FUNCTION__));

  Status = Tpm2RequestUseTpm ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: TPM2 detected\n", __FUNCTION__));
    Size = sizeof (gEfiTpmDeviceInstanceTpm20DtpmGuid);
    Status = PcdSetPtrS (
               PcdTpmInstanceGuid,
               &Size,
               &gEfiTpmDeviceInstanceTpm20DtpmGuid
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((DEBUG_INFO, "%a: no TPM2 detected\n", __FUNCTION__));
    //
    // If no TPM2 was detected, we still need to install
    // TpmInitializationDonePpi. Namely, Tcg2Pei will exit early upon seeing
    // the default (all-bits-zero) contents of PcdTpmInstanceGuid, thus we have
    // to install the PPI in its place, in order to unblock any dependent
    // PEIMs.
    //
    Status = PeiServicesInstallPpi (&mTpmInitializationDonePpiList);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Selection done
  //
  Status = PeiServicesInstallPpi (&mTpmSelectedPpi);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
