/** @file
  Set TPM device type

  In SecurityPkg, this module initializes the TPM device type based on a UEFI
  variable and/or hardware detection. In OvmfPkg, the module only performs TPM2
  hardware detection.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/


#include <PiPei.h>

#include <Guid/TpmInstance.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/Tpm12CommandLib.h>
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

#pragma pack (1)

typedef struct {
  TPM_RSP_COMMAND_HDR   Hdr;
  TPM_CURRENT_TICKS     CurrentTicks;
} TPM_RSP_GET_TICKS;

#pragma pack ()

/**
  Probe for the TPM for 1.2 version, by sending TPM1.2 GetTicks

  Sending a TPM1.2 command to a TPM2 should return a TPM1.2
  header (tag = 0xc4) and error code (TPM_BADTAG = 0x1e)
**/
static
EFI_STATUS
TestTpm12 (
  )
{
  EFI_STATUS           Status;
  TPM_RQU_COMMAND_HDR  Command;
  TPM_RSP_GET_TICKS    Response;
  UINT32               Length;

  Command.tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.paramSize = SwapBytes32 (sizeof (Command));
  Command.ordinal   = SwapBytes32 (TPM_ORD_GetTicks);

  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

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

  Status = Tpm12RequestUseTpm ();
  if (!EFI_ERROR (Status) && !EFI_ERROR (TestTpm12 ())) {
    DEBUG ((DEBUG_INFO, "%a: TPM1.2 detected\n", __FUNCTION__));
    Size = sizeof (gEfiTpmDeviceInstanceTpm12Guid);
    Status = PcdSetPtrS (
               PcdTpmInstanceGuid,
               &Size,
               &gEfiTpmDeviceInstanceTpm12Guid
               );
    ASSERT_EFI_ERROR (Status);
  } else {
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
      DEBUG ((DEBUG_INFO, "%a: no TPM detected\n", __FUNCTION__));
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
  }

  //
  // Selection done
  //
  Status = PeiServicesInstallPpi (&mTpmSelectedPpi);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
