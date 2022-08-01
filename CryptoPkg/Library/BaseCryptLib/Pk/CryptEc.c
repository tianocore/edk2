/** @file
  Elliptic Curve Wrapper Implementation over OpenSSL.

  RFC 8422 - Elliptic Curve Cryptography (ECC) Cipher Suites
  FIPS 186-4 - Digital Signature Standard (DSS)

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/objects.h>

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
  EC_KEY    *EcKey;
  EC_GROUP  *Group;
  BOOLEAN   RetVal;
  INT32     OpenSslNid;

  EcKey = EC_KEY_new ();
  if (EcKey == NULL) {
    return NULL;
  }

  switch (Nid) {
    case CRYPTO_NID_SECP256R1:
      OpenSslNid = NID_X9_62_prime256v1;
      break;
    case CRYPTO_NID_SECP384R1:
      OpenSslNid = NID_secp384r1;
      break;
    case CRYPTO_NID_SECP521R1:
      OpenSslNid = NID_secp521r1;
      break;
    default:
      return NULL;
  }

  Group = EC_GROUP_new_by_curve_name (OpenSslNid);
  if (Group == NULL) {
    return NULL;
  }

  RetVal = (BOOLEAN)EC_KEY_set_group (EcKey, Group);
  EC_GROUP_free (Group);
  if (!RetVal) {
    return NULL;
  }

  return (VOID *)EcKey;
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
  EC_KEY_free ((EC_KEY *)EcContext);
}

/**
  Sets the public key component into the established EC context.

  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.

  @param[in, out]  EcContext      Pointer to EC context being set.
  @param[in]       Public         Pointer to the buffer to receive generated public X,Y.
  @param[in]       PublicSize     The size of Public buffer in bytes.

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
  EC_KEY          *EcKey;
  CONST EC_GROUP  *Group;
  BOOLEAN         RetVal;
  BIGNUM          *BnX;
  BIGNUM          *BnY;
  EC_POINT        *Point;
  INT32           OpenSslNid;
  UINTN           HalfSize;

  if ((EcContext == NULL) || (PublicKey == NULL)) {
    return FALSE;
  }

  EcKey      = (EC_KEY *)EcContext;
  OpenSslNid = EC_GROUP_get_curve_name (EC_KEY_get0_group (EcKey));
  switch (OpenSslNid) {
    case NID_X9_62_prime256v1:
      HalfSize = 32;
      break;
    case NID_secp384r1:
      HalfSize = 48;
      break;
    case NID_secp521r1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (PublicKeySize != HalfSize * 2) {
    return FALSE;
  }

  Group = EC_KEY_get0_group (EcKey);
  Point = NULL;

  BnX = BN_bin2bn (PublicKey, (UINT32)HalfSize, NULL);
  BnY = BN_bin2bn (PublicKey + HalfSize, (UINT32)HalfSize, NULL);
  if ((BnX == NULL) || (BnY == NULL)) {
    RetVal = FALSE;
    goto Done;
  }

  Point = EC_POINT_new (Group);
  if (Point == NULL) {
    RetVal = FALSE;
    goto Done;
  }

  RetVal = (BOOLEAN)EC_POINT_set_affine_coordinates (Group, Point, BnX, BnY, NULL);
  if (!RetVal) {
    goto Done;
  }

  RetVal = (BOOLEAN)EC_KEY_set_public_key (EcKey, Point);
  if (!RetVal) {
    goto Done;
  }

  RetVal = TRUE;

Done:
  if (BnX != NULL) {
    BN_free (BnX);
  }

  if (BnY != NULL) {
    BN_free (BnY);
  }

  if (Point != NULL) {
    EC_POINT_free (Point);
  }

  return RetVal;
}

/**
  Gets the public key component from the established EC context.

  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.

  @param[in, out]  EcContext      Pointer to EC context being set.
  @param[out]      Public         Pointer to the buffer to receive generated public X,Y.
  @param[in, out]  PublicSize     On input, the size of Public buffer in bytes.
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
  EC_KEY          *EcKey;
  CONST EC_GROUP  *Group;
  BOOLEAN         RetVal;
  CONST EC_POINT  *EcPoint;
  BIGNUM          *BnX;
  BIGNUM          *BnY;
  INT32           OpenSslNid;
  UINTN           HalfSize;
  INTN            XSize;
  INTN            YSize;

  if ((EcContext == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  if ((PublicKey == NULL) && (*PublicKeySize != 0)) {
    return FALSE;
  }

  EcKey = (EC_KEY *)EcContext;

  OpenSslNid = EC_GROUP_get_curve_name (EC_KEY_get0_group (EcKey));
  switch (OpenSslNid) {
    case NID_X9_62_prime256v1:
      HalfSize = 32;
      break;
    case NID_secp384r1:
      HalfSize = 48;
      break;
    case NID_secp521r1:
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

  Group   = EC_KEY_get0_group (EcKey);
  EcPoint = EC_KEY_get0_public_key (EcKey);
  if (EcPoint == NULL) {
    return FALSE;
  }

  BnX = BN_new ();
  BnY = BN_new ();
  if ((BnX == NULL) || (BnY == NULL)) {
    RetVal = FALSE;
    goto Done;
  }

  RetVal = (BOOLEAN)EC_POINT_get_affine_coordinates (Group, EcPoint, BnX, BnY, NULL);
  if (!RetVal) {
    goto Done;
  }

  XSize = BN_num_bytes (BnX);
  YSize = BN_num_bytes (BnY);
  if ((XSize <= 0) || (YSize <= 0)) {
    RetVal = FALSE;
    goto Done;
  }

  ASSERT ((UINTN)XSize <= HalfSize && (UINTN)YSize <= HalfSize);

  if (PublicKey != NULL) {
    ZeroMem (PublicKey, *PublicKeySize);
    BN_bn2bin (BnX, &PublicKey[0 + HalfSize - XSize]);
    BN_bn2bin (BnY, &PublicKey[HalfSize + HalfSize - YSize]);
  }

  RetVal = TRUE;

Done:
  if (BnX != NULL) {
    BN_free (BnX);
  }

  if (BnY != NULL) {
    BN_free (BnY);
  }

  return RetVal;
}

/**
  Validates key components of EC context.
  NOTE: This function performs integrity checks on all the EC key material, so
        the EC key structure must contain all the private key data.

  If EcContext is NULL, then return FALSE.

  @param[in]  EcContext  Pointer to EC context to check.

  @retval  TRUE   EC key components are valid.
  @retval  FALSE  EC key components are not valid.

**/
BOOLEAN
EFIAPI
EcCheckKey (
  IN  VOID  *EcContext
  )
{
  EC_KEY   *EcKey;
  BOOLEAN  RetVal;

  if (EcContext == NULL) {
    return FALSE;
  }

  EcKey = (EC_KEY *)EcContext;

  RetVal = (BOOLEAN)EC_KEY_check_key (EcKey);
  if (!RetVal) {
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
  EC_KEY          *EcKey;
  CONST EC_GROUP  *Group;
  BOOLEAN         RetVal;
  CONST EC_POINT  *EcPoint;
  BIGNUM          *BnX;
  BIGNUM          *BnY;
  INT32           OpenSslNid;
  UINTN           HalfSize;
  INTN            XSize;
  INTN            YSize;

  if ((EcContext == NULL) || (PublicSize == NULL)) {
    return FALSE;
  }

  if ((Public == NULL) && (*PublicSize != 0)) {
    return FALSE;
  }

  EcKey  = (EC_KEY *)EcContext;
  RetVal = (BOOLEAN)EC_KEY_generate_key (EcKey);
  if (!RetVal) {
    return FALSE;
  }

  OpenSslNid = EC_GROUP_get_curve_name (EC_KEY_get0_group (EcKey));
  switch (OpenSslNid) {
    case NID_X9_62_prime256v1:
      HalfSize = 32;
      break;
    case NID_secp384r1:
      HalfSize = 48;
      break;
    case NID_secp521r1:
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

  Group   = EC_KEY_get0_group (EcKey);
  EcPoint = EC_KEY_get0_public_key (EcKey);
  if (EcPoint == NULL) {
    return FALSE;
  }

  BnX = BN_new ();
  BnY = BN_new ();
  if ((BnX == NULL) || (BnY == NULL)) {
    RetVal = FALSE;
    goto Done;
  }

  RetVal = (BOOLEAN)EC_POINT_get_affine_coordinates (Group, EcPoint, BnX, BnY, NULL);
  if (!RetVal) {
    goto Done;
  }

  XSize = BN_num_bytes (BnX);
  YSize = BN_num_bytes (BnY);
  if ((XSize <= 0) || (YSize <= 0)) {
    RetVal = FALSE;
    goto Done;
  }

  ASSERT ((UINTN)XSize <= HalfSize && (UINTN)YSize <= HalfSize);

  if (Public != NULL) {
    ZeroMem (Public, *PublicSize);
    BN_bn2bin (BnX, &Public[0 + HalfSize - XSize]);
    BN_bn2bin (BnY, &Public[HalfSize + HalfSize - YSize]);
  }

  RetVal = TRUE;

Done:
  if (BnX != NULL) {
    BN_free (BnX);
  }

  if (BnY != NULL) {
    BN_free (BnY);
  }

  return RetVal;
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
  EC_KEY          *EcKey;
  CONST EC_GROUP  *Group;
  BOOLEAN         RetVal;
  BIGNUM          *BnX;
  BIGNUM          *BnY;
  EC_POINT        *Point;
  INT32           OpenSslNid;
  UINTN           HalfSize;
  INTN            Size;

  if ((EcContext == NULL) || (PeerPublic == NULL) || (KeySize == NULL) || (Key == NULL)) {
    return FALSE;
  }

  if (PeerPublicSize > INT_MAX) {
    return FALSE;
  }

  EcKey      = (EC_KEY *)EcContext;
  OpenSslNid = EC_GROUP_get_curve_name (EC_KEY_get0_group (EcKey));
  switch (OpenSslNid) {
    case NID_X9_62_prime256v1:
      HalfSize = 32;
      break;
    case NID_secp384r1:
      HalfSize = 48;
      break;
    case NID_secp521r1:
      HalfSize = 66;
      break;
    default:
      return FALSE;
  }

  if (PeerPublicSize != HalfSize * 2) {
    return FALSE;
  }

  Group = EC_KEY_get0_group (EcKey);
  Point = NULL;

  BnX = BN_bin2bn (PeerPublic, (UINT32)HalfSize, NULL);
  BnY = BN_bin2bn (PeerPublic + HalfSize, (UINT32)HalfSize, NULL);
  if ((BnX == NULL) || (BnY == NULL)) {
    RetVal = FALSE;
    goto Done;
  }

  Point = EC_POINT_new (Group);
  if (Point == NULL) {
    RetVal = FALSE;
    goto Done;
  }

  RetVal = (BOOLEAN)EC_POINT_set_affine_coordinates (Group, Point, BnX, BnY, NULL);
  if (!RetVal) {
    goto Done;
  }

  Size = ECDH_compute_key (Key, *KeySize, Point, EcKey, NULL);
  if (Size < 0) {
    RetVal = FALSE;
    goto Done;
  }

  if (*KeySize < (UINTN)Size) {
    *KeySize = Size;
    RetVal   = FALSE;
    goto Done;
  }

  *KeySize = Size;

  RetVal = TRUE;

Done:
  if (BnX != NULL) {
    BN_free (BnX);
  }

  if (BnY != NULL) {
    BN_free (BnY);
  }

  if (Point != NULL) {
    EC_POINT_free (Point);
  }

  return RetVal;
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
  EC_KEY     *EcKey;
  ECDSA_SIG  *EcDsaSig;
  INT32      OpenSslNid;
  UINT8      HalfSize;
  BIGNUM     *R;
  BIGNUM     *S;
  INTN       RSize;
  INTN       SSize;

  if ((EcContext == NULL) || (MessageHash == NULL)) {
    return FALSE;
  }

  if (Signature == NULL) {
    return FALSE;
  }

  EcKey      = (EC_KEY *)EcContext;
  OpenSslNid = EC_GROUP_get_curve_name (EC_KEY_get0_group (EcKey));
  switch (OpenSslNid) {
    case NID_X9_62_prime256v1:
      HalfSize = 32;
      break;
    case NID_secp384r1:
      HalfSize = 48;
      break;
    case NID_secp521r1:
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

  EcDsaSig = ECDSA_do_sign (
               MessageHash,
               (UINT32)HashSize,
               (EC_KEY *)EcContext
               );
  if (EcDsaSig == NULL) {
    return FALSE;
  }

  ECDSA_SIG_get0 (EcDsaSig, (CONST BIGNUM **)&R, (CONST BIGNUM **)&S);

  RSize = BN_num_bytes (R);
  SSize = BN_num_bytes (S);
  if ((RSize <= 0) || (SSize <= 0)) {
    ECDSA_SIG_free (EcDsaSig);
    return FALSE;
  }

  ASSERT ((UINTN)RSize <= HalfSize && (UINTN)SSize <= HalfSize);

  BN_bn2bin (R, &Signature[0 + HalfSize - RSize]);
  BN_bn2bin (S, &Signature[HalfSize + HalfSize - SSize]);

  ECDSA_SIG_free (EcDsaSig);

  return TRUE;
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
  INT32      Result;
  EC_KEY     *EcKey;
  ECDSA_SIG  *EcDsaSig;
  INT32      OpenSslNid;
  UINT8      HalfSize;
  BIGNUM     *R;
  BIGNUM     *S;

  if ((EcContext == NULL) || (MessageHash == NULL) || (Signature == NULL)) {
    return FALSE;
  }

  if ((SigSize > INT_MAX) || (SigSize == 0)) {
    return FALSE;
  }

  EcKey      = (EC_KEY *)EcContext;
  OpenSslNid = EC_GROUP_get_curve_name (EC_KEY_get0_group (EcKey));
  switch (OpenSslNid) {
    case NID_X9_62_prime256v1:
      HalfSize = 32;
      break;
    case NID_secp384r1:
      HalfSize = 48;
      break;
    case NID_secp521r1:
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

  EcDsaSig = ECDSA_SIG_new ();
  if (EcDsaSig == NULL) {
    ECDSA_SIG_free (EcDsaSig);
    return FALSE;
  }

  R = BN_bin2bn (Signature, (UINT32)HalfSize, NULL);
  S = BN_bin2bn (Signature + HalfSize, (UINT32)HalfSize, NULL);
  if ((R == NULL) || (S == NULL)) {
    ECDSA_SIG_free (EcDsaSig);
    return FALSE;
  }

  ECDSA_SIG_set0 (EcDsaSig, R, S);

  Result = ECDSA_do_verify (
             MessageHash,
             (UINT32)HashSize,
             EcDsaSig,
             (EC_KEY *)EcContext
             );

  ECDSA_SIG_free (EcDsaSig);

  return (Result == 1);
}
