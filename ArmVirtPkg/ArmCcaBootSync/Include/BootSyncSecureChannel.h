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
  A macro defining the size of the rolling hash of the app info for the
  derived key(s) and the messages in bytes.
*/
#define ROLLING_MSG_HASH_SIZE  32

/**
  A macro defining the binding key size in bytes.
*/
#define BINDING_KEY_SIZE  (ARM_CCA_MAX_CHALLENGE_DATA_SIZE_BITS >> 3)

/**
  A macro defining the encryption key size in bytes.
*/
#define ENCRYPTION_KEY_SIZE  AES_KEY_SIZE

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

  /// Rolling hash of the messages.
  UINT8                     RmHash[ROLLING_MSG_HASH_SIZE];

  /// Binding Key Salt.
  UINT8                     SaltKeyBinding[SALT_SIZE];

  /// Encryption Key Salt.
  UINT8                     SaltKeyEncryption[SALT_SIZE];

  /// Binding Key.
  UINT8                     Kb[BINDING_KEY_SIZE];

  /// Encryption Key.
  UINT8                     Ke[ENCRYPTION_KEY_SIZE];
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
  Derive Binding Key and Encryption Key from the Common Key.

  @param[in] SecChannel         Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelDeriveKeys (
  IN SECURE_CHANNEL  *SecChannel
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
