/** @file
  RFC3161 Timestamp Countersignature Verification Wrapper Implementation which does
  not provide real capabilities.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/asn1.h>

//
// OID ASN.1 Value for SPC_RFC3161_OBJID ("1.3.6.1.4.1.311.3.3.1")
//
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  mSpcRFC3161OidValue[] = {
  0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x03, 0x03, 0x01
};

/**
  Convert ASN.1 GeneralizedTime to EFI Time.

  @param[in]  Ptr          Pointer to the ASN.1 GeneralizedTime to be converted.
  @param[out] EfiTime      Return the corresponding EFI Time.

  @retval  TRUE   The time conversion succeeds.
  @retval  FALSE  Invalid parameters.

**/
STATIC
BOOLEAN
ConvertAsn1TimeToEfiTime (
  IN  UINT8     *Ptr,
  OUT EFI_TIME  *EfiTime
  )
{
  CONST CHAR8  *Str;
  UINTN        Index;

  if ((Ptr == NULL) || (EfiTime == NULL)) {
    return FALSE;
  }

  Str = (CONST CHAR8 *)Ptr;
  SetMem (EfiTime, sizeof (EFI_TIME), 0);

  Index = 0;

  /* four digit year */
  EfiTime->Year  = (Str[Index++] - '0') * 1000;
  EfiTime->Year += (Str[Index++] - '0') * 100;
  EfiTime->Year += (Str[Index++] - '0') * 10;
  EfiTime->Year += (Str[Index++] - '0');
  if ((EfiTime->Year < 1900) || (EfiTime->Year > 9999)) {
    return FALSE;
  }

  EfiTime->Month  = (Str[Index++] - '0') * 10;
  EfiTime->Month += (Str[Index++] - '0');
  if ((EfiTime->Month < 1) || (EfiTime->Month > 12)) {
    return FALSE;
  }

  EfiTime->Day  = (Str[Index++] - '0') * 10;
  EfiTime->Day += (Str[Index++] - '0');
  if ((EfiTime->Day < 1) || (EfiTime->Day > 31)) {
    return FALSE;
  }

  EfiTime->Hour  = (Str[Index++] - '0') * 10;
  EfiTime->Hour += (Str[Index++] - '0');
  if (EfiTime->Hour > 23) {
    return FALSE;
  }

  EfiTime->Minute  = (Str[Index++] - '0') * 10;
  EfiTime->Minute += (Str[Index++] - '0');
  if (EfiTime->Minute > 59) {
    return FALSE;
  }

  EfiTime->Second  = (Str[Index++] - '0') * 10;
  EfiTime->Second += (Str[Index++] - '0');
  if (EfiTime->Second > 59) {
    return FALSE;
  }

  /* Note: we did not adjust the time based on time zone information */

  return TRUE;
}

/**
  Verifies the validity of a RFC3161 Timestamp CounterSignature embedded in PE/COFF Authenticode
  signature.

  Return FALSE to indicate this interface is not supported.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TsaCert      Pointer to a trusted/root TSA certificate encoded in DER, which
                           is used for TSA certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[out] SigningTime  Return the time of timestamp generation time if the timestamp
                           signature is valid.

  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
ImageTimestampVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TsaCert,
  IN  UINTN        CertSize,
  OUT EFI_TIME     *SigningTime
  )
{
  UINT8  *Ptr;
  UINT8  *End;
  INT32  Len;
  UINTN  ObjLen;
  UINT8  *TempPtr;

  //
  // Initializations
  //
  if (SigningTime != NULL) {
    SetMem (SigningTime, sizeof (EFI_TIME), 0);
  }

  //
  // Input Parameters Checking.
  //
  if ((AuthData == NULL) || (TsaCert == NULL)) {
    return FALSE;
  }

  if ((DataSize > INT_MAX) || (CertSize > INT_MAX)) {
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

  Ptr += ObjLen;

  // cert
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  TempPtr = Ptr;
  // OPTIONAL CRLs
  if (mbedtls_asn1_get_tag (&TempPtr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) == 0) {
    Ptr = TempPtr + ObjLen;
  }

  // signerInfo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) != 0) {
    return FALSE;
  }

  // sub parse
  // signerInfo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  End = Ptr + ObjLen;

  // version
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_INTEGER) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // sid
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // digestalgo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // OPTIONAL AuthenticatedAttributes
  TempPtr = Ptr;
  if (mbedtls_asn1_get_tag (&TempPtr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) == 0) {
    Ptr = TempPtr + ObjLen;
  }

  // signaturealgo
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // signature
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OCTET_STRING) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // OPTIONAL UnauthenticatedAttributes
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, 0xA1) != 0) {
    return FALSE;
  }

  // Attribute
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // type
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  if (CompareMem (Ptr, mSpcRFC3161OidValue, sizeof (mSpcRFC3161OidValue)) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // values
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) != 0) {
    return FALSE;
  }

  // values
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // signedData OID
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // [0]
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) != 0) {
    return FALSE;
  }

  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // integer
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_INTEGER) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // SET
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // tST OID
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) != 0) {
    return FALSE;
  }

  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OCTET_STRING) != 0) {
    return FALSE;
  }

  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  // Integer
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_INTEGER) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // policy OID
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // sequence
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;
  // Integer
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_INTEGER) != 0) {
    return FALSE;
  }

  Ptr += ObjLen;

  // GeneralizedTime
  if (mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_GENERALIZED_TIME) != 0) {
    return FALSE;
  }

  //
  // Retrieve the signing time from TS_TST_INFO structure.
  //
  if (SigningTime != NULL) {
    SetMem (SigningTime, sizeof (EFI_TIME), 0);
    return ConvertAsn1TimeToEfiTime (Ptr, SigningTime);
  }

  return TRUE;
}
