/** @file
  PKCS#7 Encrypt Wrapper Implementation which does not provide real
  capabilities.

  Copyright (c) 2023, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Creates a DER-encoded PKCS#7 ContentInfo containing an envelopedData structure
  that wraps content encrypted for secure transmission to one or more recipients.

  Return FALSE to indicate this interface is not supported.

  @param[in]  X509Stack        Pointer to a stack of X.509 certificates for the
                               intended recipients of this message.
  @param[in]  InData           Pointer to the content to be encrypted.
  @param[in]  InDataSize       Size of the content to be encrypted in bytes.
  @param[in]  CipherNid        NID of the symmetric cipher to use for encryption.
  @param[in]  Flags            Flags for the encryption operation.
  @param[out] ContentInfo      Receives a pointer to the output.
  @param[out] ContentInfoSize  Receives the size of the output in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Encrypt (
  IN   UINT8   *X509Stack,
  IN   UINT8   *InData,
  IN   UINTN   InDataSize,
  IN   UINT32  CipherNid,
  IN   UINT32  Flags,
  OUT  UINT8   **ContentInfo,
  OUT  UINTN   *ContentInfoSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
