/** @file

  Blob verifier library that uses Arm CCA measurements.

  Copyright (C) 2023, Linaro Ltd

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/BlobVerifierLib.h>

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
  /*
   * TODO: use same measurement algorithm as the RMM. Obtain it from
   * RsiRealmConfig?
   */
  EFI_STATUS  Status;
  UINT8       Hash[SHA256_DIGEST_SIZE];

  /* If fetch failed, then the REM will be wrong */
  if (EFI_ERROR (FetchStatus)) {
    return FetchStatus;
  }

  if ((BufSize == 0) || !IsRealm ()) {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: adding measurement of '%s' (%d)\n",
    __func__,
    BlobName,
    BufSize
    ));

  Sha256HashAll (Buf, BufSize, Hash);

  Status = RsiExtendMeasurement (1, Hash, sizeof (Hash));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
