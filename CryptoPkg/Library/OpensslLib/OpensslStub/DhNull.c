/** @file
  Null implementation of DH functions called by BaseCryptLib.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#undef OPENSSL_NO_DH
#include <openssl/bn.h>
#include <openssl/dh.h>

DH *
DH_new (
  void
  )
{
  ASSERT (FALSE);
  return NULL;
}

void
DH_free (
  DH  *dh
  )
{
  ASSERT (FALSE);
}

int
DH_generate_parameters_ex (
  DH        *dh,
  int       prime_len,
  int       generator,
  BN_GENCB  *cb
  )
{
  ASSERT (FALSE);
  return 0;
}

void
DH_get0_pqg (
  const DH      *dh,
  const BIGNUM  **p,
  const BIGNUM  **q,
  const BIGNUM  **g
  )
{
  ASSERT (FALSE);
}

int
DH_set0_pqg (
  DH      *dh,
  BIGNUM  *p,
  BIGNUM  *q,
  BIGNUM  *g
  )
{
  ASSERT (FALSE);
  return 0;
}

int
DH_generate_key (
  DH  *dh
  )
{
  ASSERT (FALSE);
  return 0;
}

void
DH_get0_key (
  const DH      *dh,
  const BIGNUM  **pub_key,
  const BIGNUM  **priv_key
  )
{
  ASSERT (FALSE);
}

int
DH_compute_key (
  unsigned char  *key,
  const BIGNUM   *pub_key,
  DH             *dh
  )
{
  ASSERT (FALSE);
  return 0;
}
