/** @file
  Null implementation of InternalTpm12Detect(), always returning
  EFI_UNSUPPORTED.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Tpm12Support.h"

/**
  Detect the presence of a TPM with interface version 1.2.

  @retval EFI_UNSUPPORTED  The platform that includes this particular
                           implementation of the function does not support
                           TPM-1.2.
**/
EFI_STATUS
InternalTpm12Detect (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
