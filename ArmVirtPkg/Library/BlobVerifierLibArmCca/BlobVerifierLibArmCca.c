/** @file

  Blob verifier library that uses Arm CCA measurements.

  Copyright (C) 2023, Linaro Ltd

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BlobVerifierLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Add Blob to the Extended Realm Measurement

  @param[in] BlobName           The name of the blob
  @param[in] Buf                The data of the blob
  @param[in] BufSize            The size of the blob in bytes
  @param[in] FetchStatus        The status of fetching this blob

  @retval EFI_SUCCESS           The blob was verified successfully or was not
                                found in the hash table.
  @retval EFI_ACCESS_DENIED     Kernel hashes not supported but the boot can
                                continue safely.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
EFIAPI
VerifyBlob (
  IN  CONST CHAR16  *BlobName,
  IN  CONST VOID    *Buf,
  IN  UINT32        BufSize,
  IN  EFI_STATUS    FetchStatus
  )
{
  EFI_STATUS     Status;
  UINT8          Hash[SHA512_DIGEST_SIZE];
  RETURN_STATUS  RetStatus;
  REALM_CONFIG   *Config;
  UINTN          HashLen;

  /* If fetch failed, then the REM will be wrong */
  if (EFI_ERROR (FetchStatus)) {
    return FetchStatus;
  }

  if ((BufSize == 0) || !IsRealm ()) {
    return EFI_SUCCESS;
  }

  Config = (REALM_CONFIG *)AllocateAlignedPages (
                             EFI_SIZE_TO_PAGES (sizeof (REALM_CONFIG)),
                             REALM_GRANULE_SIZE
                             );
  if (Config == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (REALM_CONFIG));

  RetStatus = RsiGetRealmConfig (Config);
  if (RETURN_ERROR (RetStatus)) {
    ASSERT (0);
    Status = (EFI_STATUS)RetStatus;
    goto ErrorHandler;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: adding measurement of '%s' (%d)\n",
    __func__,
    BlobName,
    BufSize
    ));

  if (Config->HashAlgorithm == RSI_HASH_SHA_256) {
    Sha256HashAll (Buf, BufSize, Hash);
    HashLen = SHA256_DIGEST_SIZE;
  } else if (Config->HashAlgorithm == RSI_HASH_SHA_512) {
    Sha512HashAll (Buf, BufSize, Hash);
    HashLen = SHA512_DIGEST_SIZE;
  } else {
    Status = EFI_INVALID_PARAMETER;
    goto ErrorHandler;
  }

  Status = RsiExtendMeasurement (1, Hash, HashLen);
  ASSERT_EFI_ERROR (Status);

ErrorHandler:
  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (REALM_CONFIG)));
  return Status;
}
