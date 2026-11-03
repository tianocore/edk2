/** @file
  PKCS#7 Decrypt Wrapper Implementation which does not provide real
  capabilities.

  Copyright (c) 2026, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Decrypts a DER-encoded PKCS#7 ContentInfo containing an envelopedData structure
  (such as one produced by Pkcs7Encrypt) and recovers the original content.

  Return FALSE to indicate this interface is not supported.

  @param[in]  PemData           Pointer to the PEM-encoded private key of a recipient
                                of the message.
  @param[in]  PemSize           Size of the PEM key data in bytes.
  @param[in]  Password          [Optional] NULL-terminated passphrase used to decrypt an
                                encrypted PEM key.
  @param[in]  RecipientCert     [Optional] Pointer to the DER-encoded X.509 certificate
                                of the recipient whose private key is supplied in PemData.
  @param[in]  RecipientCertSize Size of the DER-encoded certificate in bytes.
  @param[in]  ContentInfo       Pointer to the PKCS#7 DER-encoded ContentInfo that wraps
                                an envelopedData to be decrypted.
  @param[in]  ContentInfoSize   Size of the ContentInfo in bytes.
  @param[in]  Flags             Flags for the decryption operation.
  @param[out] OutData           Receives a pointer to the decrypted content.
  @param[out] OutDataSize       Receives the size of the decrypted content in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Decrypt (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password           OPTIONAL,
  IN   CONST UINT8  *RecipientCert      OPTIONAL,
  IN   UINTN        RecipientCertSize   OPTIONAL,
  IN   CONST UINT8  *ContentInfo,
  IN   UINTN        ContentInfoSize,
  IN   UINT32       Flags,
  OUT  UINT8        **OutData,
  OUT  UINTN        *OutDataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
