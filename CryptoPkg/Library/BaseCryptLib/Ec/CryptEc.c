/** @file
  Elliptic Curve and ECDH API implementation based on OpenSSL

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <openssl/opensslv.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/ec.h>

/**
  Temp comment.

  @param[in]  Group  Identifying number for the ECC group (IANA "Group
                     Description" attribute registrty for RFC 2409).

  @retval EcGroup object  On success.
  @retval NULL            On failure.
**/
STATIC
INT32
GroupToNid (
  UINTN  Group
  )
{
  INT32  Nid;

  switch (Group) {
    case 19:
      Nid = NID_X9_62_prime256v1;
      break;
    case 20:
      Nid = NID_secp384r1;
      break;
    case 21:
      Nid = NID_secp521r1;
      break;
    case 25:
      Nid = NID_X9_62_prime192v1;
      break;
    case 26:
      Nid = NID_secp224r1;
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

  @param[in]  Group  Identifying number for the ECC group (IANA "Group
                     Description" attribute registrty for RFC 2409).

  @retval EcGroup object  On success.
  @retval NULL            On failure.
**/
VOID *
EFIAPI
EcGroupInit (
  IN UINTN  Group
  )
{
  INT32  Nid;

  Nid = GroupToNid (Group);

  if (Nid < 0) {
    return NULL;
  }

  return EC_GROUP_new_by_curve_name (Nid);
}

/**
  Get EC curve parameters. While elliptic curve equation is Y^2 mod P = (X^3 + AX + B) Mod P.
  This function will set the provided Big Number objects  to the corresponding
  values. The caller needs to make sure all the "out" BigNumber parameters
  are properly initialized.

  @param[in]  EcGroup    EC group object.
  @param[out] BnPrime    Group prime number.
  @param[out] BnA        A coofecient.
  @param[out] BnB        B coofecient.
  @param[in]  BnCtx      BN context.

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcGroupGetCurve (
  IN CONST VOID  *EcGroup,
  OUT VOID       *BnPrime,
  OUT VOID       *BnA,
  OUT VOID       *BnB,
  IN VOID        *BnCtx
  )
{
  return EC_GROUP_get_curve (EcGroup, BnPrime, BnA, BnB, BnCtx) ?
         EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

/**
  Get EC group order.
  This function will set the provided Big Number object to the corresponding
  value. The caller needs to make sure that the "out" BigNumber parameter
  is properly initialized.

  @param[in]  EcGroup   EC group object.
  @param[out] BnOrder   Group prime number.

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcGroupGetOrder (
  IN VOID   *EcGroup,
  OUT VOID  *BnOrder
  )
{
  return EC_GROUP_get_order (EcGroup, BnOrder, NULL) ?
         EFI_SUCCESS : EFI_PROTOCOL_ERROR;
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
  EC_GROUP_free (EcGroup);
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
  return EC_POINT_new (EcGroup);
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
  if (Clear) {
    EC_POINT_clear_free (EcPoint);
  } else {
    EC_POINT_free (EcPoint);
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
  @param[in]  BnCtx      BN context, created with BigNumNewContext().

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcPointGetAffineCoordinates (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint,
  OUT VOID       *BnX,
  OUT VOID       *BnY,
  IN VOID        *BnCtx
  )
{
  return EC_POINT_get_affine_coordinates (EcGroup, EcPoint, BnX, BnY, BnCtx) ?
         EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Set EC point affine (x,y) coordinates.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC point object.
  @param[in]  BnX        X coordinate.
  @param[in]  BnY        Y coordinate.
  @param[in]  BnCtx      BN context, created with BigNumNewContext().

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcPointSetAffineCoordinates (
  IN CONST VOID  *EcGroup,
  IN VOID        *EcPoint,
  IN CONST VOID  *BnX,
  IN CONST VOID  *BnY,
  IN VOID        *BnCtx
  )
{
  return EC_POINT_set_affine_coordinates (EcGroup, EcPoint, BnX, BnY, BnCtx) ?
         EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  EC Point addition. EcPointResult = EcPointA + EcPointB.

  @param[in]  EcGroup          EC group object.
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPointA         EC Point.
  @param[in]  EcPointB         EC Point.
  @param[in]  BnCtx            BN context, created with BigNumNewContext().

  @retval EFI_SUCCESS        On success
  @retval EFI_PROTOCOL_ERROR On failure
**/
EFI_STATUS
EFIAPI
EcPointAdd (
  IN CONST VOID  *EcGroup,
  OUT VOID       *EcPointResult,
  IN CONST VOID  *EcPointA,
  IN CONST VOID  *EcPointB,
  IN VOID        *BnCtx
  )
{
  return EC_POINT_add (EcGroup, EcPointResult, EcPointA, EcPointB, BnCtx) ?
         EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Variable EC point multiplication. EcPointResult = EcPoint * BnPScalar.

  @param[in]  EcGroup          EC group object.
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPoint          EC Point.
  @param[in]  BnPScalar        P Scalar.
  @param[in]  BnCtx            BN context, created with BigNumNewContext().

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcPointMul (
  IN CONST VOID  *EcGroup,
  OUT VOID       *EcPointResult,
  IN CONST VOID  *EcPoint,
  IN CONST VOID  *BnPScalar,
  IN VOID        *BnCtx
  )
{
  return EC_POINT_mul (EcGroup, EcPointResult, NULL, EcPoint, BnPScalar, BnCtx) ?
         EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Calculate the inverse of the supplied EC point.

  @param[in]     EcGroup   EC group object.
  @param[in,out] EcPoint   EC point to invert.
  @param[in]     BnCtx     BN context, created with BigNumNewContext().

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcPointInvert (
  IN CONST VOID  *EcGroup,
  IN OUT VOID    *EcPoint,
  IN VOID        *BnCtx
  )
{
  return EC_POINT_invert (EcGroup, EcPoint, BnCtx) ?
         EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Check if the supplied point is on EC curve.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPoint   EC point to check.
  @param[in]  BnCtx     BN context, created with BigNumNewContext().

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
  return EC_POINT_is_on_curve (EcGroup, EcPoint, BnCtx) == 1;
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
  return EC_POINT_is_at_infinity (EcGroup, EcPoint) == 1;
}

/**
  Check if EC points are equal.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPointA  EC point A.
  @param[in]  EcPointB  EC point B.
  @param[in]  BnCtx     BN context, created with BigNumNewContext().

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
  return EC_POINT_cmp (EcGroup, EcPointA, EcPointB, BnCtx) == 0;
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
  @param[in]  BnCtx      BN context, created with BigNumNewContext().

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcPointSetCompressedCoordinates (
  IN CONST VOID  *EcGroup,
  IN VOID        *EcPoint,
  IN CONST VOID  *BnX,
  IN UINT8       YBit,
  IN VOID        *BnCtx
  )
{
  return EC_POINT_set_compressed_coordinates (EcGroup, EcPoint, BnX, YBit, BnCtx) ?
         EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Generate a key using ECDH algorithm. Please note, this function uses
  pseudo random number generator. The caller must make sure RandomSeed()
  funtion was properly called before.

  @param[in]  Group    Identifying number for the ECC group (IANA "Group
                       Description" attribute registrty for RFC 2409).
  @param[out] PKey     Pointer to an object that will hold the ECDH key.

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcDhGenKey (
  IN UINTN  Group,
  OUT VOID  **PKey
  )
{
  EVP_PKEY      *Params;
  EC_KEY        *EcParams;
  EVP_PKEY_CTX  *Kctx;
  EFI_STATUS    Status;
  INT32         Nid;

  Params   = NULL;
  EcParams = NULL;
  Kctx     = NULL;
  Status   = EFI_PROTOCOL_ERROR;
  Nid      = GroupToNid (Group);

  if (Nid < 0) {
    return EFI_UNSUPPORTED;
  }

  EcParams = EC_KEY_new_by_curve_name (Nid);
  if (EcParams == NULL) {
    goto fail;
  }

  EC_KEY_set_asn1_flag (EcParams, OPENSSL_EC_NAMED_CURVE);
  Params = EVP_PKEY_new ();
  if ((Params == NULL) || (EVP_PKEY_set1_EC_KEY (Params, EcParams) != 1)) {
    goto fail;
  }

  Kctx = EVP_PKEY_CTX_new (Params, NULL);
  if (Kctx == NULL) {
    goto fail;
  }

  if (EVP_PKEY_keygen_init (Kctx) != 1) {
    goto fail;
  }

  // Assume RAND_seed was called
  if (EVP_PKEY_keygen (Kctx, (EVP_PKEY **)PKey) != 1) {
    goto fail;
  }

  Status = EFI_SUCCESS;

fail:
  EC_KEY_free (EcParams);
  EVP_PKEY_free (Params);
  EVP_PKEY_CTX_free (Kctx);

  return Status;
}

/**
  Free ECDH Key object previously created by EcDhGenKey().

  @param[in] PKey  ECDH Key.
**/
VOID
EFIAPI
EcDhKeyFree (
  IN VOID  *PKey
  )
{
  EVP_PKEY_free (PKey);
}

/**
  Get the public key EC point. The provided EC point's coordinates will
  be set accordingly.

  @param[in]  PKey     ECDH Key object.
  @param[out] EcPoint  Properly initialized EC Point to hold the public key.

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcDhGetPubKey (
  IN VOID   *PKey,
  OUT VOID  *EcPoint
  )
{
  EC_KEY          *EcKey;
  const EC_POINT  *Pubkey;
  EFI_STATUS      Status;

  Status = EFI_PROTOCOL_ERROR;

  if (EcPoint == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EcKey = EVP_PKEY_get1_EC_KEY (PKey);
  if (EcKey == NULL) {
    return EFI_PROTOCOL_ERROR;
  }

  Pubkey = EC_KEY_get0_public_key (EcKey);
  if (Pubkey == NULL) {
    goto out;
  }

  if (EC_POINT_copy (EcPoint, Pubkey) == 0) {
    goto out;
  }

  Status = EFI_SUCCESS;
out:
  EC_KEY_free (EcKey);
  return Status;
}

/**
  Derive ECDH secret.

  @param[in]  PKey           ECDH Key object.
  @param[in]  Group          Identifying number for the ECC group (IANA "Group
                             Description" attribute registrty for RFC 2409).
  @param[in]  EcPointPublic  Peer public key.
  @param[out] SecretSize     On success, holds secret size.
  @param[out] Secret         On success, holds the derived secret.
                             Should be freed by caller using FreePool()
                             function.

  @retval EFI_SUCCESS        On success.
  @retval EFI_PROTOCOL_ERROR On failure.
**/
EFI_STATUS
EFIAPI
EcDhDeriveSecret (
  IN VOID    *PKey,
  IN UINT8   Group,
  IN VOID    *EcPointPublic,
  OUT UINTN  *SecretSize,
  OUT UINT8  **Secret
  )
{
  EVP_PKEY_CTX  *Ctx;
  EVP_PKEY      *PeerKey;
  EC_KEY        *EcKey;
  INT32         Nid;
  UINTN         Len;
  UINT8         *Buf;
  EFI_STATUS    Status;

  Ctx     = NULL;
  PeerKey = NULL;
  EcKey   = NULL;
  Nid     = GroupToNid (Group);
  Status  = EFI_PROTOCOL_ERROR;

  if (Nid < 0) {
    return EFI_UNSUPPORTED;
  }

  if ((Secret == NULL) || (SecretSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  EcKey = EC_KEY_new_by_curve_name (Nid);
  if ((EcKey == NULL) || (EC_KEY_set_public_key (EcKey, EcPointPublic) != 1)) {
    goto fail;
  }

  PeerKey = EVP_PKEY_new ();
  if ((PeerKey == NULL) || (EVP_PKEY_set1_EC_KEY (PeerKey, EcKey) != 1)) {
    goto fail;
  }

  Ctx = EVP_PKEY_CTX_new (PKey, NULL);
  if ((Ctx == NULL) || (EVP_PKEY_derive_init (Ctx) != 1) ||
      (EVP_PKEY_derive_set_peer (Ctx, PeerKey) != 1) ||
      (EVP_PKEY_derive (Ctx, NULL, &Len) != 1))
  {
    goto fail;
  }

  Buf = AllocatePool (Len);
  if (!Buf) {
    goto fail;
  }

  if (EVP_PKEY_derive (Ctx, Buf, &Len) != 1) {
    FreePool (Buf);
    goto fail;
  }

  *SecretSize = Len;
  *Secret     = Buf;
  Status      = EFI_SUCCESS;

fail:
  EC_KEY_free (EcKey);
  EVP_PKEY_CTX_free (Ctx);
  EVP_PKEY_free (PeerKey);

  return Status;
}
