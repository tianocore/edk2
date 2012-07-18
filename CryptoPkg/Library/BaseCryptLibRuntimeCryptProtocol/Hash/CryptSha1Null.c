/** @file
  SHA-1 Digest Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"


/**
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  Return zero to indicate this interface is not supported.

  @return  The size, in bytes, of the context buffer required for SHA-1 hash operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Sha1GetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;  
}

/**
  Initializes user-supplied memory pointed by Sha1Context as SHA-1 hash context for
  subsequent use.

  Return FALSE to indicate this interface is not supported.
  
  @param[out]  Sha1Context  Pointer to SHA-1 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Init (
  OUT  VOID  *Sha1Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing SHA-1 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Sha1Context     Pointer to SHA-1 context being copied.
  @param[out] NewSha1Context  Pointer to new SHA-1 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Duplicate (
  IN   CONST VOID  *Sha1Context,
  OUT  VOID        *NewSha1Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates SHA-1 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Update (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the SHA-1 digest value.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[out]      HashValue    Pointer to a buffer that receives the SHA-1 digest
                                value (20 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Final (
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}
