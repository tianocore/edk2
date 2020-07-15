/** @file
  This module verifies that Enhanced Key Usages (EKU's) are present within
  a PKCS7 signature blob using OpenSSL.

  Copyright (C) Microsoft Corporation. All Rights Reserved.
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  This function receives a PKCS#7 formatted signature blob,
  looks for the EKU SEQUENCE blob, and if found then looks
  for all the required EKUs.  This function was created so that
  the Surface team can cut down on the number of Certificate
  Authorities (CA's) by checking EKU's on leaf signers for
  a specific product.  This prevents one product's certificate
  from signing another product's firmware or unlock blobs.

  Return RETURN_UNSUPPORTED to indicate this interface is not supported.

  @param[in]  Pkcs7Signature        The PKCS#7 signed information content block. An array
                                    containing the content block with both the signature,
                                    the signer's certificate, and any necessary intermediate
                                    certificates.
  @param[in]  Pkcs7SignatureSize    Number of bytes in pPkcs7Signature.
  @param[in]  RequiredEKUs          Array of null-terminated strings listing OIDs of
                                    required EKUs that must be present in the signature.
                                    All specified EKU's must be present in order to
                                    succeed.
  @param[in]  RequiredEKUsSize      Number of elements in the rgRequiredEKUs string.
                                    This parameter has a maximum of MAX_EKU_SEARCH.
  @param[in]  RequireAllPresent     If this is TRUE, then all of the specified EKU's
                                    must be present in the leaf signer.  If it is
                                    FALSE, then we will succeed if we find any
                                    of the specified EKU's.

  @retval RETURN_UNSUPPORTED        The operation is not supported.

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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
}

