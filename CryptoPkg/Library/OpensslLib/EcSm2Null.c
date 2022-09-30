/** @file
  Null implementation of EC and SM2 functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>

#undef OPENSSL_NO_EC

#include <openssl/objects.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

void
EC_GROUP_free (
  EC_GROUP  *group
  )
{
  ASSERT (FALSE);
}

int
EC_GROUP_get_order (
  const EC_GROUP  *group,
  BIGNUM          *order,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_GROUP_get_curve_name (
  const EC_GROUP  *group
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_GROUP_get_curve (
  const EC_GROUP  *group,
  BIGNUM          *p,
  BIGNUM          *a,
  BIGNUM          *b,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_GROUP_get_degree (
  const EC_GROUP  *group
  )
{
  ASSERT (FALSE);
  return 0;
}

EC_GROUP *
EC_GROUP_new_by_curve_name (
  int  nid
  )
{
  ASSERT (FALSE);
  return NULL;
}

EC_POINT *
EC_POINT_new (
  const EC_GROUP  *group
  )
{
  ASSERT (FALSE);
  return NULL;
}

void
EC_POINT_free (
  EC_POINT  *point
  )
{
  ASSERT (FALSE);
}

void
EC_POINT_clear_free (
  EC_POINT  *point
  )
{
  ASSERT (FALSE);
}

int
EC_POINT_set_affine_coordinates (
  const EC_GROUP  *group,
  EC_POINT        *p,
  const BIGNUM    *x,
  const BIGNUM    *y,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_POINT_get_affine_coordinates (
  const EC_GROUP  *group,
  const EC_POINT  *p,
  BIGNUM          *x,
  BIGNUM          *y,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_POINT_set_compressed_coordinates (
  const EC_GROUP  *group,
  EC_POINT        *p,
  const BIGNUM    *x,
  int             y_bit,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_POINT_add (
  const EC_GROUP  *group,
  EC_POINT        *r,
  const EC_POINT  *a,
  const EC_POINT  *b,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_POINT_invert (
  const EC_GROUP  *group,
  EC_POINT        *a,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_POINT_is_at_infinity (
  const EC_GROUP  *group,
  const EC_POINT  *p
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_POINT_is_on_curve (
  const EC_GROUP  *group,
  const EC_POINT  *point,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return -1;
}

int
EC_POINT_cmp (
  const EC_GROUP  *group,
  const EC_POINT  *a,
  const EC_POINT  *b,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return -1;
}

int
EC_POINT_mul (
  const EC_GROUP  *group,
  EC_POINT        *r,
  const BIGNUM    *n,
  const EC_POINT  *q,
  const BIGNUM    *m,
  BN_CTX          *ctx
  )
{
  ASSERT (FALSE);
  return -0;
}

EC_KEY *
EC_KEY_new_by_curve_name (
  int  nid
  )
{
  ASSERT (FALSE);
  return NULL;
}

void
EC_KEY_free (
  EC_KEY  *key
  )
{
  ASSERT (FALSE);
}

EC_KEY *
EC_KEY_dup (
  const EC_KEY  *src
  )
{
  ASSERT (FALSE);
  return NULL;
}

const EC_GROUP *
EC_KEY_get0_group (
  const EC_KEY  *key
  )
{
  ASSERT (FALSE);
  return NULL;
}

const EC_POINT *
EC_KEY_get0_public_key (
  const EC_KEY  *key
  )
{
  ASSERT (FALSE);
  return NULL;
}

int
EC_KEY_set_public_key (
  EC_KEY          *key,
  const EC_POINT  *pub
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_KEY_generate_key (
  EC_KEY  *key
  )
{
  ASSERT (FALSE);
  return 0;
}

int
EC_KEY_check_key (
  const EC_KEY  *key
  )
{
  ASSERT (FALSE);
  return 0;
}

int
ECDH_compute_key (
  void            *out,
  size_t          outlen,
  const EC_POINT  *pub_key,
  const EC_KEY    *ecdh,
  void *(*KDF)(
  const void *in,
  size_t inlen,
  void *out,
  size_t *outlen
  )
  )
{
  ASSERT (FALSE);
  return 0;
}

struct ec_key_st *
EVP_PKEY_get0_EC_KEY (
  EVP_PKEY  *pkey
  )
{
  ASSERT (FALSE);
  return NULL;
}

EC_KEY *
PEM_read_bio_ECPrivateKey (
  BIO              *bp,
  EC_KEY           **key,
  pem_password_cb  *cb,
  void             *u
  )
{
  ASSERT (FALSE);
  return NULL;
}

ECDSA_SIG *
ECDSA_SIG_new (
  void
  )
{
  ASSERT (FALSE);
  return NULL;
}

void
ECDSA_SIG_free (
  ECDSA_SIG  *sig
  )
{
  ASSERT (FALSE);
}

void
ECDSA_SIG_get0 (
  const ECDSA_SIG  *sig,
  const BIGNUM     **pr,
  const BIGNUM     **ps
  )
{
  ASSERT (FALSE);
}

int
ECDSA_SIG_set0 (
  ECDSA_SIG  *sig,
  BIGNUM     *r,
  BIGNUM     *s
  )
{
  return 0;
  ASSERT (FALSE);
}

ECDSA_SIG *
ECDSA_do_sign (
  const unsigned char  *dgst,
  int                  dgst_len,
  EC_KEY               *eckey
  )
{
  ASSERT (FALSE);
  return NULL;
}

int
ECDSA_do_verify (
  const unsigned char  *dgst,
  int                  dgst_len,
  const ECDSA_SIG      *sig,
  EC_KEY               *eckey
  )
{
  ASSERT (FALSE);
  return -1;
}
