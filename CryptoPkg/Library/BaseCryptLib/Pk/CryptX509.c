/** @file
  X.509 Certificate Handler Wrapper Implementation over OpenSSL.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"
#include <openssl/x509.h>

/**
  Retrieve the subject bytes from one X.509 certificate.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     CertSubject  Pointer to the retrieved certificate subject bytes.
  @param[in, out] SubjectSize  The size in bytes of the CertSubject buffer on input,
                               and the size of buffer returned CertSubject on output.

  If Cert is NULL, then ASSERT().
  If SubjectSize is NULL, then ASSERT().

  @retval  TRUE   The certificate subject retrieved successfully.
  @retval  FALSE  Invalid certificate, or the SubjectSize is too small for the result.
                  The SubjectSize will be updated with the required size.

**/
BOOLEAN
EFIAPI
X509GetSubjectName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  )
{
  BOOLEAN    Status;
  BIO        *CertBio;
  X509       *X509Cert;
  X509_NAME  *X509Name;

  //
  // ASSERT if Cert is NULL or SubjectSize is NULL.
  //
  ASSERT (Cert        != NULL);
  ASSERT (SubjectSize != NULL);

  Status   = FALSE;
  X509Cert = NULL;

  //
  // Read DER-encoded X509 Certificate and Construct X509 object.
  //
  CertBio = BIO_new (BIO_s_mem ());
  BIO_write (CertBio, Cert, (int)CertSize);
  if (CertBio == NULL) {
    goto _Exit;
  }
  X509Cert = d2i_X509_bio (CertBio, NULL);
  if (Cert == NULL) {
    goto _Exit;
  }

  //
  // Retrieve subject name from certificate object.
  //
  X509Name = X509_get_subject_name (X509Cert);
  if (*SubjectSize < (UINTN) X509Name->bytes->length) {
    *SubjectSize = (UINTN) X509Name->bytes->length;
    goto _Exit;
  }
  *SubjectSize = (UINTN) X509Name->bytes->length;
  if (CertSubject != NULL) {
    CopyMem (CertSubject, (UINT8 *)X509Name->bytes->data, *SubjectSize);
    Status = TRUE;
  }

_Exit:
  //
  // Release Resources.
  //
  BIO_free (CertBio);
  X509_free (X509Cert);

  return Status;
}

/**
  Retrieve the RSA Public Key from one DER-encoded X509 certificate.

  @param[in]  Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]  CertSize     Size of the X509 certificate in bytes.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA public key component. Use RsaFree() function to free the
                           resource.

  If Cert is NULL, then ASSERT().
  If RsaContext is NULL, then ASSERT().

  @retval  TRUE   RSA Public Key was retrieved successfully.
  @retval  FALSE  Fail to retrieve RSA public key from X509 certificate.

**/
BOOLEAN
EFIAPI
RsaGetPublicKeyFromX509 (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  )
{
  BOOLEAN   Status;
  EVP_PKEY  *Pkey;
  BIO       *CertBio;
  X509      *X509Cert;

  //
  // ASSERT if Cert is NULL or RsaContext is NULL.
  //
  ASSERT (Cert       != NULL);
  ASSERT (RsaContext != NULL);

  Status   = FALSE;
  Pkey     = NULL;
  CertBio  = NULL;
  X509Cert = NULL;

  //
  // Read DER-encoded X509 Certificate and Construct X509 object.
  //
  CertBio = BIO_new (BIO_s_mem ());
  BIO_write (CertBio, Cert, (int)CertSize);
  if (CertBio == NULL) {
    goto _Exit;
  }
  X509Cert = d2i_X509_bio (CertBio, NULL);
  if (X509Cert == NULL) {
    goto _Exit;
  }

  //
  // Retrieve and check EVP_PKEY data from X509 Certificate.
  //
  Pkey = X509_get_pubkey (X509Cert);
  if ((Pkey == NULL) || (Pkey->type != EVP_PKEY_RSA)) {
    goto _Exit;
  }

  //
  // Duplicate RSA Context from the retrieved EVP_PKEY.
  //
  if ((*RsaContext = RSAPublicKey_dup (Pkey->pkey.rsa)) != NULL) {
    Status = TRUE;
  }

_Exit:
  //
  // Release Resources.
  //
  BIO_free (CertBio);
  X509_free (X509Cert);
  EVP_PKEY_free (Pkey);

  return Status;
}

/**
  Verify one X509 certificate was issued by the trusted CA.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate to be verified.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      CACert       Pointer to the DER-encoded trusted CA certificate.
  @param[in]      CACertSize   Size of the CA Certificate in bytes.

  If Cert is NULL, then ASSERT().
  If CACert is NULL, then ASSERT().

  @retval  TRUE   The certificate was issued by the trusted CA.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.

**/
BOOLEAN
EFIAPI
X509VerifyCert (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  )
{
  BOOLEAN         Status;
  BIO             *BioCert;
  BIO             *BioCACert;
  X509            *X509Cert;
  X509            *X509CACert;
  X509_STORE      *CertStore;
  X509_STORE_CTX  CertCtx;

  //
  // ASSERT if Cert is NULL or CACert is NULL.
  //
  ASSERT (Cert   != NULL);
  ASSERT (CACert != NULL);

  Status     = FALSE;
  BioCert    = NULL;
  BioCACert  = NULL;
  X509Cert   = NULL;
  X509CACert = NULL;
  CertStore  = NULL;

  //
  // Register & Initialize necessary digest algorithms for certificate verification.
  //
  EVP_add_digest (EVP_md5());
  EVP_add_digest (EVP_sha1());
  EVP_add_digest (EVP_sha256());

  //
  // Read DER-encoded certificate to be verified and Construct X509 object.
  //
  BioCert = BIO_new (BIO_s_mem ());
  BIO_write (BioCert, Cert, (int)CertSize);
  if (BioCert == NULL) {
    goto _Exit;
  }
  X509Cert = d2i_X509_bio (BioCert, NULL);
  if (X509Cert == NULL) {
    goto _Exit;
  }

  //
  // Read DER-encoded root certificate and Construct X509 object.
  //
  BioCACert = BIO_new (BIO_s_mem());
  BIO_write (BioCACert, CACert, (int)CACertSize);
  if (BioCert == NULL) {
    goto _Exit;
  }
  X509CACert = d2i_X509_bio (BioCACert, NULL);
  if (CACert == NULL) {
    goto _Exit;
  }

  //
  // Set up X509 Store for trusted certificate.
  //
  CertStore = X509_STORE_new ();
  if (CertStore == NULL) {
    goto _Exit;
  }
  if (!(X509_STORE_add_cert (CertStore, X509CACert))) {
    goto _Exit;
  }

  //
  // Set up X509_STORE_CTX for the subsequent verification operation.
  //
  if (!X509_STORE_CTX_init (&CertCtx, CertStore, X509Cert, NULL)) {
    goto _Exit;
  }

  //
  // X509 Certificate Verification.
  //
  Status = (BOOLEAN) X509_verify_cert (&CertCtx);

_Exit:
  //
  // Release Resources.
  //
  BIO_free (BioCert);
  BIO_free (BioCACert);
  X509_free (X509Cert);
  X509_free (X509CACert);
  X509_STORE_free (CertStore);
  X509_STORE_CTX_cleanup (&CertCtx);

  return Status;
}
