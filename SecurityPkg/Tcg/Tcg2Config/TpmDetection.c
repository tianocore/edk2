/** @file
  TPM1.2/dTPM2.0 auto detection.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/ReadOnlyVariable2.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/Tpm12CommandLib.h>
#include <IndustryStandard/Tpm12.h>

#include "Tcg2ConfigNvData.h"
#include "Tcg2Internal.h"

/**
  This routine check both SetupVariable and real TPM device, and return final TpmDevice configuration.

  @param  SetupTpmDevice  TpmDevice configuration in setup driver

  @return TpmDevice configuration
**/
UINT8
DetectTpmDevice (
  IN UINT8  SetupTpmDevice
  )
{
  EFI_STATUS                       Status;
  EFI_BOOT_MODE                    BootMode;
  TCG2_DEVICE_DETECTION            Tcg2DeviceDetection;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *VariablePpi;
  UINTN                            Size;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // In S3, we rely on normal boot Detection, because we save to ReadOnly Variable in normal boot.
  //
  if (BootMode == BOOT_ON_S3_RESUME) {
    DEBUG ((DEBUG_INFO, "DetectTpmDevice: S3 mode\n"));

    Status = PeiServicesLocatePpi (&gEfiPeiReadOnlyVariable2PpiGuid, 0, NULL, (VOID **)&VariablePpi);
    ASSERT_EFI_ERROR (Status);

    Size = sizeof (TCG2_DEVICE_DETECTION);
    ZeroMem (&Tcg2DeviceDetection, sizeof (Tcg2DeviceDetection));
    Status = VariablePpi->GetVariable (
                            VariablePpi,
                            TCG2_DEVICE_DETECTION_NAME,
                            &gTcg2ConfigFormSetGuid,
                            NULL,
                            &Size,
                            &Tcg2DeviceDetection
                            );
    if (!EFI_ERROR (Status) &&
        (Tcg2DeviceDetection.TpmDeviceDetected >= TPM_DEVICE_MIN) &&
        (Tcg2DeviceDetection.TpmDeviceDetected <= TPM_DEVICE_MAX))
    {
      DEBUG ((DEBUG_ERROR, "TpmDevice from DeviceDetection: %x\n", Tcg2DeviceDetection.TpmDeviceDetected));
      return Tcg2DeviceDetection.TpmDeviceDetected;
    }
  }

  DEBUG ((DEBUG_INFO, "DetectTpmDevice:\n"));

  // dTPM available and not disabled by setup
  // We need check if it is TPM1.2 or TPM2.0
  // So try TPM1.2 command at first

  Status = Tpm12RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    //
    // dTPM not available
    //
    return TPM_DEVICE_NULL;
  }

  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = Tpm12Startup (TPM_ST_STATE);
  } else {
    Status = Tpm12Startup (TPM_ST_CLEAR);
  }

  if (EFI_ERROR (Status)) {
    return TPM_DEVICE_2_0_DTPM;
  }

  // NO initialization needed again.
  Status = PcdSet8S (PcdTpmInitializationPolicy, 0);
  ASSERT_EFI_ERROR (Status);
  return TPM_DEVICE_1_2;
}
