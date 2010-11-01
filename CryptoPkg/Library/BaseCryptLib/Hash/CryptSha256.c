/** @file
  SHA-256 Digest Wrapper Implementation over OpenSSL.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include <Library/BaseCryptLib.h>
#include <openssl/sha.h>


/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 operations.

**/
UINTN
EFIAPI
Sha256GetContextSize (
  VOID
  )
{
  //
  // Retrieves OpenSSL SHA-256 Context Size
  //
  return (UINTN)(sizeof (SHA256_CTX));
}


/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then ASSERT().

  @param[in, out]  Sha256Context  Pointer to SHA-256 Context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha256Init (
  IN OUT  VOID  *Sha256Context
  )
{
  //
  // ASSERT if Sha256Context is NULL
  //
  ASSERT (Sha256Context != NULL);

  //
  // OpenSSL SHA-256 Context Initialization
  //
  return (BOOLEAN) (SHA256_Init ((SHA256_CTX *)Sha256Context));
}


/**
  Performs SHA-256 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Sha256Context is NULL, then ASSERT().

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength     Length of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  Invalid SHA-256 context. After Sha256Final function has been called, the
                 SHA-256 context cannot be reused.

**/
BOOLEAN
EFIAPI
Sha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  //
  // ASSERT if Sha256Context is NULL
  //
  ASSERT (Sha256Context != NULL);

  //
  // ASSERT if invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL) {
    ASSERT (DataLength == 0);
  }

  //
  // OpenSSL SHA-256 Hash Update
  //
  return (BOOLEAN) (SHA256_Update ((SHA256_CTX *)Sha256Context, Data, DataLength));
}


/**
  Completes SHA-256 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the SHA-256 context cannot be used again.

  If Sha256Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Sha256Context  Pointer to SHA-256 context
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-256 digest
                                  value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha256Final (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // ASSERT if Sha256Context is NULL or HashValue is NULL
  //
  ASSERT (Sha256Context != NULL);
  ASSERT (HashValue     != NULL);

  //
  // OpenSSL SHA-256 Hash Finalization
  //
  return (BOOLEAN) (SHA256_Final (HashValue, (SHA256_CTX *)Sha256Context));
}
