/** @file
  Boot Sync Crypto Lib.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - BS           - Boot Sync

  @par Reference(s):
   - Realm Host Interface (RHI) Specification, version 1.0-alp0
     (https://developer.arm.com/documentation/den0148/)
**/

#pragma once

#include <Uefi/UefiBaseType.h>

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
  );

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
  );

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
  );

/**
  Get the peer key using the peer public PEM data.

  Note: The caller is responsible for freeing the peer key by calling
        ArmCcaBootSyncCryptoDeleteKey() for the PeerContext.

  @param[out] Context             Pointer to the peer key handle.
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
  );

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
  );

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
  );

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
  );

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
  );

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
  );
