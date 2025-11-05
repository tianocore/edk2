/** @file
  Declare the InternalTpm12Detect() function, hiding the TPM-1.2 detection
  internals.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TPM12_SUPPORT_H_
#define TPM12_SUPPORT_H_

#include <Uefi/UefiBaseType.h>

/**
  Detect the presence of a TPM with interface version 1.2.

  @retval EFI_UNSUPPORTED  The platform that includes this particular
                           implementation of the function does not support
                           TPM-1.2.

  @retval EFI_SUCCESS      TPM-1.2 available. The Tpm12RequestUseTpm() and
                           Tpm12SubmitCommand(TPM_ORD_GetTicks) operations
                           (from the Tpm12DeviceLib class) have succeeded.

  @return                  Error codes propagated from Tpm12RequestUseTpm() and
                           Tpm12SubmitCommand().
**/
EFI_STATUS
InternalTpm12Detect (
  VOID
  );

#endif // TPM12_SUPPORT_H_
