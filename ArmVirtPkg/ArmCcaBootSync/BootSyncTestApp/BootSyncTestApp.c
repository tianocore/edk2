/** @file
  EFI application to test Arm CCA Boot Sync.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncProtocol.h"

/**
  Entrypoint for the Boot Sync Test application.

  @param[in] ImageHandle  Handle to the image
  @param[in] SystemTable  Pointer to System Table.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS      Status;
  SECURE_CHANNEL  SecChannel;

  Print (L"Boot Sync Test Application\n");

  Status = ArmCcaBootSyncCryptoInit ();
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to init Crypto interfaces, Status = %r\n", Status);
    return Status;
  }

  ZeroMem (&SecChannel, sizeof (SECURE_CHANNEL));
  Status = EstablishSecureChannel (&SecChannel);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to establish Secure Session, Status = %r\n", Status);
    return Status;
  }

  DBG_DUMP_KEYS ("Sec", &SecChannel);

  // Send the FIN message to indicate End of Transmission.
  Status = SendFin (&SecChannel, BOOT_SYNC_COMM_END_REASON_EOT);
  ASSERT_EFI_ERROR (Status);

  Status = TerminateSecureChannel (&SecChannel);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to Close Secure Session Status = %r\n", Status);
  }

  return Status;
}
