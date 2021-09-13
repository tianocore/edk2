/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/MemEncryptSevLib.h>

#include "SnpPageStateChange.h"

typedef struct {
  UINT64    StartAddress;
  UINT64    EndAddress;
} SNP_PRE_VALIDATED_RANGE;

STATIC SNP_PRE_VALIDATED_RANGE mPreValidatedRange[] = {
  // The below address range was part of the OVMF metadata, and range
  // should be pre-validated by the Hypervisor.
  {
    FixedPcdGet32 (PcdOvmfSecPageTablesBase),
    FixedPcdGet32 (PcdOvmfSecPageTablesBase) + FixedPcdGet32 (PcdOvmfSecPageTablesSize),
  },
  {
    FixedPcdGet32 (PcdOvmfLockBoxStorageBase),
    FixedPcdGet32 (PcdOvmfLockBoxStorageBase) + FixedPcdGet32 (PcdOvmfLockBoxStorageSize),
  },
  {
    FixedPcdGet64 (PcdGuidedExtractHandlerTableAddress),
    FixedPcdGet64 (PcdGuidedExtractHandlerTableAddress) + FixedPcdGet32 (PcdGuidedExtractHandlerTableSize)
  },
  {
    FixedPcdGet32 (PcdOvmfSecGhcbPageTableBase),
    FixedPcdGet32 (PcdOvmfSecGhcbPageTableBase) + FixedPcdGet32 (PcdOvmfSecGhcbPageTableSize),
  },
  {
    FixedPcdGet32 (PcdOvmfSecGhcbBase),
    FixedPcdGet32 (PcdOvmfSecGhcbBase) + FixedPcdGet32 (PcdOvmfSecGhcbSize),
  },
  {
    FixedPcdGet32 (PcdOvmfWorkAreaBase),
    FixedPcdGet32 (PcdOvmfWorkAreaBase) + FixedPcdGet32 (PcdOvmfWorkAreaSize),
  },
  {
    FixedPcdGet32 (PcdOvmfSecGhcbBackupBase),
    FixedPcdGet32 (PcdOvmfSecGhcbBackupBase) + FixedPcdGet32 (PcdOvmfSecGhcbBackupSize),
  },
  {
    FixedPcdGet32 (PcdOvmfSnpSecretsBase),
    FixedPcdGet32 (PcdOvmfSnpSecretsBase) + FixedPcdGet32 (PcdOvmfSnpSecretsSize),
  },
  {
    FixedPcdGet32 (PcdOvmfCpuidBase),
    FixedPcdGet32 (PcdOvmfCpuidBase) + FixedPcdGet32 (PcdOvmfCpuidSize),
  },
  {
    FixedPcdGet32 (PcdOvmfSecPeiTempRamBase),
    FixedPcdGet32 (PcdOvmfSecPeiTempRamBase) + FixedPcdGet32 (PcdOvmfSecPeiTempRamSize),
  },
};

STATIC
BOOLEAN
DetectPreValidatedOverLap (
  IN    PHYSICAL_ADDRESS            StartAddress,
  IN    PHYSICAL_ADDRESS            EndAddress,
  OUT   SNP_PRE_VALIDATED_RANGE     *OverlapRange
  )
{
  UINTN               i;

  //
  // Check if the specified address range exist in pre-validated array.
  //
  for (i = 0; i < ARRAY_SIZE (mPreValidatedRange); i++) {
    if ((mPreValidatedRange[i].StartAddress < EndAddress) &&
        (StartAddress < mPreValidatedRange[i].EndAddress)) {
      OverlapRange->StartAddress = mPreValidatedRange[i].StartAddress;
      OverlapRange->EndAddress = mPreValidatedRange[i].EndAddress;
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Pre-validate the system RAM when SEV-SNP is enabled in the guest VM.

  @param[in]  BaseAddress             Base address
  @param[in]  NumPages                Number of pages starting from the base address

**/
VOID
EFIAPI
MemEncryptSevSnpPreValidateSystemRam (
  IN PHYSICAL_ADDRESS                   BaseAddress,
  IN UINTN                              NumPages
  )
{
  PHYSICAL_ADDRESS          EndAddress;
  SNP_PRE_VALIDATED_RANGE   OverlapRange;

  if (!MemEncryptSevSnpIsEnabled ()) {
    return;
  }

  EndAddress = BaseAddress + EFI_PAGES_TO_SIZE (NumPages);

  while (BaseAddress < EndAddress) {
    //
    // Check if the range overlaps with the pre-validated ranges.
    //
    if (DetectPreValidatedOverLap (BaseAddress, EndAddress, &OverlapRange)) {
      // Validate the non-overlap regions.
      if (BaseAddress < OverlapRange.StartAddress) {
        NumPages = EFI_SIZE_TO_PAGES (OverlapRange.StartAddress - BaseAddress);

        InternalSetPageState (BaseAddress, NumPages, SevSnpPagePrivate, TRUE);
      }

      BaseAddress = OverlapRange.EndAddress;
      continue;
    }

    // Validate the remaining pages.
    NumPages = EFI_SIZE_TO_PAGES (EndAddress - BaseAddress);
    InternalSetPageState (BaseAddress, NumPages, SevSnpPagePrivate, TRUE);
    BaseAddress = EndAddress;
  }
}
