/** @file
  Null implementation of Dh functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "openssl/dh.h"

int
DH_up_ref (
  DH  *r
  )
{
  return 0;
}

void
DH_free (
  DH  *r
  )
{
}

int
DH_size (
  const DH  *dh
  )
{
  return 0;
}

void
DH_get0_key (
  const DH      *dh,
  const BIGNUM  **pub_key,
  const BIGNUM  **priv_key
  )
{
}

int
DH_compute_key_padded (
  unsigned char  *key,
  const BIGNUM   *pub_key,
  DH             *dh
  )
{
  return 0;
}

int
DH_compute_key (
  unsigned char  *key,
  const BIGNUM   *pub_key,
  DH             *dh
  )
{
  return 0;
}

int
DH_generate_key (
  DH  *dh
  )
{
  return -1;
}

void
DH_get0_pqg (
  const DH      *dh,
  const BIGNUM  **p,
  const BIGNUM  **q,
  const BIGNUM  **g
  )
{
}

int
DH_set0_pqg (
  DH      *dh,
  BIGNUM  *p,
  BIGNUM  *q,
  BIGNUM  *g
  )
{
  return -1;
}

DH *
DH_new (
  void
  )
{
  return NULL;
}

int
DH_generate_parameters_ex (
  DH        *ret,
  int       prime_len,
  int       generator,
  BN_GENCB  *cb
  )
{
  return -1;
}
