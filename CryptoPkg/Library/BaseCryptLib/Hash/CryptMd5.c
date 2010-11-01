/** @file
  MD5 Digest Wrapper Implementation over OpenSSL.

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
#include <openssl/md5.h>


/**
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  @return  The size, in bytes, of the context buffer required for MD5 hash operations.

**/
UINTN
EFIAPI
Md5GetContextSize (
  VOID
  )
{
  //
  // Retrieves the OpenSSL MD5 Context Size
  //
  return (UINTN)(sizeof (MD5_CTX));
}


/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  If Md5Context is NULL, then ASSERT().

  @param[in, out]  Md5Context  Pointer to MD5 Context being initialized.

  @retval TRUE   MD5 context initialization succeeded.
  @retval FALSE  MD5 context initialization failed.

**/
BOOLEAN
EFIAPI
Md5Init (
  IN OUT  VOID  *Md5Context
  )
{
  //
  // ASSERT if Md5Context is NULL.
  //
  ASSERT (Md5Context != NULL);

  //
  // OpenSSL MD5 Context Initialization
  //
  return (BOOLEAN) (MD5_Init ((MD5_CTX *)Md5Context));
}


/**
  Performs MD5 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Md5Context is NULL, then ASSERT().

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength  Length of Data buffer in bytes.

  @retval TRUE   MD5 data digest succeeded.
  @retval FALSE  Invalid MD5 context. After Md5Final function has been called, the
                 MD5 context cannot be reused.

**/
BOOLEAN
EFIAPI
Md5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  //
  // ASSERT if Md5Context is NULL
  //
  ASSERT (Md5Context != NULL);

  //
  // ASSERT if invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL) {
    ASSERT (DataLength == 0);
  }

  //
  // OpenSSL MD5 Hash Update
  //
  return (BOOLEAN) (MD5_Update ((MD5_CTX *)Md5Context, Data, DataLength));
}


/**
  Completes MD5 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the MD5 context cannot be used again.

  If Md5Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Md5Context  Pointer to the MD5 context
  @param[out]      HashValue   Pointer to a buffer that receives the MD5 digest
                               value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.

**/
BOOLEAN
EFIAPI
Md5Final (
  IN OUT  VOID   *Md5Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // ASSERT if Md5Context is NULL or HashValue is NULL
  //
  ASSERT (Md5Context != NULL);
  ASSERT (HashValue  != NULL);

  //
  // OpenSSL MD5 Hash Finalization
  //
  return (BOOLEAN) (MD5_Final (HashValue, (MD5_CTX *)Md5Context));
}
