/** @file
  Authenticode Portable Executable Signature Verification which does not provide
  real capabilities.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/pkcs7.h>

//
// OID ASN.1 Value for SPC_INDIRECT_DATA_OBJID
//
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  mSpcIndirectOidValue[] = {
  0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x01, 0x04
};

/**
  Verifies the validity of a PE/COFF Authenticode Signature as described in "Windows
  Authenticode Portable Executable Signature Format".

  Return FALSE to indicate this interface is not supported.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[in]  ImageHash    Pointer to the original image file hash value. The procedure
                           for calculating the image hash value is described in Authenticode
                           specification.
  @param[in]  HashSize     Size of Image hash value in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AuthenticodeVerify (
  IN CONST UINT8  *AuthData,
  IN UINTN        DataSize,
  IN CONST UINT8  *TrustedCert,
  IN UINTN        CertSize,
  IN CONST UINT8  *ImageHash,
  IN UINTN        HashSize
  )
{
  BOOLEAN      Status;
  CONST UINT8  *OrigAuthData;
  UINT8        *SpcIndirectDataContent;
  UINT8        Asn1Byte;
  UINTN        ContentSize;
  CONST UINT8  *SpcIndirectDataOid;
  UINT8        *Ptr;
  UINT8        *End;
  INT32        Len;
  UINTN        ObjLen;

  OrigAuthData = AuthData;

  //
  // Check input parameters.
  //
  if ((AuthData == NULL) || (TrustedCert == NULL) || (ImageHash == NULL)) {
    return FALSE;
  }

  if ((DataSize > INT_MAX) || (CertSize > INT_MAX) || (HashSize > INT_MAX)) {
    return FALSE;
  }

  if (DataSize <= HashSize) {
    return FALSE;
  }

  Ptr = (UINT8 *)(UINTN)AuthData;
  Len = (UINT32)DataSize;
  End = Ptr + Len;

  // ContentInfo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // ContentType
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // content
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) != 0) {
    return FALSE;
  }

  End = Ptr + ObjLen;
  // signedData
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // version
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_INTEGER) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // digestAlgo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // encapContentInfo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  End = Ptr + ObjLen;
  // eContentType
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  Status = FALSE;

  SpcIndirectDataOid = Ptr;
  if ((ObjLen != sizeof (mSpcIndirectOidValue)) ||
      (CompareMem (
         SpcIndirectDataOid,
         mSpcIndirectOidValue,
         sizeof (mSpcIndirectOidValue)
         ) != 0))
  {
    //
    // Un-matched SPC_INDIRECT_DATA_OBJID.
    //
    goto _Exit;
  }

  Ptr += ObjLen;
  // eContent
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) != 0) {
    return FALSE;
  }

  SpcIndirectDataContent = Ptr;

  //
  // Retrieve the SEQUENCE data size from ASN.1-encoded SpcIndirectDataContent.
  //
  Asn1Byte = *(SpcIndirectDataContent + 1);

  if ((Asn1Byte & 0x80) == 0) {
    //
    // Short Form of Length Encoding (Length < 128)
    //
    ContentSize = (UINTN)(Asn1Byte & 0x7F);
    //
    // Skip the SEQUENCE Tag;
    //
    SpcIndirectDataContent += 2;
  } else if ((Asn1Byte & 0x81) == 0x81) {
    //
    // Long Form of Length Encoding (128 <= Length < 255, Single Octet)
    //
    ContentSize = (UINTN)(*(UINT8 *)(SpcIndirectDataContent + 2));
    //
    // Skip the SEQUENCE Tag;
    //
    SpcIndirectDataContent += 3;
  } else if ((Asn1Byte & 0x82) == 0x82) {
    //
    // Long Form of Length Encoding (Length > 255, Two Octet)
    //
    ContentSize = (UINTN)(*(UINT8 *)(SpcIndirectDataContent + 2));
    ContentSize = (ContentSize << 8) + (UINTN)(*(UINT8 *)(SpcIndirectDataContent + 3));
    //
    // Skip the SEQUENCE Tag;
    //
    SpcIndirectDataContent += 4;
  } else {
    goto _Exit;
  }

  //
  // Compare the original file hash value to the digest retrieve from SpcIndirectDataContent
  // defined in Authenticode
  // NOTE: Need to double-check HashLength here!
  //
  if (ContentSize < HashSize) {
    return FALSE;
  }

  if (CompareMem (SpcIndirectDataContent + ContentSize - HashSize, ImageHash, HashSize) != 0) {
    //
    // Un-matched PE/COFF Hash Value
    //
    goto _Exit;
  }

  //
  // Verifies the PKCS#7 Signed Data in PE/COFF Authenticode Signature
  //
  Status = (BOOLEAN)Pkcs7Verify (OrigAuthData, DataSize, TrustedCert, CertSize, SpcIndirectDataContent, ContentSize);

_Exit:

  return Status;
}
