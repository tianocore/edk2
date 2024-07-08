/** @file
  This module verifies that Enhanced Key Usages (EKU's) are present within
  a PKCS7 signature blob using MbedTLS.

  Copyright (C) Microsoft Corporation. All Rights Reserved.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include "InternalCryptLib.h"
#include <mbedtls/pkcs7.h>
#include <mbedtls/asn1write.h>

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  EkuOID[] = { 0x55, 0x1D, 0x25 };

/*leaf Cert basic_constraints case1: CA: false and CA object is excluded */
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gBasicConstraintsCase1[] = { 0x30, 0x00 };

/*leaf Cert basic_constraints case2: CA: false */
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gBasicConstraintsCase2[] = { 0x30, 0x06, 0x01, 0x01, 0xFF, 0x02, 0x01, 0x00 };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gOidBasicConstraints[] = { 0x55, 0x1D, 0x13 };

/**
  Find first Extension data match with given OID

  @param[in]      Start             Pointer to the DER-encoded extensions data
  @param[in]      End               extensions data size in bytes
  @param[in ]     Oid               OID for match
  @param[in ]     OidSize           OID size in bytes
  @param[out]     FindExtensionData output matched extension data.
  @param[out]     FindExtensionDataLen matched extension data size.

**/
STATIC
BOOLEAN
InternalX509FindExtensionData (
  UINT8        *Start,
  UINT8        *End,
  CONST UINT8  *Oid,
  UINTN        OidSize,
  UINT8        **FindExtensionData,
  UINTN        *FindExtensionDataLen
  )
{
  UINT8    *Ptr;
  UINT8    *ExtensionPtr;
  UINTN    ObjLen;
  INT32    Ret;
  BOOLEAN  Status;
  UINTN    FindExtensionLen;
  UINTN    HeaderLen;

  /*If no Extension entry match Oid*/
  Status = FALSE;
  Ptr    = Start;

  Ret = 0;

  while (TRUE) {
    //
    // Extension  ::=  SEQUENCE  {
    //     extnID      OBJECT IDENTIFIER,
    //     critical    BOOLEAN DEFAULT FALSE,
    //     extnValue   OCTET STRING  }
    //
    ExtensionPtr = Ptr;
    Ret          = mbedtls_asn1_get_tag (
                     &Ptr,
                     End,
                     &ObjLen,
                     MBEDTLS_ASN1_CONSTRUCTED |
                     MBEDTLS_ASN1_SEQUENCE
                     );
    if (Ret == 0) {
      HeaderLen        = (UINTN)(Ptr - ExtensionPtr);
      FindExtensionLen = ObjLen;
      /* Get Object Identifier*/
      Ret = mbedtls_asn1_get_tag (
              &Ptr,
              End,
              &ObjLen,
              MBEDTLS_ASN1_OID
              );
    } else {
      break;
    }

    if ((Ret == 0) && !CompareMem (Ptr, Oid, OidSize)) {
      Ptr += ObjLen;

      Ret = mbedtls_asn1_get_tag (
              &Ptr,
              End,
              &ObjLen,
              MBEDTLS_ASN1_BOOLEAN
              );
      if (Ret == 0) {
        Ptr += ObjLen;
      }

      Ret = mbedtls_asn1_get_tag (
              &Ptr,
              End,
              &ObjLen,
              MBEDTLS_ASN1_OCTET_STRING
              );
    } else {
      Ret = 1;
    }

    if (Ret == 0) {
      *FindExtensionData    = Ptr;
      *FindExtensionDataLen = ObjLen;
      Status                = TRUE;
      break;
    }

    /* move to next*/
    Ptr = ExtensionPtr + HeaderLen + FindExtensionLen;
    Ret = 0;
  }

  return Status;
}

/**
  Retrieve Extension data from one X.509 certificate.

  @param[in]      Cert             Pointer to the  X509 certificate.
  @param[in]      Oid              Object identifier buffer
  @param[in]      OidSize          Object identifier buffer size
  @param[out]     ExtensionData    Extension bytes.
  @param[in, out] ExtensionDataSize Extension bytes size.

  @retval RETURN_SUCCESS           The certificate Extension data retrieved successfully.
  @retval RETURN_INVALID_PARAMETER If Cert is NULL.
                                  If ExtensionDataSize is NULL.
                                  If ExtensionData is not NULL and *ExtensionDataSize is 0.
                                  If Certificate is invalid.
  @retval RETURN_NOT_FOUND         If no Extension entry match Oid.
  @retval RETURN_BUFFER_TOO_SMALL  If the ExtensionData is NULL. The required buffer size
                                  is returned in the ExtensionDataSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.
**/
STATIC
BOOLEAN
GetExtensionData (
  CONST mbedtls_x509_crt  *Cert,
  CONST UINT8             *Oid,
  UINTN                   OidSize,
  UINT8                   *ExtensionData,
  UINTN                   *ExtensionDataSize
  )
{
  CONST mbedtls_x509_crt  *Crt;
  INT32                   Ret;
  BOOLEAN                 Status;
  UINT8                   *Ptr;
  UINT8                   *End;
  UINTN                   ObjLen;

  Ptr    = NULL;
  End    = NULL;
  ObjLen = 0;

  if ((Cert == NULL) || (Oid == NULL) || (OidSize == 0) ||
      (ExtensionDataSize == NULL))
  {
    return FALSE;
  }

  Status = FALSE;

  Crt = Cert;

  Ptr = Crt->v3_ext.p;
  End = Crt->v3_ext.p + Crt->v3_ext.len;
  Ret = mbedtls_asn1_get_tag (
          &Ptr,
          End,
          &ObjLen,
          MBEDTLS_ASN1_CONSTRUCTED |
          MBEDTLS_ASN1_SEQUENCE
          );

  if (Ret == 0) {
    Status = InternalX509FindExtensionData (
               Ptr,
               End,
               Oid,
               OidSize,
               &Ptr,
               &ObjLen
               );
  }

  if (Status) {
    if (*ExtensionDataSize < ObjLen) {
      *ExtensionDataSize = ObjLen;
      Status             = FALSE;
      goto Cleanup;
    }

    if (Oid != NULL) {
      if (ExtensionData == NULL) {
        return FALSE;
      }

      CopyMem (ExtensionData, Ptr, ObjLen);
    }

    *ExtensionDataSize = ObjLen;
  } else {
    *ExtensionDataSize = 0;
  }

Cleanup:
  return Status;
}

/**
  Determines if the specified EKU represented in ASN1 form is present
  in a given certificate.

  @param[in]  Cert                  The certificate to check.
  @param[in]  EKU                   The EKU to look for.
  @param[in]  EkuLen                The size of EKU.

  @retval EFI_SUCCESS               We successfully identified the signing type.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid.
  @retval EFI_NOT_FOUND             One or more EKU's were not found in the signature.

**/
STATIC
EFI_STATUS
IsEkuInCertificate (
  IN CONST mbedtls_x509_crt  *Cert,
  IN UINT8                   *EKU,
  IN UINTN                   EkuLen
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Ret;
  UINT8       *Buffer;
  UINTN       Index;
  UINTN       Len;

  if ((Cert == NULL) || (EKU == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  Len    = 0;
  Buffer = NULL;
  Ret    = GetExtensionData (
             Cert,
             (CONST UINT8 *)EkuOID,
             sizeof (EkuOID),
             NULL,
             &Len
             );
  if (Len == 0) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  Buffer = AllocateZeroPool (Len);
  if (Buffer == NULL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  Ret = GetExtensionData (
          Cert,
          (CONST UINT8 *)EkuOID,
          sizeof (EkuOID),
          Buffer,
          &Len
          );

  if ((Len == 0) || (!Ret)) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  Status = EFI_NOT_FOUND;
  for (Index = 0; Index <= Len - EkuLen; Index++) {
    if (!CompareMem (Buffer + Index, EKU, EkuLen)) {
      // check sub EKU
      if (Index == Len - EkuLen) {
        Status = EFI_SUCCESS;
        break;
        // Ensure that the OID is complete
      } else if (Buffer[Index + EkuLen] == 0x06) {
        Status = EFI_SUCCESS;
        break;
      } else {
        break;
      }
    }
  }

Exit:
  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return Status;
}

/**
  Get OID from txt.

  @param[in]  RequiredEKUs         Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.
  @param[in]  RequiredEKUsSize     Number of elements in the RequiredEKUs string array.
  @param[in,out]  CheckOid         OID.
  @param[out]     OidLen           The size of OID.

**/
VOID
GetOidFromTxt (
  IN CONST CHAR8  *RequiredEKUs,
  IN UINTN        RequiredEKUsSize,
  IN OUT UINT8    *CheckOid,
  OUT UINT8       *OidLen
  )
{
  UINT8   *Ptr;
  UINT16  Index;
  UINT32  Data;
  UINT8   OidIndex;
  UINTN   EKUsSize;

  EKUsSize = RequiredEKUsSize;
  // https://learn.microsoft.com/en-us/windows/win32/seccertenroll/about-object-identifier?redirectedfrom=MSDN
  CheckOid[0] = (UINT8)((RequiredEKUs[0] - '0') * 40 + (RequiredEKUs[2] - '0'));

  EKUsSize = EKUsSize - 4;
  Ptr      = (UINT8 *)(RequiredEKUs + 4);

  OidIndex = 1;

  while (EKUsSize) {
    Index = 0;
    Data  = 0;

    while ((*Ptr != '.') && (*Ptr != '\0')) {
      Index++;
      Ptr++;
      EKUsSize--;
    }

    while (Index) {
      Data = 10 * Data + (*(Ptr - Index) - '0');
      Index--;
    }

    if (EKUsSize != 0) {
      Ptr++;
      EKUsSize--;
    }

    if (Data < 128) {
      CheckOid[OidIndex] = (UINT8)Data;
      OidIndex++;
    } else {
      CheckOid[OidIndex + 1] = (UINT8)(Data & 0xFF);
      CheckOid[OidIndex]     = (UINT8)(((((Data & 0xFF00) << 1) | 0x8000) >> 8) & 0xFF);
      OidIndex               = OidIndex + 2;
    }
  }

  *OidLen = OidIndex;
}

/**
  Verify the Cert is signer cert

  @param[in]  Start        Pointer to the DER-encoded certificate data Start.
  @param[in]  End          Pointer to the DER-encoded certificate data End.

  @retval  true            verify pass
  @retval  false           verify fail
**/
STATIC
BOOLEAN
IsCertSignerCert (
  UINT8  *Start,
  UINT8  *End
  )
{
  BOOLEAN           Status;
  UINT8             *Buffer;
  UINTN             Len;
  mbedtls_x509_crt  Cert;
  UINTN             ObjLen;

  mbedtls_x509_crt_init (&Cert);

  ObjLen = End - Start;

  if (mbedtls_x509_crt_parse_der (&Cert, Start, ObjLen) != 0) {
    return FALSE;
  }

  Len    = 0;
  Buffer = NULL;
  Status = GetExtensionData (
             &Cert,
             (CONST UINT8 *)gOidBasicConstraints,
             sizeof (gOidBasicConstraints),
             NULL,
             &Len
             );
  if (Len == 0) {
    /* basic constraints is not present in Cert */
    return TRUE;
  }

  Buffer = AllocateZeroPool (Len);
  if (Buffer == NULL) {
    return FALSE;
  }

  Status = GetExtensionData (
             &Cert,
             (CONST UINT8 *)gOidBasicConstraints,
             sizeof (gOidBasicConstraints),
             Buffer,
             &Len
             );

  if (Len == 0) {
    /* basic constraints is not present in Cert */
    Status = TRUE;
    goto Exit;
  } else if (!Status) {
    Status = FALSE;
    goto Exit;
  }

  if ((Len == sizeof (gBasicConstraintsCase1)) &&
      (!CompareMem (Buffer, gBasicConstraintsCase1, sizeof (gBasicConstraintsCase1))))
  {
    Status = TRUE;
    goto Exit;
  }

  if ((Len == sizeof (gBasicConstraintsCase2)) &&
      (!CompareMem (Buffer, gBasicConstraintsCase2, sizeof (gBasicConstraintsCase2))))
  {
    Status = TRUE;
    goto Exit;
  }

  Status = FALSE;

Exit:
  mbedtls_x509_crt_free (&Cert);

  if (Buffer != NULL) {
    FreePool (Buffer);
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
STATIC
EFI_STATUS
CheckEKUs (
  IN CONST mbedtls_x509_crt  *SignerCert,
  IN CONST CHAR8             *RequiredEKUs[],
  IN CONST UINT32            RequiredEKUsSize,
  IN BOOLEAN                 RequireAllPresent
  )
{
  EFI_STATUS  Status;
  UINT32      NumEkusFound;
  UINT32      Index;
  UINT8       *EKU;
  UINTN       EkuLen;
  UINT8       CheckOid[20];
  UINT8       OidLen;

  Status       = EFI_SUCCESS;
  NumEkusFound = 0;

  if ((SignerCert == NULL) || (RequiredEKUs == NULL) || (RequiredEKUsSize == 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  for (Index = 0; Index < RequiredEKUsSize; Index++) {
    //
    // Finding required EKU in Cert.
    //
    GetOidFromTxt (RequiredEKUs[Index], strlen (RequiredEKUs[Index]), CheckOid, &OidLen);

    EKU    = CheckOid;
    EkuLen = OidLen;

    Status = IsEkuInCertificate (SignerCert, EKU, EkuLen);
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
      // Fail to find Eku in Cert
      break;
    }
  }

Exit:
  if (RequireAllPresent &&
      (NumEkusFound == RequiredEKUsSize))
  {
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
  IN CONST UINT8   *Pkcs7Signature,
  IN CONST UINT32  SignatureSize,
  IN CONST CHAR8   *RequiredEKUs[],
  IN CONST UINT32  RequiredEKUsSize,
  IN BOOLEAN       RequireAllPresent
  )
{
  EFI_STATUS        Status;
  mbedtls_x509_crt  Cert;
  UINT8             *Ptr;
  UINT8             *End;
  INT32             Len;
  UINTN             ObjLen;
  UINT8             *OldEnd;

  //
  // Check input parameter.
  //
  if ((RequiredEKUs == NULL) || (Pkcs7Signature == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  mbedtls_x509_crt_init (&Cert);

  Ptr = (UINT8 *)(UINTN)Pkcs7Signature;
  Len = (UINT32)SignatureSize;
  End = Ptr + Len;

  // Cert
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // tbscert
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_INTEGER) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // signature algo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // signature
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  Ptr   += ObjLen;
  OldEnd = Ptr;
  // Cert
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) != 0) {
    return FALSE;
  }

  End = Ptr + ObjLen;

  // leaf Cert
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  while ((Ptr != End) && (Ptr < End)) {
    if (IsCertSignerCert (OldEnd, Ptr)) {
      break;
    }

    OldEnd = Ptr;
    if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
      return FALSE;
    }

    Ptr += ObjLen;
  }

  if (Ptr != End) {
    return FALSE;
  } else {
    Ptr = End - ObjLen;
  }

  // leaf Cert
  ObjLen += Ptr - OldEnd;
  Ptr     = OldEnd;

  if (mbedtls_x509_crt_parse_der (&Cert, Ptr, ObjLen) != 0) {
    return FALSE;
  }

  Status = CheckEKUs (&Cert, RequiredEKUs, RequiredEKUsSize, RequireAllPresent);
  if (Status != EFI_SUCCESS) {
    goto Exit;
  }

Exit:
  //
  // Release Resources
  //
  mbedtls_x509_crt_free (&Cert);

  return Status;
}
