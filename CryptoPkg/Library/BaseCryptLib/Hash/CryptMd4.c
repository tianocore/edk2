/** @file
  MD4 Digest Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"
#include <openssl/md4.h>

/**
  Retrieves the size, in bytes, of the context buffer required for MD4 hash operations.

  @return  The size, in bytes, of the context buffer required for MD4 hash operations.

**/
UINTN
EFIAPI
Md4GetContextSize (
  VOID
  )
{
  //
  // Retrieves the OpenSSL MD4 Context Size
  //
  return (UINTN) (sizeof (MD4_CTX));
}

/**
  Initializes user-supplied memory pointed by Md4Context as MD4 hash context for
  subsequent use.

  If Md4Context is NULL, then return FALSE.

  @param[out]  Md4Context  Pointer to MD4 context being initialized.

  @retval TRUE   MD4 context initialization succeeded.
  @retval FALSE  MD4 context initialization failed.

**/
BOOLEAN
EFIAPI
Md4Init (
  OUT  VOID  *Md4Context
  )
{
  //
  // Check input parameters.
  //
  if (Md4Context == NULL) {
    return FALSE;
  }

  //
  // OpenSSL MD4 Context Initialization
  //
  return (BOOLEAN) (MD4_Init ((MD4_CTX *) Md4Context));
}

/**
  Makes a copy of an existing MD4 context.

  If Md4Context is NULL, then return FALSE.
  If NewMd4Context is NULL, then return FALSE.

  @param[in]  Md4Context     Pointer to MD4 context being copied.
  @param[out] NewMd4Context  Pointer to new MD4 context.

  @retval TRUE   MD4 context copy succeeded.
  @retval FALSE  MD4 context copy failed.

**/
BOOLEAN
EFIAPI
Md4Duplicate (
  IN   CONST VOID  *Md4Context,
  OUT  VOID        *NewMd4Context
  )
{
  //
  // Check input parameters.
  //
  if (Md4Context == NULL || NewMd4Context == NULL) {
    return FALSE;
  }

  CopyMem (NewMd4Context, Md4Context, sizeof (MD4_CTX));

  return TRUE;
}

/**
  Digests the input data and updates MD4 context.

  This function performs MD4 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD4 context should be already correctly intialized by Md4Init(), and should not be finalized
  by Md4Final(). Behavior with invalid context is undefined.

  If Md4Context is NULL, then return FALSE.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD4 data digest succeeded.
  @retval FALSE  MD4 data digest failed.

**/
BOOLEAN
EFIAPI
Md4Update (
  IN OUT  VOID        *Md4Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (Md4Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL MD4 Hash Update
  //
  return (BOOLEAN) (MD4_Update ((MD4_CTX *) Md4Context, Data, DataSize));
}

/**
  Completes computation of the MD4 digest value.

  This function completes MD4 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD4 context cannot
  be used again.
  MD4 context should be already correctly intialized by Md4Init(), and should not be
  finalized by Md4Final(). Behavior with invalid MD4 context is undefined.

  If Md4Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD4 digest
                               value (16 bytes).

  @retval TRUE   MD4 digest computation succeeded.
  @retval FALSE  MD4 digest computation failed.

**/
BOOLEAN
EFIAPI
Md4Final (
  IN OUT  VOID   *Md4Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // Check input parameters.
  //
  if (Md4Context == NULL || HashValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL MD4 Hash Finalization
  //
  return (BOOLEAN) (MD4_Final (HashValue, (MD4_CTX *) Md4Context));
}
