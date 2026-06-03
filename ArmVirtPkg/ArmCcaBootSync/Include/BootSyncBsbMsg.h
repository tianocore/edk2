/** @file
  Interfaces to send/receive encrypted messages over a Secure Channel.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "Include/BootSyncSecureChannel.h"

/**
  Encrypt and send a BSB message over the secure channel.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Bsb           Pointer to the BSB message to send.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SendEncryptBsbMessage (
  IN SECURE_CHANNEL        *SecChannel,
  IN BOOT_SYNC_BSB_HEADER  *Bsb
  );

/**
  Receive and decrypt a BSB message.

  @param[in]  SecChannel  Pointer to the secure channel.
  @param[out] Message     Pointer to the received message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error was detected, resulting in
                                  the connection being terminated.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ReceiveDecryptBsbMsg (
  IN SECURE_CHANNEL        *SecChannel,
  OUT BOOT_SYNC_GUID_BLOB  **Message
  );
