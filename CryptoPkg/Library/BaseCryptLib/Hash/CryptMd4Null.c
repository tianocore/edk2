/** @file
  MD4 Digest Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

/**
  Retrieves the size, in bytes, of the context buffer required for MD4 hash
  operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Md4GetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory pointed by Md4Context as MD4 hash context for
  subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Md4Context  Pointer to MD4 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Init (
  OUT  VOID  *Md4Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing MD4 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Md4Context     Pointer to MD4 context being copied.
  @param[out] NewMd4Context  Pointer to new MD4 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Duplicate (
  IN   CONST VOID  *Md4Context,
  OUT  VOID        *NewMd4Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates MD4 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Update (
  IN OUT  VOID        *Md4Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the MD4 digest value.

  Return FALSE to indicate this interface is not supported.  

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD4 digest
                               value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Final (
  IN OUT  VOID   *Md4Context,
  OUT     UINT8  *HashValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Computes the MD4 message digest of a input data buffer.

  Return FALSE to indicate this interface is not supported.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD4 digest
                           value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}
