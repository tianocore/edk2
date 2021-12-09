/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>

#include "SnpPageStateChange.h"
#include "VirtualMemory.h"

typedef struct {
  UINT64    StartAddress;
  UINT64    EndAddress;
} SNP_PRE_VALIDATED_RANGE;

STATIC SNP_PRE_VALIDATED_RANGE  mPreValidatedRange[] = {
  // The below address range was part of the SEV OVMF metadata, and range
  // should be pre-validated by the Hypervisor.
  {
    FixedPcdGet32 (PcdOvmfSecPageTablesBase),
    FixedPcdGet32 (PcdOvmfPeiMemFvBase),
  },
  // The below range is pre-validated by the Sec/SecMain.c
  {
    FixedPcdGet32 (PcdOvmfSecValidatedStart),
    FixedPcdGet32 (PcdOvmfSecValidatedEnd)
  },
};

STATIC
BOOLEAN
DetectPreValidatedOverLap (
  IN    PHYSICAL_ADDRESS         StartAddress,
  IN    PHYSICAL_ADDRESS         EndAddress,
  OUT   SNP_PRE_VALIDATED_RANGE  *OverlapRange
  )
{
  UINTN  i;

  //
  // Check if the specified address range exist in pre-validated array.
  //
  for (i = 0; i < ARRAY_SIZE (mPreValidatedRange); i++) {
    if ((mPreValidatedRange[i].StartAddress < EndAddress) &&
        (StartAddress < mPreValidatedRange[i].EndAddress))
    {
      OverlapRange->StartAddress = mPreValidatedRange[i].StartAddress;
      OverlapRange->EndAddress   = mPreValidatedRange[i].EndAddress;
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
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             NumPages
  )
{
  PHYSICAL_ADDRESS         EndAddress;
  SNP_PRE_VALIDATED_RANGE  OverlapRange;
  EFI_STATUS               Status;

  if (!MemEncryptSevSnpIsEnabled ()) {
    return;
  }

  EndAddress = BaseAddress + EFI_PAGES_TO_SIZE (NumPages);

  //
  // The page table used in PEI can address up to 4GB memory. If we are asked to
  // validate a range above the 4GB, then create an identity mapping so that the
  // PVALIDATE instruction can execute correctly. If the page table entry is not
  // present then PVALIDATE will #GP.
  //
  if (BaseAddress >= SIZE_4GB) {
    Status = InternalMemEncryptSevCreateIdentityMap1G (
               0,
               BaseAddress,
               EFI_PAGES_TO_SIZE (NumPages)
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      CpuDeadLoop ();
    }
  }

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
