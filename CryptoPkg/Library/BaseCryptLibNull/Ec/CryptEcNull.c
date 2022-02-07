/** @file
  Elliptic Curve and ECDH API implementation based on OpenSSL

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>

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
  ASSERT (FALSE);
  return NULL;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
  return NULL;
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
