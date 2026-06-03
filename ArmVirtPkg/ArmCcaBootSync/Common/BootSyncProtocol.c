/** @file
  Arm CCA Boot Sync Protocol interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/**
  Boot Sync Key Exchange protocol

           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                |<---------Key Xchg Request-----------|
                |                                     |
                |                                     |
                |---------Key Xchg Response---------->|
  Compute       |                                     |     Compute
  Common        |                                     |     Common
  Key (Kcomm)   |                                     |     Key (Kcomm)
                |                                     |
  Derive        |                                     |     Derive
  Keys (Ke, Kb) |                                     |     Keys (Ke, Kb)
                |                                     |
                /            Other Protocol           \
                \            Communication            /
                |                                     |
                |<----------------FIN-----------------|
  End of        |                                     |     End of
  Communication |                                     |     Communication

                                  OR

                .                                     .
                .                                     .
                .                                     .
                /                                     \
                \                                     /
                |                                     |
                |----------------NACK---------------->|
  End of        |                                     |     End of
  Communication |                                     |     Communication
*/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncSecureChannel.h"

/**
  Send a FIN or NACK message.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Fin           If TRUE send a FIN message or else
                            send a NACK message.
  @param[in]  Reason        Reason code for the FIN/NACK message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
SendFinNack (
  IN SECURE_CHANNEL  *SecChannel,
  IN BOOLEAN         Fin,
  IN UINT64          Reason
  )
{
  EFI_STATUS     Status;
  UINTN          PayloadSize;
  BOOT_SYNC_FIN  *Req;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Allocate memory for the Request message.
  PayloadSize = sizeof (BOOT_SYNC_FIN);
  Req         = AllocateZeroPool (PayloadSize);
  if (Req == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Step 2: Poulate the Bib Request message header.
  if (Fin) {
    CopyGuid (&Req->Header.Name, &gArmBootSyncFinGuid);
  } else {
    CopyGuid (&Req->Header.Name, &gArmBootSyncNackGuid);
  }

  Req->Header.Length = PayloadSize;
  Req->Reason        = Reason;

  // Step 3: Send message.
  Status = SecureChannelSendMessage (SecChannel, (BOOT_SYNC_GUID_BLOB *)Req);
  ASSERT_EFI_ERROR (Status);

  // Step 4: The messgae is sent so free the associated memory.
  FreePool (Req);
  return Status;
}

/**
  Send FIN request message.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Reason        Reason code for the FIN message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SendFin (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT64          Reason
  )
{
  return SendFinNack (SecChannel, TRUE, Reason);
}

/**
  Send a negetive acknowlegdement (NACK) message.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Reason        Reason code for the NACK message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SendNack (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT64          Reason
  )
{
  return SendFinNack (SecChannel, FALSE, Reason);
}
