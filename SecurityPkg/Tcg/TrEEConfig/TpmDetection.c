/** @file
  TPM1.2/dTPM2.0 auto detection.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/Tpm12CommandLib.h>
#include <IndustryStandard/Tpm12.h>

#include "TrEEConfigNvData.h"

/**
  This routine return if dTPM (1.2 or 2.0) present.

  @retval TRUE  dTPM present
  @retval FALSE dTPM not present
**/
BOOLEAN
IsDtpmPresent (
  VOID
  )
{
  UINT8                             RegRead;
  
  RegRead = MmioRead8 ((UINTN)PcdGet64 (PcdTpmBaseAddress));
  if (RegRead == 0xFF) {
    DEBUG ((EFI_D_ERROR, "DetectTpmDevice: Dtpm not present\n"));
    return FALSE;
  } else {
    DEBUG ((EFI_D_ERROR, "DetectTpmDevice: Dtpm present\n"));
    return TRUE;
  }
}

/**
  This routine check both SetupVariable and real TPM device, and return final TpmDevice configuration.

  @param  SetupTpmDevice  TpmDevice configuration in setup driver

  @return TpmDevice configuration
**/
UINT8
DetectTpmDevice (
  IN UINT8 SetupTpmDevice
  )
{
  EFI_STATUS                        Status;
  EFI_BOOT_MODE                     BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // In S3, we rely on Setup option, because we save to Setup in normal boot.
  //
  if (BootMode == BOOT_ON_S3_RESUME) {
    DEBUG ((EFI_D_ERROR, "DetectTpmDevice: S3 mode\n"));
    return SetupTpmDevice;
  }

  if (PcdGetBool (PcdHideTpmSupport) && PcdGetBool (PcdHideTpm)) {
    DEBUG ((EFI_D_ERROR, "DetectTpmDevice: Tpm is hide\n"));
    return TPM_DEVICE_NULL;
  }

  DEBUG ((EFI_D_ERROR, "DetectTpmDevice:\n"));
  if ((!IsDtpmPresent ()) || (SetupTpmDevice == TPM_DEVICE_NULL)) {
    // dTPM not available
    return TPM_DEVICE_NULL;
  }

  // dTPM available and not disabled by setup
  // We need check if it is TPM1.2 or TPM2.0
  // So try TPM1.2 command at first

  Status = Tpm12RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    return TPM_DEVICE_2_0_DTPM;
  }

  Status = Tpm12Startup (TPM_ST_CLEAR);
  if (EFI_ERROR (Status)) {
    return TPM_DEVICE_2_0_DTPM;
  }

  // NO initialization needed again.
  PcdSet8 (PcdTpmInitializationPolicy, 0);
  return TPM_DEVICE_1_2;
}
