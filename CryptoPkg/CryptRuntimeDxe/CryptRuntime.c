/** @file
  Runtime Cryptographic Driver Implementation, which produce one crypto
  protocol.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CryptRuntime.h"

//
// The handle onto which the Runtime Crypt Protocol instance is installed
//
EFI_HANDLE  mRuntimeCryptHandle = NULL;

//
// The Runtime Crypt Protocol instance produced by this driver
//
EFI_RUNTIME_CRYPT_PROTOCOL  mRuntimeCryptProtocol = {
  RuntimeCryptSha256GetContextSize,
  RuntimeCryptSha256Init,
  RuntimeCryptSha256Update,
  RuntimeCryptSha256Final,
  RuntimeCryptRsaNew,
  RuntimeCryptRsaFree,
  RuntimeCryptRsaSetKey,
  RuntimeCryptRsaPkcs1Verify
};

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 operations.

**/
UINTN
EFIAPI
RuntimeCryptSha256GetContextSize (
  VOID
  )
{
  return Sha256GetContextSize ();
}

/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to SHA-256 Context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
RuntimeCryptSha256Init (
  IN OUT  VOID  *Sha256Context
  )
{
  return Sha256Init (Sha256Context);
}

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
BOOLEAN
EFIAPI
RuntimeCryptSha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  return Sha256Update (Sha256Context, Data, DataLength);
}

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
BOOLEAN
EFIAPI
RuntimeCryptSha256Final (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  )
{
  return Sha256Final (Sha256Context, HashValue);
}

/**
  Allocates and Initializes one RSA Context for subsequent use.

  @return  Pointer to the RSA Context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
RuntimeCryptRsaNew (
  VOID
  )
{
  return RsaNew ();
}

/**
  Release the specified RSA Context.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
RuntimeCryptRsaFree (
  IN  VOID  *RsaContext
  )
{
  RsaFree (RsaContext);
}

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
BOOLEAN
EFIAPI
RuntimeCryptRsaSetKey (
  IN OUT VOID         *RsaContext,
  IN     RSA_KEY_TAG  KeyTag,
  IN     CONST UINT8  *BigNumber,
  IN     UINTN        BnLength
  )
{
  return RsaSetKey (RsaContext, KeyTag, BigNumber, BnLength);
}

/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashLength is not equal to the size of MD5, SHA-1 or SHA-256 digest, return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashLength   Length of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigLength    Length of signature in bytes.

  @return  TRUE   Valid signature encoded in PKCS1-v1_5.
  @return  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RuntimeCryptRsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashLength,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigLength
  )
{
  return RsaPkcs1Verify (RsaContext, MessageHash, HashLength, Signature, SigLength);
}

/**
  Entry Point for Runtime Cryptographic Driver.

  This function installs Runtime Crypt Protocol.

  @param ImageHandle     Image handle of this driver.
  @param SystemTable     a Pointer to the EFI System Table.

  @retval  EFI_SUCEESS  Runtime Crypt Protocol is successfully installed
  @return  Others       Some error occurs when installing Runtime Crypt Protocol.

**/
EFI_STATUS
EFIAPI
CryptRuntimeDriverInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install the Runtime Crypt Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mRuntimeCryptHandle,
                  &gEfiRuntimeCryptProtocolGuid,
                  &mRuntimeCryptProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
