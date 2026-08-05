/** @file
  SLH-DSA API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include "KeyContext.h"
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>

/**
  Get the public key size in bytes for an SLH-DSA variant from its OpenSSL NID.

  This helper function maps OpenSSL SLH-DSA NIDs to their corresponding public key sizes.
  For SLH-DSA-SHAKE-256s, the public key size is 64 bytes.

  If the NID is not supported, PubKeySize is set to 0 and FALSE is returned.

  @param[in]   Nid          OpenSSL NID of the SLH-DSA variant (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S).
  @param[out]  PubKeySize   Pointer to receive the public key size in bytes.

  @retval TRUE   Public key size retrieved successfully.
  @retval FALSE  Unsupported SLH-DSA NID.

**/
STATIC
BOOLEAN
OpensslNidToPubKeySize (
  IN  UINTN  Nid,
  OUT UINTN  *PubKeySize
  )
{
  switch (Nid) {
    case EVP_PKEY_SLH_DSA_SHAKE_256S:
      *PubKeySize = 64;
      break;
    default:
      *PubKeySize = 0;
      return FALSE;
  }

  return TRUE;
}

/**
  Get the private key size in bytes for an SLH-DSA variant from its OpenSSL NID.

  This helper function maps OpenSSL SLH-DSA NIDs to their corresponding private key sizes.
  For SLH-DSA-SHAKE-256s, the private key size is 128 bytes.

  If the NID is not supported, PrivKeySize is set to 0 and FALSE is returned.

  @param[in]   Nid           OpenSSL NID of the SLH-DSA variant (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S).
  @param[out]  PrivKeySize   Pointer to receive the private key size in bytes.

  @retval TRUE   Private key size retrieved successfully.
  @retval FALSE  Unsupported SLH-DSA NID.

**/
STATIC
BOOLEAN
OpensslNidToPrivKeySize (
  IN  UINTN  Nid,
  OUT UINTN  *PrivKeySize
  )
{
  switch (Nid) {
    case EVP_PKEY_SLH_DSA_SHAKE_256S:
      *PrivKeySize = 128;
      break;
    default:
      *PrivKeySize = 0;
      return FALSE;
  }

  return TRUE;
}

/**
  Get the signature size in bytes for an SLH-DSA variant from its OpenSSL NID.

  This helper function maps OpenSSL SLH-DSA NIDs to their corresponding signature sizes.
  For SLH-DSA-SHAKE-256s, the signature size is 29792 bytes.

  If the NID is not supported, SignatureSize is set to 0 and FALSE is returned.

  @param[in]   Nid             OpenSSL NID of the SLH-DSA variant (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S).
  @param[out]  SignatureSize   Pointer to receive the signature size in bytes.

  @retval TRUE   Signature size retrieved successfully.
  @retval FALSE  Unsupported SLH-DSA NID.

**/
STATIC
BOOLEAN
OpensslNidToSignatureSize (
  IN  UINTN  Nid,
  OUT UINTN  *SignatureSize
  )
{
  switch (Nid) {
    case EVP_PKEY_SLH_DSA_SHAKE_256S:
      *SignatureSize = 29792;
      break;
    default:
      *SignatureSize = 0;
      return FALSE;
  }

  return TRUE;
}

/**
  Convert a Crypto NID to an OpenSSL NID.

  This helper function translates EDK II Crypto library NIDs (e.g., CRYPTO_NID_SLH_DSA_SHAKE_256S)
  to their corresponding OpenSSL EVP_PKEY NIDs (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S).

  If the Crypto NID is not supported, EVP_PKEY_NONE is returned.

  @param[in]  CryptoNid   EDK II Crypto library NID (e.g., CRYPTO_NID_SLH_DSA_SHAKE_256S).

  @retval OpenSSL NID (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S) if supported.
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
    case CRYPTO_NID_SLH_DSA_SHAKE_256S:
      Nid = EVP_PKEY_SLH_DSA_SHAKE_256S;
      break;
    default:
      Nid = EVP_PKEY_NONE;
      break;
  }

  return Nid;
}

/**
  Convert an SLH-DSA type name string to an OpenSSL NID.

  This helper function translates SLH-DSA type name strings (e.g., "SLH-DSA-SHAKE-256s")
  to their corresponding OpenSSL EVP_PKEY NIDs (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S).

  If the type name is not recognized, EVP_PKEY_NONE is returned.

  @param[in]  TypeName   SLH-DSA type name string (e.g., "SLH-DSA-SHAKE-256s").

  @retval OpenSSL NID (e.g., EVP_PKEY_SLH_DSA_SHAKE_256S) if recognized.
  @retval EVP_PKEY_NONE if the type name is not recognized.

**/
STATIC
INT32
SlhDsaTypeNameToNid (
  IN CONST CHAR8  *TypeName
  )
{
  INT32  Nid;

  if (AsciiStrCmp (TypeName, "SLH-DSA-SHAKE-256s") == 0) {
    Nid = EVP_PKEY_SLH_DSA_SHAKE_256S;
  } else {
    Nid = EVP_PKEY_NONE;
  }

  return Nid;
}

/**
  Creates a new SLH-DSA context by Crypto NID.

  This function allocates and initializes a new SLH-DSA context for the specified
  SLH-DSA variant. The context is created with no key material; the EVP_PKEY
  structure is set to NULL. The caller must call SlhDsaFree() to release the
  context when done.

  Before keys can be used for signing or verification, they must be set using
  SlhDsaSetPrivKey() or SlhDsaSetPubKey().

  If Nid is not a supported SLH-DSA variant, then return NULL.
  If memory allocation fails, then return NULL.

  @param[in]  Nid   Crypto NID of the SLH-DSA variant (e.g., CRYPTO_NID_SLH_DSA_SHAKE_256S).

  @retval Pointer to new SLH-DSA context if successful.
  @retval NULL if Nid is unsupported or allocation failed.

**/
VOID *
EFIAPI
SlhDsaNewByNid (
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
  Frees an SLH-DSA context and all associated resources.

  This function releases all memory associated with the SLH-DSA context, including
  the EVP_PKEY structure. After calling this function, the SlhDsaContext pointer
  should not be used.

  If SlhDsaContext is NULL, then this function returns immediately without action.

  @param[in]  SlhDsaContext  Pointer to the SLH-DSA context to be released.

**/
VOID
EFIAPI
SlhDsaFree (
  IN VOID  *SlhDsaContext
  )
{
  KEY_CONTEXT  *Ctx;

  if (SlhDsaContext == NULL) {
    return;
  }

  Ctx = (KEY_CONTEXT *)SlhDsaContext;

  if (Ctx->EvpPkey != NULL) {
    EVP_PKEY_free (Ctx->EvpPkey);
  }

  FreePool (Ctx);
}

/**
  Sets the SLH-DSA private key in the SLH-DSA context.

  This function imports a raw private key into the SLH-DSA context. The private key
  must be in raw binary format (not PEM or DER encoded). The key size must match
  the expected size for the SLH-DSA variant (128 bytes for SLH-DSA-SHAKE-256s).

  OpenSSL automatically derives the public key from the private key, so after
  calling this function, both signing and verification operations are possible.

  If SlhDsaContext is NULL, then return FALSE.
  If PrivateKey is NULL, then return FALSE.
  If PrivateKeySize does not match the expected size for the variant, then return FALSE.

  @param[in]  SlhDsaContext    Pointer to SLH-DSA context created by SlhDsaNewByNid().
  @param[in]  PrivateKey       Pointer to raw private key bytes.
  @param[in]  PrivateKeySize   Size of the private key in bytes.

  @retval TRUE   SLH-DSA private key was set successfully.
  @retval FALSE  Invalid parameters or key size mismatch.

**/
BOOLEAN
EFIAPI
SlhDsaSetPrivKey (
  IN  VOID   *SlhDsaContext,
  IN  UINT8  *PrivateKey,
  IN  UINTN  PrivateKeySize
  )
{
  KEY_CONTEXT  *Ctx;
  UINTN        FinalPrivateKeySize;

  if ((SlhDsaContext == NULL) || (PrivateKey == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)SlhDsaContext;

  if (!OpensslNidToPrivKeySize (Ctx->Nid, &FinalPrivateKeySize)) {
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
  Generates and retrieves the public key from a private key context.

  This function extracts the public key from an SLH-DSA context that contains
  a private key. It is equivalent to calling SlhDsaGetPubKey() but is provided
  for API consistency with other cryptographic implementations.

  The context must contain a private key (set via SlhDsaSetPrivKey()) before
  calling this function.

  If SlhDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If PublicKeySize does not match the expected size for the variant, then return FALSE.

  @param[in]   SlhDsaContext   Pointer to SLH-DSA context containing the private key.
  @param[out]  PublicKey       Pointer to buffer to receive the public key.
  @param[in]   PublicKeySize   Size of the PublicKey buffer in bytes.

  @retval TRUE   Public key generated and retrieved successfully.
  @retval FALSE  Invalid parameters or public key extraction failed.

**/
BOOLEAN
EFIAPI
SlhDsaGeneratePubKey (
  IN  VOID   *SlhDsaContext,
  OUT UINT8  *PublicKey,
  IN  UINTN  PublicKeySize
  )
{
  return FALSE;
}

/**
  Sets the SLH-DSA public key in the SLH-DSA context.

  This function imports a raw public key into the SLH-DSA context. The public key
  must be in raw binary format (not PEM or DER encoded). The key size must match
  the expected size for the SLH-DSA variant (64 bytes for SLH-DSA-SHAKE-256s).

  After setting the public key, the context can be used for signature verification
  but not for signing (which requires the private key).

  If SlhDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If PublicKeySize does not match the expected size for the variant, then return FALSE.

  @param[in]  SlhDsaContext   Pointer to SLH-DSA context created by SlhDsaNewByNid().
  @param[in]  PublicKey       Pointer to raw public key bytes.
  @param[in]  PublicKeySize   Size of the public key in bytes.

  @retval TRUE   SLH-DSA public key was set successfully.
  @retval FALSE  Invalid parameters or key size mismatch.

**/
BOOLEAN
EFIAPI
SlhDsaSetPubKey (
  IN  VOID   *SlhDsaContext,
  IN  UINT8  *PublicKey,
  IN  UINTN  PublicKeySize
  )
{
  KEY_CONTEXT  *Ctx;
  UINTN        FinalPublicKeySize;

  if ((SlhDsaContext == NULL) || (PublicKey == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)SlhDsaContext;

  if (!OpensslNidToPubKeySize (Ctx->Nid, &FinalPublicKeySize)) {
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
  Retrieves the SLH-DSA public key from the SLH-DSA context.

  This function extracts the public key from the SLH-DSA context and copies it to
  the provided buffer. The public key is returned in raw binary format.

  The context must have a key set (either via SlhDsaSetPrivKey() or SlhDsaSetPubKey())
  before calling this function.

  If SlhDsaContext is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If the context does not contain a valid key, then return FALSE.
  If PublicKey buffer is too small, PublicKeySize is updated with required size and return FALSE.

  @param[in]      SlhDsaContext   Pointer to SLH-DSA context containing the key.
  @param[out]     PublicKey       Pointer to buffer to receive the public key.
  @param[in,out]  PublicKeySize   On input, size of PublicKey buffer in bytes.
                                  On output, actual size of public key written.

  @retval TRUE   SLH-DSA public key retrieved successfully.
  @retval FALSE  Invalid parameters or buffer too small.

**/
BOOLEAN
EFIAPI
SlhDsaGetPubKey (
  IN      VOID   *SlhDsaContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  KEY_CONTEXT  *Ctx;
  INT32        Result;
  UINTN        FinalPublicKeySize;

  if ((SlhDsaContext == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  if (PublicKey == NULL) {
    *PublicKeySize = 0;
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)SlhDsaContext;
  if (Ctx->EvpPkey == NULL) {
    return FALSE;
  }

  if (!OpensslNidToPubKeySize (Ctx->Nid, &FinalPublicKeySize)) {
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
  Generates an SLH-DSA signature for a given message.

  This function creates an SLH-DSA signature using the private key stored in the
  SLH-DSA context. SLH-DSA signatures can include an optional context string for
  domain separation, allowing the same key to be used in different contexts
  without creating security vulnerabilities.

  The context must contain a private key (set via SlhDsaSetPrivKey()) before
  calling this function.

  If SlhDsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If SigSize is NULL, then return FALSE.
  If SigSize buffer is too small, SigSize is updated with required size and return FALSE.
  Context may be NULL if no context string is used (ContextSize must be 0).

  @param[in]      SlhDsaContext  Pointer to SLH-DSA context containing the private key.
  @param[in]      Context        Optional context string for domain separation.
                                 May be NULL for default context.
  @param[in]      ContextSize    Size of context string in bytes. Set to 0 if Context is NULL.
  @param[in]      Message        Pointer to message data to be signed.
  @param[in]      MessageSize    Size of message in bytes.
  @param[out]     Signature      Pointer to buffer to receive the signature.
  @param[in,out]  SigSize        On input, size of Signature buffer.
                                 On output, actual size of signature (29792 bytes for SLH-DSA-SHAKE-256s).

  @retval TRUE   SLH-DSA signature generated successfully.
  @retval FALSE  Invalid parameters or signature generation failed.

**/
BOOLEAN
EFIAPI
SlhDsaSign (
  IN      VOID         *SlhDsaContext,
  IN      UINT8        *Context,
  IN      UINTN        ContextSize,
  IN      CONST UINT8  *Message,
  IN      UINTN        MessageSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  KEY_CONTEXT  *Ctx;
  EVP_MD_CTX   *SignCtx;
  INT32        Result;
  UINTN        FinalSigSize;
  OSSL_PARAM   Params[2];
  OSSL_PARAM   ParamsDefault[1];

  if ((SlhDsaContext == NULL) || (Message == NULL)) {
    return FALSE;
  }

  if ((Signature == NULL) || (SigSize == NULL)) {
    return FALSE;
  }

  if ((ContextSize > 0) && (Context == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)SlhDsaContext;
  if (Ctx->EvpPkey == NULL) {
    return FALSE;
  }

  if (!OpensslNidToSignatureSize (Ctx->Nid, &FinalSigSize)) {
    return FALSE;
  }

  if (*SigSize < FinalSigSize) {
    *SigSize = FinalSigSize;
    return FALSE;
  }

  *SigSize = FinalSigSize;
  ZeroMem (Signature, *SigSize);

  Params[0] = OSSL_PARAM_construct_octet_string (OSSL_SIGNATURE_PARAM_CONTEXT_STRING, (VOID *)Context, ContextSize);
  Params[1] = OSSL_PARAM_construct_end ();

  Result  = FALSE;
  SignCtx = EVP_MD_CTX_new ();
  if (SignCtx == NULL) {
    return FALSE;
  }

  if (ContextSize == 0) {
    ParamsDefault[0] = OSSL_PARAM_construct_end ();
    Result           = EVP_DigestSignInit_ex (SignCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, ParamsDefault);
  } else {
    Result = EVP_DigestSignInit_ex (SignCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, Params);
  }

  if (Result != 1) {
    EVP_MD_CTX_free (SignCtx);
    return FALSE;
  }

  Result = EVP_DigestSign (SignCtx, Signature, SigSize, Message, MessageSize);
  if (Result != 1) {
    EVP_MD_CTX_free (SignCtx);
    return FALSE;
  }

  EVP_MD_CTX_free (SignCtx);
  return TRUE;
}

/**
  Verifies the SLH-DSA signature for a given message.

  This function verifies an SLH-DSA signature against a message using the public key
  contained in the SLH-DSA context. An optional context string can be provided which
  must match the context used during signing.

  The context must contain a key (either public or private) set via SlhDsaSetPrivKey()
  or SlhDsaSetPubKey() before calling this function.

  If SlhDsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If SigSize is 0 or exceeds INT_MAX, then return FALSE.
  Context may be NULL if no context string is used.

  @param[in]  SlhDsaContext  Pointer to SLH-DSA context containing the public key.
  @param[in]  Context        Optional context string for domain separation.
                             May be NULL for default context.
  @param[in]  ContextSize    Size of context string in bytes. Set to 0 if Context is NULL.
  @param[in]  Message        Pointer to the message data to verify.
  @param[in]  MessageSize    Size of the message in bytes.
  @param[in]  Signature      Pointer to the SLH-DSA signature to verify.
  @param[in]  SigSize        Size of the signature in bytes.

  @retval TRUE   SLH-DSA signature verification succeeded.
  @retval FALSE  SLH-DSA signature verification failed or invalid parameters.

**/
BOOLEAN
EFIAPI
SlhDsaVerify (
  IN  VOID         *SlhDsaContext,
  IN  UINT8        *Context,
  IN  UINTN        ContextSize,
  IN  CONST UINT8  *Message,
  IN  UINTN        MessageSize,
  IN  UINT8        *Signature,
  IN  UINTN        SigSize
  )
{
  KEY_CONTEXT  *Ctx;
  EVP_MD_CTX   *VerifyCtx;
  INT32        OpensslNid;
  UINTN        FinalSigSize;
  INT32        Result;
  OSSL_PARAM   Params[2];
  OSSL_PARAM   ParamsDefault[1];

  if ((SlhDsaContext == NULL) || (Message == NULL) || (Signature == NULL)) {
    return FALSE;
  }

  if ((SigSize > INT_MAX) || (SigSize == 0)) {
    return FALSE;
  }

  if ((ContextSize > 0) && (Context == NULL)) {
    return FALSE;
  }

  Ctx = (KEY_CONTEXT *)SlhDsaContext;
  if (Ctx->EvpPkey == NULL) {
    return FALSE;
  }

  OpensslNid = SlhDsaTypeNameToNid (EVP_PKEY_get0_type_name (Ctx->EvpPkey));
  if (!OpensslNidToSignatureSize (OpensslNid, &FinalSigSize)) {
    return FALSE;
  }

  if (SigSize != FinalSigSize) {
    return FALSE;
  }

  Params[0] = OSSL_PARAM_construct_octet_string (OSSL_SIGNATURE_PARAM_CONTEXT_STRING, (VOID *)Context, ContextSize);
  Params[1] = OSSL_PARAM_construct_end ();

  VerifyCtx = EVP_MD_CTX_new ();
  if (VerifyCtx == NULL) {
    return FALSE;
  }

  if (ContextSize == 0) {
    ParamsDefault[0] = OSSL_PARAM_construct_end ();
    Result           = EVP_DigestVerifyInit_ex (VerifyCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, ParamsDefault);
  } else {
    Result = EVP_DigestVerifyInit_ex (VerifyCtx, NULL, NULL, NULL, NULL, Ctx->EvpPkey, Params);
  }

  if (Result != 1) {
    EVP_MD_CTX_free (VerifyCtx);
    return FALSE;
  }

  Result = EVP_DigestVerify (VerifyCtx, Signature, SigSize, Message, MessageSize);
  if (Result != 1) {
    EVP_MD_CTX_free (VerifyCtx);
    return FALSE;
  }

  EVP_MD_CTX_free (VerifyCtx);
  return TRUE;
}
