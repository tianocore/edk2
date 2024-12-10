/** @file
  Arm CCA Boot Sync Dxe.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Library/ArmCcaLib.h>

/**
  Entrypoint of Arm CCA Boot Sync Dxe.

  @param[in]  ImageHandle   Handle to the Image.
  @param[in]  SystemTable   Pointer to the system table.

  @retval EFI_SUCCESS           Success.
  @retval EFI_UNSUPPORTED       Unsupported.
  @retval EFI_ABORTED           An operation was aborted.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (!ArmCcaIsRealm ()) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Boot Sync Dxe.\n"));

  Status = ArmCcaBootSyncCryptoInit ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to init Crypto interfaces, Status = %r\n",
      Status
      ));
  }

  return Status;
}
