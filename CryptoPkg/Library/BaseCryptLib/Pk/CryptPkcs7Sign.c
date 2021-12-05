/** @file
  PKCS#7 SignedData Sign Wrapper Implementation over OpenSSL.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pkcs7.h>

/**
  Creates a PKCS#7 signedData as described in "PKCS #7: Cryptographic Message
  Syntax Standard, version 1.5". This interface is only intended to be used for
  application to perform PKCS#7 functionality validation.

  @param[in]  PrivateKey       Pointer to the PEM-formatted private key data for
                               data signing.
  @param[in]  PrivateKeySize   Size of the PEM private key data in bytes.
  @param[in]  KeyPassword      NULL-terminated passphrase used for encrypted PEM
                               key data.
  @param[in]  InData           Pointer to the content to be signed.
  @param[in]  InDataSize       Size of InData in bytes.
  @param[in]  SignCert         Pointer to signer's DER-encoded certificate to sign with.
  @param[in]  OtherCerts       Pointer to an optional additional set of certificates to
                               include in the PKCS#7 signedData (e.g. any intermediate
                               CAs in the chain).
  @param[out] SignedData       Pointer to output PKCS#7 signedData. It's caller's
                               responsibility to free the buffer with FreePool().
  @param[out] SignedDataSize   Size of SignedData in bytes.

  @retval     TRUE             PKCS#7 data signing succeeded.
  @retval     FALSE            PKCS#7 data signing failed.

**/
BOOLEAN
EFIAPI
Pkcs7Sign (
  IN   CONST UINT8  *PrivateKey,
  IN   UINTN        PrivateKeySize,
  IN   CONST UINT8  *KeyPassword,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   UINT8        *SignCert,
  IN   UINT8        *OtherCerts      OPTIONAL,
  OUT  UINT8        **SignedData,
  OUT  UINTN        *SignedDataSize
  )
{
  BOOLEAN   Status;
  EVP_PKEY  *Key;
  BIO       *DataBio;
  PKCS7     *Pkcs7;
  UINT8     *RsaContext;
  UINT8     *P7Data;
  UINTN     P7DataSize;
  UINT8     *Tmp;

  //
  // Check input parameters.
  //
  if ((PrivateKey == NULL) || (KeyPassword == NULL) || (InData == NULL) ||
      (SignCert == NULL) || (SignedData == NULL) || (SignedDataSize == NULL) || (InDataSize > INT_MAX))
  {
    return FALSE;
  }

  RsaContext = NULL;
  Key        = NULL;
  Pkcs7      = NULL;
  DataBio    = NULL;
  Status     = FALSE;

  //
  // Retrieve RSA private key from PEM data.
  //
  Status = RsaGetPrivateKeyFromPem (
             PrivateKey,
             PrivateKeySize,
             (CONST CHAR8 *)KeyPassword,
             (VOID **)&RsaContext
             );
  if (!Status) {
    return Status;
  }

  Status = FALSE;

  //
  // Register & Initialize necessary digest algorithms and PRNG for PKCS#7 Handling
  //
  if (EVP_add_digest (EVP_md5 ()) == 0) {
    goto _Exit;
  }

  if (EVP_add_digest (EVP_sha1 ()) == 0) {
    goto _Exit;
  }

  if (EVP_add_digest (EVP_sha256 ()) == 0) {
    goto _Exit;
  }

  RandomSeed (NULL, 0);

  //
  // Construct OpenSSL EVP_PKEY for private key.
  //
  Key = EVP_PKEY_new ();
  if (Key == NULL) {
    goto _Exit;
  }

  if (EVP_PKEY_assign_RSA (Key, (RSA *)RsaContext) == 0) {
    goto _Exit;
  }

  //
  // Convert the data to be signed to BIO format.
  //
  DataBio = BIO_new (BIO_s_mem ());
  if (DataBio == NULL) {
    goto _Exit;
  }

  if (BIO_write (DataBio, InData, (int)InDataSize) <= 0) {
    goto _Exit;
  }

  //
  // Create the PKCS#7 signedData structure.
  //
  Pkcs7 = PKCS7_sign (
            (X509 *)SignCert,
            Key,
            (STACK_OF (X509) *) OtherCerts,
            DataBio,
            PKCS7_BINARY | PKCS7_NOATTR | PKCS7_DETACHED
            );
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // Convert PKCS#7 signedData structure into DER-encoded buffer.
  //
  P7DataSize = i2d_PKCS7 (Pkcs7, NULL);
  if (P7DataSize <= 19) {
    goto _Exit;
  }

  P7Data = malloc (P7DataSize);
  if (P7Data == NULL) {
    goto _Exit;
  }

  Tmp        = P7Data;
  P7DataSize = i2d_PKCS7 (Pkcs7, (unsigned char **)&Tmp);
  ASSERT (P7DataSize > 19);

  //
  // Strip ContentInfo to content only for signeddata. The data be trimmed off
  // is totally 19 bytes.
  //
  *SignedDataSize = P7DataSize - 19;
  *SignedData     = AllocatePool (*SignedDataSize);
  if (*SignedData == NULL) {
    OPENSSL_free (P7Data);
    goto _Exit;
  }

  CopyMem (*SignedData, P7Data + 19, *SignedDataSize);

  OPENSSL_free (P7Data);

  Status = TRUE;

_Exit:
  //
  // Release Resources
  //
  if (Key != NULL) {
    EVP_PKEY_free (Key);
  }

  if (DataBio != NULL) {
    BIO_free (DataBio);
  }

  if (Pkcs7 != NULL) {
    PKCS7_free (Pkcs7);
  }

  return Status;
}
