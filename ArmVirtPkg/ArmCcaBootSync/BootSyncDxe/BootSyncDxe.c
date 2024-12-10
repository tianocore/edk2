/** @file
  Arm CCA Boot Sync Dxe.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Library/ArmCcaLib.h>
#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncBsbParser.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncProtocol.h"

#include "Guest/BootSyncProtocolGuest.h"

/**
  Perform Arm CCA Boot Sync.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error occured.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
PerformSync (
  VOID
  )
{
  EFI_STATUS      Status;
  EFI_STATUS      Status1;
  SECURE_CHANNEL  SecChannel;

  ZeroMem (&SecChannel, sizeof (SECURE_CHANNEL));
  Status = EstablishSecureChannel (&SecChannel);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to establish Secure Session, Status = %r\n",
      Status
      ));
    return Status;
  }

  DBG_DUMP_KEYS ("Sec", &SecChannel);

  Status = BootSyncPerformAttestation (&SecChannel);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Attestation Failed!\n", Status));
    goto exit_handler;
  }

  DEBUG ((DEBUG_INFO, "Attestation Status = %r\n", Status));

  // Send the FIN message to indicate End of Transmission.
  Status = SendFin (&SecChannel, BOOT_SYNC_COMM_END_REASON_EOT);
  ASSERT_EFI_ERROR (Status);

exit_handler:
  Status1 = TerminateSecureChannel (&SecChannel);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to Close Secure Session Status = %r\n",
      Status1
      ));
  }

  // Return the first error otherwise return Status1.
  if (!EFI_ERROR (Status)) {
    Status = Status1;
  }

  return Status;
}

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
    return Status;
  }

  Status = PerformSync ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to perform Boot Sync, Status = %r\n",
      Status
      ));
    CpuDeadLoop ();
  }

  return Status;
}
