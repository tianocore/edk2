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

//
// Maximum number of IGVM data ranges that can be registered as
// pre-validated.  When booting via IGVM, pages loaded through
// SNP_LAUNCH_UPDATE are already validated by the PSP and must
// not be validated again.
//
#define MAX_IGVM_PRE_VALIDATED_RANGES  16

typedef struct {
  UINT64    StartAddress;
  UINT64    EndAddress;
} SNP_PRE_VALIDATED_RANGE;

//
// Compile-time pre-validated ranges (hypervisor metadata + SEC).
//
STATIC SNP_PRE_VALIDATED_RANGE  mStaticRanges[] = {
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

//
// IGVM pre-validated ranges, populated at runtime from HOBs.
//
STATIC SNP_PRE_VALIDATED_RANGE  mIgvmRanges[MAX_IGVM_PRE_VALIDATED_RANGES];
STATIC UINTN                    mIgvmRangeCount      = 0;
STATIC BOOLEAN                  mIgvmRangesCollected  = FALSE;

/**
  Scan HOB list for gEfiIgvmDataHobGuid entries and populate
  the IGVM pre-validated range array.  Called once on the first
  invocation of MemEncryptSevSnpPreValidateSystemRam().

**/
STATIC
VOID
CollectIgvmPreValidatedRanges (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_IGVM_DATA_HOB     *IgvmData;

  mIgvmRangesCollected = TRUE;

  for (Hob.Raw = GetFirstGuidHob (&gEfiIgvmDataHobGuid);
       Hob.Raw != NULL;
       Hob.Raw = GetNextGuidHob (&gEfiIgvmDataHobGuid, GET_NEXT_HOB (Hob)))
  {
    if (mIgvmRangeCount >= ARRAY_SIZE (mIgvmRanges)) {
      DEBUG ((DEBUG_WARN, "%a: Too many IGVM ranges, some will be re-validated\n", __func__));
      break;
    }

    IgvmData = GET_GUID_HOB_DATA (Hob.Guid);
    mIgvmRanges[mIgvmRangeCount].StartAddress = IgvmData->Address;
    mIgvmRanges[mIgvmRangeCount].EndAddress   = IgvmData->Address +
                                                  ALIGN_VALUE (IgvmData->Length, EFI_PAGE_SIZE);
    DEBUG ((
      DEBUG_INFO,
      "%a: IGVM pre-validated range [0x%lx, 0x%lx)\n",
      __func__,
      mIgvmRanges[mIgvmRangeCount].StartAddress,
      mIgvmRanges[mIgvmRangeCount].EndAddress
      ));
    mIgvmRangeCount++;
  }
}

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
  // Check the compile-time pre-validated ranges.
  //
  for (i = 0; i < ARRAY_SIZE (mStaticRanges); i++) {
    if ((mStaticRanges[i].StartAddress < EndAddress) &&
        (StartAddress < mStaticRanges[i].EndAddress))
    {
      OverlapRange->StartAddress = mStaticRanges[i].StartAddress;
      OverlapRange->EndAddress   = mStaticRanges[i].EndAddress;
      return TRUE;
    }
  }

  //
  // Check IGVM pre-validated ranges.
  //
  for (i = 0; i < mIgvmRangeCount; i++) {
    if ((mIgvmRanges[i].StartAddress < EndAddress) &&
        (StartAddress < mIgvmRanges[i].EndAddress))
    {
      OverlapRange->StartAddress = mIgvmRanges[i].StartAddress;
      OverlapRange->EndAddress   = mIgvmRanges[i].EndAddress;
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

  //
  // On first call, collect IGVM data ranges from HOBs so they are
  // treated as pre-validated alongside the compile-time ranges.
  //
  if (!mIgvmRangesCollected) {
    CollectIgvmPreValidatedRanges ();
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
