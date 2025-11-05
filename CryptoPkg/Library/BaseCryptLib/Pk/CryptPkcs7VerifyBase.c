/** @file
  Non-runtime specific implementation of PKCS#7 SignedData Verification Wrapper.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs7.h>

/**
  Check the contents of PKCS7 is not data.

  It is copied from PKCS7_type_is_other() in pk7_doit.c.

  @param[in] P7 Pointer to the location at which the PKCS7 is located.

  @retval TRUE  If the type is others.
  @retval FALSE If the type is expected.
**/
STATIC
BOOLEAN
Pkcs7TypeIsOther (
  IN PKCS7  *P7
  )
{
  BOOLEAN  Others;
  INTN     Nid = OBJ_obj2nid (P7->type);

  switch (Nid) {
    case NID_pkcs7_data:
    case NID_pkcs7_signed:
    case NID_pkcs7_enveloped:
    case NID_pkcs7_signedAndEnveloped:
    case NID_pkcs7_encrypted:
      Others = FALSE;
      break;
    default:
      Others = TRUE;
  }

  return Others;
}

/**
  Get the ASN.1 string for the PKCS7.

  It is copied from PKCS7_get_octet_string() in pk7_doit.c.

  @param[in] P7 Pointer to the location at which the PKCS7 is located.

  @return ASN1_OCTET_STRING ASN.1 string.
**/
STATIC
ASN1_OCTET_STRING *
Pkcs7GetOctetString (
  IN PKCS7  *P7
  )
{
  if (PKCS7_type_is_data (P7)) {
    return P7->d.data;
  }

  if (Pkcs7TypeIsOther (P7) && (P7->d.other != NULL) &&
      (P7->d.other->type == V_ASN1_OCTET_STRING))
  {
    return P7->d.other->value.octet_string;
  }

  return NULL;
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
                            It's caller's responsibility to free the buffer with FreePool().
  @param[out]  ContentSize  The size of the extracted content in bytes.

  @retval     TRUE          The P7Data was correctly formatted for processing.
  @retval     FALSE         The P7Data was not correctly formatted for processing.

**/
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
    OctStr = Pkcs7GetOctetString (Pkcs7->d.sign->contents);
    if (OctStr == NULL) {
      goto _Exit;
    }

    if ((OctStr->length > 0) && (OctStr->data != NULL)) {
      *ContentSize = OctStr->length;
      *Content     = AllocatePool (*ContentSize);
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
