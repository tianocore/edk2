/** @file
  This library is a TPM2 DTPM instance, supporting SVSM based vTPMs and regular
  TPM2s at the same time.

  It can be registered to Tpm2 Device router, to be active TPM2 engine,
  based on platform setting.

Copyright (c) 2024 Red Hat
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>

#include <Guid/TpmInstance.h>

#include "Tpm2Ptp.h"
#include "Tpm2DeviceLibDTpm.h"
#include "Tpm2PtpSvsmShim.h"

TPM2_DEVICE_INTERFACE  mDTpm2InternalTpm2Device = {
  TPM_DEVICE_INTERFACE_TPM20_DTPM,
  SvsmDTpm2SubmitCommand,
  SvsmDTpm2RequestUseTpm,
};

/**
  The function register DTPM2.0 instance and caches current active TPM interface type.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support register DTPM2.0 instance
**/
EFI_STATUS
EFIAPI
Tpm2InstanceLibDTpmConstructorSvsm (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = Tpm2RegisterTpm2DeviceLib (&mDTpm2InternalTpm2Device);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    if (Status == EFI_SUCCESS) {
      if (TryUseSvsmVTpm ()) {
        DEBUG ((DEBUG_INFO, "Tpm2InstanceLib: Found SVSM vTPM\n"));
      } else {
        Status = InternalTpm2DeviceLibDTpmCommonConstructor ();
        DumpPtpInfo ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
      }
    }

    return EFI_SUCCESS;
  }

  return Status;
}
