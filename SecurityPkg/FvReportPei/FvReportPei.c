/** @file
  This driver verifies and reports OBB FVs.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FvReportPei.h"

STATIC CONST HASH_ALG_INFO  mHashAlgInfo[] = {
  { TPM_ALG_SHA256, SHA256_DIGEST_SIZE, Sha256Init, Sha256Update, Sha256Final, Sha256HashAll }, // 000B
  { TPM_ALG_SHA384, SHA384_DIGEST_SIZE, Sha384Init, Sha384Update, Sha384Final, Sha384HashAll }, // 000C
  { TPM_ALG_SHA512, SHA512_DIGEST_SIZE, Sha512Init, Sha512Update, Sha512Final, Sha512HashAll }, // 000D
};

/**
  Find hash algorithm information from mHashAlgInfo according to given ID.

  @param[in]  HashAlgId          Hash algorithm type id.

  @retval Pointer to HASH_ALG_INFO if given hash algorithm is supported.
  @retval NULL if given algorithm is not supported.
**/
STATIC
CONST
HASH_ALG_INFO *
FindHashAlgInfo (
  IN UINT16  HashAlgId
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashAlgInfo); ++Index) {
    if (mHashAlgInfo[Index].HashAlgId == HashAlgId) {
      return &mHashAlgInfo[Index];
    }
  }

  return NULL;
}

/**
  Install a EDKII_PEI_FIRMWARE_VOLUME_INFO_PREHASHED_FV_PPI instance so that
  TCG driver may use to extend PCRs.

  @param[in]  FvBuffer            Buffer containing the whole FV.
  @param[in]  FvLength            Length of the FV.
  @param[in]  HashAlgoId          Hash algorithm type id.
  @param[in]  HashSize            Hash size.
  @param[in]  HashValue           Hash value buffer.
**/
STATIC
VOID
InstallPreHashFvPpi (
  IN VOID    *FvBuffer,
  IN UINTN   FvLength,
  IN UINT16  HashAlgoId,
  IN UINT16  HashSize,
  IN UINT8   *HashValue
  )
{
  EFI_STATUS                                       Status;
  EFI_PEI_PPI_DESCRIPTOR                           *FvInfoPpiDescriptor;
  EDKII_PEI_FIRMWARE_VOLUME_INFO_PREHASHED_FV_PPI  *PreHashedFvPpi;
  UINTN                                            PpiSize;
  HASH_INFO                                        *HashInfo;

  PpiSize = sizeof (EDKII_PEI_FIRMWARE_VOLUME_INFO_PREHASHED_FV_PPI)
            + sizeof (HASH_INFO)
            + HashSize;

  PreHashedFvPpi = AllocatePool (PpiSize);
  ASSERT (PreHashedFvPpi != NULL);

  PreHashedFvPpi->FvBase   = (UINT32)(UINTN)FvBuffer;
  PreHashedFvPpi->FvLength = (UINT32)FvLength;
  PreHashedFvPpi->Count    = 1;

  HashInfo             = HASH_INFO_PTR (PreHashedFvPpi);
  HashInfo->HashAlgoId = HashAlgoId;
  HashInfo->HashSize   = HashSize;
  CopyMem (HASH_VALUE_PTR (HashInfo), HashValue, HashSize);

  FvInfoPpiDescriptor = AllocatePool (sizeof (EFI_PEI_PPI_DESCRIPTOR));
  ASSERT (FvInfoPpiDescriptor != NULL);

  FvInfoPpiDescriptor->Guid  = &gEdkiiPeiFirmwareVolumeInfoPrehashedFvPpiGuid;
  FvInfoPpiDescriptor->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  FvInfoPpiDescriptor->Ppi   = (VOID *)PreHashedFvPpi;

  Status = PeiServicesInstallPpi (FvInfoPpiDescriptor);
  ASSERT_EFI_ERROR (Status);
}

/**
  Calculate and verify hash value for given FV.

  @param[in]  HashInfo            Hash information of the FV.
  @param[in]  FvInfo              Information of FV used for verification.
  @param[in]  FvNumber            Length of the FV.
  @param[in]  BootMode            Length of the FV.

  @retval EFI_SUCCESS           The given FV is integrate.
  @retval EFI_VOLUME_CORRUPTED  The given FV is corrupted (hash mismatch).
  @retval EFI_UNSUPPORTED       The hash algorithm is not supported.
**/
STATIC
EFI_STATUS
VerifyHashedFv (
  IN FV_HASH_INFO    *HashInfo,
  IN HASHED_FV_INFO  *FvInfo,
  IN UINTN           FvNumber,
  IN EFI_BOOT_MODE   BootMode
  )
{
  UINTN                                 FvIndex;
  CONST HASH_ALG_INFO                   *AlgInfo;
  UINT8                                 *HashValue;
  UINT8                                 *FvHashValue;
  VOID                                  *FvBuffer;
  EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI  *FvShadowPpi;
  EFI_STATUS                            Status;

  if ((HashInfo == NULL) ||
      (HashInfo->HashSize == 0) ||
      (HashInfo->HashAlgoId == TPM_ALG_NULL))
  {
    DEBUG ((DEBUG_INFO, "Bypass FV hash verification\r\n"));
    return EFI_SUCCESS;
  }

  AlgInfo = FindHashAlgInfo (HashInfo->HashAlgoId);
  if ((AlgInfo == NULL) || (AlgInfo->HashSize != HashInfo->HashSize)) {
    DEBUG ((
      DEBUG_ERROR,
      "Unsupported or wrong hash algorithm: %04X (size=%d)\r\n",
      HashInfo->HashAlgoId,
      HashInfo->HashSize
      ));
    return EFI_UNSUPPORTED;
  }

  ASSERT (FvInfo != NULL);
  ASSERT (FvNumber > 0);

  //
  // We need a hash value for each FV as well as one for all FVs.
  //
  HashValue = AllocateZeroPool (AlgInfo->HashSize * (FvNumber + 1));
  ASSERT (HashValue != NULL);

  Status = PeiServicesLocatePpi (
             &gEdkiiPeiFirmwareVolumeShadowPpiGuid,
             0,
             NULL,
             (VOID **)&FvShadowPpi
             );
  if (EFI_ERROR (Status)) {
    FvShadowPpi = NULL;
  }

  //
  // Calculate hash value for each FV first.
  //
  FvHashValue = HashValue;
  for (FvIndex = 0; FvIndex < FvNumber; ++FvIndex) {
    //
    // Not meant for verified boot and/or measured boot?
    //
    if (((FvInfo[FvIndex].Flag & HASHED_FV_FLAG_VERIFIED_BOOT) == 0) &&
        ((FvInfo[FvIndex].Flag & HASHED_FV_FLAG_MEASURED_BOOT) == 0))
    {
      continue;
    }

    //
    // Skip any FV not meant for current boot mode.
    //
    if ((FvInfo[FvIndex].Flag & HASHED_FV_FLAG_SKIP_BOOT_MODE (BootMode)) != 0) {
      DEBUG ((
        DEBUG_INFO,
        "Skip FV[%016lX] for boot mode[%d]\r\n",
        FvInfo[FvIndex].Base,
        BootMode
        ));
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "Pre-hashed[alg=%04X,size=%d,flag=%016lX] FV: 0x%016lX (%08lX) (Flag=%016lX)\r\n",
      HashInfo->HashAlgoId,
      HashInfo->HashSize,
      HashInfo->HashFlag,
      FvInfo[FvIndex].Base,
      FvInfo[FvIndex].Length,
      FvInfo[FvIndex].Flag
      ));

    //
    // Copy FV to permanent memory to avoid potential TOC/TOU.
    //
    FvBuffer = AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)FvInfo[FvIndex].Length));

    ASSERT (FvBuffer != NULL);

    if (FvShadowPpi != NULL) {
      Status = FvShadowPpi->FirmwareVolumeShadow (
                              (EFI_PHYSICAL_ADDRESS)FvInfo[FvIndex].Base,
                              FvBuffer,
                              (UINTN)FvInfo[FvIndex].Length
                              );
    }

    if ((FvShadowPpi == NULL) || (EFI_ERROR (Status))) {
      CopyMem (
        FvBuffer,
        (CONST VOID *)(UINTN)FvInfo[FvIndex].Base,
        (UINTN)FvInfo[FvIndex].Length
        );
    }

    if (!AlgInfo->HashAll (FvBuffer, (UINTN)FvInfo[FvIndex].Length, FvHashValue)) {
      Status = EFI_ABORTED;
      goto Done;
    }

    //
    // Report the FV measurement.
    //
    if ((FvInfo[FvIndex].Flag & HASHED_FV_FLAG_MEASURED_BOOT) != 0) {
      InstallPreHashFvPpi (
        FvBuffer,
        (UINTN)FvInfo[FvIndex].Length,
        HashInfo->HashAlgoId,
        HashInfo->HashSize,
        FvHashValue
        );
    }

    //
    // Don't keep the hash value of current FV if we don't need to verify it.
    //
    if ((FvInfo[FvIndex].Flag & HASHED_FV_FLAG_VERIFIED_BOOT) != 0) {
      FvHashValue += AlgInfo->HashSize;
    }

    //
    // Use memory copy of the FV from now on.
    //
    FvInfo[FvIndex].Base = (UINT64)(UINTN)FvBuffer;
  }

  //
  // Check final hash for all FVs.
  //
  if ((FvHashValue == HashValue) ||
      (AlgInfo->HashAll (HashValue, FvHashValue - HashValue, FvHashValue) &&
       (CompareMem (HashInfo->Hash, FvHashValue, AlgInfo->HashSize) == 0)))
  {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_VOLUME_CORRUPTED;
  }

Done:
  FreePool (HashValue);
  return Status;
}

/**
  Report FV to PEI and/or DXE core for dispatch.

  @param[in] FvInfo     Information of a FV.

**/
STATIC
VOID
ReportHashedFv (
  IN HASHED_FV_INFO  *FvInfo
  )
{
  CONST EFI_GUID  *FvFormat;

  if ((FvInfo->Flag & HASHED_FV_FLAG_REPORT_FV_HOB) != 0) {
    //
    // Require DXE core to process this FV.
    //
    BuildFvHob (
      (EFI_PHYSICAL_ADDRESS)FvInfo->Base,
      FvInfo->Length
      );
    DEBUG ((DEBUG_INFO, "Reported FV HOB: %016lX (%08lX)\r\n", FvInfo->Base, FvInfo->Length));
  }

  if ((FvInfo->Flag & HASHED_FV_FLAG_REPORT_FV_INFO_PPI) != 0) {
    //
    // Require PEI core to process this FV.
    //
    FvFormat = &((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvInfo->Base)->FileSystemGuid;
    PeiServicesInstallFvInfoPpi (
      FvFormat,
      (VOID *)(UINTN)FvInfo->Base,
      (UINT32)FvInfo->Length,
      NULL,
      NULL
      );
    DEBUG ((DEBUG_INFO, "Reported FV PPI: %016lX (%08lX)\r\n", FvInfo->Base, FvInfo->Length));
  }
}

/**
  Verify and report pre-hashed FVs.

  Doing this must be at post-memory to make sure there's enough memory to hold
  all FVs to be verified. This is necessary for mitigating TOCTOU issue.

  This function will never return if the verification is failed.

  @param[in] StoredHashFvPpi  Pointer to PPI containing hash information.
  @param[in] BootMode         Current boot mode.

  @retval Pointer to structure containing valid hash information for current boot mode.
  @retval NULL if there's no hash associated with current boot mode.
**/
STATIC
FV_HASH_INFO *
GetHashInfo (
  IN EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI  *StoredHashFvPpi,
  IN EFI_BOOT_MODE                                      BootMode
  )
{
  FV_HASH_INFO  *HashInfo;

  if ((StoredHashFvPpi->HashInfo.HashFlag & FV_HASH_FLAG_BOOT_MODE (BootMode)) != 0) {
    HashInfo = &StoredHashFvPpi->HashInfo;
  } else {
    HashInfo = NULL;
  }

  return HashInfo;
}

/**
  Verify and report pre-hashed FVs.

  Doing this must be at post-memory to make sure there's enough memory to hold
  all FVs to be verified. This is necessary for mitigating TOCTOU issue.

  This function will never return if the verification is failed.

  @param[in] PeiServices      General purpose services available to every PEIM.
  @param[in] BootMode         Current boot mode.

  @retval EFI_SUCCESS         The function completed successfully.
**/
STATIC
EFI_STATUS
CheckStoredHashFv (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_BOOT_MODE           BootMode
  )
{
  EFI_STATUS                                         Status;
  EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI  *StoredHashFvPpi;
  FV_HASH_INFO                                       *HashInfo;
  UINTN                                              FvIndex;

  //
  // Check pre-hashed FV list
  //
  StoredHashFvPpi = NULL;
  Status          = PeiServicesLocatePpi (
                      &gEdkiiPeiFirmwareVolumeInfoStoredHashFvPpiGuid,
                      0,
                      NULL,
                      (VOID **)&StoredHashFvPpi
                      );
  if (!EFI_ERROR (Status) && (StoredHashFvPpi != NULL) && (StoredHashFvPpi->FvNumber > 0)) {
    HashInfo = GetHashInfo (StoredHashFvPpi, BootMode);
    Status   = VerifyHashedFv (
                 HashInfo,
                 StoredHashFvPpi->FvInfo,
                 StoredHashFvPpi->FvNumber,
                 BootMode
                 );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OBB verification passed (%r)\r\n", Status));

      //
      // Report the FVs to PEI core and/or DXE core.
      //
      for (FvIndex = 0; FvIndex < StoredHashFvPpi->FvNumber; ++FvIndex) {
        if ((StoredHashFvPpi->FvInfo[FvIndex].Flag
             & HASHED_FV_FLAG_SKIP_BOOT_MODE (BootMode)) == 0)
        {
          ReportHashedFv (&StoredHashFvPpi->FvInfo[FvIndex]);
        }
      }

      REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        PcdGet32 (PcdStatusCodeFvVerificationPass)
        );
    } else {
      DEBUG ((DEBUG_ERROR, "ERROR: Failed to verify OBB FVs (%r)\r\n", Status));

      REPORT_STATUS_CODE_EX (
        EFI_PROGRESS_CODE,
        PcdGet32 (PcdStatusCodeFvVerificationFail),
        0,
        NULL,
        &gEdkiiPeiFirmwareVolumeInfoStoredHashFvPpiGuid,
        StoredHashFvPpi,
        sizeof (*StoredHashFvPpi)
        );

      ASSERT_EFI_ERROR (Status);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "ERROR: No/invalid StoredHashFvPpi located\r\n"));

    ASSERT_EFI_ERROR (Status);
    ASSERT (StoredHashFvPpi != NULL && StoredHashFvPpi->FvNumber > 0);

    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/**
  Main entry for FvReport PEIM.

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS  If all FVs reported by StoredHashFvPpi are verified.

**/
EFI_STATUS
EFIAPI
FvReportEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  Status = CheckStoredHashFv (PeiServices, BootMode);
  if (EFI_ERROR (Status)) {
    //
    // Never pass control to left part of BIOS if any error.
    //
    CpuDeadLoop ();
  }

  return Status;
}
