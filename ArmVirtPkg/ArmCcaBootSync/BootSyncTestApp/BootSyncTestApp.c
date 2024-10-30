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
#include <Library/MemoryAllocationLib.h>
#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncBsbParser.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncProtocol.h"
#include "Guest/BootSyncProtocolGuest.h"

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
  EFI_STATUS             Status;
  EFI_STATUS             Status2;
  SECURE_CHANNEL         SecChannel;
  BOOT_SYNC_BSB_HEADER   *BibHeader;
  BOOT_SYNC_BSB_ELEMENT  *BsbElement;

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

  Status = BootSyncPerformAttestation (&SecChannel);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Attestation Failed!, Status = %r\n", Status);
    goto exit_handler;
  }

  Print (L"Attestation Status = %r\n", Status);

  Status = BootSyncGetBib (&SecChannel, &BibHeader);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Boot Sync Failed!, Status = %r\n", Status);
    goto exit_handler;
  }

  Print (L"Boot Sync Status = %r\n", Status);

  // Retrive the BIB elements.
  Status = BsbGetElement (
             BibHeader,
             &gArmBootSyncVarData,
             &BsbElement
             );
  if (EFI_ERROR (Status)) {
    Print (
      L"Error: Boot Sync to retrieve Variable Store Data!, Status = %r\n",
      Status
      );
    goto exit_handler1;
  }

  DBG_DUMP_RAW (
    "Variable Store Data",
    (UINT8 *)(BsbElement + 1),
    (BsbElement->Header.Length - sizeof (BOOT_SYNC_BSB_ELEMENT))
    );

  Status = BsbGetElement (
             BibHeader,
             &gArmBootSyncSecretData,
             &BsbElement
             );
  if (EFI_ERROR (Status)) {
    Print (
      L"Error: Boot Sync to retrieve Disk Secret Key Data!, Status = %r\n",
      Status
      );
    goto exit_handler1;
  }

  DBG_DUMP_RAW (
    "Disk Secret Key Data",
    (UINT8 *)(BsbElement + 1),
    (BsbElement->Header.Length - sizeof (BOOT_SYNC_BSB_ELEMENT))
    );

  // Send the FIN message to indicate End of Transmission.
  Status = SendFin (&SecChannel, BOOT_SYNC_COMM_END_REASON_EOT);
  ASSERT_EFI_ERROR (Status);

exit_handler1:
  // Free the BIB
  FreePool (BibHeader);

exit_handler:
  Status2 = TerminateSecureChannel (&SecChannel);
  if (EFI_ERROR (Status2)) {
    Print (L"Error: Failed to Close Secure Session Status2 = %r\n", Status2);
  }

  return Status;
}
