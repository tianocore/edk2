/** @file
  PKCS#7 SignedData Verification Wrapper Implementation over OpenSSL.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
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
  UINT8       *Content;
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
  // Check PKCS#7 embedded signed content with InData.
  //
  if (InData != NULL) {
    //
    // NOTE: PKCS7_dataDecode() didn't work for Authenticode-format signed data due to
    //       some authenticode-specific structure. Use opaque ASN.1 string to retrieve
    //       PKCS#7 ContentInfo here.
    //
    Content = (UINT8 *)(Pkcs7->d.sign->contents->d.other->value.asn1_string->data);

    // Ignore two bytes for DER encoding of ASN.1 "SEQUENCE"
    if (CompareMem (Content + 2, InData, DataLength) != 0) {
      goto _Exit;
    }
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
  // For generic PKCS#7 handling, InData may be NULL if the content is present
  // in PKCS#7 structure. So ignore NULL checking here.
  //
  DataBio = BIO_new (BIO_s_mem ());
  BIO_write (DataBio, InData, (int)DataLength);

  //
  // Verifies the PKCS#7 signedData structure
  //
  Status = (BOOLEAN) PKCS7_verify (Pkcs7, NULL, CertStore, DataBio, NULL, 0);

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
