/** @file
  SHA-384 and SHA-512 Digest Wrapper Implementations over OpenSSL.

Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/sha.h>

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-384 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-384 hash operations.

**/
UINTN
EFIAPI
Sha384GetContextSize (
  VOID
  )
{
  //
  // Retrieves OpenSSL SHA-384 Context Size
  //
  return (UINTN) (sizeof (SHA512_CTX));
}

/**
  Initializes user-supplied memory pointed by Sha384Context as SHA-384 hash context for
  subsequent use.

  If Sha384Context is NULL, then return FALSE.

  @param[out]  Sha384Context  Pointer to SHA-384 context being initialized.

  @retval TRUE   SHA-384 context initialization succeeded.
  @retval FALSE  SHA-384 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha384Init (
  OUT  VOID  *Sha384Context
  )
{
  //
  // Check input parameters.
  //
  if (Sha384Context == NULL) {
    return FALSE;
  }

  //
  // OpenSSL SHA-384 Context Initialization
  //
  return (BOOLEAN) (SHA384_Init ((SHA512_CTX *) Sha384Context));
}

/**
  Makes a copy of an existing SHA-384 context.

  If Sha384Context is NULL, then return FALSE.
  If NewSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha384Context     Pointer to SHA-384 context being copied.
  @param[out] NewSha384Context  Pointer to new SHA-384 context.

  @retval TRUE   SHA-384 context copy succeeded.
  @retval FALSE  SHA-384 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384Duplicate (
  IN   CONST VOID  *Sha384Context,
  OUT  VOID        *NewSha384Context
  )
{
  //
  // Check input parameters.
  //
  if (Sha384Context == NULL || NewSha384Context == NULL) {
    return FALSE;
  }

  CopyMem (NewSha384Context, Sha384Context, sizeof (SHA512_CTX));

  return TRUE;
}

/**
  Digests the input data and updates SHA-384 context.

  This function performs SHA-384 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-384 context should be already correctly initialized by Sha384Init(), and should not be finalized
  by Sha384Final(). Behavior with invalid context is undefined.

  If Sha384Context is NULL, then return FALSE.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-384 data digest succeeded.
  @retval FALSE  SHA-384 data digest failed.

**/
BOOLEAN
EFIAPI
Sha384Update (
  IN OUT  VOID        *Sha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (Sha384Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL SHA-384 Hash Update
  //
  return (BOOLEAN) (SHA384_Update ((SHA512_CTX *) Sha384Context, Data, DataSize));
}

/**
  Completes computation of the SHA-384 digest value.

  This function completes SHA-384 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-384 context cannot
  be used again.
  SHA-384 context should be already correctly initialized by Sha384Init(), and should not be
  finalized by Sha384Final(). Behavior with invalid SHA-384 context is undefined.

  If Sha384Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-384 digest
                                  value (48 bytes).

  @retval TRUE   SHA-384 digest computation succeeded.
  @retval FALSE  SHA-384 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha384Final (
  IN OUT  VOID   *Sha384Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // Check input parameters.
  //
  if (Sha384Context == NULL || HashValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL SHA-384 Hash Finalization
  //
  return (BOOLEAN) (SHA384_Final (HashValue, (SHA512_CTX *) Sha384Context));
}

/**
  Computes the SHA-384 message digest of a input data buffer.

  This function performs the SHA-384 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-384 digest
                           value (48 bytes).

  @retval TRUE   SHA-384 digest computation succeeded.
  @retval FALSE  SHA-384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384HashAll (
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
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL SHA-384 Hash Computation.
  //
  if (SHA384 (Data, DataSize, HashValue) == NULL) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-512 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-512 hash operations.

**/
UINTN
EFIAPI
Sha512GetContextSize (
  VOID
  )
{
  //
  // Retrieves OpenSSL SHA-512 Context Size
  //
  return (UINTN) (sizeof (SHA512_CTX));
}

/**
  Initializes user-supplied memory pointed by Sha512Context as SHA-512 hash context for
  subsequent use.

  If Sha512Context is NULL, then return FALSE.

  @param[out]  Sha512Context  Pointer to SHA-512 context being initialized.

  @retval TRUE   SHA-512 context initialization succeeded.
  @retval FALSE  SHA-512 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha512Init (
  OUT  VOID  *Sha512Context
  )
{
  //
  // Check input parameters.
  //
  if (Sha512Context == NULL) {
    return FALSE;
  }

  //
  // OpenSSL SHA-512 Context Initialization
  //
  return (BOOLEAN) (SHA512_Init ((SHA512_CTX *) Sha512Context));
}

/**
  Makes a copy of an existing SHA-512 context.

  If Sha512Context is NULL, then return FALSE.
  If NewSha512Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha512Context     Pointer to SHA-512 context being copied.
  @param[out] NewSha512Context  Pointer to new SHA-512 context.

  @retval TRUE   SHA-512 context copy succeeded.
  @retval FALSE  SHA-512 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512Duplicate (
  IN   CONST VOID  *Sha512Context,
  OUT  VOID        *NewSha512Context
  )
{
  //
  // Check input parameters.
  //
  if (Sha512Context == NULL || NewSha512Context == NULL) {
    return FALSE;
  }

  CopyMem (NewSha512Context, Sha512Context, sizeof (SHA512_CTX));

  return TRUE;
}

/**
  Digests the input data and updates SHA-512 context.

  This function performs SHA-512 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-512 context should be already correctly initialized by Sha512Init(), and should not be finalized
  by Sha512Final(). Behavior with invalid context is undefined.

  If Sha512Context is NULL, then return FALSE.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-512 data digest succeeded.
  @retval FALSE  SHA-512 data digest failed.

**/
BOOLEAN
EFIAPI
Sha512Update (
  IN OUT  VOID        *Sha512Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (Sha512Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL SHA-512 Hash Update
  //
  return (BOOLEAN) (SHA512_Update ((SHA512_CTX *) Sha512Context, Data, DataSize));
}

/**
  Completes computation of the SHA-512 digest value.

  This function completes SHA-512 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-512 context cannot
  be used again.
  SHA-512 context should be already correctly initialized by Sha512Init(), and should not be
  finalized by Sha512Final(). Behavior with invalid SHA-512 context is undefined.

  If Sha512Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-512 digest
                                  value (64 bytes).

  @retval TRUE   SHA-512 digest computation succeeded.
  @retval FALSE  SHA-512 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha512Final (
  IN OUT  VOID   *Sha512Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // Check input parameters.
  //
  if (Sha512Context == NULL || HashValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL SHA-512 Hash Finalization
  //
  return (BOOLEAN) (SHA384_Final (HashValue, (SHA512_CTX *) Sha512Context));
}

/**
  Computes the SHA-512 message digest of a input data buffer.

  This function performs the SHA-512 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-512 digest
                           value (64 bytes).

  @retval TRUE   SHA-512 digest computation succeeded.
  @retval FALSE  SHA-512 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512HashAll (
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
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL SHA-512 Hash Computation.
  //
  if (SHA512 (Data, DataSize, HashValue) == NULL) {
    return FALSE;
  } else {
    return TRUE;
  }
}
