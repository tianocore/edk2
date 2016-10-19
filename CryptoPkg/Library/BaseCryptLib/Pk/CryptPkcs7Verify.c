/** @file
  PKCS#7 SignedData Verification Wrapper Implementation over OpenSSL.

  Caution: This module requires additional review when modified.
  This library will have external input - signature (e.g. UEFI Authenticated
  Variable). It may by input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  WrapPkcs7Data(), Pkcs7GetSigners(), Pkcs7Verify() will get UEFI Authenticated
  Variable and will do basic check for data structure.

Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
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
#include <openssl/x509v3.h>
#include <openssl/pkcs7.h>

UINT8 mOidValue[9] = { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02 };

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
  Pop single certificate from STACK_OF(X509).

  If X509Stack, Cert, or CertSize is NULL, then return FALSE.

  @param[in]  X509Stack       Pointer to a X509 stack object.
  @param[out] Cert            Pointer to a X509 certificate.
  @param[out] CertSize        Length of output X509 certificate in bytes.

  @retval     TRUE            The X509 stack pop succeeded.
  @retval     FALSE           The pop operation failed.

**/
BOOLEAN
X509PopCertificate (
  IN  VOID  *X509Stack,
  OUT UINT8 **Cert,
  OUT UINTN *CertSize
  )
{
  BIO             *CertBio;
  X509            *X509Cert;
  STACK_OF(X509)  *CertStack;
  BOOLEAN         Status;
  INT32           Result;
  INT32           Length;
  VOID            *Buffer;

  Status = FALSE;

  if ((X509Stack == NULL) || (Cert == NULL) || (CertSize == NULL)) {
    return Status;
  }

  CertStack = (STACK_OF(X509) *) X509Stack;

  X509Cert = sk_X509_pop (CertStack);

  if (X509Cert == NULL) {
    return Status;
  }

  Buffer = NULL;

  CertBio = BIO_new (BIO_s_mem ());
  if (CertBio == NULL) {
    return Status;
  }

  Result = i2d_X509_bio (CertBio, X509Cert);
  if (Result == 0) {
    goto _Exit;
  }

  Length = (INT32)(((BUF_MEM *) CertBio->ptr)->length);
  if (Length <= 0) {
    goto _Exit;
  }

  Buffer = malloc (Length);
  if (Buffer == NULL) {
    goto _Exit;
  }

  Result = BIO_read (CertBio, Buffer, Length);
  if (Result != Length) {
    goto _Exit;
  }

  *Cert     = Buffer;
  *CertSize = Length;

  Status = TRUE;

_Exit:

  BIO_free (CertBio);

  if (!Status && (Buffer != NULL)) {
    free (Buffer);
  }

  return Status;
}

/**
  Get the signer's certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, CertStack, StackLength, TrustedCert or CertLength is NULL, then
  return FALSE. If P7Length overflow, then return FALSE.

  Caution: This function may receive untrusted input.
  UEFI Authenticated Variable is external input, so this function will do basic
  check for PKCS#7 data structure.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] CertStack    Pointer to Signer's certificates retrieved from P7Data.
                           It's caller's responsibility to free the buffer.
  @param[out] StackLength  Length of signer's certificates in bytes.
  @param[out] TrustedCert  Pointer to a trusted certificate from Signer's certificates.
                           It's caller's responsibility to free the buffer.
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
  CONST UINT8      *Temp;
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
  Retrieves all embedded certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard", and outputs two certificate lists chained and
  unchained to the signer's certificates.
  The input signed data could be wrapped in a ContentInfo structure.

  @param[in]  P7Data            Pointer to the PKCS#7 message.
  @param[in]  P7Length          Length of the PKCS#7 message in bytes.
  @param[out] SignerChainCerts  Pointer to the certificates list chained to signer's
                                certificate. It's caller's responsibility to free the buffer.
  @param[out] ChainLength       Length of the chained certificates list buffer in bytes.
  @param[out] UnchainCerts      Pointer to the unchained certificates lists. It's caller's
                                responsibility to free the buffer.
  @param[out] UnchainLength     Length of the unchained certificates list buffer in bytes.

  @retval  TRUE         The operation is finished successfully.
  @retval  FALSE        Error occurs during the operation.

**/
BOOLEAN
EFIAPI
Pkcs7GetCertificatesList (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **SignerChainCerts,
  OUT UINTN        *ChainLength,
  OUT UINT8        **UnchainCerts,
  OUT UINTN        *UnchainLength
  )
{
  BOOLEAN          Status;
  UINT8            *NewP7Data;
  UINTN            NewP7Length;
  BOOLEAN          Wrapped;
  UINT8            Index;
  PKCS7            *Pkcs7;
  X509_STORE_CTX   CertCtx;
  STACK_OF(X509)   *Signers;
  X509             *Signer;
  X509             *Cert;
  X509             *TempCert;
  X509             *Issuer;
  UINT8            *CertBuf;
  UINT8            *OldBuf;
  UINTN            BufferSize;
  UINTN            OldSize;
  UINT8            *SingleCert;
  UINTN            CertSize;

  //
  // Initializations
  //
  Status         = FALSE;
  NewP7Data      = NULL;
  Pkcs7          = NULL;
  Cert           = NULL;
  TempCert       = NULL;
  SingleCert     = NULL;
  CertBuf        = NULL;
  OldBuf         = NULL;
  Signers        = NULL;

  ZeroMem (&CertCtx, sizeof (CertCtx));

  //
  // Parameter Checking
  //
  if ((P7Data == NULL) || (SignerChainCerts == NULL) || (ChainLength == NULL) ||
      (UnchainCerts == NULL) || (UnchainLength == NULL) || (P7Length > INT_MAX)) {
    return Status;
  }

  *SignerChainCerts = NULL;
  *ChainLength      = 0;
  *UnchainCerts     = NULL;
  *UnchainLength    = 0;

  //
  // Construct a new PKCS#7 data wrapping with ContentInfo structure if needed.
  //
  Status = WrapPkcs7Data (P7Data, P7Length, &Wrapped, &NewP7Data, &NewP7Length);
  if (!Status || (NewP7Length > INT_MAX)) {
    goto _Error;
  }

  //
  // Decodes PKCS#7 SignedData
  //
  Pkcs7 = d2i_PKCS7 (NULL, (const unsigned char **) &NewP7Data, (int) NewP7Length);
  if ((Pkcs7 == NULL) || (!PKCS7_type_is_signed (Pkcs7))) {
    goto _Error;
  }

  //
  // Obtains Signer's Certificate from PKCS#7 data
  // NOTE: Only one signer case will be handled in this function, which means SignerInfos
  //       should include only one signer's certificate.
  //
  Signers = PKCS7_get0_signers (Pkcs7, NULL, PKCS7_BINARY);
  if ((Signers == NULL) || (sk_X509_num (Signers) != 1)) {
    goto _Error;
  }
  Signer = sk_X509_value (Signers, 0);

  if (!X509_STORE_CTX_init (&CertCtx, NULL, Signer, Pkcs7->d.sign->cert)) {
    goto _Error;
  }
  //
  // Initialize Chained & Untrusted stack
  //
  if (CertCtx.chain == NULL) {
    if (((CertCtx.chain = sk_X509_new_null ()) == NULL) ||
        (!sk_X509_push (CertCtx.chain, CertCtx.cert))) {
      goto _Error;
    }
  }
  (VOID)sk_X509_delete_ptr (CertCtx.untrusted, Signer);

  //
  // Build certificates stack chained from Signer's certificate.
  //
  Cert = Signer;
  for (; ;) {
    //
    // Self-Issue checking
    //
    if (CertCtx.check_issued (&CertCtx, Cert, Cert)) {
      break;
    }

    //
    // Found the issuer of the current certificate
    //
    if (CertCtx.untrusted != NULL) {
      Issuer = NULL;
      for (Index = 0; Index < sk_X509_num (CertCtx.untrusted); Index++) {
        TempCert = sk_X509_value (CertCtx.untrusted, Index);
        if (CertCtx.check_issued (&CertCtx, Cert, TempCert)) {
          Issuer = TempCert;
          break;
        }
      }
      if (Issuer != NULL) {
        if (!sk_X509_push (CertCtx.chain, Issuer)) {
          goto _Error;
        }
        (VOID)sk_X509_delete_ptr (CertCtx.untrusted, Issuer);

        Cert = Issuer;
        continue;
      }
    }

    break;
  }

  //
  // Converts Chained and Untrusted Certificate to Certificate Buffer in following format:
  //      UINT8  CertNumber;
  //      UINT32 Cert1Length;
  //      UINT8  Cert1[];
  //      UINT32 Cert2Length;
  //      UINT8  Cert2[];
  //      ...
  //      UINT32 CertnLength;
  //      UINT8  Certn[];
  //

  if (CertCtx.chain != NULL) {
    BufferSize = sizeof (UINT8);
    OldSize    = BufferSize;
    CertBuf    = NULL;

    for (Index = 0; ; Index++) {
      Status = X509PopCertificate (CertCtx.chain, &SingleCert, &CertSize);
      if (!Status) {
        break;
      }

      OldSize    = BufferSize;
      OldBuf     = CertBuf;
      BufferSize = OldSize + CertSize + sizeof (UINT32);
      CertBuf    = malloc (BufferSize);

      if (CertBuf == NULL) {
        Status = FALSE;
        goto _Error;
      }
      if (OldBuf != NULL) {
        CopyMem (CertBuf, OldBuf, OldSize);
        free (OldBuf);
        OldBuf = NULL;
      }

      WriteUnaligned32 ((UINT32 *) (CertBuf + OldSize), (UINT32) CertSize);
      CopyMem (CertBuf + OldSize + sizeof (UINT32), SingleCert, CertSize);

      free (SingleCert);
      SingleCert = NULL;
    }

    if (CertBuf != NULL) {
      //
      // Update CertNumber.
      //
      CertBuf[0] = Index;

      *SignerChainCerts = CertBuf;
      *ChainLength      = BufferSize;
    }
  }

  if (CertCtx.untrusted != NULL) {
    BufferSize = sizeof (UINT8);
    OldSize    = BufferSize;
    CertBuf    = NULL;

    for (Index = 0; ; Index++) {
      Status = X509PopCertificate (CertCtx.untrusted, &SingleCert, &CertSize);
      if (!Status) {
        break;
      }

      OldSize    = BufferSize;
      OldBuf     = CertBuf;
      BufferSize = OldSize + CertSize + sizeof (UINT32);
      CertBuf    = malloc (BufferSize);

      if (CertBuf == NULL) {
        Status = FALSE;
        goto _Error;
      }
      if (OldBuf != NULL) {
        CopyMem (CertBuf, OldBuf, OldSize);
        free (OldBuf);
        OldBuf = NULL;
      }

      WriteUnaligned32 ((UINT32 *) (CertBuf + OldSize), (UINT32) CertSize);
      CopyMem (CertBuf + OldSize + sizeof (UINT32), SingleCert, CertSize);

      free (SingleCert);
      SingleCert = NULL;
    }

    if (CertBuf != NULL) {
      //
      // Update CertNumber.
      //
      CertBuf[0] = Index;

      *UnchainCerts  = CertBuf;
      *UnchainLength = BufferSize;
    }
  }

  Status = TRUE;

_Error:
  //
  // Release Resources.
  //
  if (!Wrapped && (NewP7Data != NULL)) {
    free (NewP7Data);
  }

  if (Pkcs7 != NULL) {
    PKCS7_free (Pkcs7);
  }
  sk_X509_free (Signers);

  X509_STORE_CTX_cleanup (&CertCtx);

  if (SingleCert != NULL) {
    free (SingleCert);
  }

  if (OldBuf != NULL) {
    free (OldBuf);
  }

  if (!Status && (CertBuf != NULL)) {
    free (CertBuf);
    *SignerChainCerts = NULL;
    *UnchainCerts     = NULL;
  }

  return Status;
}

/**
  Verifies the validity of a PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, TrustedCert or InData is NULL, then return FALSE.
  If P7Length, CertLength or DataLength overflow, then return FALSE.

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
  BIO         *DataBio;
  BOOLEAN     Status;
  X509        *Cert;
  X509_STORE  *CertStore;
  UINT8       *SignedData;
  CONST UINT8 *Temp;
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
  DataBio   = NULL;
  Cert      = NULL;
  CertStore = NULL;

  //
  // Register & Initialize necessary digest algorithms for PKCS#7 Handling
  //
  if (EVP_add_digest (EVP_md5 ()) == 0) {
    return FALSE;
  }
  if (EVP_add_digest (EVP_sha1 ()) == 0) {
    return FALSE;
  }
  if (EVP_add_digest (EVP_sha256 ()) == 0) {
    return FALSE;
  }
  if (EVP_add_digest (EVP_sha384 ()) == 0) {
    return FALSE;
  }
  if (EVP_add_digest (EVP_sha512 ()) == 0) {
    return FALSE;
  }
  if (EVP_add_digest_alias (SN_sha1WithRSAEncryption, SN_sha1WithRSA) == 0) {
    return FALSE;
  }

  Status = WrapPkcs7Data (P7Data, P7Length, &Wrapped, &SignedData, &SignedDataSize);
  if (!Status) {
    return Status;
  }

  Status = FALSE;

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
  Temp = TrustedCert;
  Cert = d2i_X509 (NULL, &Temp, (long) CertLength);
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
  if (DataBio == NULL) {
    goto _Exit;
  }

  if (BIO_write (DataBio, InData, (int) DataLength) <= 0) {
    goto _Exit;
  }

  //
  // Allow partial certificate chains, terminated by a non-self-signed but
  // still trusted intermediate certificate. Also disable time checks.
  //
  X509_STORE_set_flags (CertStore,
                        X509_V_FLAG_PARTIAL_CHAIN | X509_V_FLAG_NO_CHECK_TIME);

  //
  // OpenSSL PKCS7 Verification by default checks for SMIME (email signing) and
  // doesn't support the extended key usage for Authenticode Code Signing.
  // Bypass the certificate purpose checking by enabling any purposes setting.
  //
  X509_STORE_set_purpose (CertStore, X509_PURPOSE_ANY);

  //
  // Verifies the PKCS#7 signedData structure
  //
  Status = (BOOLEAN) PKCS7_verify (Pkcs7, NULL, CertStore, DataBio, NULL, PKCS7_BINARY);

_Exit:
  //
  // Release Resources
  //
  BIO_free (DataBio);
  X509_free (Cert);
  X509_STORE_free (CertStore);
  PKCS7_free (Pkcs7);

  if (!Wrapped) {
    OPENSSL_free (SignedData);
  }

  return Status;
}

/**
  Extracts the attached content from a PKCS#7 signed data if existed. The input signed
  data could be wrapped in a ContentInfo structure.

  If P7Data, Content, or ContentSize is NULL, then return FALSE. If P7Length overflow,
  then return FALSE. If the P7Data is not correctly formatted, then return FALSE.

  Caution: This function may receive untrusted input. So this function will do
           basic check for PKCS#7 data structure.

  @param[in]   P7Data       Pointer to the PKCS#7 signed data to process.
  @param[in]   P7Length     Length of the PKCS#7 signed data in bytes.
  @param[out]  Content      Pointer to the extracted content from the PKCS#7 signedData.
                            It's caller's responsibility to free the buffer.
  @param[out]  ContentSize  The size of the extracted content in bytes.

  @retval     TRUE          The P7Data was correctly formatted for processing.
  @retval     FALSE         The P7Data was not correctly formatted for processing.

*/
BOOLEAN
EFIAPI
Pkcs7GetAttachedContent (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT VOID         **Content,
  OUT UINTN        *ContentSize
  )
{
  BOOLEAN            Status;
  PKCS7              *Pkcs7;
  UINT8              *SignedData;
  UINTN              SignedDataSize;
  BOOLEAN            Wrapped;
  CONST UINT8        *Temp;
  ASN1_OCTET_STRING  *OctStr;

  //
  // Check input parameter.
  //
  if ((P7Data == NULL) || (P7Length > INT_MAX) || (Content == NULL) || (ContentSize == NULL)) {
    return FALSE;
  }

  *Content   = NULL;
  Pkcs7      = NULL;
  SignedData = NULL;
  OctStr     = NULL;

  Status = WrapPkcs7Data (P7Data, P7Length, &Wrapped, &SignedData, &SignedDataSize);
  if (!Status || (SignedDataSize > INT_MAX)) {
    goto _Exit;
  }

  Status = FALSE;

  //
  // Decoding PKCS#7 SignedData
  //
  Temp  = SignedData;
  Pkcs7 = d2i_PKCS7 (NULL, (const unsigned char **)&Temp, (int)SignedDataSize);
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // The type of Pkcs7 must be signedData
  //
  if (!PKCS7_type_is_signed (Pkcs7)) {
    goto _Exit;
  }

  //
  // Check for detached or attached content
  //
  if (PKCS7_get_detached (Pkcs7)) {
    //
    // No Content supplied for PKCS7 detached signedData
    //
    *Content     = NULL;
    *ContentSize = 0;
  } else {
    //
    // Retrieve the attached content in PKCS7 signedData
    //
    OctStr = Pkcs7->d.sign->contents->d.data;
    if ((OctStr->length > 0) && (OctStr->data != NULL)) {
      *ContentSize = OctStr->length;
      *Content     = malloc (*ContentSize);
      if (*Content == NULL) {
        *ContentSize = 0;
        goto _Exit;
      }
      CopyMem (*Content, OctStr->data, *ContentSize);
    }
  }
  Status = TRUE;

_Exit:
  //
  // Release Resources
  //
  PKCS7_free (Pkcs7);

  if (!Wrapped) {
    OPENSSL_free (SignedData);
  }

  return Status;
}
