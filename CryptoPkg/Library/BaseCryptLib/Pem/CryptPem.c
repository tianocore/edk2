/** @file
  PEM (Privacy Enhanced Mail) Format Handler Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include "KeyContext.h"
#include <openssl/pem.h>
#include <openssl/bio.h>

/**
  Callback function for password phrase conversion used for retrieving the encrypted PEM.

  @param[out]  Buf      Pointer to the buffer to write the passphrase to.
  @param[in]   Size     Maximum length of the passphrase (i.e. the size of Buf).
  @param[in]   Flag     A flag which is set to 0 when reading and 1 when writing.
  @param[in]   Key      Key data to be passed to the callback routine.

  @retval  The number of characters in the passphrase or 0 if an error occurred.

**/
STATIC
INTN
PasswordCallback (
  OUT  CHAR8  *Buf,
  IN   INTN   Size,
  IN   INTN   Flag,
  IN   VOID   *Key
  )
{
  INTN  KeyLength;

  ZeroMem ((VOID *)Buf, (UINTN)Size);
  if (Key != NULL) {
    //
    // Duplicate key phrase directly.
    //
    KeyLength = (INTN)AsciiStrLen ((CHAR8 *)Key);
    KeyLength = (KeyLength > Size) ? Size : KeyLength;
    CopyMem (Buf, Key, (UINTN)KeyLength);
    return KeyLength;
  } else {
    return 0;
  }
}

/**
  Allocate and initialize a KEY_CONTEXT structure wrapping an EVP_PKEY.
  This helper function allocates a KEY_CONTEXT structure and wraps the provided
  EVP_PKEY pointer within it.

  @param[in]  Pkey     Pointer to an EVP_PKEY structure to be wrapped.
  @param[in]  Nid      The NID representing the type of key (e.g., EVP_PKEY_ED448).
  @param[out] Context  Pointer to receive the allocated KEY_CONTEXT structure.

  @retval TRUE   KEY_CONTEXT was allocated and initialized successfully.
  @retval FALSE  Memory allocation failed.
**/
STATIC
BOOLEAN
AllocateKeyContext (
  IN   EVP_PKEY  *Pkey,
  IN   INT32     Nid,
  OUT  VOID      **Context
  )
{
  KEY_CONTEXT  *Ctx;

  Ctx = (KEY_CONTEXT *)AllocateZeroPool (sizeof (KEY_CONTEXT));
  if (Ctx == NULL) {
    return FALSE;
  }

  Ctx->EvpPkey = Pkey;
  Ctx->Nid     = Nid;
  *Context     = (VOID *)Ctx;
  return TRUE;
}

/**
  Convert an ML-DSA type name string to an OpenSSL NID.

  This helper function translates ML-DSA type name strings (e.g., "ML-DSA-87")
  to their corresponding OpenSSL EVP_PKEY NIDs (e.g., EVP_PKEY_ML_DSA_87).

  If the type name is not recognized, EVP_PKEY_NONE is returned.

  @param[in]  TypeName   ML-DSA type name string (e.g., "ML-DSA-87").

  @retval OpenSSL NID (e.g., EVP_PKEY_ML_DSA_87) if recognized.
  @retval EVP_PKEY_NONE if the type name is not recognized.

**/
STATIC
INT32
MlDsaTypeNameToNid (
  IN CONST CHAR8  *TypeName
  )
{
  INT32  Nid;

  if (AsciiStrCmp (TypeName, "ML-DSA-87") == 0) {
    Nid = EVP_PKEY_ML_DSA_87;
  } else {
    Nid = EVP_PKEY_NONE;
  }

  return Nid;
}

/**
  Check if the given NID is supported for ML-DSA.

  This helper function checks if the provided NID corresponds to a supported
  ML-DSA type. Currently, only EVP_PKEY_ML_DSA_87 is supported.

  @param[in]  Nid   The NID to check.

  @retval TRUE   The NID is supported for ML-DSA.
  @retval FALSE  The NID is not supported for ML-DSA.

**/
STATIC
BOOLEAN
IsMlDsaNidSupported (
  IN INT32  Nid
  )
{
  switch (Nid) {
    case EVP_PKEY_ML_DSA_87:
      return TRUE;
    default:
      return FALSE;
  }
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
  Check if the given NID is supported for SLH-DSA.

  This helper function checks if the provided NID corresponds to a supported
  SLH-DSA type. Currently, only EVP_PKEY_SLH_DSA_SHAKE_256S is supported.

  @param[in]  Nid   The NID to check.

  @retval TRUE   The NID is supported for SLH-DSA.
  @retval FALSE  The NID is not supported for SLH-DSA.

**/
STATIC
BOOLEAN
IsSlhDsaNidSupported (
  IN INT32  Nid
  )
{
  switch (Nid) {
    case EVP_PKEY_SLH_DSA_SHAKE_256S:
      return TRUE;
    default:
      return FALSE;
  }
}

/**
  Retrieve the EC Public Key from PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted
                           PEM key data.
  @param[out] EcContext    Pointer to new-generated EC DSA context which
                           contain the retrieved EC public key component.
                           Use EcFree() function to free the resource.

  If PemData is NULL, then return FALSE.
  If EcContext is NULL, then return FALSE.

  @retval  TRUE   EC Public Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

**/
BOOLEAN
EFIAPI
EcGetPublicKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **EcContext
  )
{
  BOOLEAN     Status;
  BIO         *PemBio;
  EC_CONTEXT  *EcCtx;

  //
  // Check input parameters.
  //
  if ((PemData == NULL) || (EcContext == NULL) || (PemSize > INT_MAX)) {
    return FALSE;
  }

  *EcContext = NULL;

  //
  // Add possible block-cipher descriptor for PEM data decryption.
  // NOTE: Only support most popular ciphers AES for the encrypted PEM.
  //
  if (EVP_add_cipher (EVP_aes_128_cbc ()) == 0) {
    return FALSE;
  }

  if (EVP_add_cipher (EVP_aes_192_cbc ()) == 0) {
    return FALSE;
  }

  if (EVP_add_cipher (EVP_aes_256_cbc ()) == 0) {
    return FALSE;
  }

  Status = FALSE;

  EcCtx = (EC_CONTEXT *)OPENSSL_zalloc (sizeof (EC_CONTEXT));
  if (EcCtx == NULL) {
    return FALSE;
  }

  //
  // Read PEM Data.
  //
  PemBio = BIO_new (BIO_s_mem ());
  if (PemBio == NULL) {
    goto _Exit;
  }

  if (BIO_write (PemBio, PemData, (int)PemSize) <= 0) {
    goto _Exit;
  }

  //
  // Retrieve EC Public Key from PEM data.
  //
  EcCtx->EvpPkey = PEM_read_bio_PUBKEY (PemBio, NULL, (pem_password_cb *)&PasswordCallback, (void *)Password);
  if (EcCtx->EvpPkey == NULL) {
    goto _Exit;
  }

  if (EVP_PKEY_id (EcCtx->EvpPkey) != EVP_PKEY_EC) {
    EVP_PKEY_free (EcCtx->EvpPkey);
    EcCtx->EvpPkey = NULL;
    goto _Exit;
  }

  Status     = TRUE;
  *EcContext = EcCtx;
  EcCtx      = NULL;

_Exit:
  //
  // Release Resources.
  //
  BIO_free (PemBio);

  if (EcCtx != NULL) {
    OPENSSL_free (EcCtx);
  }

  return Status;
}

/**
  Convert the EC Public Key to PEM key data.

  @param[in]      EcContext   Pointer to EC DSA context.
  @param[out]     PemData     Pointer to the PEM-encoded key data to be
                              retrieved.
  @param[in, out] PemSize     On input, size of PemData in bytes.
                              On output, size of data returned or required size.

  If EcContext is NULL, then return FALSE.
  If PemSize is NULL, then return FALSE.
  If PemData is NULL and *PemSize is zero, then return FALSE and set
  *PemSize to the required size of the PemData buffer.
  If PemData is NULL and *PemSize is not zero, then return FALSE.
  If PemData is not NULL and *PemSize is too small, then return FALSE and
  set *PemSize to the required size of the PemData buffer.

  @retval  TRUE   EC Public Key was converted to the PEM data successfully.
  @retval  FALSE  Invalid EC Context.

**/
BOOLEAN
EFIAPI
EcPublicKeyToPEM (
  IN  VOID      *EcContext,
  OUT UINT8     *PemData,
  IN OUT UINTN  *PemSize
  )
{
  BOOLEAN     Status;
  BIO         *PemBio;
  UINTN       KeyDataSize;
  EC_CONTEXT  *EcCtx;

  //
  // Check input parameters.
  //
  if ((EcContext == NULL) || (PemSize == NULL)) {
    return FALSE;
  }

  EcCtx = (EC_CONTEXT *)EcContext;
  if (EcCtx->EvpPkey == NULL) {
    return FALSE;
  }

  if ((PemData == NULL) && (*PemSize != 0)) {
    return FALSE;
  }

  //
  // Allocate memory for PEM Data.
  //
  PemBio = BIO_new (BIO_s_mem ());
  if (PemBio == NULL) {
    Status = FALSE;
    goto _Exit;
  }

  //
  // Write the EC Public in PEM format.
  //
  if (PEM_write_bio_PUBKEY (PemBio, EcCtx->EvpPkey) <= 0) {
    Status = FALSE;
    goto _Exit;
  }

  //
  // Check if the output buffer is large enough to store the EC Public key.
  //
  KeyDataSize = (UINTN)BIO_number_written (PemBio);
  if (*PemSize < KeyDataSize) {
    *PemSize = KeyDataSize;
    Status   = FALSE;
    goto _Exit;
  } else {
    //
    // Copy the PEM formatted EC PublicKey to the output buffer.
    //
    if (BIO_read_ex (PemBio, PemData, *PemSize, &KeyDataSize) <= 0) {
      Status = FALSE;
      goto _Exit;
    }

    *PemSize = KeyDataSize;
    Status   = TRUE;
  }

_Exit:
  //
  // Release Resources.
  //
  BIO_free (PemBio);

  return Status;
}
