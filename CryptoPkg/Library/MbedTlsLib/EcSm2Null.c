/** @file
  Null implementation of EC and SM2 functions called by BaseCryptLib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>

#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/bignum.h>
#include <bignum_core.h>

/*
 * Get the curve info for the internal identifier
 */
const mbedtls_ecp_curve_info *
mbedtls_ecp_curve_info_from_grp_id (
  mbedtls_ecp_group_id  grp_id
  )
{
  ASSERT (FALSE);
  return (NULL);
}

void
mbedtls_ecdh_init (
  mbedtls_ecdh_context  *ctx
  )
{
  ASSERT (FALSE);
}

/*
 * Free context
 */
void
mbedtls_ecdh_free (
  mbedtls_ecdh_context  *ctx
  )
{
  ASSERT (FALSE);
}

int
mbedtls_ecdh_calc_secret (
  mbedtls_ecdh_context *ctx,
  size_t *olen,
  unsigned char *buf,
  size_t blen,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

void
mbedtls_ecp_keypair_init (
  mbedtls_ecp_keypair  *key
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecp_keypair_free (
  mbedtls_ecp_keypair  *key
  )
{
  ASSERT (FALSE);
}

int
mbedtls_ecp_check_pub_priv (
  const mbedtls_ecp_keypair *pub,
  const mbedtls_ecp_keypair *prv,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdsa_write_signature (
  mbedtls_ecdsa_context *ctx,
  mbedtls_md_type_t md_alg,
  const unsigned char *hash,
  size_t hlen,
  unsigned char *sig,
  size_t sig_size,
  size_t *slen,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdsa_write_signature_restartable (
  mbedtls_ecdsa_context *ctx,
  mbedtls_md_type_t md_alg,
  const unsigned char *hash,
  size_t hlen,
  unsigned char *sig,
  size_t sig_size,
  size_t *slen,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng,
  mbedtls_ecdsa_restart_ctx *rs_ctx
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdsa_read_signature (
  mbedtls_ecdsa_context  *ctx,
  const unsigned char    *hash,
  size_t                 hlen,
  const unsigned char    *sig,
  size_t                 slen
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdsa_read_signature_restartable (
  mbedtls_ecdsa_context      *ctx,
  const unsigned char        *hash,
  size_t                     hlen,
  const unsigned char        *sig,
  size_t                     slen,
  mbedtls_ecdsa_restart_ctx  *rs_ctx
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdsa_from_keypair (
  mbedtls_ecdsa_context      *ctx,
  const mbedtls_ecp_keypair  *key
  )
{
  ASSERT (FALSE);
  return -1;
}

void
mbedtls_ecdsa_init (
  mbedtls_ecdsa_context  *ctx
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecdsa_free (
  mbedtls_ecdsa_context  *ctx
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecdsa_restart_init (
  mbedtls_ecdsa_restart_ctx  *ctx
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecdsa_restart_free (
  mbedtls_ecdsa_restart_ctx  *ctx
  )
{
  ASSERT (FALSE);
}

int
mbedtls_ecp_point_write_binary (
  const mbedtls_ecp_group  *grp,
  const mbedtls_ecp_point  *P,
  int                      format,
  size_t                   *olen,
  unsigned char            *buf,
  size_t                   buflen
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_point_read_binary (
  const mbedtls_ecp_group  *grp,
  mbedtls_ecp_point        *P,
  const unsigned char      *buf,
  size_t                   ilen
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_write_key (
  mbedtls_ecp_keypair  *key,
  unsigned char        *buf,
  size_t               buflen
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_group_load (
  mbedtls_ecp_group     *grp,
  mbedtls_ecp_group_id  id
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_mul (
  mbedtls_ecp_group *grp,
  mbedtls_ecp_point *R,
  const mbedtls_mpi *m,
  const mbedtls_ecp_point *P,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_check_pubkey (
  const mbedtls_ecp_group  *grp,
  const mbedtls_ecp_point  *pt
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_check_privkey (
  const mbedtls_ecp_group  *grp,
  const mbedtls_mpi        *d
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_restart_is_enabled (
  void
  )
{
  ASSERT (FALSE);
  return -1;
}

const mbedtls_ecp_curve_info *
mbedtls_ecp_curve_info_from_tls_id (
  uint16_t  tls_id
  )
{
  ASSERT (FALSE);
  return (NULL);
}

int
mbedtls_ecdh_setup (
  mbedtls_ecdh_context  *ctx,
  mbedtls_ecp_group_id  grp_id
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_make_params (
  mbedtls_ecdh_context *ctx,
  size_t *olen,
  unsigned char *buf,
  size_t blen,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_get_params (
  mbedtls_ecdh_context       *ctx,
  const mbedtls_ecp_keypair  *key,
  mbedtls_ecdh_side          side
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_read_public (
  mbedtls_ecdh_context  *ctx,
  const unsigned char   *buf,
  size_t                blen
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_read_params (
  mbedtls_ecdh_context  *ctx,
  const unsigned char   **buf,
  const unsigned char   *end
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_make_public (
  mbedtls_ecdh_context *ctx,
  size_t *olen,
  unsigned char *buf,
  size_t blen,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

void
mbedtls_ecdh_enable_restart (
  mbedtls_ecdh_context  *ctx
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecp_point_init (
  mbedtls_ecp_point  *pt
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecp_group_init (
  mbedtls_ecp_group  *grp
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecp_point_free (
  mbedtls_ecp_point  *pt
  )
{
  ASSERT (FALSE);
}

void
mbedtls_ecp_group_free (
  mbedtls_ecp_group  *grp
  )
{
  ASSERT (FALSE);
}

int
mbedtls_ecp_is_zero (
  mbedtls_ecp_point  *pt
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_point_cmp (
  const mbedtls_ecp_point  *P,
  const mbedtls_ecp_point  *Q
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecp_muladd (
  mbedtls_ecp_group        *grp,
  mbedtls_ecp_point        *R,
  const mbedtls_mpi        *m,
  const mbedtls_ecp_point  *P,
  const mbedtls_mpi        *n,
  const mbedtls_ecp_point  *Q
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_gen_public (
  mbedtls_ecp_group *grp,
  mbedtls_mpi *d,
  mbedtls_ecp_point *Q,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdh_compute_shared (
  mbedtls_ecp_group *grp,
  mbedtls_mpi *z,
  const mbedtls_ecp_point *Q,
  const mbedtls_mpi *d,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}

int
mbedtls_ecdsa_verify (
  mbedtls_ecp_group        *grp,
  const unsigned char      *buf,
  size_t                   blen,
  const mbedtls_ecp_point  *Q,
  const mbedtls_mpi        *r,
  const mbedtls_mpi        *s
  )
{
  ASSERT (FALSE);
  return -1;
}

/*
 * Compute ECDSA signature of a hashed message
 */
int
mbedtls_ecdsa_sign (
  mbedtls_ecp_group *grp,
  mbedtls_mpi *r,
  mbedtls_mpi *s,
  const mbedtls_mpi *d,
  const unsigned char *buf,
  size_t blen,
  int ( *f_rng )(void *, unsigned char *, size_t),
  void *p_rng
  )
{
  ASSERT (FALSE);
  return -1;
}
