/** @file
  Arm CCA Boot Sync Secure Channel internal definitions and interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmCcaRhiHostSessionLib.h>
#include "Include/BootSyncProtocolDefines.h"

/**
  A structure for storing the data relevant for the Secure session.
*/
typedef struct SecureChannel {
  /// RHI Session Connection Mode
  UINT64                    ConnectionMode;

  /// RHI Host Session Id
  ARM_CCA_RHI_SESSION_ID    SessionId;

  /// Pointer to the BSKEY_CONTEXT
  VOID                      *SessionCtx;

  /// Pointer to the peer BSKEY_CONTEXT
  VOID                      *PeerSessionCtx;
} SECURE_CHANNEL;

/**
  Compute the common key using the peer public key PEM data.

  Note: Common key is freed when SecChannel is deleted.

  @param[in] SecChannel         Pointer to the secure channel.
  @param[in] PeerPubKeyPem      Pointer to the peer public key PEM data.
  @param[in] PeerPubKeyPemSize  Length of the peer public key PEM data.


  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelComputeCommonKey (
  IN  SECURE_CHANNEL  *SecChannel,
  IN  UINT8           *PeerPubKeyPem,
  IN  UINTN           PeerPubKeyPemSize
  );

/**
  Delete the secure channel keys.

  @param[in]    SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelDeleteKeys (
  IN SECURE_CHANNEL  *SecChannel
  );
