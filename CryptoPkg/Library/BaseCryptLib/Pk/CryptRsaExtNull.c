/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file does not provide real capabilities for following APIs in RSA handling:
  1) RsaGetKey
  2) RsaGenerateKey
  3) RsaCheckKey

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Gets the tag-designated RSA key component from the established RSA context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[out]      BigNumber   Pointer to octet integer buffer.
  @param[in, out]  BnSize      On input, the size of big number buffer in bytes.
                               On output, the size of data returned in big number buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  OUT     UINT8        *BigNumber,
  IN OUT  UINTN        *BnSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
