/** @file
  Boot Sync Crypto definitions.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - BS           - Boot Sync

  @par Reference(s):
   - Realm Host Interface (RHI) Specification, version 1.0-alp0
     (https://developer.arm.com/documentation/den0148/)
**/

#pragma once

/**
  A structure for storing the cryptographic key context and associated
  parameters and data.
*/
typedef struct ArmCcaBootSyncKeyContext {
  /// Pointer to the key context.
  VOID     *EcContext;
  /// Cryptographic curve Nid.
  UINTN    EcCurveNid;
  /// Pointer to the public key.
  UINT8    *PublicKey;
  /// Public key size
  UINTN    PublicKeySize;
  /// Pointer to the Common key.
  UINT8    *CommonKey;
  /// Common key size.
  UINTN    CommonKeySize;
} ARM_CCA_BOOTSYNC_KEY_CONTEXT;

// For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
#define ECC_CURVE_P384_PUB_KEY_SIZE  96
