/** @file
  MD5 Digest Wrapper Implementation which does not provide real capabilities.

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
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Md5GetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Md5Context  Pointer to MD5 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Init (
  OUT  VOID  *Md5Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing MD5 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Md5Context     Pointer to MD5 context being copied.
  @param[out] NewMd5Context  Pointer to new MD5 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Duplicate (
  IN   CONST VOID  *Md5Context,
  OUT  VOID        *NewMd5Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates MD5 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the MD5 digest value.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD5 digest
                               value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Final (
  IN OUT  VOID   *Md5Context,
  OUT     UINT8  *HashValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}
