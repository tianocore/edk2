/** @file
  This module verifies that Enhanced Key Usages (EKU's) are present within
  a PKCS7 signature blob using OpenSSL.

  Copyright (C) Microsoft Corporation. All Rights Reserved.
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include "InternalCryptLib.h"
#include <openssl/x509v3.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <internal/x509_int.h>
#include <openssl/pkcs7.h>
#include <openssl/bn.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <internal/asn1_int.h>

/**
  This function will return the leaf signer certificate in a chain.  This is
  required because certificate chains are not guaranteed to have the
  certificates in the order that they were issued.

  A typical certificate chain looks like this:


                 ----------------------------
                |            Root            |
                 ----------------------------
                               ^
                               |
                 ----------------------------
                |          Policy CA         | <-- Typical Trust Anchor.
                 ----------------------------
                               ^
                               |
                 ----------------------------
                |         Issuing CA         |
                 ----------------------------
                               ^
                               |
                 -----------------------------
                /  End-Entity (leaf) signer  / <-- Bottom certificate.
                -----------------------------  EKU: "1.3.6.1.4.1.311.76.9.21.1"
                                                    (Firmware Signing)


  @param[in]   CertChain            Certificate chain.

  @param[out]  SignerCert           Last certificate in the chain.  For PKCS7 signatures,
                                    this will be the end-entity (leaf) signer cert.

  @retval EFI_SUCCESS               The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid.
  @retval EFI_NOT_FOUND             The number of signers found was not 1.

**/
EFI_STATUS
GetSignerCertificate (
  IN CONST PKCS7 *CertChain,
  OUT X509       **SignerCert
  )
{
  EFI_STATUS      Status;
  STACK_OF(X509)  *Signers;
  INT32           NumberSigners;

  Status         = EFI_SUCCESS;
  Signers        = NULL;
  NumberSigners  = 0;

  if (CertChain == NULL || SignerCert == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Get the signers from the chain.
  //
  Signers = PKCS7_get0_signers ((PKCS7*) CertChain, NULL, PKCS7_BINARY);
  if (Signers == NULL) {
    //
    // Fail to get signers form PKCS7
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // There should only be one signer in the PKCS7 stack.
  //
  NumberSigners = sk_X509_num (Signers);
  if (NumberSigners != 1) {
    //
    // The number of singers should have been 1
    //
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  *SignerCert = sk_X509_value (Signers, 0);

Exit:
  //
  // Release Resources
  //
  if (Signers) {
    sk_X509_free (Signers);
  }

  return Status;
}


/**
  Determines if the specified EKU represented in ASN1 form is present
  in a given certificate.

  @param[in]  Cert                  The certificate to check.

  @param[in]  Asn1ToFind            The EKU to look for.

  @retval EFI_SUCCESS               We successfully identified the signing type.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid.
  @retval EFI_NOT_FOUND             One or more EKU's were not found in the signature.

**/
EFI_STATUS
IsEkuInCertificate (
  IN CONST X509  *Cert,
  IN ASN1_OBJECT *Asn1ToFind
  )
{
  EFI_STATUS          Status;
  X509                *ClonedCert;
  X509_EXTENSION      *Extension;
  EXTENDED_KEY_USAGE  *Eku;
  INT32               ExtensionIndex;
  INTN                NumExtensions;
  ASN1_OBJECT         *Asn1InCert;
  INTN                Index;

  Status            = EFI_NOT_FOUND;
  ClonedCert        = NULL;
  Extension         = NULL;
  Eku               = NULL;
  ExtensionIndex    = -1;
  NumExtensions     = 0;
  Asn1InCert        = NULL;

  if (Cert == NULL || Asn1ToFind == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Clone the certificate.  This is required because the Extension API's
  // only work once per instance of an X509 object.
  //
  ClonedCert = X509_dup ((X509*)Cert);
  if (ClonedCert == NULL) {
    //
    // Fail to duplicate cert.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Look for the extended key usage.
  //
  ExtensionIndex = X509_get_ext_by_NID (ClonedCert, NID_ext_key_usage, -1);

  if (ExtensionIndex < 0) {
    //
    // Fail to find 'NID_ext_key_usage' in Cert.
    //
    goto Exit;
  }

  Extension = X509_get_ext (ClonedCert, ExtensionIndex);
  if (Extension == NULL) {
    //
    // Fail to get Extension form cert.
    //
    goto Exit;
  }

  Eku = (EXTENDED_KEY_USAGE*)X509V3_EXT_d2i (Extension);
  if (Eku == NULL) {
    //
    // Fail to get Eku from extension.
    //
    goto Exit;
  }

  NumExtensions = sk_ASN1_OBJECT_num (Eku);

  //
  // Now loop through the extensions, looking for the specified Eku.
  //
  for (Index = 0; Index < NumExtensions; Index++) {
    Asn1InCert = sk_ASN1_OBJECT_value (Eku, (INT32)Index);
    if (Asn1InCert == NULL) {
      //
      // Fail to get ASN object from Eku.
      //
      goto Exit;
    }

    if (Asn1InCert->length == Asn1ToFind->length &&
        CompareMem (Asn1InCert->data, Asn1ToFind->data, Asn1InCert->length) == 0) {
      //
      // Found Eku in certificate.
      //
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

Exit:

  //
  // Release Resources
  //
  if (ClonedCert) {
    X509_free (ClonedCert);
  }

  if (Eku) {
    sk_ASN1_OBJECT_pop_free (Eku, ASN1_OBJECT_free);
  }

  return Status;
}


/**
  Determines if the specified EKUs are present in a signing certificate.

  @param[in]  SignerCert            The certificate to check.
  @param[in]  RequiredEKUs          The EKUs to look for.
  @param[in]  RequiredEKUsSize      The number of EKUs
  @param[in]  RequireAllPresent     If TRUE, then all the specified EKUs
                                    must be present in the certificate.

  @retval EFI_SUCCESS               We successfully identified the signing type.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid.
  @retval EFI_NOT_FOUND             One or more EKU's were not found in the signature.
**/
EFI_STATUS
CheckEKUs(
  IN CONST X509     *SignerCert,
  IN CONST CHAR8    *RequiredEKUs[],
  IN CONST UINT32   RequiredEKUsSize,
  IN BOOLEAN        RequireAllPresent
  )
{
  EFI_STATUS    Status;
  ASN1_OBJECT   *Asn1ToFind;
  UINT32        NumEkusFound;
  UINT32        Index;

  Status       = EFI_SUCCESS;
  Asn1ToFind   = NULL;
  NumEkusFound = 0;

  if (SignerCert == NULL || RequiredEKUs == NULL || RequiredEKUsSize == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  for (Index = 0; Index < RequiredEKUsSize; Index++) {
    //
    // Finding required EKU in cert.
    //
    if (Asn1ToFind) {
      ASN1_OBJECT_free(Asn1ToFind);
      Asn1ToFind = NULL;
    }

    Asn1ToFind = OBJ_txt2obj (RequiredEKUs[Index], 0);
    if (!Asn1ToFind) {
      //
      // Fail to convert required EKU to ASN1.
      //
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    Status = IsEkuInCertificate (SignerCert, Asn1ToFind);
    if (Status == EFI_SUCCESS) {
      NumEkusFound++;
      if (!RequireAllPresent) {
        //
        // Found at least one, so we are done.
        //
        goto Exit;
      }
    } else {
      //
      // Fail to find Eku in cert
      break;
    }
  }

Exit:

  if (Asn1ToFind) {
    ASN1_OBJECT_free(Asn1ToFind);
  }

  if (RequireAllPresent &&
      NumEkusFound == RequiredEKUsSize) {
    //
    // Found all required EKUs in certificate.
    //
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  This function receives a PKCS#7 formatted signature blob,
  looks for the EKU SEQUENCE blob, and if found then looks
  for all the required EKUs. This function was created so that
  the Surface team can cut down on the number of Certificate
  Authorities (CA's) by checking EKU's on leaf signers for
  a specific product. This prevents one product's certificate
  from signing another product's firmware or unlock blobs.

  Note that this function does not validate the certificate chain.
  That needs to be done before using this function.

  @param[in]  Pkcs7Signature       The PKCS#7 signed information content block. An array
                                   containing the content block with both the signature,
                                   the signer's certificate, and any necessary intermediate
                                   certificates.
  @param[in]  Pkcs7SignatureSize   Number of bytes in Pkcs7Signature.
  @param[in]  RequiredEKUs         Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.
  @param[in]  RequiredEKUsSize     Number of elements in the RequiredEKUs string array.
  @param[in]  RequireAllPresent    If this is TRUE, then all of the specified EKU's
                                   must be present in the leaf signer.  If it is
                                   FALSE, then we will succeed if we find any
                                   of the specified EKU's.

  @retval EFI_SUCCESS              The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER    A parameter was invalid.
  @retval EFI_NOT_FOUND            One or more EKU's were not found in the signature.

**/
EFI_STATUS
EFIAPI
VerifyEKUsInPkcs7Signature (
  IN CONST UINT8    *Pkcs7Signature,
  IN CONST UINT32   SignatureSize,
  IN CONST CHAR8    *RequiredEKUs[],
  IN CONST UINT32   RequiredEKUsSize,
  IN BOOLEAN        RequireAllPresent
  )
{
  EFI_STATUS        Status;
  PKCS7             *Pkcs7;
  STACK_OF(X509)    *CertChain;
  INT32             SignatureType;
  INT32             NumberCertsInSignature;
  X509              *SignerCert;
  UINT8             *SignedData;
  UINT8             *Temp;
  UINTN             SignedDataSize;
  BOOLEAN           IsWrapped;
  BOOLEAN           Ok;

  Status                    = EFI_SUCCESS;
  Pkcs7                     = NULL;
  CertChain                 = NULL;
  SignatureType             = 0;
  NumberCertsInSignature    = 0;
  SignerCert                = NULL;
  SignedData                = NULL;
  SignedDataSize            = 0;
  IsWrapped                 = FALSE;
  Ok                        = FALSE;

  //
  //Validate the input parameters.
  //
  if (Pkcs7Signature   == NULL ||
      SignatureSize    == 0    ||
      RequiredEKUs     == NULL ||
      RequiredEKUsSize == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (RequiredEKUsSize == 1) {
    RequireAllPresent = TRUE;
  }

  //
  // Wrap the PKCS7 data if needed.
  //
  Ok = WrapPkcs7Data (Pkcs7Signature,
                      SignatureSize,
                      &IsWrapped,
                      &SignedData,
                      &SignedDataSize);
  if (!Ok) {
    //
    // Fail to Wrap the PKCS7 data.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Temp = SignedData;

  //
  // Create the PKCS7 object.
  //
  Pkcs7 = d2i_PKCS7 (NULL, (const unsigned char **)&Temp, (INT32)SignedDataSize);
  if (Pkcs7 == NULL) {
    //
    // Fail to read PKCS7 data.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Get the certificate chain.
  //
  SignatureType = OBJ_obj2nid (Pkcs7->type);
  switch (SignatureType) {
  case NID_pkcs7_signed:
    if (Pkcs7->d.sign != NULL) {
      CertChain = Pkcs7->d.sign->cert;
    }
    break;
  case NID_pkcs7_signedAndEnveloped:
    if (Pkcs7->d.signed_and_enveloped != NULL) {
      CertChain = Pkcs7->d.signed_and_enveloped->cert;
    }
    break;
  default:
    break;
  }

  //
  // Ensure we have a certificate stack
  //
  if (CertChain == NULL) {
    //
    // Fail to get the certificate stack from signature.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Find out how many certificates were in the PKCS7 signature.
  //
  NumberCertsInSignature = sk_X509_num (CertChain);

  if (NumberCertsInSignature == 0) {
    //
    // Fail to find any certificates in signature.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Get the leaf signer.
  //
  Status = GetSignerCertificate (Pkcs7, &SignerCert);
  if (Status != EFI_SUCCESS || SignerCert == NULL) {
    //
    // Fail to get the end-entity leaf signer certificate.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Status = CheckEKUs (SignerCert, RequiredEKUs, RequiredEKUsSize, RequireAllPresent);
  if (Status != EFI_SUCCESS) {
    goto Exit;
  }

Exit:

  //
  // Release Resources
  //
  // If the signature was not wrapped, then the call to WrapData() will allocate
  // the data and add a header to it
  //
  if (!IsWrapped && SignedData) {
    free (SignedData);
  }

  if (SignerCert) {
    X509_free (SignerCert);
  }

  if (Pkcs7) {
    PKCS7_free (Pkcs7);
  }

  return Status;
}

