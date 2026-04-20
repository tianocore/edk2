/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 - 2024, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemEncryptSevLib.h>
#include <IndustryStandard/IgvmData.h>

#include "SnpPageStateChange.h"
#include "VirtualMemory.h"

STATIC UINT8  mPscBufferPage[EFI_PAGE_SIZE];

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
  EFI_HOB_GUID_TYPE  *Hob;
  EFI_IGVM_DATA_HOB  *IgvmData;
  UINT64             IgvmStart;
  UINT64             IgvmEnd;
  UINTN              i;

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

  //
  // Check if the specified address range exist in igvm data hobs ranges.
  //
  for (Hob = GetFirstGuidHob (&gEfiIgvmDataHobGuid);
       Hob != NULL;
       Hob = GetNextGuidHob (&gEfiIgvmDataHobGuid, GET_NEXT_HOB (Hob)))
  {
    IgvmData  = (VOID *)(Hob + 1);
    IgvmStart = IgvmData->Address;
    IgvmEnd   = ALIGN_VALUE (IgvmData->Address + IgvmData->Length, 4096);

    if ((IgvmStart < EndAddress) &&
        (StartAddress < IgvmEnd))
    {
      OverlapRange->StartAddress = IgvmStart;
      OverlapRange->EndAddress   = IgvmEnd;
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

        InternalSetPageState (
          BaseAddress,
          NumPages,
          SevSnpPagePrivate,
          TRUE,
          mPscBufferPage,
          sizeof (mPscBufferPage)
          );
      }

      BaseAddress = OverlapRange.EndAddress;
      continue;
    }

    // Validate the remaining pages.
    NumPages = EFI_SIZE_TO_PAGES (EndAddress - BaseAddress);
    InternalSetPageState (
      BaseAddress,
      NumPages,
      SevSnpPagePrivate,
      TRUE,
      mPscBufferPage,
      sizeof (mPscBufferPage)
      );
    BaseAddress = EndAddress;
  }
}
