/** @file
  Elliptic Curve Wrapper Implementation over MbedTLS.

  RFC 8422 - Elliptic Curve Cryptography (ECC) Cipher Suites
  FIPS 186-4 - Digital Signature Standard (DSS)

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/bignum.h>
#include <bignum_core.h>

// =====================================================================================
//    Basic Elliptic Curve Primitives
// =====================================================================================

/**
  Return the Nid of certain ECC curve.

  @param[in]  CryptoNid   Identifying number for the ECC curve (Defined in
                          BaseCryptLib.h).

  @retval !=-1    On success.
  @retval -1      ECC curve not supported.
**/
STATIC
INT32
CryptoNidToMbedtlsNid (
  IN UINTN  CryptoNid
  )
{
  INT32  Nid;

  switch (CryptoNid) {
    case CRYPTO_NID_SECP256R1:
      Nid = MBEDTLS_ECP_DP_SECP256R1;
      break;
    case CRYPTO_NID_SECP384R1:
      Nid = MBEDTLS_ECP_DP_SECP384R1;
      break;
    case CRYPTO_NID_SECP521R1:
      Nid = MBEDTLS_ECP_DP_SECP521R1;
      break;
    default:
      return -1;
  }

  return Nid;
}

/**
  Initialize new opaque EcGroup object. This object represents an EC curve and
  and is used for calculation within this group. This object should be freed
  using EcGroupFree() function.

  @param[in]  CryptoNid   Identifying number for the ECC curve (Defined in
                          BaseCryptLib.h).

  @retval EcGroup object  On success.
  @retval NULL            On failure.
**/
VOID *
EFIAPI
EcGroupInit (
  IN UINTN  CryptoNid
  )
{
  INT32              Nid;
  mbedtls_ecp_group  *grp;

  Nid = CryptoNidToMbedtlsNid (CryptoNid);

  if (Nid < 0) {
    return NULL;
  }

  grp = AllocateZeroPool (sizeof (mbedtls_ecp_group));
  if (grp == NULL) {
    return NULL;
  }

  mbedtls_ecp_group_init (grp);

  mbedtls_ecp_group_load (grp, Nid);

  return grp;
}

/**
  Get EC curve parameters. While elliptic curve equation is Y^2 mod P = (X^3 + AX + B) Mod P.
  This function will set the provided Big Number objects  to the corresponding
  values. The caller needs to make sure all the "out" BigNumber parameters
  are properly initialized.

  @param[in]  EcGroup    EC group object.
  @param[out] BnPrime    Group prime number.
  @param[out] BnA        A coefficient.
  @param[out] BnB        B coefficient..
  @param[in]  BnCtx      This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                         The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcGroupGetCurve (
  IN CONST VOID  *EcGroup,
  OUT VOID       *BnPrime,
  OUT VOID       *BnA,
  OUT VOID       *BnB,
  IN VOID        *BnCtx
  )
{
  mbedtls_ecp_group  *grp;

  grp = (mbedtls_ecp_group *)EcGroup;

  //
  // Check input parameters.
  //
  if ((EcGroup == NULL) || (BnPrime == NULL) ||
      (grp->P.n == 0) || (grp->P.n > INT_MAX) ||
      (grp->P.p == NULL) ||
      ((grp->P.s != 1) && (grp->P.s != -1)))
  {
    return FALSE;
  }

  if (mbedtls_mpi_copy ((mbedtls_mpi *)BnPrime, &grp->P) != 0) {
    return FALSE;
  }

  if (BnA != NULL) {
    if ((grp->A.n == 0) || (grp->A.n > INT_MAX) ||
        (grp->A.p == NULL) ||
        ((grp->A.s != 1) && (grp->A.s != -1)))
    {
      return FALSE;
    }

    if (mbedtls_mpi_copy ((mbedtls_mpi *)BnA, &grp->A) != 0) {
      return FALSE;
    }
  }

  if (BnB != NULL) {
    if ((grp->B.n == 0) || (grp->B.n > INT_MAX) ||
        (grp->B.p == NULL) ||
        ((grp->B.s != 1) && (grp->B.s != -1)))
    {
      return FALSE;
    }

    if (mbedtls_mpi_copy ((mbedtls_mpi *)BnB, &grp->B) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Get EC group order.
  This function will set the provided Big Number object to the corresponding
  value. The caller needs to make sure that the "out" BigNumber parameter
  is properly initialized.

  @param[in]  EcGroup   EC group object.
  @param[out] BnOrder   Group prime number.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcGroupGetOrder (
  IN VOID   *EcGroup,
  OUT VOID  *BnOrder
  )
{
  mbedtls_ecp_group  *grp;

  grp = (mbedtls_ecp_group *)EcGroup;

  //
  // Check input parameters.
  //
  if ((EcGroup == NULL) || (BnOrder == NULL) ||
      (grp->N.n == 0) || (grp->N.n > INT_MAX) ||
      (grp->N.p == NULL) ||
      ((grp->N.s != 1) && (grp->N.s != -1)))
  {
    return FALSE;
  }

  if (mbedtls_mpi_copy ((mbedtls_mpi *)BnOrder, &grp->N) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Free previously allocated EC group object using EcGroupInit().

  @param[in]  EcGroup   EC group object to free.
**/
VOID
EFIAPI
EcGroupFree (
  IN VOID  *EcGroup
  )
{
  mbedtls_ecp_group_free (EcGroup);
  if (EcGroup != NULL) {
    FreePool (EcGroup);
  }
}

/**
  Initialize new opaque EC Point object. This object represents an EC point
  within the given EC group (curve).

  @param[in]  EC Group, properly initialized using EcGroupInit().

  @retval EC Point object  On success.
  @retval NULL             On failure.
**/
VOID *
EFIAPI
EcPointInit (
  IN CONST VOID  *EcGroup
  )
{
  mbedtls_ecp_point  *pt;

  pt = AllocateZeroPool (sizeof (mbedtls_ecp_point));
  if (pt == NULL) {
    return NULL;
  }

  mbedtls_ecp_point_init (pt);

  return pt;
}

/**
  Free previously allocated EC Point object using EcPointInit().

  @param[in]  EcPoint   EC Point to free.
  @param[in]  Clear     TRUE iff the memory should be cleared.
**/
VOID
EFIAPI
EcPointDeInit (
  IN VOID     *EcPoint,
  IN BOOLEAN  Clear
  )
{
  mbedtls_ecp_point_free (EcPoint);
  if (EcPoint != NULL) {
    FreePool (EcPoint);
  }
}

/**
  Get EC point affine (x,y) coordinates.
  This function will set the provided Big Number objects to the corresponding
  values. The caller needs to make sure all the "out" BigNumber parameters
  are properly initialized.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC point object.
  @param[out] BnX        X coordinate.
  @param[out] BnY        Y coordinate.
  @param[in]  BnCtx      This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                         The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointGetAffineCoordinates (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint,
  OUT VOID       *BnX,
  OUT VOID       *BnY,
  IN VOID        *BnCtx
  )
{
  mbedtls_ecp_point  *pt;

  pt = (mbedtls_ecp_point *)EcPoint;

  //
  // Check input parameters.
  //
  if ((EcPoint == NULL) || (BnX == NULL) || (BnY == NULL) ||
      (pt->X.n == 0) || (pt->X.n > INT_MAX) ||
      (pt->X.p == NULL) ||
      ((pt->X.s != 1) && (pt->X.s != -1)) ||
      (pt->Y.n == 0) || (pt->Y.n > INT_MAX) ||
      (pt->Y.p == NULL) ||
      ((pt->Y.s != 1) && (pt->Y.s != -1)))
  {
    return FALSE;
  }

  if (mbedtls_mpi_copy ((mbedtls_mpi *)BnX, &pt->X) != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_copy ((mbedtls_mpi *)BnY, &pt->Y) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Set EC point affine (x,y) coordinates.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC point object.
  @param[in]  BnX        X coordinate.
  @param[in]  BnY        Y coordinate.
  @param[in]  BnCtx      This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                         The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointSetAffineCoordinates (
  IN CONST VOID  *EcGroup,
  IN VOID        *EcPoint,
  IN CONST VOID  *BnX,
  IN CONST VOID  *BnY,
  IN VOID        *BnCtx
  )
{
  mbedtls_ecp_point  *pt;

  pt = (mbedtls_ecp_point *)EcPoint;

  //
  // Check input parameters.
  //
  if ((EcPoint == NULL) || (BnX == NULL) || (BnY == NULL) ||
      (((mbedtls_mpi *)BnY)->n == 0) || (((mbedtls_mpi *)BnY)->n > INT_MAX) ||
      (((mbedtls_mpi *)BnX)->n == 0) || (((mbedtls_mpi *)BnX)->n > INT_MAX) ||
      (((mbedtls_mpi *)BnX)->p == NULL) ||
      ((((mbedtls_mpi *)BnX)->s != 1) && (((mbedtls_mpi *)BnX)->s != -1)) ||
      (((mbedtls_mpi *)BnY)->p == NULL) ||
      ((((mbedtls_mpi *)BnY)->s != 1) && (((mbedtls_mpi *)BnY)->s != -1)))
  {
    return FALSE;
  }

  if (mbedtls_mpi_copy (&pt->X, (mbedtls_mpi *)BnX) != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_copy (&pt->Y, (mbedtls_mpi *)BnY) != 0) {
    return FALSE;
  }

  mbedtls_mpi_lset (&pt->Z, 1);

  return TRUE;
}

/**
  EC Point addition. EcPointResult = EcPointA + EcPointB.

  @param[in]  EcGroup          EC group object.
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPointA         EC Point.
  @param[in]  EcPointB         EC Point.
  @param[in]  BnCtx            This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                               The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointAdd (
  IN CONST VOID  *EcGroup,
  OUT VOID       *EcPointResult,
  IN CONST VOID  *EcPointA,
  IN CONST VOID  *EcPointB,
  IN VOID        *BnCtx
  )
{
  mbedtls_mpi  BnOne;

  mbedtls_mpi_init(&BnOne);
  if (mbedtls_mpi_lset (&BnOne, 1) != 0 ) {
    return FALSE;
  }

  if (mbedtls_ecp_muladd (
                          (mbedtls_ecp_group *)EcGroup,
                          (mbedtls_ecp_point *)EcPointResult,
                          (const mbedtls_mpi *)&BnOne,
                          (const mbedtls_ecp_point *)EcPointA,
                          (const mbedtls_mpi *)&BnOne,
                          (const mbedtls_ecp_point *)EcPointB
                          ) != 0)
  {
    mbedtls_mpi_free(&BnOne);
    return FALSE;
  }

  mbedtls_mpi_free(&BnOne);
  return TRUE;
}

/**
  Variable EC point multiplication. EcPointResult = EcPoint * BnPScalar.

  @param[in]  EcGroup          EC group object.
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPoint          EC Point.
  @param[in]  BnPScalar        P Scalar.
  @param[in]  BnCtx            This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                               The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointMul (
  IN CONST VOID  *EcGroup,
  OUT VOID       *EcPointResult,
  IN CONST VOID  *EcPoint,
  IN CONST VOID  *BnPScalar,
  IN VOID        *BnCtx
  )
{
  return (mbedtls_ecp_mul ((mbedtls_ecp_group *)EcGroup, EcPointResult, BnPScalar, EcPoint, MbedtlsRand, NULL) == 0);
}

/**
  Calculate the inverse of the supplied EC point.

  @param[in]     EcGroup   EC group object.
  @param[in,out] EcPoint   EC point to invert.
  @param[in]     BnCtx     This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                           The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointInvert (
  IN CONST VOID  *EcGroup,
  IN OUT VOID    *EcPoint,
  IN VOID        *BnCtx
  )
{
  mbedtls_ecp_point  *pt;
  mbedtls_ecp_group  *grp;
  mbedtls_mpi        InvBnY;
  mbedtls_mpi        P;
  BOOLEAN            Status;

  pt  = (mbedtls_ecp_point *)EcPoint;
  grp = (mbedtls_ecp_group *)EcGroup;

  //
  // Check input parameters.
  //
  if ((EcGroup == NULL) || (EcPoint == NULL) ||
      (pt->Y.n == 0) || (pt->Y.n > INT_MAX) ||
      (pt->Y.p == NULL) || ((pt->Y.s != 1) && (pt->Y.s != -1)) ||
      (grp->P.n == 0) || (grp->P.n  > INT_MAX) ||
      (grp->P.p == NULL) || ((grp->P.s != 1) && (grp->P.s != -1)))
  {
    return FALSE;
  }

  mbedtls_mpi_init (&InvBnY);
  mbedtls_mpi_init (&P);

  if (mbedtls_mpi_copy (&InvBnY, &pt->Y) != 0) {
    Status = FALSE;
    goto Clean;
  }

  if (mbedtls_mpi_copy (&P, &grp->P) != 0) {
    Status = FALSE;
    goto Clean;
  }

  InvBnY.s = 0 - InvBnY.s;

  if (mbedtls_mpi_mod_mpi (&InvBnY, &InvBnY, &P) != 0) {
    Status = FALSE;
    goto Clean;
  }

  if (mbedtls_mpi_copy (&pt->Y, &InvBnY) != 0) {
    Status = FALSE;
    goto Clean;
  }

  Status = TRUE;

Clean:
  mbedtls_mpi_free (&P);
  mbedtls_mpi_free (&InvBnY);
  return Status;
}

/**
  Check if the supplied point is on EC curve.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPoint   EC point to check.
  @param[in]  BnCtx     This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                        The pointer can be NULL or other value.

  @retval TRUE          On curve.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointIsOnCurve (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint,
  IN VOID        *BnCtx
  )
{
  return (mbedtls_ecp_check_pubkey (EcGroup, EcPoint) == 0);
}

/**
  Check if the supplied point is at infinity.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPoint   EC point to check.

  @retval TRUE          At infinity.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointIsAtInfinity (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint
  )
{
  mbedtls_ecp_point  *pt;

  pt = (mbedtls_ecp_point *)EcPoint;

  return (mbedtls_ecp_is_zero (pt) == 1);
}

/**
  Check if EC points are equal.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPointA  EC point A.
  @param[in]  EcPointB  EC point B.
  @param[in]  BnCtx     This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                        The pointer can be NULL or other value.

  @retval TRUE          A == B.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointEqual (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPointA,
  IN CONST VOID  *EcPointB,
  IN VOID        *BnCtx
  )
{
  return mbedtls_ecp_point_cmp (EcPointA, EcPointB) == 0;
}

/**
  Set EC point compressed coordinates. Points can be described in terms of
  their compressed coordinates. For a point (x, y), for any given value for x
  such that the point is on the curve there will only ever be two possible
  values for y. Therefore, a point can be set using this function where BnX is
  the x coordinate and YBit is a value 0 or 1 to identify which of the two
  possible values for y should be used.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC Point.
  @param[in]  BnX        X coordinate.
  @param[in]  YBit       0 or 1 to identify which Y value is used.
  @param[in]  BnCtx      This paramter is not used in Mbedtls. It is only compatible for Openssl implementation.
                         The pointer can be NULL or other value.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
EcPointSetCompressedCoordinates (
  IN CONST VOID  *EcGroup,
  IN VOID        *EcPoint,
  IN CONST VOID  *BnX,
  IN UINT8       YBit,
  IN VOID        *BnCtx
  )
{
  BOOLEAN            Status;
  mbedtls_mpi        r;
  mbedtls_mpi        n;
  mbedtls_ecp_point  *pt;
  mbedtls_ecp_group  *grp;

  pt  = (mbedtls_ecp_point *)EcPoint;
  grp = (mbedtls_ecp_group *)EcGroup;

  //
  // Check input parameters.
  //
  if ((EcGroup == NULL) || (EcPoint == NULL) || (BnX == NULL) ||
      (((mbedtls_mpi *)BnX)->n == 0) || (((mbedtls_mpi *)BnX)->n > INT_MAX) ||
      (((mbedtls_mpi *)BnX)->p == NULL) ||
      ((((mbedtls_mpi *)BnX)->s != 1) && (((mbedtls_mpi *)BnX)->s != -1)) ||
      (grp->P.n == 0) || (grp->P.n  > INT_MAX) ||
      (grp->A.p == NULL) || ((grp->A.s != 1) && (grp->A.s != -1)) ||
      (grp->A.n == 0) || (grp->A.n  > INT_MAX) ||
      (grp->B.p == NULL) || ((grp->B.s != 1) && (grp->B.s != -1)) ||
      (grp->B.n == 0) || (grp->B.n  > INT_MAX))
  {
    return FALSE;
  }

  mbedtls_mpi_init (&r);
  mbedtls_mpi_init (&n);

  // r = x^2
  if (mbedtls_mpi_mul_mpi (&r, BnX, BnX) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // r = x^2 + a
  if (mbedtls_mpi_add_mpi (&r, &r, &grp->A) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // r = x^3 + ax
  if (mbedtls_mpi_mul_mpi (&r, &r, BnX) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // r = x^3 + ax + b
  if (mbedtls_mpi_add_mpi (&r, &r, &grp->B) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // Calculate square root of r over finite field P:
  //   r = sqrt(x^3 + ax + b) = (x^3 + ax + b) ^ ((P + 1) / 4) (mod P)

  // n = P + 1
  if (mbedtls_mpi_add_int (&n, &grp->P, 1) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // n = (P + 1) / 4
  if (mbedtls_mpi_shift_r (&n, 2) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // r ^ ((P + 1) / 4) (mod p)
  if (mbedtls_mpi_exp_mod (&r, &r, &n, &grp->P, NULL) != 0) {
    Status = FALSE;
    goto Clean;
  }

  // Select solution that has the correct "sign" (equals odd/even solution in finite group)
  if (YBit == 0) {
    // r = p - r
    if (mbedtls_mpi_sub_mpi (&r, &grp->P, &r) != 0) {
      Status = FALSE;
      goto Clean;
    }
  }

  if (mbedtls_mpi_copy (&pt->X, (mbedtls_mpi *)BnX) != 0) {
    Status = FALSE;
    goto Clean;
  }

  if (mbedtls_mpi_copy (&pt->Y, &r) != 0) {
    Status = FALSE;
    goto Clean;
  }

  mbedtls_mpi_lset (&pt->Z, 1);

Clean:
  mbedtls_mpi_free (&r);
  mbedtls_mpi_free (&n);
  return Status;
}

// =====================================================================================
//    Elliptic Curve Diffie Hellman Primitives
// =====================================================================================

/**
  Allocates and Initializes one Elliptic Curve Context for subsequent use
  with the NID.

  @param Nid cipher NID

  @return  Pointer to the Elliptic Curve Context that has been initialized.
           If the allocations fails, EcNewByNid() returns NULL.

**/
VOID *
EFIAPI
EcNewByNid (
  IN UINTN  Nid
  )
{
  mbedtls_ecdh_context  *ctx;
  mbedtls_ecp_group_id  grp_id;
  INT32                 Ret;

  ctx = AllocateZeroPool (sizeof (mbedtls_ecdh_context));
  if (ctx == NULL) {
    return NULL;
  }

  switch (Nid) {
    case CRYPTO_NID_SECP256R1:
      grp_id = MBEDTLS_ECP_DP_SECP256R1;
      break;
    case CRYPTO_NID_SECP384R1:
      grp_id = MBEDTLS_ECP_DP_SECP384R1;
      break;
    case CRYPTO_NID_SECP521R1:
      grp_id = MBEDTLS_ECP_DP_SECP521R1;
      break;
    default:
      goto Error;
  }

  Ret = mbedtls_ecdh_setup (ctx, grp_id);
  if (Ret != 0) {
    goto Error;
  }

  return ctx;
Error:
  FreePool (ctx);
  return NULL;
}

/**
  Release the specified EC context.

  @param[in]  EcContext  Pointer to the EC context to be released.

**/
VOID
EFIAPI
EcFree (
  IN  VOID  *EcContext
  )
{
  mbedtls_ecdh_free (EcContext);
  if (EcContext != NULL) {
    FreePool (EcContext);
  }
}

/**
  Sets the public key component into the established EC context.

  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.

  @param[in, out]  EcContext      Pointer to EC context being set.
  @param[in]       PublicKey      Pointer to the buffer to receive generated public X,Y.
  @param[in]       PublicKeySize  The size of Public buffer in bytes.

  @retval  TRUE   EC public key component was set successfully.
  @retval  FALSE  Invalid EC public key component.

**/
BOOLEAN
EFIAPI
EcSetPubKey (
  IN OUT  VOID   *EcContext,
  IN      UINT8  *PublicKey,
  IN      UINTN  PublicKeySize
  )
{
  mbedtls_ecdh_context  *ctx;
  INT32                 Ret;
  UINTN                 HalfSize;

  if ((EcContext == NULL) || (PublicKey == NULL)) {
    return FALSE;
  }

  ctx = EcContext;
  switch (ctx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (PublicKeySize != HalfSize * 2) {
    return FALSE;
  }

  Ret = mbedtls_mpi_read_binary (&ctx->Q.X, PublicKey, HalfSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_read_binary (&ctx->Q.Y, PublicKey + HalfSize, HalfSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_lset (&ctx->Q.Z, 1);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Gets the public key component from the established EC context.

  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.

  @param[in, out]  EcContext      Pointer to EC context being set.
  @param[out]      PublicKey      Pointer to the buffer to receive generated public X,Y.
  @param[in, out]  PublicKeySize  On input, the size of Public buffer in bytes.
                                  On output, the size of data returned in Public buffer in bytes.

  @retval  TRUE   EC key component was retrieved successfully.
  @retval  FALSE  Invalid EC key component.

**/
BOOLEAN
EFIAPI
EcGetPubKey (
  IN OUT  VOID   *EcContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  mbedtls_ecdh_context  *ctx;
  INT32                 Ret;
  UINTN                 HalfSize;
  UINTN                 XSize;
  UINTN                 YSize;

  if ((EcContext == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  if ((PublicKey == NULL) && (*PublicKeySize != 0)) {
    return FALSE;
  }

  ctx = EcContext;
  switch (ctx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (*PublicKeySize < HalfSize * 2) {
    *PublicKeySize = HalfSize * 2;
    return FALSE;
  }

  *PublicKeySize = HalfSize * 2;
  ZeroMem (PublicKey, *PublicKeySize);

  XSize = mbedtls_mpi_size (&ctx->Q.X);
  YSize = mbedtls_mpi_size (&ctx->Q.Y);
  ASSERT (XSize <= HalfSize && YSize <= HalfSize);

  Ret = mbedtls_mpi_write_binary (&ctx->Q.X, &PublicKey[0 + HalfSize - XSize], XSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_write_binary (&ctx->Q.Y, &PublicKey[HalfSize + HalfSize - YSize], YSize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Computes exchanged common key.
  Given peer's public key (X, Y), this function computes the exchanged common key,
  based on its own context including value of curve parameter and random secret.
  X is the first half of PeerPublic with size being PeerPublicSize / 2,
  Y is the second half of PeerPublic with size being PeerPublicSize / 2.
  If public key is compressed, the PeerPublic will only contain half key (X).
  If EcContext is NULL, then return FALSE.
  If PeerPublic is NULL, then return FALSE.
  If PeerPublicSize is 0, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize is not large enough, then return FALSE.
  For P-256, the PeerPublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PeerPublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PeerPublicSize is 132. First 66-byte is X, Second 66-byte is Y.
  @param[in, out]  EcContext          Pointer to the EC context.
  @param[in]       PeerPublic         Pointer to the peer's public X,Y.
  @param[in]       PeerPublicSize     Size of peer's public X,Y in bytes.
  @param[in]       CompressFlag       Flag of PeerPublic is compressed or not.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                      On output, the size of data returned in Key buffer in bytes.
  @retval TRUE   EC exchanged key generation succeeded.
  @retval FALSE  EC exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.
**/
BOOLEAN
EFIAPI
EcDhComputeKey (
  IN OUT  VOID         *EcContext,
  IN      CONST UINT8  *PeerPublic,
  IN      UINTN        PeerPublicSize,
  IN      CONST INT32  *CompressFlag,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  )
{
  UINTN                 HalfSize;
  mbedtls_ecdh_context  *EcdCtx;
  INT32                 Ret;

  if ((EcContext == NULL) || (PeerPublic == NULL) || (KeySize == NULL)) {
    return FALSE;
  }

  if ((Key == NULL) && (*KeySize != 0)) {
    return FALSE;
  }

  if (PeerPublicSize > INT_MAX) {
    return FALSE;
  }

  EcdCtx = EcContext;
  switch (EcdCtx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (PeerPublicSize != HalfSize * 2) {
    return FALSE;
  }

  Ret = mbedtls_mpi_read_binary (&EcdCtx->Qp.X, PeerPublic, HalfSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_read_binary (
                                 &EcdCtx->Qp.Y,
                                 PeerPublic + HalfSize,
                                 HalfSize
                                 );
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_lset (&EcdCtx->Qp.Z, 1);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_ecdh_compute_shared (
                                     &EcdCtx->grp,
                                     &EcdCtx->z,
                                     &EcdCtx->Qp,
                                     &EcdCtx->d,
                                     MbedtlsRand,
                                     NULL
                                     );
  if (Ret != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_size (&EcdCtx->z) > *KeySize) {
    return FALSE;
  }

  *KeySize = EcdCtx->grp.pbits / 8 + ((EcdCtx->grp.pbits % 8) != 0);
  Ret      = mbedtls_mpi_write_binary (&EcdCtx->z, Key, *KeySize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates EC key and returns EC public key (X, Y).

  This function generates random secret, and computes the public key (X, Y), which is
  returned via parameter Public, PublicSize.
  X is the first half of Public with size being PublicSize / 2,
  Y is the second half of Public with size being PublicSize / 2.
  EC context is updated accordingly.
  If the Public buffer is too small to hold the public X, Y, FALSE is returned and
  PublicSize is set to the required buffer size to obtain the public X, Y.

  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.

  If EcContext is NULL, then return FALSE.
  If PublicSize is NULL, then return FALSE.
  If PublicSize is large enough but Public is NULL, then return FALSE.

  @param[in, out]  EcContext      Pointer to the EC context.
  @param[out]      Public         Pointer to the buffer to receive generated public X,Y.
  @param[in, out]  PublicSize     On input, the size of Public buffer in bytes.
                                  On output, the size of data returned in Public buffer in bytes.

  @retval TRUE   EC public X,Y generation succeeded.
  @retval FALSE  EC public X,Y generation failed.
  @retval FALSE  PublicSize is not large enough.

**/
BOOLEAN
EFIAPI
EcGenerateKey (
  IN OUT  VOID   *EcContext,
  OUT     UINT8  *Public,
  IN OUT  UINTN  *PublicSize
  )
{
  mbedtls_ecdh_context  *ctx;
  INT32                 Ret;
  UINTN                 HalfSize;
  UINTN                 XSize;
  UINTN                 YSize;

  if ((EcContext == NULL) || (PublicSize == NULL)) {
    return FALSE;
  }

  if ((Public == NULL) && (*PublicSize != 0)) {
    return FALSE;
  }

  ctx = EcContext;
  Ret = mbedtls_ecdh_gen_public (&ctx->grp, &ctx->d, &ctx->Q, MbedtlsRand, NULL);
  if (Ret != 0) {
    return FALSE;
  }

  switch (ctx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (*PublicSize < HalfSize * 2) {
    *PublicSize = HalfSize * 2;
    return FALSE;
  }

  *PublicSize = HalfSize * 2;
  ZeroMem (Public, *PublicSize);

  XSize = mbedtls_mpi_size (&ctx->Q.X);
  YSize = mbedtls_mpi_size (&ctx->Q.Y);
  ASSERT (XSize <= HalfSize && YSize <= HalfSize);

  Ret = mbedtls_mpi_write_binary (&ctx->Q.X, &Public[0 + HalfSize - XSize], XSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_write_binary (&ctx->Q.Y, &Public[HalfSize + HalfSize - YSize], YSize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Computes exchanged common key.

  Given peer's public key (X, Y), this function computes the exchanged common key,
  based on its own context including value of curve parameter and random secret.
  X is the first half of PeerPublic with size being PeerPublicSize / 2,
  Y is the second half of PeerPublic with size being PeerPublicSize / 2.

  If EcContext is NULL, then return FALSE.
  If PeerPublic is NULL, then return FALSE.
  If PeerPublicSize is 0, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize is not large enough, then return FALSE.

  For P-256, the PeerPublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PeerPublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PeerPublicSize is 132. First 66-byte is X, Second 66-byte is Y.

  @param[in, out]  EcContext          Pointer to the EC context.
  @param[in]       PeerPublic         Pointer to the peer's public X,Y.
  @param[in]       PeerPublicSize     Size of peer's public X,Y in bytes.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                      On output, the size of data returned in Key buffer in bytes.

  @retval TRUE   EC exchanged key generation succeeded.
  @retval FALSE  EC exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.

**/
BOOLEAN
EFIAPI
EcComputeKey (
  IN OUT  VOID         *EcContext,
  IN      CONST UINT8  *PeerPublic,
  IN      UINTN        PeerPublicSize,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  )
{
  mbedtls_ecdh_context  *ctx;
  UINTN                 HalfSize;
  INT32                 Ret;

  if ((EcContext == NULL) || (PeerPublic == NULL) || (KeySize == NULL) || (Key == NULL)) {
    return FALSE;
  }

  if (PeerPublicSize > INT_MAX) {
    return FALSE;
  }

  ctx = EcContext;
  switch (ctx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (PeerPublicSize != HalfSize * 2) {
    return FALSE;
  }

  Ret = mbedtls_mpi_read_binary (&ctx->Qp.X, PeerPublic, HalfSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_read_binary (&ctx->Qp.Y, PeerPublic + HalfSize, HalfSize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_mpi_lset (&ctx->Qp.Z, 1);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_ecdh_compute_shared (
                                     &ctx->grp,
                                     &ctx->z,
                                     &ctx->Qp,
                                     &ctx->d,
                                     MbedtlsRand,
                                     NULL
                                     );
  if (Ret != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_size (&ctx->z) > *KeySize) {
    return FALSE;
  }

  *KeySize = ctx->grp.pbits / 8 + ((ctx->grp.pbits % 8) != 0);
  Ret      = mbedtls_mpi_write_binary (&ctx->z, Key, *KeySize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Carries out the EC-DSA signature.

  This function carries out the EC-DSA signature.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If EcContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If HashSize need match the HashNid. HashNid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.
  If SigSize is large enough but Signature is NULL, then return FALSE.

  For P-256, the SigSize is 64. First 32-byte is R, Second 32-byte is S.
  For P-384, the SigSize is 96. First 48-byte is R, Second 48-byte is S.
  For P-521, the SigSize is 132. First 66-byte is R, Second 66-byte is S.

  @param[in]       EcContext    Pointer to EC context for signature generation.
  @param[in]       HashNid      hash NID
  @param[in]       MessageHash  Pointer to octet message hash to be signed.
  @param[in]       HashSize     Size of the message hash in bytes.
  @param[out]      Signature    Pointer to buffer to receive EC-DSA signature.
  @param[in, out]  SigSize      On input, the size of Signature buffer in bytes.
                                On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in EC-DSA.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.

**/
BOOLEAN
EFIAPI
EcDsaSign (
  IN      VOID         *EcContext,
  IN      UINTN        HashNid,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  INT32                 Ret;
  mbedtls_ecdh_context  *ctx;
  mbedtls_mpi           R;
  mbedtls_mpi           S;
  UINTN                 RSize;
  UINTN                 SSize;
  UINTN                 HalfSize;
  BOOLEAN               Status;

  if ((EcContext == NULL) || (MessageHash == NULL)) {
    return FALSE;
  }

  if (Signature == NULL) {
    return FALSE;
  }

  ctx = EcContext;
  switch (ctx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (*SigSize < (UINTN)(HalfSize * 2)) {
    *SigSize = HalfSize * 2;
    return FALSE;
  }

  *SigSize = HalfSize * 2;
  ZeroMem (Signature, *SigSize);

  switch (HashNid) {
    case CRYPTO_NID_SHA256:
      if (HashSize != SHA256_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    case CRYPTO_NID_SHA384:
      if (HashSize != SHA384_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    case CRYPTO_NID_SHA512:
      if (HashSize != SHA512_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    default:
      return FALSE;
  }

  mbedtls_mpi_init (&R);
  mbedtls_mpi_init (&S);

  Ret = mbedtls_ecdsa_sign (
                            &ctx->grp,
                            &R,
                            &S,
                            &ctx->d,
                            MessageHash,
                            HashSize,
                            MbedtlsRand,
                            NULL
                            );
  if (Ret != 0) {
    Status = FALSE;
    goto Clean;
  }

  RSize = mbedtls_mpi_size (&R);
  SSize = mbedtls_mpi_size (&S);
  ASSERT (RSize <= HalfSize && SSize <= HalfSize);

  Ret = mbedtls_mpi_write_binary (&R, &Signature[0 + HalfSize - RSize], RSize);
  if (Ret != 0) {
    Status = FALSE;
    goto Clean;
  }

  Ret = mbedtls_mpi_write_binary (&S, &Signature[HalfSize + HalfSize - SSize], SSize);
  if (Ret != 0) {
    Status = FALSE;
    goto Clean;
  }

  Status = TRUE;

Clean:
  mbedtls_mpi_free (&R);
  mbedtls_mpi_free (&S);

  return Status;
}

/**
  Verifies the EC-DSA signature.

  If EcContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize need match the HashNid. HashNid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.

  For P-256, the SigSize is 64. First 32-byte is R, Second 32-byte is S.
  For P-384, the SigSize is 96. First 48-byte is R, Second 48-byte is S.
  For P-521, the SigSize is 132. First 66-byte is R, Second 66-byte is S.

  @param[in]  EcContext    Pointer to EC context for signature verification.
  @param[in]  HashNid      hash NID
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashSize     Size of the message hash in bytes.
  @param[in]  Signature    Pointer to EC-DSA signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in EC-DSA.
  @retval  FALSE  Invalid signature or invalid EC context.

**/
BOOLEAN
EFIAPI
EcDsaVerify (
  IN  VOID         *EcContext,
  IN  UINTN        HashNid,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  INT32                 Ret;
  mbedtls_ecdh_context  *ctx;
  mbedtls_mpi           R;
  mbedtls_mpi           S;
  UINTN                 HalfSize;
  BOOLEAN               Status;

  if ((EcContext == NULL) || (MessageHash == NULL) || (Signature == NULL)) {
    return FALSE;
  }

  if ((SigSize > INT_MAX) || (SigSize == 0)) {
    return FALSE;
  }

  ctx = EcContext;
  switch (ctx->grp.id) {
    case MBEDTLS_ECP_DP_SECP256R1:
      HalfSize = 32;
      break;
    case MBEDTLS_ECP_DP_SECP384R1:
      HalfSize = 48;
      break;
    case MBEDTLS_ECP_DP_SECP521R1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (SigSize != (UINTN)(HalfSize * 2)) {
    return FALSE;
  }

  switch (HashNid) {
    case CRYPTO_NID_SHA256:
      if (HashSize != SHA256_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    case CRYPTO_NID_SHA384:
      if (HashSize != SHA384_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    case CRYPTO_NID_SHA512:
      if (HashSize != SHA512_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    default:
      return FALSE;
  }

  mbedtls_mpi_init (&R);
  mbedtls_mpi_init (&S);

  Ret = mbedtls_mpi_read_binary (&R, Signature, HalfSize);
  if (Ret != 0) {
    Status = FALSE;
    goto Clean;
  }

  Ret = mbedtls_mpi_read_binary (&S, Signature + HalfSize, HalfSize);
  if (Ret != 0) {
    Status = FALSE;
    goto Clean;
  }

  Ret = mbedtls_ecdsa_verify (
                              &ctx->grp,
                              MessageHash,
                              HashSize,
                              &ctx->Q,
                              &R,
                              &S
                              );
  if (Ret != 0) {
    Status = FALSE;
    goto Clean;
  }

  Status = TRUE;

Clean:
  mbedtls_mpi_free (&R);
  mbedtls_mpi_free (&S);

  return Status;
}
