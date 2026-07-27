/** @file
  EdDSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include "KeyContext.h"
#include <openssl/crypto.h>
#include <openssl/evp.h>

/**
  Get the key size in bytes for an EdDSA curve from its OpenSSL NID.

  This helper function maps OpenSSL curve NIDs to their corresponding key sizes.
  For Ed448, the key size is 57 bytes.

  If the NID is not supported, CoordLen is set to 0 and FALSE is returned.

  @param[in]   Nid        OpenSSL NID of the EdDSA curve (e.g., EVP_PKEY_ED448).
  @param[out]  CoordLen   Pointer to receive the key size in bytes.

  @retval TRUE   Key size retrieved successfully.
  @retval FALSE  Unsupported curve NID.

**/
STATIC
BOOLEAN
OpensslNidToKeySize (
  IN  UINTN  Nid,
  OUT UINTN  *CoordLen
  )
{
  switch (Nid) {
    case EVP_PKEY_ED448:
      *CoordLen = 57;
      break;
    default:
      *CoordLen = 0;
      return FALSE;
  }

  return TRUE;
}

/**
  Convert a Crypto NID to an OpenSSL NID.

  This helper function translates EDK II Crypto library NIDs (e.g., CRYPTO_NID_ED448)
  to their corresponding OpenSSL EVP_PKEY NIDs (e.g., EVP_PKEY_ED448).

  If the Crypto NID is not supported, EVP_PKEY_NONE is returned.

  @param[in]  CryptoNid   EDK II Crypto library NID (e.g., CRYPTO_NID_ED448).

  @retval OpenSSL NID (e.g., EVP_PKEY_ED448) if supported.
  @retval EVP_PKEY_NONE if the Crypto NID is unsupported.

**/
STATIC
INT32
CryptoNidToOpensslNid (
  IN UINTN  CryptoNid
  )
{
  INT32  Nid;

  switch (CryptoNid) {
    case CRYPTO_NID_ED448:
      Nid = EVP_PKEY_ED448;
      break;
    default:
      Nid = EVP_PKEY_NONE;
      break;
  }

  return Nid;
}

/**
  Creates a new EdDSA context by Crypto NID.

  This function allocates and initializes a new EdDSA context for the specified
  curve. The context stores the curve NID and will hold an EVP_PKEY structure
  after a key is set. The caller must call EdDsaFree() to release the context
  when done.

  Before keys can be used for signing or verification, they must be set using
  EdDsaSetPrivKey() or EdDsaSetPubKey().

  If Nid is not a supported EdDSA curve, then return NULL.
  If memory allocation fails, then return NULL.

  @param[in]  Nid   Crypto NID of the EdDSA curve (e.g., CRYPTO_NID_ED448).

  @retval Pointer to new EdDSA context if successful.
  @retval NULL if Nid is unsupported or allocation failed.

**/
VOID *
EFIAPI
EdDsaNewByNid (
  IN UINTN  Nid
  )
{
  KEY_CONTEXT  *Ctx;
  INT32        OpensslNid;

  OpensslNid = CryptoNidToOpensslNid (Nid);
  if (OpensslNid <= EVP_PKEY_NONE) {
    return NULL;
  }

  Ctx = (KEY_CONTEXT *)AllocateZeroPool (sizeof (KEY_CONTEXT));
  if (Ctx == NULL) {
    return NULL;
  }

  Ctx->Nid     = OpensslNid;
  Ctx->EvpPkey = NULL;

  return (VOID *)Ctx;
}

/**
  Frees an EdDSA context and all associated resources.

  This function releases all memory associated with the EdDSA context, including
  the EVP_PKEY structure. After calling this function, the EdDsaContext pointer
  should not be used.

  If EdDsaContext is NULL, then this function returns immediately without action.

  @param[in]  EdDsaContext  Pointer to the EdDSA context to be released.

**/
VOID
EFIAPI
EdDsaFree (
  IN VOID  *EdDsaContext
  )
{
  KEY_CONTEXT  *Ctx;

  if (EdDsaContext == NULL) {
    return;
  }

  Ctx = (KEY_CONTEXT *)EdDsaContext;

  if (Ctx->EvpPkey != NULL) {
    EVP_PKEY_free (Ctx->EvpPkey);
  }

  FreePool (Ctx);

  Ctx = NULL;
}

/**
  Sets the EdDSA private key in the EdDSA context.

  This function imports a raw private key into the EdDSA context. The private key
  must be in raw binary format (not PEM or DER encoded). The key size must match
  the expected size for the curve type (57 bytes for Ed448).

  OpenSSL automatically derives the public key from the private key, so after
  calling this function, both signing and verification operations are possible.

  If EdDsaContext is NULL, then return FALSE.
  If PrivateKey is NULL, then return FALSE.
  If PrivateKeySize is 0, then return FALSE.
  If PrivateKeySize does not match the expected size for the curve, then return FALSE.

  @param[in]  EdDsaContext    Pointer to EdDSA context created by EdDsaNewByNid().
  @param[in]  PrivateKey      Pointer to raw private key bytes.
  @param[in]  PrivateKeySize  Size of the private key in bytes.

  @retval TRUE   EdDSA private key was set successfully.
  @retval FALSE  Invalid parameters or key size mismatch.

**/
BOOLEAN
EFIAPI
EdDsaSetPrivKey (
  IN    VOID   *EdDsaContext,
  IN    UINT8  *PrivateKey,
  IN    UINTN  PrivateKeySize
  )
{
  KEY_CONTEXT  *Ctx;
  UINTN        FinalPrivateKeySize;

  if ((EdDsaContext == NULL) || (PrivateKey == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)EdDsaContext;
  if (Ctx == NULL) {
    return FALSE;
  }

  if (!OpensslNidToKeySize (Ctx->Nid, &FinalPrivateKeySize)) {
    return FALSE;
  }

  if (FinalPrivateKeySize != PrivateKeySize) {
    return FALSE;
  }

  if (Ctx->EvpPkey != NULL) {
    EVP_PKEY_free (Ctx->EvpPkey);
    Ctx->EvpPkey = NULL;
  }

  Ctx->EvpPkey = EVP_PKEY_new_raw_private_key (Ctx->Nid, NULL, PrivateKey, PrivateKeySize);
  if (Ctx->EvpPkey == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Extracts the EdDSA public key from the EdDSA context.

  This function retrieves the public key from the EdDSA context and copies it to
  the provided buffer. The context must have a key set (either via EdDsaSetPrivKey()
  or EdDsaSetPubKey()) before calling this function. OpenSSL automatically derives
  the public key when a private key is set.

  The public key is returned in raw binary format (57 bytes for Ed448).

  If EdDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If the context does not contain a valid key, then return FALSE.
  If PublicKeySize does not match the expected size for the curve, then return FALSE.

  @param[in]   EdDsaContext    Pointer to EdDSA context containing the key.
  @param[out]  PublicKey       Pointer to buffer to receive the public key.
  @param[in]   PublicKeySize   Size of the public key buffer in bytes.
                               Must match the key size for the curve (57 bytes for Ed448).

  @retval TRUE   EdDSA public key extracted successfully.
  @retval FALSE  Invalid parameters or key extraction failed.

**/
BOOLEAN
EFIAPI
EdDsaGeneratePubKey (
  IN   VOID   *EdDsaContext,
  OUT  UINT8  *PublicKey,
  IN   UINTN  PublicKeySize
  )
{
  return FALSE;
}

/**
  Sets the EdDSA public key in the EdDSA context.

  This function imports a raw public key into the EdDSA context. The public key
  must be in raw binary format (not PEM or DER encoded). The key size must match
  the expected size for the curve type (57 bytes for Ed448).

  After setting the public key, the context can be used for signature verification
  but not for signing (which requires the private key).

  If EdDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If PublicKeySize is 0, then return FALSE.
  If PublicKeySize does not match the expected size for the curve, then return FALSE.

  @param[in]  EdDsaContext    Pointer to EdDSA context created by EdDsaNewByNid().
  @param[in]  PublicKey       Pointer to raw public key bytes.
  @param[in]  PublicKeySize   Size of the public key in bytes.

  @retval TRUE   EdDSA public key was set successfully.
  @retval FALSE  Invalid parameters or key size mismatch.

**/
BOOLEAN
EFIAPI
EdDsaSetPubKey (
  IN  VOID         *EdDsaContext,
  IN  CONST UINT8  *PublicKey,
  IN  UINTN        PublicKeySize
  )
{
  KEY_CONTEXT  *Ctx;
  UINTN        FinalPublicKeySize;

  if ((EdDsaContext == NULL) || (PublicKey == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)EdDsaContext;
  if (Ctx == NULL) {
    return FALSE;
  }

  if (!OpensslNidToKeySize (Ctx->Nid, &FinalPublicKeySize)) {
    return FALSE;
  }

  if (FinalPublicKeySize != PublicKeySize) {
    return FALSE;
  }

  if (Ctx->EvpPkey != NULL) {
    EVP_PKEY_free (Ctx->EvpPkey);
    Ctx->EvpPkey = NULL;
  }

  Ctx->EvpPkey = EVP_PKEY_new_raw_public_key (Ctx->Nid, NULL, PublicKey, PublicKeySize);
  if (Ctx->EvpPkey == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Retrieves the EdDSA public key from the EdDSA context.

  This function extracts the public key from the EdDSA context and copies it to
  the provided buffer. The public key is returned in raw binary format.

  The context must have a key set (either via EdDsaSetPrivKey() or EdDsaSetPubKey())
  before calling this function.

  If EdDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If the context does not contain a valid key, then return FALSE.
  If PublicKey buffer is too small, PublicKeySize is updated with required size and return FALSE.

  @param[in]      EdDsaContext    Pointer to EdDSA context containing the key.
  @param[out]     PublicKey       Pointer to buffer to receive the public key.
  @param[in,out]  PublicKeySize   On input, size of PublicKey buffer in bytes.
                                  On output, actual size of public key written.

  @retval TRUE   EdDSA public key retrieved successfully.
  @retval FALSE  Invalid parameters or buffer too small.

**/
BOOLEAN
EFIAPI
EdDsaGetPubKey (
  IN      VOID   *EdDsaContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  KEY_CONTEXT  *Ctx;
  INT32        Result;
  UINTN        FinalPublicKeySize;

  if ((EdDsaContext == NULL) || (PublicKey == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)EdDsaContext;
  if (Ctx == NULL) {
    return FALSE;
  }

  if (!OpensslNidToKeySize (Ctx->Nid, &FinalPublicKeySize)) {
    return FALSE;
  }

  if (*PublicKeySize < FinalPublicKeySize) {
    *PublicKeySize = FinalPublicKeySize;
    return FALSE;
  }

  *PublicKeySize = FinalPublicKeySize;

  Result = EVP_PKEY_get_raw_public_key (Ctx->EvpPkey, PublicKey, PublicKeySize);
  if (Result != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates an EdDSA signature for a given message.

  This function creates an EdDSA signature using the private key stored in the
  EdDSA context. EdDSA uses a 'pure' signature scheme where the entire message
  is processed directly without pre-computing a hash digest.

  For Ed448, an optional context string can be provided for domain separation.
  This allows the same key to be used in different contexts without creating
  security vulnerabilities.

  The context must contain a private key (set via EdDsaSetPrivKey() or loaded
  from PEM) before calling this function.

  If EdDsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If MessageSize is 0 or exceeds INT_MAX, then return FALSE.
  If Signature is NULL, then return FALSE.
  If SigSize is NULL, then return FALSE.
  For Ed448: Context may be NULL if no context string is used (ContextSize must be 0).

  @param[in]      EdDsaContext    Pointer to EdDSA context containing the private key.
  @param[in]      Context         Optional context string for Ed448 domain separation.
                                  May be NULL for default context.
  @param[in]      ContextSize     Size of context string in bytes. Set to 0 if Context is NULL.
  @param[in]      Message         Pointer to message data to be signed.
  @param[in]      MessageSize     Size of message in bytes.
  @param[out]     Signature       Pointer to buffer to receive the signature.
  @param[in,out]  SigSize         On input, size of Signature buffer.
                                  On output, actual size of signature (114 bytes for Ed448).

  @retval TRUE   EdDSA signature generated successfully.
  @retval FALSE  Invalid parameters or signature generation failed.

**/
BOOLEAN
EFIAPI
EdDsaSign (
  IN      VOID         *EdDsaContext,
  IN      CONST UINT8  *Context,
  IN      UINTN        ContextSize,
  IN      CONST UINT8  *Message,
  IN      UINTN        MessageSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  EVP_MD_CTX        *SignCtx;
  KEY_CONTEXT       *Ctx;
  INT32             Result;
  UINTN             HalfSize;
  CONST OSSL_PARAM  ParamsDefault[] = {
    OSSL_PARAM_END
  };

  CONST OSSL_PARAM  ParamsEd448[] = {
    OSSL_PARAM_octet_string ("context-string", (VOID *)Context, ContextSize),
    OSSL_PARAM_END
  };

  if ((EdDsaContext == NULL) || (Message == NULL)) {
    return FALSE;
  }

  if ((Signature == NULL) && (SigSize == NULL)) {
    return FALSE;
  }

  if ((ContextSize > 0) && (Context == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)EdDsaContext;
  if (Ctx == NULL) {
    return FALSE;
  }

  if (!OpensslNidToKeySize (Ctx->Nid, &HalfSize)) {
    return FALSE;
  }

  if (*SigSize < (UINTN)(HalfSize * 2)) {
    *SigSize = HalfSize * 2;
    return FALSE;
  }

  *SigSize = HalfSize * 2;
  ZeroMem (Signature, *SigSize);

  SignCtx = EVP_MD_CTX_new ();
  if (SignCtx == NULL) {
    return FALSE;
  }

  switch (Ctx->Nid) {
    case EVP_PKEY_ED448:
      if ((Context == NULL) || (ContextSize == 0)) {
        Result = EVP_DigestSignInit_ex (SignCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, ParamsDefault);
      } else {
        Result = EVP_DigestSignInit_ex (SignCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, ParamsEd448);
      }

      break;
    default:
      return FALSE;
  }

  if (Result != 1) {
    EVP_MD_CTX_free (SignCtx);
    return FALSE;
  }

  if (EVP_DigestSign (SignCtx, Signature, SigSize, Message, MessageSize) != 1) {
    EVP_MD_CTX_free (SignCtx);
    return FALSE;
  }

  EVP_MD_CTX_free (SignCtx);
  return TRUE;
}

/**
  Verifies the EdDSA signature for a given message.

  This function verifies an EdDSA signature against a message using the public key
  contained in the EdDSA context. EdDSA signatures use a 'pure' implementation,
  meaning the message digest cannot be computed ahead of time - the raw message
  data is passed directly to the verification function.

  For Ed448, an optional context string can be provided as additional domain
  separation.

  If EdDsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If MessageSize is 0, then return FALSE.
  If Signature is NULL, then return FALSE.
  If SigSize is 0 or exceeds INT_MAX, then return FALSE.
  If SigSize does not match expected signature size for the key type, then return FALSE.
  For Ed448: Context may be NULL if no context string is used.

  @param[in]      EdDsaContext    Pointer to EdDSA context containing the public key.
  @param[in]      Context         Optional context string for Ed448 (domain separation).
                                  May be NULL for default context.
  @param[in]      ContextSize     Size of context string in bytes. Set to 0 if Context is NULL.
  @param[in]      Message         Pointer to the message data to verify.
  @param[in]      MessageSize     Size of the message in bytes.
  @param[in]      Signature       Pointer to the EdDSA signature to verify.
  @param[in]      SigSize         Size of the signature in bytes.
                                  Must be 2 * key_size (114 bytes for Ed448).

  @retval TRUE   EdDSA signature verification succeeded.
  @retval FALSE  EdDSA signature verification failed or invalid parameters.

**/
BOOLEAN
EFIAPI
EdDsaVerify (
  IN     VOID         *EdDsaContext,
  IN     CONST UINT8  *Context,
  IN     UINTN        ContextSize,
  IN     CONST UINT8  *Message,
  IN     UINTN        MessageSize,
  IN     UINT8        *Signature,
  IN     UINTN        SigSize
  )
{
  EVP_MD_CTX        *VerifyCtx;
  KEY_CONTEXT       *Ctx;
  INT32             Result;
  UINTN             HalfSize;
  CONST OSSL_PARAM  ParamsDefault[] = {
    OSSL_PARAM_END
  };

  CONST OSSL_PARAM  ParamsEd448[] = {
    OSSL_PARAM_octet_string ("context-string", (VOID *)Context, ContextSize),
    OSSL_PARAM_END
  };

  if ((EdDsaContext == NULL) || (Message == NULL) || (Signature == NULL)) {
    return FALSE;
  }

  if ((SigSize > INT_MAX) || (SigSize == 0)) {
    return FALSE;
  }

  if ((ContextSize > 0) && (Context == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)EdDsaContext;
  if (Ctx == NULL) {
    return FALSE;
  }

  if (!OpensslNidToKeySize (Ctx->Nid, &HalfSize)) {
    return FALSE;
  }

  if (SigSize != (UINTN)(HalfSize * 2)) {
    return FALSE;
  }

  VerifyCtx = EVP_MD_CTX_new ();
  if (VerifyCtx == NULL) {
    return FALSE;
  }

  switch (Ctx->Nid) {
    case EVP_PKEY_ED448:
      if ((Context == NULL) || (ContextSize == 0)) {
        Result = EVP_DigestVerifyInit_ex (VerifyCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, ParamsDefault);
      } else {
        Result = EVP_DigestVerifyInit_ex (VerifyCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, ParamsEd448);
      }

      break;
    default:
      return FALSE;
  }

  if (Result != 1) {
    EVP_MD_CTX_free (VerifyCtx);
    return FALSE;
  }

  if (EVP_DigestVerify (VerifyCtx, Signature, SigSize, Message, MessageSize) != 1) {
    EVP_MD_CTX_free (VerifyCtx);
    return FALSE;
  }

  EVP_MD_CTX_free (VerifyCtx);
  return TRUE;
}
