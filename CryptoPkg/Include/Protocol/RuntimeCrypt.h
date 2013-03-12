/** @file
  The runtime cryptographic protocol.
  Only limited crypto primitives (SHA-256 and RSA) are provided for runtime
  authenticated variable service.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_RUNTIME_CRYPT_PROTOCOL_H__
#define __EFI_RUNTIME_CRYPT_PROTOCOL_H__

#include <Library/BaseCryptLib.h>

///
/// Runtime Cryptographic Protocol GUID.
///
#define EFI_RUNTIME_CRYPT_PROTOCOL_GUID \
  { \
    0xe1475e0c, 0x1746, 0x4802, { 0x86, 0x2e, 0x1, 0x1c, 0x2c, 0x2d, 0x9d, 0x86 } \
  }

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 operations.

**/
typedef
UINTN
(EFIAPI *EFI_RUNTIME_CRYPT_SHA256_GET_CONTEXT_SIZE) (
  VOID
  );


/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to SHA-256 Context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *EFI_RUNTIME_CRYPT_SHA256_INIT) (
  IN OUT  VOID  *Sha256Context
  );


/**
  Performs SHA-256 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength     Length of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  Invalid SHA-256 context. After Sha256Final function has been called, the
                 SHA-256 context cannot be reused.

**/
typedef
BOOLEAN
(EFIAPI *EFI_RUNTIME_CRYPT_SHA256_UPDATE) (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );


/**
  Completes SHA-256 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the SHA-256 context cannot be used again.

  If Sha256Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to SHA-256 context
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-256 digest
                                  value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.

**/
typedef
BOOLEAN
(EFIAPI *EFI_RUNTIME_CRYPT_SHA256_FINAL) (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  );


/**
  Allocates and Initializes one RSA Context for subsequent use.

  @return  Pointer to the RSA Context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
typedef
VOID *
(EFIAPI *EFI_RUNTIME_CRYPT_RSA_NEW) (
  VOID
  );

/**
  Release the specified RSA Context.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
typedef
VOID
(EFIAPI *EFI_RUNTIME_CRYPT_RSA_FREE) (
  IN  VOID  *RsaContext
  );

/**
  Sets the tag-designated RSA key component into the established RSA context from
  the user-specified nonnegative integer (octet string format represented in RSA
  PKCS#1).

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
  @param[in]       BnLength    Length of big number buffer in bytes.

  @return  TRUE   RSA key component was set successfully.
  @return  FALSE  Invalid RSA key component tag.

**/
typedef
BOOLEAN
(EFIAPI *EFI_RUNTIME_CRYPT_RSA_SET_KEY) (
  IN OUT VOID         *RsaContext,
  IN     RSA_KEY_TAG  KeyTag,
  IN     CONST UINT8  *BigNumber,
  IN     UINTN        BnLength
  );

/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashLength is not equal to the size of MD5, SHA-1 or SHA-256 digest, then return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashLength   Length of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigLength    Length of signature in bytes.

  @return  TRUE   Valid signature encoded in PKCS1-v1_5.
  @return  FALSE  Invalid signature or invalid RSA context.

**/
typedef
BOOLEAN
(EFIAPI *EFI_RUNTIME_CRYPT_RSA_PKCS1_VERIFY) (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashLength,
  IN  UINT8        *Signature,
  IN  UINTN        SigLength
  );

///
/// Runtime Cryptographic Protocol Structure.
///
typedef struct {
  EFI_RUNTIME_CRYPT_SHA256_GET_CONTEXT_SIZE  Sha256GetContextSize;
  EFI_RUNTIME_CRYPT_SHA256_INIT              Sha256Init;
  EFI_RUNTIME_CRYPT_SHA256_UPDATE            Sha256Update;
  EFI_RUNTIME_CRYPT_SHA256_FINAL             Sha256Final;
  EFI_RUNTIME_CRYPT_RSA_NEW                  RsaNew;
  EFI_RUNTIME_CRYPT_RSA_FREE                 RsaFree;
  EFI_RUNTIME_CRYPT_RSA_SET_KEY              RsaSetKey;
  EFI_RUNTIME_CRYPT_RSA_PKCS1_VERIFY         RsaPkcs1Verify;
} EFI_RUNTIME_CRYPT_PROTOCOL;

extern EFI_GUID gEfiRuntimeCryptProtocolGuid;

#endif
