/** @file
  MD5 Digest Wrapper Implementation over OpenSSL.

Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
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
  return (UINTN) (sizeof (MD5_CTX));
}


/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  If Md5Context is NULL, then return FALSE.

  @param[out]  Md5Context  Pointer to MD5 context being initialized.

  @retval TRUE   MD5 context initialization succeeded.
  @retval FALSE  MD5 context initialization failed.

**/
BOOLEAN
EFIAPI
Md5Init (
  OUT  VOID  *Md5Context
  )
{
  //
  // Check input parameters.
  //
  if (Md5Context == NULL) {
    return FALSE;
  }

  //
  // OpenSSL MD5 Context Initialization
  //
  return (BOOLEAN) (MD5_Init ((MD5_CTX *) Md5Context));
}

/**
  Makes a copy of an existing MD5 context.

  If Md5Context is NULL, then return FALSE.
  If NewMd5Context is NULL, then return FALSE.

  @param[in]  Md5Context     Pointer to MD5 context being copied.
  @param[out] NewMd5Context  Pointer to new MD5 context.

  @retval TRUE   MD5 context copy succeeded.
  @retval FALSE  MD5 context copy failed.

**/
BOOLEAN
EFIAPI
Md5Duplicate (
  IN   CONST VOID  *Md5Context,
  OUT  VOID        *NewMd5Context
  )
{
  //
  // Check input parameters.
  //
  if (Md5Context == NULL || NewMd5Context == NULL) {
    return FALSE;
  }

  CopyMem (NewMd5Context, Md5Context, sizeof (MD5_CTX));

  return TRUE;
}

/**
  Digests the input data and updates MD5 context.

  This function performs MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD5 context should be already correctly initialized by Md5Init(), and should not be finalized
  by Md5Final(). Behavior with invalid context is undefined.

  If Md5Context is NULL, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD5 data digest succeeded.
  @retval FALSE  MD5 data digest failed.

**/
BOOLEAN
EFIAPI
Md5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (Md5Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL && (DataSize != 0)) {
    return FALSE;
  }

  //
  // OpenSSL MD5 Hash Update
  //
  return (BOOLEAN) (MD5_Update ((MD5_CTX *) Md5Context, Data, DataSize));
}

/**
  Completes computation of the MD5 digest value.

  This function completes MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD5 context cannot
  be used again.
  MD5 context should be already correctly initialized by Md5Init(), and should not be
  finalized by Md5Final(). Behavior with invalid MD5 context is undefined.

  If Md5Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
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
  // Check input parameters.
  //
  if (Md5Context == NULL || HashValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL MD5 Hash Finalization
  //
  return (BOOLEAN) (MD5_Final (HashValue, (MD5_CTX *) Md5Context));
}

/**
  Computes the MD5 message digest of a input data buffer.

  This function performs the MD5 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD5 digest
                           value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  //
  // Check input parameters.
  //
  if (HashValue == NULL) {
    return FALSE;
  }
  if (Data == NULL && (DataSize != 0)) {
    return FALSE;
  }

  //
  // OpenSSL MD5 Hash Computation.
  //
  if (MD5 (Data, DataSize, HashValue) == NULL) {
    return FALSE;
  } else {
    return TRUE;
  }
}
