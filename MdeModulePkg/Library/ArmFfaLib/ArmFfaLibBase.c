/** @file
  This file is originally sourced from ArmFfaStandaloneMmLib, with
  Rx/Tx-related APIs removed to focus exclusively on FFA primitives.

  Provides FF-A ABI Library used in all environments.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#include <Uefi.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <Library/DebugLib.h>

#include "ArmFfaCommon.h"

/**
  ArmFfaLib Constructor.

  @retval EFI_SUCCESS            Success
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibBaseConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = ArmFfaLibCommonInit ();
  if (Status == EFI_UNSUPPORTED) {
    /*
     * EFI_UNSUPPORTED means FF-A interface isn't available.
     * However, for Standalone MM modules, FF-A availability is not required.
     * i.e. Standalone MM could use SpmMm as a legitimate protocol.
     * Thus, returning EFI_SUCCESS here to avoid the entrypoint to assert.
     */
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a failed. Status = %r\n", __func__, Status));
  }

  return Status;
}
