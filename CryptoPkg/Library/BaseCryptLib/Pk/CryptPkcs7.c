/** @file
  PKCS#7 SignedData Verification Wrapper Implementation over OpenSSL.

  Caution: This module requires additional review when modified.
  This library will have external input - signature (e.g. UEFI Authenticated
  Variable). It may by input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  WrapPkcs7Data(), Pkcs7GetSigners(), Pkcs7Verify() will get UEFI Authenticated
  Variable and will do basic check for data structure.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
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

UINT8 mOidValue[9] = { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02 };

/**
  Verification callback function to override any existing callbacks in OpenSSL
  for intermediate certificate supports.

  @param[in]  Status   Original status before calling this callback.
  @param[in]  Context  X509 store context.

  @retval     1        Current X509 certificate is verified successfully.
  @retval     0        Verification failed.

**/
int
X509VerifyCb (
  IN int            Status,
  IN X509_STORE_CTX *Context
  )
{
  X509_OBJECT  *Obj;
  INTN         Error;
  INTN         Index;
  INTN         Count;

  Obj   = NULL;
  Error = (INTN) X509_STORE_CTX_get_error (Context);

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
    Obj = (X509_OBJECT *) malloc (sizeof (X509_OBJECT));
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
        Count = (INTN) sk_X509_num (Context->chain);
        for (Index = 0; Index < Count; Index++) {
          Obj->data.x509 = sk_X509_value (Context->chain, (int) Index);
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
  UINTN     P7DataSize;
  UINT8     *Tmp;

  //
  // Check input parameters.
  //
  if (PrivateKey == NULL || KeyPassword == NULL || InData == NULL ||
    SignCert == NULL || SignedData == NULL || SignedDataSize == NULL || InDataSize > INT_MAX) {
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
    Status = FALSE;
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
            PKCS7_BINARY | PKCS7_NOATTR | PKCS7_DETACHED
            );
  if (Pkcs7 == NULL) {
    Status = FALSE;
    goto _Exit;
  }

  //
  // Convert PKCS#7 signedData structure into DER-encoded buffer.
  //
  P7DataSize = i2d_PKCS7 (Pkcs7, NULL);
  if (P7DataSize <= 19) {
    Status = FALSE;
    goto _Exit;
  }

  P7Data     = malloc (P7DataSize);
  if (P7Data == NULL) {
    Status = FALSE;
    goto _Exit;
  }

  Tmp        = P7Data;
  P7DataSize = i2d_PKCS7 (Pkcs7, (unsigned char **) &Tmp);

  //
  // Strip ContentInfo to content only for signeddata. The data be trimmed off
  // is totally 19 bytes.
  //
  *SignedDataSize = P7DataSize - 19;
  *SignedData     = malloc (*SignedDataSize);
  if (*SignedData == NULL) {
    Status = FALSE;
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
  Check input P7Data is a wrapped ContentInfo structure or not. If not construct
  a new structure to wrap P7Data.

  Caution: This function may receive untrusted input.
  UEFI Authenticated Variable is external input, so this function will do basic
  check for PKCS#7 data structure.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] WrapFlag     If TRUE P7Data is a ContentInfo structure, otherwise
                           return FALSE.
  @param[out] WrapData     If return status of this function is TRUE: 
                           1) when WrapFlag is TRUE, pointer to P7Data.
                           2) when WrapFlag is FALSE, pointer to a new ContentInfo
                           structure. It's caller's responsibility to free this
                           buffer.
  @param[out] WrapDataSize Length of ContentInfo structure in bytes.

  @retval     TRUE         The operation is finished successfully.
  @retval     FALSE        The operation is failed due to lack of resources.

**/
BOOLEAN
WrapPkcs7Data (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT BOOLEAN      *WrapFlag,
  OUT UINT8        **WrapData,
  OUT UINTN        *WrapDataSize
  )
{
  BOOLEAN          Wrapped;
  UINT8            *SignedData;

  //
  // Check whether input P7Data is a wrapped ContentInfo structure or not.
  //
  Wrapped = FALSE;
  if ((P7Data[4] == 0x06) && (P7Data[5] == 0x09)) {
    if (CompareMem (P7Data + 6, mOidValue, sizeof (mOidValue)) == 0) {
      if ((P7Data[15] == 0xA0) && (P7Data[16] == 0x82)) {
        Wrapped = TRUE;
      }
    }
  }

  if (Wrapped) {
    *WrapData     = (UINT8 *) P7Data;
    *WrapDataSize = P7Length;
  } else {
    //
    // Wrap PKCS#7 signeddata to a ContentInfo structure - add a header in 19 bytes.
    //
    *WrapDataSize = P7Length + 19;
    *WrapData     = malloc (*WrapDataSize);
    if (*WrapData == NULL) {
      *WrapFlag = Wrapped;
      return FALSE;
    }

    SignedData = *WrapData;

    //
    // Part1: 0x30, 0x82.
    //
    SignedData[0] = 0x30;
    SignedData[1] = 0x82;

    //
    // Part2: Length1 = P7Length + 19 - 4, in big endian.
    //
    SignedData[2] = (UINT8) (((UINT16) (*WrapDataSize - 4)) >> 8);
    SignedData[3] = (UINT8) (((UINT16) (*WrapDataSize - 4)) & 0xff);

    //
    // Part3: 0x06, 0x09.
    //
    SignedData[4] = 0x06;
    SignedData[5] = 0x09;

    //
    // Part4: OID value -- 0x2A 0x86 0x48 0x86 0xF7 0x0D 0x01 0x07 0x02.
    //
    CopyMem (SignedData + 6, mOidValue, sizeof (mOidValue));

    //
    // Part5: 0xA0, 0x82.
    //
    SignedData[15] = 0xA0;
    SignedData[16] = 0x82;

    //
    // Part6: Length2 = P7Length, in big endian.
    //
    SignedData[17] = (UINT8) (((UINT16) P7Length) >> 8);
    SignedData[18] = (UINT8) (((UINT16) P7Length) & 0xff);

    //
    // Part7: P7Data.
    //
    CopyMem (SignedData + 19, P7Data, P7Length);
  }

  *WrapFlag = Wrapped;
  return TRUE;
}

/**
  Get the signer's certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, CertStack, StackLength, TrustedCert or CertLength is NULL, then
  return FALSE. If P7Length overflow, then return FAlSE.

  Caution: This function may receive untrusted input.
  UEFI Authenticated Variable is external input, so this function will do basic
  check for PKCS#7 data structure.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] CertStack    Pointer to Signer's certificates retrieved from P7Data.
                           It's caller's responsiblity to free the buffer.
  @param[out] StackLength  Length of signer's certificates in bytes.
  @param[out] TrustedCert  Pointer to a trusted certificate from Signer's certificates.
                           It's caller's responsiblity to free the buffer.
  @param[out] CertLength   Length of the trusted certificate in bytes.

  @retval  TRUE            The operation is finished successfully.
  @retval  FALSE           Error occurs during the operation.

**/
BOOLEAN
EFIAPI
Pkcs7GetSigners (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **CertStack,
  OUT UINTN        *StackLength,
  OUT UINT8        **TrustedCert,
  OUT UINTN        *CertLength
  )
{
  PKCS7            *Pkcs7;
  BOOLEAN          Status;
  UINT8            *SignedData;
  UINT8            *Temp;
  UINTN            SignedDataSize;
  BOOLEAN          Wrapped;
  STACK_OF(X509)   *Stack;
  UINT8            Index;
  UINT8            *CertBuf;
  UINT8            *OldBuf;
  UINTN            BufferSize;
  UINTN            OldSize;
  UINT8            *SingleCert;
  UINTN            SingleCertSize;

  if ((P7Data == NULL) || (CertStack == NULL) || (StackLength == NULL) ||
      (TrustedCert == NULL) || (CertLength == NULL) || (P7Length > INT_MAX)) {
    return FALSE;
  }
  
  Status = WrapPkcs7Data (P7Data, P7Length, &Wrapped, &SignedData, &SignedDataSize);
  if (!Status) {
    return Status;
  }

  Status     = FALSE;
  Pkcs7      = NULL;
  Stack      = NULL;
  CertBuf    = NULL;
  OldBuf     = NULL;
  SingleCert = NULL;

  //
  // Retrieve PKCS#7 Data (DER encoding)
  //
  if (SignedDataSize > INT_MAX) {
    goto _Exit;
  }

  Temp = SignedData;
  Pkcs7 = d2i_PKCS7 (NULL, (const unsigned char **) &Temp, (int) SignedDataSize);
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // Check if it's PKCS#7 Signed Data (for Authenticode Scenario)
  //
  if (!PKCS7_type_is_signed (Pkcs7)) {
    goto _Exit;
  }

  Stack = PKCS7_get0_signers(Pkcs7, NULL, PKCS7_BINARY);
  if (Stack == NULL) {
    goto _Exit;
  }

  //
  // Convert CertStack to buffer in following format:
  // UINT8  CertNumber;
  // UINT32 Cert1Length;
  // UINT8  Cert1[];
  // UINT32 Cert2Length;
  // UINT8  Cert2[];
  // ...
  // UINT32 CertnLength;
  // UINT8  Certn[];
  //
  BufferSize = sizeof (UINT8);
  OldSize    = BufferSize;
  
  for (Index = 0; ; Index++) {
    Status = X509PopCertificate (Stack, &SingleCert, &SingleCertSize);
    if (!Status) {
      break;
    }

    OldSize    = BufferSize;
    OldBuf     = CertBuf;
    BufferSize = OldSize + SingleCertSize + sizeof (UINT32);
    CertBuf    = malloc (BufferSize);

    if (CertBuf == NULL) {
      goto _Exit;
    }

    if (OldBuf != NULL) {
      CopyMem (CertBuf, OldBuf, OldSize);
      free (OldBuf);
      OldBuf = NULL;
    }

    WriteUnaligned32 ((UINT32 *) (CertBuf + OldSize), (UINT32) SingleCertSize);
    CopyMem (CertBuf + OldSize + sizeof (UINT32), SingleCert, SingleCertSize);

    free (SingleCert);
    SingleCert = NULL;
  }

  if (CertBuf != NULL) {
    //
    // Update CertNumber.
    //
    CertBuf[0] = Index;

    *CertLength = BufferSize - OldSize - sizeof (UINT32);
    *TrustedCert = malloc (*CertLength);
    if (*TrustedCert == NULL) {
      goto _Exit;
    }

    CopyMem (*TrustedCert, CertBuf + OldSize + sizeof (UINT32), *CertLength);
    *CertStack   = CertBuf;
    *StackLength = BufferSize;
    Status = TRUE;
  } 

_Exit:
  //
  // Release Resources
  //
  if (!Wrapped) {
    free (SignedData);
  }

  if (Pkcs7 != NULL) {
    PKCS7_free (Pkcs7);
  }

  if (Stack != NULL) {
    sk_X509_pop_free(Stack, X509_free);
  }

  if (SingleCert !=  NULL) {
    free (SingleCert);
  }

  if (!Status && (CertBuf != NULL)) {
    free (CertBuf);
    *CertStack = NULL;
  }

  if (OldBuf != NULL) {
    free (OldBuf);
  }
  
  return Status;
}

/**
  Wrap function to use free() to free allocated memory for certificates.

  @param[in]  Certs        Pointer to the certificates to be freed.

**/
VOID
EFIAPI
Pkcs7FreeSigners (
  IN  UINT8        *Certs
  )
{
  if (Certs == NULL) {
    return;
  }

  free (Certs);
}

/**
  Verifies the validility of a PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, TrustedCert or InData is NULL, then return FALSE.
  If P7Length, CertLength or DataLength overflow, then return FAlSE.

  Caution: This function may receive untrusted input.
  UEFI Authenticated Variable is external input, so this function will do basic
  check for PKCS#7 data structure.

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
  UINT8       *SignedData;
  UINT8       *Temp;
  UINTN       SignedDataSize;
  BOOLEAN     Wrapped;

  //
  // Check input parameters.
  //
  if (P7Data == NULL || TrustedCert == NULL || InData == NULL || 
    P7Length > INT_MAX || CertLength > INT_MAX || DataLength > INT_MAX) {
    return FALSE;
  }
  
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

  Status = WrapPkcs7Data (P7Data, P7Length, &Wrapped, &SignedData, &SignedDataSize);
  if (!Status) {
    return Status;
  }
  
  //
  // Retrieve PKCS#7 Data (DER encoding)
  //
  if (SignedDataSize > INT_MAX) {
    goto _Exit;
  }

  Temp = SignedData;
  Pkcs7 = d2i_PKCS7 (NULL, (const unsigned char **) &Temp, (int) SignedDataSize);
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

  if (!Wrapped) {
    OPENSSL_free (SignedData);
  }

  return Status;
}
