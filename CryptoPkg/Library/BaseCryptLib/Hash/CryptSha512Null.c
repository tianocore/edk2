/** @file
  SHA-384 and SHA-512 Digest Wrapper Implementations which does not provide real capabilities.

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-384 hash operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Sha384GetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory pointed by Sha384Context as SHA-384 hash context for
  subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Sha384Context  Pointer to SHA-384 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384Init (
  OUT  VOID  *Sha384Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing SHA-384 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Sha384Context     Pointer to SHA-384 context being copied.
  @param[out] NewSha384Context  Pointer to new SHA-384 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384Duplicate (
  IN   CONST VOID  *Sha384Context,
  OUT  VOID        *NewSha384Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates SHA-384 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384Update (
  IN OUT  VOID        *Sha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the SHA-384 digest value.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-384 digest
                                  value (48 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384Final (
  IN OUT  VOID   *Sha384Context,
  OUT     UINT8  *HashValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-512 hash operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Sha512GetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory pointed by Sha512Context as SHA-512 hash context for
  subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Sha512Context  Pointer to SHA-512 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512Init (
  OUT  VOID  *Sha512Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing SHA-512 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Sha512Context     Pointer to SHA-512 context being copied.
  @param[out] NewSha512Context  Pointer to new SHA-512 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512Duplicate (
  IN   CONST VOID  *Sha512Context,
  OUT  VOID        *NewSha512Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates SHA-512 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512Update (
  IN OUT  VOID        *Sha512Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the SHA-512 digest value.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-512 digest
                                  value (64 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512Final (
  IN OUT  VOID   *Sha512Context,
  OUT     UINT8  *HashValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}
