/** @file

  Blob verification library

  This library class allows verifiying whether blobs from external sources
  (such as QEMU's firmware config) are trusted.

  Copyright (C) 2021, IBM Corporation

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BLOB_VERIFIER_LIB_H_
#define BLOB_VERIFIER_LIB_H_

#include <Uefi/UefiBaseType.h>
#include <Base.h>

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
  );

#endif
