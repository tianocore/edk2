/** @file
  PKCS#7 SignedData Verification Wrapper Implementation over OpenSSL.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pkcs7.h>


/**
  Verification callback function to override any existing callbacks in OpenSSL
  for intermediate certificate supports.

  @param[in]  Status   Original status before calling this callback.
  @param[in]  Context  X509 store context.

  @retval     1        Current X509 certificate is verified successfully.
  @retval     0        Verification failed.

**/
STATIC int X509VerifyCb (int Status, X509_STORE_CTX *Context)
{
  X509_OBJECT  *Obj;
  int          Error;
  int          Index;
  int          Count;

  Obj   = NULL;
  Error = X509_STORE_CTX_get_error (Context);

  //
  // X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT and X509_V_ERR_UNABLE_TO_GET_ISSUER_
  // CERT_LOCALLY mean a X509 certificate is not self signed and its issuer
  // can not be found in X509_verify_cert of X509_vfy.c.
  // In order to support intermediate certificate node, we override the
  // errors if the certification is obtained from X509 store, i.e. it is
  // a trusted ceritifcate node that is enrolled by user.
  // Besides,X509_V_ERR_CERT_UNTRUSTED and X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE
  // are also ignored to enable such feature.
  //
  if ((Error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT) ||
      (Error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)) {
    Obj = (X509_OBJECT *) OPENSSL_malloc (sizeof (X509_OBJECT));
    if (Obj == NULL) {
      return 0;
    }

    Obj->type      = X509_LU_X509;
    Obj->data.x509 = Context->current_cert;

    CRYPTO_w_lock (CRYPTO_LOCK_X509_STORE);

    if (X509_OBJECT_retrieve_match (Context->ctx->objs, Obj)) {
      Status = 1;
    } else {
      //
      // If any certificate in the chain is enrolled as trusted certificate,
      // pass the certificate verification.
      //
      if (Error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) {
        Count = sk_X509_num (Context->chain);
        for (Index = 0; Index < Count; Index++) {
          Obj->data.x509 = sk_X509_value (Context->chain, Index);
          if (X509_OBJECT_retrieve_match (Context->ctx->objs, Obj)) {
            Status = 1;
            break;
          }
        }
      }
    }

    CRYPTO_w_unlock (CRYPTO_LOCK_X509_STORE);
  }

  if ((Error == X509_V_ERR_CERT_UNTRUSTED) ||
      (Error == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)) {
    Status = 1;
  }

  if (Obj != NULL) {
    OPENSSL_free (Obj);
  }

  return Status;
}

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
  @param[out] SignedData       Pointer to output PKCS#7 signedData.
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

  //
  // Check input parameters.
  //
  if ((PrivateKey == NULL) || (KeyPassword == NULL) || (InData == NULL)) {
    return FALSE;
  }
  
  if ((SignCert == NULL) || (SignedData == NULL) || (SignedDataSize == NULL)) {
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
             (CONST CHAR8 *) KeyPassword,
             (VOID **) &RsaContext
             );
  if (!Status) {
    return Status;
  }

  //
  // Register & Initialize necessary digest algorithms and PRNG for PKCS#7 Handling
  //
  EVP_add_digest (EVP_md5());
  EVP_add_digest (EVP_sha1());
  EVP_add_digest (EVP_sha256());
  RandomSeed (NULL, 0);

  //
  // Construct OpenSSL EVP_PKEY for private key.
  //
  Key = EVP_PKEY_new ();
  if (Key == NULL) {
    goto _Exit;
  }
  Key->save_type = EVP_PKEY_RSA;
  Key->type      = EVP_PKEY_type (EVP_PKEY_RSA);
  Key->pkey.rsa  = (RSA *) RsaContext;

  //
  // Convert the data to be signed to BIO format. 
  //
  DataBio = BIO_new (BIO_s_mem ());
  BIO_write (DataBio, InData, (int) InDataSize);

  //
  // Create the PKCS#7 signedData structure.
  //
  Pkcs7 = PKCS7_sign (
            (X509 *) SignCert,
            Key,
            (STACK_OF(X509) *) OtherCerts,
            DataBio,
            PKCS7_BINARY
            );
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // Convert PKCS#7 signedData structure into DER-encoded buffer.
  //
  *SignedDataSize = i2d_PKCS7 (Pkcs7, NULL);
  if (*SignedDataSize == 0) {
    goto _Exit;
  }
  *SignedData     = OPENSSL_malloc (*SignedDataSize);
  P7Data          = *SignedData;
  *SignedDataSize = i2d_PKCS7 (Pkcs7, (unsigned char **) &P7Data);

  Status = TRUE;

_Exit:
  //
  // Release Resources
  //
  if (RsaContext != NULL) {
    RsaFree (RsaContext);
    if (Key != NULL) {
      Key->pkey.rsa = NULL;
    }
  }

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

/**
  Verifies the validility of a PKCS#7 signed data as described in "PKCS #7: Cryptographic
  Message Syntax Standard".

  If P7Data is NULL, then ASSERT().

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertLength   Length of the trusted certificate in bytes.
  @param[in]  InData       Pointer to the content to be verified.
  @param[in]  DataLength   Length of InData in bytes.

  @retval  TRUE  The specified PKCS#7 signed data is valid.
  @retval  FALSE Invalid PKCS#7 signed data.

**/
BOOLEAN
EFIAPI
Pkcs7Verify (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertLength,
  IN  CONST UINT8  *InData,
  IN  UINTN        DataLength
  )
{
  PKCS7       *Pkcs7;
  BIO         *CertBio;
  BIO         *DataBio;
  BOOLEAN     Status;
  X509        *Cert;
  X509_STORE  *CertStore;

  //
  // ASSERT if P7Data is NULL
  //
  ASSERT (P7Data != NULL);

  Status    = FALSE;
  Pkcs7     = NULL;
  CertBio   = NULL;
  DataBio   = NULL;
  Cert      = NULL;
  CertStore = NULL;

  //
  // Register & Initialize necessary digest algorithms for PKCS#7 Handling
  //
  EVP_add_digest (EVP_md5());
  EVP_add_digest (EVP_sha1());
  EVP_add_digest_alias (SN_sha1WithRSAEncryption, SN_sha1WithRSA);
  EVP_add_digest (EVP_sha256());

  //
  // Retrieve PKCS#7 Data (DER encoding)
  //
  Pkcs7 = d2i_PKCS7 (NULL, &P7Data, (int)P7Length);
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // Check if it's PKCS#7 Signed Data (for Authenticode Scenario)
  //
  if (!PKCS7_type_is_signed (Pkcs7)) {
    goto _Exit;
  }

  //
  // Read DER-encoded root certificate and Construct X509 Certificate
  //
  CertBio = BIO_new (BIO_s_mem ());
  BIO_write (CertBio, TrustedCert, (int)CertLength);
  if (CertBio == NULL) {
    goto _Exit;
  }
  Cert = d2i_X509_bio (CertBio, NULL);
  if (Cert == NULL) {
    goto _Exit;
  }

  //
  // Setup X509 Store for trusted certificate
  //
  CertStore = X509_STORE_new ();
  if (CertStore == NULL) {
    goto _Exit;
  }
  if (!(X509_STORE_add_cert (CertStore, Cert))) {
    goto _Exit;
  }

  //
  // Register customized X509 verification callback function to support
  // trusted intermediate certificate anchor.
  //
  CertStore->verify_cb = X509VerifyCb;

  //
  // For generic PKCS#7 handling, InData may be NULL if the content is present
  // in PKCS#7 structure. So ignore NULL checking here.
  //
  DataBio = BIO_new (BIO_s_mem ());
  BIO_write (DataBio, InData, (int)DataLength);

  //
  // Verifies the PKCS#7 signedData structure
  //
  Status = (BOOLEAN) PKCS7_verify (Pkcs7, NULL, CertStore, DataBio, NULL, PKCS7_BINARY);

_Exit:
  //
  // Release Resources
  //
  BIO_free (DataBio);
  BIO_free (CertBio);
  X509_free (Cert);
  X509_STORE_free (CertStore);
  PKCS7_free (Pkcs7);

  return Status;
}
