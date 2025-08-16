/** @file
  Arm Ffa library code for StandaloneMmCore.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#include <PiMm.h>

#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "ArmFfaCommon.h"
#include "ArmFfaRxTxMap.h"

/**
  ArmFfaLib Constructor.

  @param  [in]  ImageHandle     The firmware allocated handle for the EFI image
  @param  [in]  MmSystemTable   A pointer to the Management mode System Table

  @retval EFI_SUCCESS            Success
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaStandaloneMmLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
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

  Status = ArmFfaLibRxTxMap ();
  if (Status == EFI_ALREADY_STARTED) {
    /*
     * When first Stmm instance (most likely core) which uses ArmFfaLib loaded,
     * It already maps Rx/Tx buffer.
     * From Next Stmm instance which uses ArmFfaLib it doesn't need to map Rx/Tx
     * buffer again but it uses the mapped one.
     */
    Status = EFI_SUCCESS;
  } else if (Status == EFI_UNSUPPORTED) {
    /*
     * StandaloneMm can be only service provider but not consumer in
     * some platform. so consider EFI_UNSUPPORTED as valid return.
     */
    Status = EFI_SUCCESS;
    DEBUG ((DEBUG_INFO, "%a Rx/Tx buffer doesn't support.\n", __func__));
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a failed. Status = %r\n", __func__, Status));
  }

  return Status;
}
