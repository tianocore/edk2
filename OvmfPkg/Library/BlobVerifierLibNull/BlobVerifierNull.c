/** @file

  Null implementation of the blob verifier library.

  Copyright (C) 2021, IBM Corporation

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BlobVerifierLib.h>

/**
  Verify blob from an external source.

  @param[in] BlobName           The name of the blob
  @param[in] Buf                The data of the blob
  @param[in] BufSize            The size of the blob in bytes

  @retval EFI_SUCCESS           The blob was verified successfully.
  @retval EFI_ACCESS_DENIED     The blob could not be verified, and therefore
                                should be considered non-secure.
**/
EFI_STATUS
EFIAPI
VerifyBlob (
  IN  CONST CHAR16    *BlobName,
  IN  CONST VOID      *Buf,
  IN  UINT32          BufSize
  )
{
  return EFI_SUCCESS;
}
