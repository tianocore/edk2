/** @file
  SLH-DSA API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>

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
  ASSERT (FALSE);
  return NULL;
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
}
