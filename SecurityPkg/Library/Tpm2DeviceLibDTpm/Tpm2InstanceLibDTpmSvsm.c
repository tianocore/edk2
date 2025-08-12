/** @file
  This library is a TPM2 DTPM instance, supporting SVSM based vTPMs and regular
  TPM2s at the same time.

  It can be registered to Tpm2 Device router, to be active TPM2 engine,
  based on platform setting.

Copyright (c) 2024 Red Hat
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2DumpLib.h>

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
  Registers DTPM2.0 instance and caches current active TPM interface type.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support registering a DTPM2.0 instance
**/
EFI_STATUS
EFIAPI
Tpm2InstanceLibDTpmConstructorSvsm (
  VOID
  )
{
  EFI_STATUS               Status;
  TPM2_PTP_INTERFACE_TYPE  PtpInterface;

  Status = Tpm2RegisterTpm2DeviceLib (&mDTpm2InternalTpm2Device);

  if (Status == EFI_UNSUPPORTED) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    return EFI_SUCCESS;
  }

  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (TryUseSvsmVTpm ()) {
    // SVSM vTPM found.
    return EFI_SUCCESS;
  }

  // No SVSM vTPM found; set up regular DTPM Ptp implementation
  Status = InternalTpm2DeviceLibDTpmCommonConstructor (&PtpInterface);
  DumpPtpInfo ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress), PtpInterface);

  return Status;
}
