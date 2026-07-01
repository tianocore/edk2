/** @file
  ML-DSA API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>

/**
  Creates a new ML-DSA context by Crypto NID.

  This function allocates and initializes a new ML-DSA context for the specified
  ML-DSA variant. The context is created with no key material; the EVP_PKEY
  structure is set to NULL. The caller must call MlDsaFree() to release the
  context when done.

  Before keys can be used for signing or verification, they must be set using
  MlDsaSetPrivKey() or MlDsaSetPubKey().

  If Nid is not a supported ML-DSA variant, then return NULL.
  If memory allocation fails, then return NULL.

  @param[in]  Nid   Crypto NID of the ML-DSA variant (e.g., CRYPTO_NID_ML_DSA_87).

  @retval Pointer to new ML-DSA context if successful.
  @retval NULL if Nid is unsupported or allocation failed.

**/
VOID *
EFIAPI
MlDsaNewByNid (
  IN UINTN  Nid
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Frees an ML-DSA context and all associated resources.

  This function releases all memory associated with the ML-DSA context, including
  the EVP_PKEY structure. After calling this function, the MlDsaContext pointer
  should not be used.

  If MlDsaContext is NULL, then this function returns immediately without action.

  @param[in]  MlDsaContext  Pointer to the ML-DSA context to be released.

**/
VOID
EFIAPI
MlDsaFree (
  IN VOID  *MlDsaContext
  )
{
  ASSERT (FALSE);
}

/**
  Retrieves the ML-DSA public key from the ML-DSA context.

  This function extracts the public key from the ML-DSA context and copies it to
  the provided buffer. The public key is returned in raw binary format.

  The context must have a key set (either via MlDsaSetPrivKey() or MlDsaSetPubKey())
  before calling this function.

  If MlDsaContext is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If the context does not contain a valid key, then return FALSE.
  If PublicKey buffer is too small, PublicKeySize is updated with required size and return FALSE.

  @param[in]      MlDsaContext    Pointer to ML-DSA context containing the key.
  @param[out]     PublicKey       Pointer to buffer to receive the public key.
  @param[in,out]  PublicKeySize   On input, size of PublicKey buffer in bytes.
                                  On output, actual size of public key written.

  @retval TRUE   ML-DSA public key retrieved successfully.
  @retval FALSE  Invalid parameters or buffer too small.

**/
BOOLEAN
EFIAPI
MlDsaGetPubKey (
  IN      VOID   *MlDsaContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Sets the ML-DSA public key in the ML-DSA context.

  This function imports a raw public key into the ML-DSA context. The public key
  must be in raw binary format (not PEM or DER encoded). The key size must match
  the expected size for the ML-DSA variant (2592 bytes for ML-DSA-87).

  After setting the public key, the context can be used for signature verification
  but not for signing (which requires the private key).

  If MlDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If PublicKeySize does not match the expected size for the variant, then return FALSE.

  @param[in]  MlDsaContext    Pointer to ML-DSA context created by MlDsaNewByNid().
  @param[in]  PublicKey       Pointer to raw public key bytes.
  @param[in]  PublicKeySize   Size of the public key in bytes.

  @retval TRUE   ML-DSA public key was set successfully.
  @retval FALSE  Invalid parameters or key size mismatch.

**/
BOOLEAN
EFIAPI
MlDsaSetPubKey (
  IN  VOID   *MlDsaContext,
  IN  UINT8  *PublicKey,
  IN  UINTN  PublicKeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Sets the ML-DSA private key in the ML-DSA context.

  This function imports a raw private key into the ML-DSA context. The private key
  must be in raw binary format (not PEM or DER encoded). The key size must match
  the expected size for the ML-DSA variant (4896 bytes for ML-DSA-87).

  OpenSSL automatically derives the public key from the private key, so after
  calling this function, both signing and verification operations are possible.

  If MlDsaContext is NULL, then return FALSE.
  If PrivateKey is NULL, then return FALSE.
  If PrivateKeySize does not match the expected size for the variant, then return FALSE.

  @param[in]  MlDsaContext     Pointer to ML-DSA context created by MlDsaNewByNid().
  @param[in]  PrivateKey       Pointer to raw private key bytes.
  @param[in]  PrivateKeySize   Size of the private key in bytes.

  @retval TRUE   ML-DSA private key was set successfully.
  @retval FALSE  Invalid parameters or key size mismatch.

**/
BOOLEAN
EFIAPI
MlDsaSetPrivKey (
  IN  VOID   *MlDsaContext,
  IN  UINT8  *PrivateKey,
  IN  UINTN  PrivateKeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Generates and retrieves the public key from a private key context.

  This function extracts the public key from an ML-DSA context that contains
  a private key. It is equivalent to calling MlDsaGetPubKey() but is provided
  for API consistency with other cryptographic implementations.

  The context must contain a private key (set via MlDsaSetPrivKey()) before
  calling this function.

  If MlDsaContext is NULL, then return FALSE.
  If PublicKey is NULL, then return FALSE.
  If PublicKeySize does not match the expected size for the variant, then return FALSE.

  @param[in]   MlDsaContext    Pointer to ML-DSA context containing the private key.
  @param[out]  PublicKey       Pointer to buffer to receive the public key.
  @param[in]   PublicKeySize   Size of the PublicKey buffer in bytes.

  @retval TRUE   Public key generated and retrieved successfully.
  @retval FALSE  Invalid parameters or public key extraction failed.

**/
BOOLEAN
EFIAPI
MlDsaGeneratePubKey (
  IN  VOID   *MlDsaContext,
  OUT UINT8  *PublicKey,
  IN  UINTN  PublicKeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Generates an ML-DSA signature for a given message.

  This function creates an ML-DSA signature using the private key stored in the
  ML-DSA context. ML-DSA signatures can include an optional context string for
  domain separation, allowing the same key to be used in different contexts
  without creating security vulnerabilities.

  The context must contain a private key (set via MlDsaSetPrivKey()) before
  calling this function.

  If MlDsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If SigSize is NULL, then return FALSE.
  If SigSize buffer is too small, SigSize is updated with required size and return FALSE.
  Context may be NULL if no context string is used (ContextSize must be 0).

  @param[in]      MlDsaContext   Pointer to ML-DSA context containing the private key.
  @param[in]      Context        Optional context string for domain separation.
                                 May be NULL for default context.
  @param[in]      ContextSize    Size of context string in bytes. Set to 0 if Context is NULL.
  @param[in]      Message        Pointer to message data to be signed.
  @param[in]      MessageSize    Size of message in bytes.
  @param[out]     Signature      Pointer to buffer to receive the signature.
  @param[in,out]  SigSize        On input, size of Signature buffer.
                                 On output, actual size of signature (4627 bytes for ML-DSA-87).

  @retval TRUE   ML-DSA signature generated successfully.
  @retval FALSE  Invalid parameters or signature generation failed.

**/
BOOLEAN
EFIAPI
MlDsaSign (
  IN      VOID         *MlDsaContext,
  IN      UINT8        *Context,
  IN      UINTN        ContextSize,
  IN      CONST UINT8  *Message,
  IN      UINTN        MessageSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Verifies the ML-DSA signature for a given message.

  This function verifies an ML-DSA signature against a message using the public key
  contained in the ML-DSA context. An optional context string can be provided which
  must match the context used during signing.

  The context must contain a key (either public or private) set via MlDsaSetPrivKey()
  or MlDsaSetPubKey() before calling this function.

  If MlDsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If SigSize is 0 or exceeds INT_MAX, then return FALSE.
  Context may be NULL if no context string is used.

  @param[in]  MlDsaContext   Pointer to ML-DSA context containing the public key.
  @param[in]  Context        Optional context string for domain separation.
                             May be NULL for default context.
  @param[in]  ContextSize    Size of context string in bytes. Set to 0 if Context is NULL.
  @param[in]  Message        Pointer to the message data to verify.
  @param[in]  MessageSize    Size of the message in bytes.
  @param[in]  Signature      Pointer to the ML-DSA signature to verify.
  @param[in]  SigSize        Size of the signature in bytes.

  @retval TRUE   ML-DSA signature verification succeeded.
  @retval FALSE  ML-DSA signature verification failed or invalid parameters.

**/
BOOLEAN
EFIAPI
MlDsaVerify (
  IN  VOID         *MlDsaContext,
  IN  UINT8        *Context,
  IN  UINTN        ContextSize,
  IN  CONST UINT8  *Message,
  IN  UINTN        MessageSize,
  IN  UINT8        *Signature,
  IN  UINTN        SigSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
