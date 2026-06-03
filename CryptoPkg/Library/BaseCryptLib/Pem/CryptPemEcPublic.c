/** @file
  PEM (Privacy Enhanced Mail) EC public key helpers over OpenSSL.

Copyright (c) 2026, ARM Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/pem.h>

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
  Retrieve the EC Public Key from PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] EcContext    Pointer to new-generated EC DSA context which contain the retrieved
                           EC public key component. Use EcFree() function to free the
                           resource.

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
  BOOLEAN  Status;
  BIO      *PemBio;

  //
  // Check input parameters.
  //
  if ((PemData == NULL) || (EcContext == NULL) || (PemSize > INT_MAX)) {
    return FALSE;
  }

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
  *EcContext = PEM_read_bio_EC_PUBKEY (PemBio, NULL, (pem_password_cb *)&PasswordCallback, (void *)Password);
  if (*EcContext != NULL) {
    Status = TRUE;
  }

_Exit:
  //
  // Release Resources.
  //
  BIO_free (PemBio);

  return Status;
}

/**
  Convert the EC Public Key to PEM key data.

  @param[in]  EcContext    Pointer to EC DSA context.
  @param[out] PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[out] PemSize      Size of the PEM key data in bytes.

  If EcContext is NULL, then return FALSE.
  If PemSize is NULL, then return FALSE.
  If PemData is NULL and PemSize is not zero, then return FALSE and
  set the PemSize to the required size of the PemData buffer.

  @retval  TRUE   EC Public Key was converted to the PEM data successfully.
  @retval  FALSE  Invalid EC Context.

**/
BOOLEAN
EFIAPI
EcPublicKeyToPEM (
  IN  VOID   *EcContext,
  OUT UINT8  *PemData,
  OUT UINTN  *PemSize
  )
{
  BOOLEAN  Status;
  BIO      *PemBio;
  UINTN    KeyDataSize;

  //
  // Check input parameters.
  //
  if ((EcContext == NULL) || (PemSize == NULL)) {
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
  if (PEM_write_bio_EC_PUBKEY (PemBio, (EC_KEY *)EcContext) <= 0) {
    Status = FALSE;
    goto _Exit;
  }

  //
  // Check if the output buffer is large enough to store the EC Public key.
  //
  KeyDataSize = BIO_number_written (PemBio);
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
