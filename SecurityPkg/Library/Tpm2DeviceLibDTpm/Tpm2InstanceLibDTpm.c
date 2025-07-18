/** @file
  This library is TPM2 DTPM instance.
  It can be registered to Tpm2 Device router, to be active TPM2 engine,
  based on platform setting.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2DumpLib.h>
#include <Library/PcdLib.h>

#include <Guid/TpmInstance.h>

#include "Tpm2Ptp.h"
#include "Tpm2DeviceLibDTpm.h"

TPM2_DEVICE_INTERFACE  mDTpm2InternalTpm2Device = {
  TPM_DEVICE_INTERFACE_TPM20_DTPM,
  DTpm2SubmitCommand,
  DTpm2RequestUseTpm,
};

/**
  Registers DTPM2.0 instance and caches current active TPM interface type.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support registering a DTPM2.0 instance
**/
EFI_STATUS
EFIAPI
Tpm2InstanceLibDTpmConstructor (
  VOID
  )
{
  EFI_STATUS               Status;
  TPM2_PTP_INTERFACE_TYPE  PtpInterface;

  Status = Tpm2RegisterTpm2DeviceLib (&mDTpm2InternalTpm2Device);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    if (Status == EFI_SUCCESS) {
      Status = InternalTpm2DeviceLibDTpmCommonConstructor (&PtpInterface);
      DumpPtpInfo ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress), PtpInterface);
    }

    return EFI_SUCCESS;
  }

  return Status;
}
