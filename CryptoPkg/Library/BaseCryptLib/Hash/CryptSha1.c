/** @file
  SHA-1 Digest Wrapper Implementation over OpenSSL.

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
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-1 hash operations.

**/
UINTN
EFIAPI
Sha1GetContextSize (
  VOID
  )
{
  //
  // Retrieves OpenSSL SHA Context Size
  //
  return (UINTN)(sizeof (SHA_CTX));
}


/**
  Initializes user-supplied memory pointed by Sha1Context as the SHA-1 hash context for
  subsequent use.

  If Sha1Context is NULL, then ASSERT().

  @param[in, out]  Sha1Context  Pointer to the SHA-1 Context being initialized.

  @retval TRUE   SHA-1 initialization succeeded.
  @retval FALSE  SHA-1 initialization failed.

**/
BOOLEAN
EFIAPI
Sha1Init (
  IN OUT  VOID  *Sha1Context
  )
{
  //
  // ASSERT if Sha1Context is NULL
  //
  ASSERT (Sha1Context != NULL);

  //
  // OpenSSL SHA-1 Context Initialization
  //
  return (BOOLEAN) (SHA1_Init ((SHA_CTX *)Sha1Context));
}


/**
  Performs SHA-1 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Sha1Context is NULL, then ASSERT().

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength   Length of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  Invalid SHA-1 context. After Sha1Final function has been called, the
                 SHA-1 context cannot be reused.

**/
BOOLEAN
EFIAPI
Sha1Update (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  //
  // ASSERT if Sha1Context is NULL
  //
  ASSERT (Sha1Context != NULL);

  //
  // ASSERT if invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL) {
    ASSERT (DataLength == 0);
  }

  //
  // OpenSSL SHA-1 Hash Update
  //
  return (BOOLEAN) (SHA1_Update ((SHA_CTX *)Sha1Context, Data, DataLength));
}


/**
  Completes SHA-1 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the SHA-1 context cannot be used again.

  If Sha1Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context
  @param[out]      HashValue    Pointer to a buffer that receives the SHA-1 digest
                                value (20 bytes).

  @retval TRUE   SHA-1 digest computation succeeded.
  @retval FALSE  SHA-1 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha1Final (
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // ASSERT if Sha1Context is NULL or HashValue is NULL
  //
  ASSERT (Sha1Context != NULL);
  ASSERT (HashValue   != NULL);

  //
  // OpenSSL SHA-1 Hash Finalization
  //
  return (BOOLEAN) (SHA1_Final (HashValue, (SHA_CTX *)Sha1Context));
}
