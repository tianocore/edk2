/** @file
  Null implementation of CAMELLIA functions called by BaseCryptLib.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#undef EDK2_OPENSSL_NOEC

#include <openssl/camellia.h>

int
Camellia_set_key (
  const unsigned char  *userKey,
  const int            bits,
  CAMELLIA_KEY         *key
  )
{
  ASSERT (FALSE);
  return -1;
}

void
Camellia_encrypt (
  const unsigned char  *in,
  unsigned char        *out,
  const CAMELLIA_KEY   *key
  )
{
  ASSERT (FALSE);
}

void
Camellia_decrypt (
  const unsigned char  *in,
  unsigned char        *out,
  const CAMELLIA_KEY   *key
  )
{
  ASSERT (FALSE);
}
