/** @file
  Non-runtime specific implementation of PKCS#7 SignedData Verification Wrapper.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include "CryptPkcs7Internal.h"
#include <mbedtls/pkcs7.h>

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
  IN CONST UINT8  *P7Data,
  IN UINTN        P7Length,
  OUT VOID        **Content,
  OUT UINTN       *ContentSize
  )
{
  BOOLEAN                 Status;
  UINT8                   *SignedData;
  UINTN                   SignedDataSize;
  BOOLEAN                 Wrapped;
  INTN                    Ret;
  mbedtls_pkcs7           Pkcs7;
  MbedtlsPkcs7SignedData  *Pkcs7Attached;
  MbedtlsPkcs7Data        *MbedtlsContent;

  ZeroMem (&Pkcs7, sizeof (MbedtlsPkcs7SignedData));

  //
  // Check input parameter.
  //
  if ((P7Data == NULL) || (P7Length > INT_MAX) || (Content == NULL) || (ContentSize == NULL)) {
    return FALSE;
  }

  *Content   = NULL;
  SignedData = NULL;

  Status = WrapPkcs7Data (P7Data, P7Length, &Wrapped, &SignedData, &SignedDataSize);
  if (!Status || (SignedDataSize > INT_MAX)) {
    goto _Exit;
  }

  Status = FALSE;

  Ret = mbedtls_pkcs7_parse_der (&Pkcs7, SignedData, (INT32)SignedDataSize);

  //
  // The type of Pkcs7 must be signedData
  //
  if (Ret != MBEDTLS_PKCS7_SIGNED_DATA) {
    goto _Exit;
  }

  //
  // Check for detached or attached content
  //
  Pkcs7Attached  = (MbedtlsPkcs7SignedData *)&Pkcs7;
  MbedtlsContent = &(Pkcs7Attached->ContentInfo);

  if (MbedtlsContent == NULL) {
    //
    // No Content supplied for PKCS7 detached signedData
    //
    *Content     = NULL;
    *ContentSize = 0;
  } else {
    //
    // Retrieve the attached content in PKCS7 signedData
    //
    if ((MbedtlsContent->Data.len > 0) && (MbedtlsContent->Data.p != NULL)) {
      *ContentSize = MbedtlsContent->Data.len;
      *Content     = AllocateZeroPool (*ContentSize);
      if (*Content == NULL) {
        *ContentSize = 0;
        goto _Exit;
      }

      CopyMem (*Content, MbedtlsContent->Data.p, *ContentSize);
    }
  }

  Status = TRUE;

_Exit:
  //
  // Release Resources
  //
  mbedtls_pkcs7_free (&Pkcs7);

  return Status;
}
