/** @file
  PKCS7 Verify Null implementation.

  Copyright (C) Microsoft Corporation. All Rights Reserved.
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

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
  IN CONST VOID  *CertChain,
  OUT VOID       **SignerCert
  )
{
  ASSERT (FALSE);
  return EFI_NOT_READY;
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
  IN CONST VOID  *Cert,
  IN VOID        *Asn1ToFind
  )
{
  ASSERT (FALSE);
  return EFI_NOT_READY;
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
CheckEKUs (
  IN CONST VOID    *SignerCert,
  IN CONST CHAR8   *RequiredEKUs[],
  IN CONST UINT32  RequiredEKUsSize,
  IN BOOLEAN       RequireAllPresent
  )
{
  ASSERT (FALSE);
  return EFI_NOT_READY;
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
  IN CONST UINT8   *Pkcs7Signature,
  IN CONST UINT32  SignatureSize,
  IN CONST CHAR8   *RequiredEKUs[],
  IN CONST UINT32  RequiredEKUsSize,
  IN BOOLEAN       RequireAllPresent
  )
{
  ASSERT (FALSE);
  return EFI_NOT_READY;
}
