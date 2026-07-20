/** @file
  Arm CCA Boot Sync Crypto library.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - BS           - Boot Sync

  @par Reference(s):
   - Realm Host Interface (RHI) Specification, version 1.0-alp0
     (https://developer.arm.com/documentation/den0148/)
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/RngLib.h>

#include "ArmCcaBootSyncCrypto.h"

/**
  Generate a 256-bit random number as initial seed and
  prime the pseudo-random number generator.

  @retval EFI_ABORTED  Failed to generate seed.
  @retval EFI_SUCCESS  Success.
**/
STATIC
EFI_STATUS
EFIAPI
ArmCcaInitRandomNumberSeed (
  VOID
  )
{
  UINT64  RngSeed[4];

  ZeroMem (RngSeed, sizeof (RngSeed));
  // Generate a 256 bit random number as an inital seed.
  if (!GetRandomNumber128 (RngSeed)) {
    return EFI_ABORTED;
  }

  if (!GetRandomNumber128 (&RngSeed[2])) {
    return EFI_ABORTED;
  }

  // Prime the pseudo-random number generator with the initial seed.
  if (!RandomSeed ((UINT8 *)RngSeed, 32)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Generate an ECDH key using the ECC Curve-P384 and retrive the public key.

  @param[out]  Context          Pointer to the key handle.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncCryptoGenerateKey (
  OUT VOID  **Context
  )
{
  BOOLEAN     Result;
  EFI_STATUS  Status;

  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKey;
  VOID                          *EcContext;
  UINT8                         *PublicKey;
  UINTN                         PublicKeySize;
  UINTN                         EcCurveNid;

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BsKey = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)AllocateZeroPool (
                                            sizeof (ARM_CCA_BOOTSYNC_KEY_CONTEXT)
                                            );
  if (BsKey == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Arm CCA Security Model document section '12.3.1 Recommended parameter
  // sizes' recommends using ECC Curve-P384
  EcCurveNid = CRYPTO_NID_SECP384R1;

  EcContext = EcNewByNid (EcCurveNid);
  if (EcContext == NULL) {
    Status = EFI_ABORTED;
    goto exit_handler;
  }

  PublicKeySize = ECC_CURVE_P384_PUB_KEY_SIZE;
  PublicKey     = (UINT8 *)AllocateZeroPool (PublicKeySize);
  if (PublicKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit_handler1;
  }

  Result = EcGenerateKey (
             EcContext,
             PublicKey,
             &PublicKeySize
             );
  if (!Result) {
    Status = EFI_ABORTED;
    goto exit_handler2;
  }

  BsKey->EcContext     = EcContext;
  BsKey->PublicKey     = PublicKey;
  BsKey->PublicKeySize = PublicKeySize;
  BsKey->EcCurveNid    = EcCurveNid;

  *Context = BsKey;
  return EFI_SUCCESS;

exit_handler2:
  FreePool (PublicKey);
exit_handler1:
  EcFree (EcContext);
exit_handler:
  FreePool (BsKey);
  return Status;
}

/**
  Delete the key and associated data.

  @param[in]  Context           Pointer to the key handle.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncCryptoDeleteKey (
  VOID  *Context
  )
{
  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKey;

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BsKey = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)Context;

  if (BsKey->PublicKey != NULL) {
    ZeroMem (BsKey->PublicKey, BsKey->PublicKeySize);
    FreePool (BsKey->PublicKey);
  }

  if (BsKey->CommonKey != NULL) {
    ZeroMem (BsKey->CommonKey, BsKey->CommonKeySize);
    FreePool (BsKey->CommonKey);
  }

  if (BsKey->EcContext != NULL) {
    EcFree (BsKey->EcContext);
  }

  FreePool (BsKey);
  return EFI_SUCCESS;
}

/**
  Get the public key in PEM format.

  Note: The caller is responsible to free the PubKeyPem buffer
        by calling FreePool().

  @param[in]  Context           Pointer to the key handle.
  @param[out] PubKeyPem         Pointer to store the PEM data.
  @param[out] PubKeyPemSize     PEM data size.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncCryptoGetPublicKey (
  IN VOID    *Context,
  OUT UINT8  **PubKeyPem,
  OUT UINTN  *PubKeyPemSize
  )
{
  BOOLEAN     Result;
  EFI_STATUS  Status;

  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKey;
  UINT8                         *PemData;
  UINTN                         PemSize;

  if ((Context == NULL) || (PubKeyPem == NULL) || (PubKeyPemSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BsKey = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)Context;

  // First get the PEM size.
  PemData = NULL;
  PemSize = 0;
  Result  = EcPublicKeyToPEM (
              BsKey->EcContext,
              PemData,
              &PemSize
              );
  if ((!Result) && (PemSize == 0)) {
    return EFI_ABORTED;
  }

  PemData = AllocateZeroPool (PemSize);
  if (PemData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Result = EcPublicKeyToPEM (
             BsKey->EcContext,
             PemData,
             &PemSize
             );
  if (!Result) {
    Status = EFI_ABORTED;
    goto exit_handler;
  }

  *PubKeyPem     = PemData;
  *PubKeyPemSize = PemSize;
  return EFI_SUCCESS;

exit_handler:
  FreePool (PemData);
  return Status;
}

/**
  Get the peer key using the peer public PEM data.

  Note: The caller is responsible for freeing the peer key by calling
        ArmCcaBootSyncCryptoDeleteKey() for the PeerContext.

  @param[out] PeerContext         Pointer to the peer key handle.
  @param[in]  PeerPubKeyPem       Pointer to the peer PEM data.
  @param[in]  PeerPubKeyPemSize   PEM data size.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncCryptoGeneratePeerKey (
  OUT VOID  **PeerContext,
  IN UINT8  *PeerPubKeyPem,
  IN UINTN  PeerPubKeyPemSize
  )
{
  BOOLEAN     Result;
  EFI_STATUS  Status;

  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKey;
  VOID                          *EcContextRetrieved;
  UINT8                         *PublicKey;
  UINTN                         PublicKeySize;
  UINTN                         PeerKeyNid;

  if ((PeerContext == NULL) ||
      (PeerPubKeyPem == NULL) ||
      (PeerPubKeyPemSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  BsKey = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)AllocateZeroPool (
                                            sizeof (ARM_CCA_BOOTSYNC_KEY_CONTEXT)
                                            );
  if (BsKey == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Result = EcGetPublicKeyFromPem (
             PeerPubKeyPem,
             PeerPubKeyPemSize,
             NULL,
             &EcContextRetrieved
             );
  if (!Result) {
    Status = EFI_ABORTED;
    goto exit_handler;
  }

  Result = EcGetCurveNid (EcContextRetrieved, &PeerKeyNid);
  if ((!Result) || (PeerKeyNid != CRYPTO_NID_SECP384R1)) {
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler1;
  }

  PublicKeySize = ECC_CURVE_P384_PUB_KEY_SIZE;
  PublicKey     = AllocateZeroPool (PublicKeySize);
  if (PublicKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit_handler1;
  }

  Result = EcGetPubKey (
             EcContextRetrieved,
             PublicKey,
             &PublicKeySize
             );
  if (!Result) {
    Status = EFI_ABORTED;
    goto exit_handler2;
  }

  BsKey->EcContext     = EcContextRetrieved;
  BsKey->PublicKey     = PublicKey;
  BsKey->PublicKeySize = PublicKeySize;
  BsKey->EcCurveNid    = CRYPTO_NID_SECP384R1;

  *PeerContext = BsKey;
  return EFI_SUCCESS;

exit_handler2:
  FreePool (PublicKey);
exit_handler1:
  EcFree (EcContextRetrieved);
exit_handler:
  FreePool (BsKey);
  return Status;
}

/**
  Generate the common key.

  Note: The common key is freed when the caller frees the Context
        by calling ArmCcaBootSyncCryptoDeleteKey().

  @param[in]  Context           Pointer to the key handle.
  @param[in]  PeerContext       Pointer to the peer key handle.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncCryptoGenerateCommonKey (
  IN VOID  *Context,
  IN VOID  *PeerContext
  )
{
  BOOLEAN     Result;
  EFI_STATUS  Status;

  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKey;
  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKeyPeer;
  UINT8                         *CommKey;
  UINTN                         CommKeySize;

  if ((Context == NULL) ||
      (PeerContext == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  BsKey     = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)Context;
  BsKeyPeer = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)PeerContext;

  if ((BsKey->CommonKey != NULL) ||
      (BsKey->CommonKeySize != 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CommKeySize = ECC_CURVE_P384_PUB_KEY_SIZE;
  CommKey     = (UINT8 *)AllocateZeroPool (CommKeySize);
  if (CommKey == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Result = EcDhComputeKey (
             BsKey->EcContext,
             BsKeyPeer->PublicKey,
             BsKeyPeer->PublicKeySize,
             NULL,
             CommKey,
             &CommKeySize
             );
  if (!Result) {
    Status = EFI_ABORTED;
    ASSERT (0);
    goto exit_handler;
  }

  BsKey->CommonKey     = CommKey;
  BsKey->CommonKeySize = CommKeySize;
  return EFI_SUCCESS;

exit_handler:
  FreePool (CommKey);
  return Status;
}

/**
  Derive keys using SHA512 HMAC-based Extract-and-Expand Key
  Derivation Function (HKDF).

  @param[in]  Context           Pointer to the key handle.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Key derevation successful.
  @retval FALSE  Key derevation failed.
**/
BOOLEAN
EFIAPI
ArmCcaBootSyncCryptoDeriveKey (
  IN   VOID         *Context,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  ARM_CCA_BOOTSYNC_KEY_CONTEXT  *BsKey;

  if ((Context == NULL) ||
      (Salt == NULL) ||
      (Info == NULL) ||
      (Out == NULL))
  {
    return FALSE;
  }

  BsKey = (ARM_CCA_BOOTSYNC_KEY_CONTEXT *)Context;

  if ((BsKey->CommonKey == NULL) ||
      (BsKey->CommonKeySize == 0))
  {
    return FALSE;
  }

  // Derive SHA512 HMAC-based Extract-and-Expand Key Derivation Function (HKDF).
  return HkdfSha512ExtractAndExpand (
           BsKey->CommonKey,
           BsKey->CommonKeySize,
           Salt,
           SaltSize,
           Info,
           InfoSize,
           Out,
           OutSize
           );
}

/**
  Performs AEAD AES-GCM authenticated encryption on a data buffer and
  additional authenticated data (AAD).

  IvSize must be 12, otherwise FALSE is returned.
  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  TagSize must be 12, 13, 14, 15, 16, otherwise FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV value.
  @param[in]   IvSize      Size of the IV value in bytes.
  @param[in]   AData       Pointer to the additional authenticated data (AAD).
  @param[in]   ADataSize   Size of the additional authenticated data (AAD) in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be encrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[out]  TagOut      Pointer to a buffer that receives the authentication tag output.
  @param[in]   TagSize     Size of the authentication tag in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the encryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AEAD AES-GCM authenticated encryption succeeded.
  @retval FALSE  AEAD AES-GCM authenticated encryption failed.
**/
BOOLEAN
EFIAPI
ArmCcaBootSyncCryptoEncrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *AData,
  IN   UINTN        ADataSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  OUT  UINT8        *TagOut,
  IN   UINTN        TagSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  return AeadAesGcmEncrypt (
           Key,
           KeySize,
           Iv,
           IvSize,
           AData,
           ADataSize,
           DataIn,
           DataInSize,
           TagOut,
           TagSize,
           DataOut,
           DataOutSize
           );
}

/**
  Performs AEAD AES-GCM authenticated decryption on a data buffer and
  additional authenticated data (AAD).

  IvSize must be 12, otherwise FALSE is returned.
  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  TagSize must be 12, 13, 14, 15, 16, otherwise FALSE is returned.
  If additional authenticated data verification fails, FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV value.
  @param[in]   IvSize      Size of the IV value in bytes.
  @param[in]   AData       Pointer to the additional authenticated data (AAD).
  @param[in]   ADataSize   Size of the additional authenticated data (AAD) in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be decrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[in]   Tag         Pointer to a buffer that contains the authentication tag.
  @param[in]   TagSize     Size of the authentication tag in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the decryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AEAD AES-GCM authenticated decryption succeeded.
  @retval FALSE  AEAD AES-GCM authenticated decryption failed.
**/
BOOLEAN
EFIAPI
ArmCcaBootSyncCryptoDecrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *AData,
  IN   UINTN        ADataSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  IN   CONST UINT8  *Tag,
  IN   UINTN        TagSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  return AeadAesGcmDecrypt (
           Key,
           KeySize,
           Iv,
           IvSize,
           AData,
           ADataSize,
           DataIn,
           DataInSize,
           Tag,
           TagSize,
           DataOut,
           DataOutSize
           );
}

/**
  Perform initialisation required for cryptographic operations.

  Note: This API must be called once before any other APIs in this library
        are used.

  @retval EFI_ABORTED  Failed to generate seed.
  @retval EFI_SUCCESS  Success.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncCryptoInit (
  VOID
  )
{
  return ArmCcaInitRandomNumberSeed ();
}
