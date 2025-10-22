/** @file
  Elliptic Curve and ECDH API implementation based on OpenSSL

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/objects.h>
#include <openssl/evp.h>
#include <openssl/core_names.h>

typedef struct {
  INT32     Nid;
  EVP_PKEY  *PKey;
} MlDsaCtx;

/**
  Return the Nid of certain SLH type.

  @param[in]  CryptoNid   Identifying number for the SLH (Defined in
                          BaseCryptLib.h).

  @retval !=-1    On success.
  @retval -1      SLH type not supported.
**/
STATIC
INT32
CryptoNidToOpensslNid (
  IN UINTN  CryptoNid
  )
{
  INT32  Nid;

  switch (CryptoNid) {
    case CRYPTO_NID_ML_DSA_44:
      Nid = EVP_PKEY_ML_DSA_44;
      break;
    case CRYPTO_NID_ML_DSA_65:
      Nid = EVP_PKEY_ML_DSA_65;
      break;
    case CRYPTO_NID_ML_DSA_87:
      Nid = EVP_PKEY_ML_DSA_87;
      break;
    default:
      return -1;
  }

  return Nid;
}

STATIC
BOOLEAN
NidToKeySize (
  IN INT32    OpensslNid,
  IN BOOLEAN  IsPrivate,
  IN UINTN    *KeySize
  )
{
  switch (OpensslNid) {
    case EVP_PKEY_ML_DSA_44:
      *KeySize = IsPrivate? 64:1312;
      break;
    case EVP_PKEY_ML_DSA_65:
      *KeySize = IsPrivate? 96:1952;
      break;
    case EVP_PKEY_ML_DSA_87:
      *KeySize = IsPrivate? 128:2592;
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

STATIC
BOOLEAN
NidToSigSize (
  IN INT32  OpensslNid,
  IN UINTN  *SigSize
  )
{
  switch (OpensslNid) {
    case EVP_PKEY_ML_DSA_44:
      *SigSize = 2420;
      break;
    case EVP_PKEY_ML_DSA_65:
      *SigSize = 3309;
      break;
    case EVP_PKEY_ML_DSA_87:
      *SigSize = 4627;
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

/**
  Allocates and Initializes one Elliptic Curve Context for subsequent use
  with the NID.

  @param[in]  Nid   Identifying number for the ECC curve (Defined in
                    BaseCryptLib.h).
  @return     Pointer to the Elliptic Curve Context that has been initialized.
              If the allocations fails, MlDsaNewByNid() returns NULL.
**/
VOID *
EFIAPI
MlDsaNewByNid (
  IN UINTN  Nid
  )
{
  INT32        OpenSslNid;
  MlDsaCtx    *Ctx;

  OpenSslNid = CryptoNidToOpensslNid (Nid);
  if (OpenSslNid < 0) {
    return NULL;
  }
  Ctx = (MlDsaCtx *)AllocateZeroPool (sizeof (MlDsaCtx));
  if (Ctx == NULL) { 
    return NULL;
  }

  Ctx->Nid  = OpenSslNid;
  Ctx->PKey = NULL;
  return (VOID *)Ctx;
}

/**
  Release the specified EC context.

  @param[in]  SlhContext  Pointer to the EC context to be released.
**/
VOID
EFIAPI
MlDsaFree (
  IN  VOID  *SlhContext
  )
{
  MlDsaCtx *Ctx;
  if (SlhContext != NULL) {
    Ctx = (MlDsaCtx *)SlhContext;
    EVP_PKEY_free (Ctx->PKey);
    FreePool (Ctx);
  }
}

/**
  Gets the public key component from the established EC context.
  The Slh context should be correctly initialized by MlDsaNewByNid, and successfully
  generate key pair from SlhGenerateKey().
  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.
  @param[in, out]  SlhContext      Pointer to EC context being set.
  @param[out]      PublicKey      Pointer to t buffer to receive generated public X,Y.
  @param[in, out]  PublicKeySize  On input, the size of Public buffer in bytes.
                                  On output, the size of data returned in Public buffer in bytes.
  @retval  TRUE   EC key component was retrieved successfully.
  @retval  FALSE  Invalid EC key component.
**/
BOOLEAN
EFIAPI
MlDsaGetPubKey (
  IN OUT  VOID   *SlhContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  MlDsaCtx *Ctx;
  UINTN    KeySize;

  if ((SlhContext == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  if ((PublicKey == NULL) && (*PublicKeySize != 0)) {
    return FALSE;
  }

  Ctx = (MlDsaCtx *)SlhContext;
  if ((Ctx->PKey == NULL) || !NidToKeySize (Ctx->Nid, FALSE, &KeySize)) {
    return FALSE;
  }
  if (*PublicKeySize < KeySize) {
    *PublicKeySize = KeySize;
    return FALSE;
  }

  *PublicKeySize = KeySize;
  if (EVP_PKEY_get_raw_public_key (Ctx->PKey, PublicKey, PublicKeySize) != 1) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
EFIAPI
MlDsaGetPrivKey (
  IN OUT  VOID   *SlhContext,
  OUT     UINT8  *PrivateKey,
  IN OUT  UINTN  *PrivateKeySize
  )
{
  MlDsaCtx *Ctx;
  UINTN    KeySize;

  if ((SlhContext == NULL) || (PrivateKeySize == NULL)) {
    return FALSE;
  }

  if ((PrivateKey == NULL) && (*PrivateKeySize != 0)) {
    return FALSE;
  }

  Ctx = (MlDsaCtx *)SlhContext;
  if ((Ctx->PKey == NULL) || !NidToKeySize (Ctx->Nid, TRUE, &KeySize)) {
    return FALSE;
  }
  if (*PrivateKeySize < KeySize) {
    *PrivateKeySize = KeySize;
    return FALSE;
  }

  *PrivateKeySize = KeySize;
  if (EVP_PKEY_get_raw_private_key (Ctx->PKey, PrivateKey, PrivateKeySize) != 1) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
EFIAPI
MlDsaSetPubKey (
  IN OUT  VOID   *SlhContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  MlDsaCtx *Ctx;
  UINTN     KeySize;
  EVP_PKEY  *TempKey;

  if ((SlhContext == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  if ((PublicKey == NULL) && (*PublicKeySize != 0)) {
    return FALSE;
  }

  Ctx = (MlDsaCtx *)SlhContext;
  if (!NidToKeySize (Ctx->Nid, FALSE, &KeySize)) {
    return FALSE;
  }
  if (*PublicKeySize != KeySize) {
    return FALSE;
  }

  TempKey = EVP_PKEY_new_raw_public_key (Ctx->Nid, NULL, PublicKey, (UINT32)*PublicKeySize);
  if (TempKey == NULL) {
    return FALSE;
  }

  if (Ctx->PKey != NULL) {
    EVP_PKEY_free (Ctx->PKey);
  }
  Ctx->PKey = TempKey;
  return TRUE;
}

BOOLEAN
EFIAPI
MlDsaSetPrivKey (
  IN OUT  VOID   *SlhContext,
  OUT     UINT8  *PrivateKey,
  IN OUT  UINTN  *PrivateKeySize
  )
{
  MlDsaCtx *Ctx;
  UINTN     KeySize;
  EVP_PKEY  *TempKey;

  if ((SlhContext == NULL) || (PrivateKeySize == NULL)) {
    return FALSE;
  }

  if ((PrivateKey == NULL) && (*PrivateKeySize != 0)) {
    return FALSE;
  }

  Ctx = (MlDsaCtx *)SlhContext;
  if (!NidToKeySize (Ctx->Nid, TRUE, &KeySize)) {
    return FALSE;
  }
  if (*PrivateKeySize != KeySize) {
    return FALSE;
  }

  TempKey = EVP_PKEY_new_raw_private_key (Ctx->Nid, NULL, PrivateKey, (UINT32)*PrivateKeySize);
  if (TempKey == NULL) {
    return FALSE;
  }

  if (Ctx->PKey != NULL) {
    EVP_PKEY_free (Ctx->PKey);
  }
  Ctx->PKey = TempKey;
  return TRUE;
}

/**
  Carries out the EC-DSA signature.

  This function carries out the EC-DSA signature.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If SlhContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If HashSize need match the HashNid. HashNid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.
  If SigSize is large enough but Signature is NULL, then return FALSE.

  For P-256, the SigSize is 64. First 32-byte is R, Second 32-byte is S.
  For P-384, the SigSize is 96. First 48-byte is R, Second 48-byte is S.
  For P-521, the SigSize is 132. First 66-byte is R, Second 66-byte is S.

  @param[in]       SlhContext    Pointer to EC context for signature generation.
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
MlDsaSign (
  IN  VOID         *SlhContext,
  IN  CONST UINT8  *Context,
  IN  UINTN        ContextSize,
  IN  CONST UINT8  *Message,
  IN  UINTN        MessageSize,
  IN  UINT8        *Signature,
  IN  UINTN        *SigSize
  )
{
  MlDsaCtx   *Ctx;
  UINTN       SigSizeRequired;
  EVP_MD_CTX  *MdCtx;
  OSSL_PARAM  Params[2];
  BOOLEAN     Result;

  if ((SlhContext == NULL) || (Message == NULL)) {
    return FALSE;
  }

  if ((*SigSize > INT_MAX) || (*SigSize == 0)) {
    return FALSE;
  }

  if ((ContextSize > 0) && (Context == NULL)) {
    return FALSE;
  }

  Ctx = (MlDsaCtx *)SlhContext;
  if ((Ctx->PKey == NULL) || !NidToSigSize (Ctx->Nid, &SigSizeRequired)) {
    return FALSE;
  }
  if ((Signature == NULL) || (*SigSize < SigSizeRequired)) {
    *SigSize = SigSizeRequired;
    return FALSE;
  }
  *SigSize = SigSizeRequired;

  Result = FALSE;
  MdCtx  = EVP_MD_CTX_new ();
  if (MdCtx == NULL) {
    return FALSE;
  }

  if (EVP_DigestSignInit (MdCtx, NULL, NULL, NULL, Ctx->PKey) != 1) {
    goto Exit;
  }

  if (ContextSize > 0) {
    Params[0] = OSSL_PARAM_construct_octet_string (
                    OSSL_SIGNATURE_PARAM_CONTEXT_STRING,
                    (VOID *)Context,
                    ContextSize
                    );
    Params[1] = OSSL_PARAM_construct_end ();
    if (EVP_PKEY_CTX_set_params (EVP_MD_CTX_pkey_ctx (MdCtx), Params) != 1) {
      goto Exit;
    }
  }

  if (EVP_DigestSign (MdCtx, Signature, SigSize, Message, (UINT32)MessageSize) != 1) {
    goto Exit;
  }
  Result = TRUE;

Exit:
  EVP_MD_CTX_free (MdCtx);
  return Result;
}

/**
  Verifies the EC-DSA signature.

  If SlhContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize need match the HashNid. HashNid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.

  For P-256, the SigSize is 64. First 32-byte is R, Second 32-byte is S.
  For P-384, the SigSize is 96. First 48-byte is R, Second 48-byte is S.
  For P-521, the SigSize is 132. First 66-byte is R, Second 66-byte is S.

  @param[in]  SlhContext    Pointer to EC context for signature verification.
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
MlDsaVerify (
  IN  VOID         *SlhContext,
  IN  CONST UINT8  *Context,
  IN  UINTN        ContextSize,
  IN  CONST UINT8  *Message,
  IN  UINTN        MessageSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  MlDsaCtx   *Ctx;
  UINTN       SigSizeRequired;
  EVP_MD_CTX  *MdCtx;
  OSSL_PARAM  Params[2];
  BOOLEAN     Result;

  if ((SlhContext == NULL) || (Message == NULL) || (Signature == NULL)) {
    return FALSE;
  }

  if ((SigSize > INT_MAX) || (SigSize == 0)) {
    return FALSE;
  }

  if ((ContextSize > 0) && (Context == NULL)) {
    return FALSE;
  }

  Ctx = (MlDsaCtx *)SlhContext;
  if ((Ctx->PKey == NULL) || !NidToSigSize (Ctx->Nid, &SigSizeRequired)) {
    return FALSE;
  }
  if (SigSize != SigSizeRequired) {
    return FALSE;
  }

  Result = FALSE;
  MdCtx  = EVP_MD_CTX_new ();
  if (MdCtx == NULL) {
    return FALSE;
  }

  if (EVP_DigestVerifyInit (MdCtx, NULL, NULL, NULL, Ctx->PKey) != 1) {
    goto Exit;
  }

  if (ContextSize > 0) {
    Params[0] = OSSL_PARAM_construct_octet_string (
                    OSSL_SIGNATURE_PARAM_CONTEXT_STRING,
                    (VOID *)Context,
                    ContextSize
                    );
    Params[1] = OSSL_PARAM_construct_end ();
    if (EVP_PKEY_CTX_set_params (EVP_MD_CTX_pkey_ctx (MdCtx), Params) != 1) {
      goto Exit;
    }
  }

  if (EVP_DigestVerify (MdCtx, Signature, (UINT32)SigSize, Message, (UINT32)MessageSize) != 1) {
    goto Exit;
  }
  Result = TRUE;

Exit:
  EVP_MD_CTX_free (MdCtx);
  return Result;
}
