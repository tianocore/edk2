/** @file
  SMM IPL that load the SMM Core into SMRAM at PEI stage

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/SmmAccess.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Guid/MmCoreData.h>

//
// MM Core Private Data structure that contains the data shared between
// the SMM IPL and the Standalone MM Core.
//
MM_CORE_PRIVATE_DATA  mMmCorePrivateData = {
  MM_CORE_PRIVATE_DATA_SIGNATURE,     // Signature
  0,                                  // MmramRangeCount
  0,                                  // MmramRanges
  0,                                  // MmEntryPoint
  FALSE,                              // MmEntryPointRegistered
  FALSE,                              // InMm
  0,                                  // Mmst
  0,                                  // CommunicationBuffer
  0,                                  // BufferSize
  EFI_SUCCESS,                        // ReturnStatus
  0,                                  // MmCoreImageBase
  0,                                  // MmCoreImageSize
  0,                                  // MmCoreEntryPoint
  0,                                  // StandaloneBfvAddress
};

//
// Global pointer used to access mMmCorePrivateData from outside and inside SMM
//
MM_CORE_PRIVATE_DATA  *gMmCorePrivate;

//
// SMM IPL global variables
//
PEI_SMM_ACCESS_PPI    *mSmmAccess;
EFI_SMRAM_DESCRIPTOR  *mCurrentSmramRange;
EFI_PHYSICAL_ADDRESS  mSmramCacheBase;
UINT64                mSmramCacheSize;
UINTN                 mSmramRangeCount;

/**
  Find the maximum SMRAM cache range that covers the range specified by SmramRange.

  This function searches and joins all adjacent ranges of SmramRange into a range to be cached.

  @param   SmramRange       The SMRAM range to search from.
  @param   SmramCacheBase   The returned cache range base.
  @param   SmramCacheSize   The returned cache range size.
**/
VOID
GetSmramCacheRange (
  IN  EFI_SMRAM_DESCRIPTOR  *SmramRange,
  OUT EFI_PHYSICAL_ADDRESS  *SmramCacheBase,
  OUT UINT64                *SmramCacheSize
  )
{
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  RangeCpuStart;
  UINT64                RangePhysicalSize;
  BOOLEAN               FoundAdjacentRange;
  EFI_SMRAM_DESCRIPTOR  *SmramRanges;

  *SmramCacheBase = SmramRange->CpuStart;
  *SmramCacheSize = SmramRange->PhysicalSize;

  SmramRanges = (EFI_SMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges;
  do {
    FoundAdjacentRange = FALSE;
    for (Index = 0; Index < gMmCorePrivate->MmramRangeCount; Index++) {
      RangeCpuStart     = SmramRanges[Index].CpuStart;
      RangePhysicalSize = SmramRanges[Index].PhysicalSize;
      if ((RangeCpuStart < *SmramCacheBase) && (*SmramCacheBase == (RangeCpuStart + RangePhysicalSize))) {
        *SmramCacheBase    = RangeCpuStart;
        *SmramCacheSize   += RangePhysicalSize;
        FoundAdjacentRange = TRUE;
      } else if (((*SmramCacheBase + *SmramCacheSize) == RangeCpuStart) && (RangePhysicalSize > 0)) {
        *SmramCacheSize   += RangePhysicalSize;
        FoundAdjacentRange = TRUE;
      }
    }
  } while (FoundAdjacentRange);
}

/**
  Get full SMRAM ranges.

  It will get SMRAM ranges from SmmAccess PPI. It will also reserve one entry
  for SMM core.

  @param[in]  PeiServices           Describes the list of possible PEI Services.
  @param[out] FullSmramRangeCount   Output pointer to full SMRAM range count.

  @return Pointer to full SMRAM ranges.

**/
EFI_SMRAM_DESCRIPTOR *
GetFullSmramRanges (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT       UINTN             *FullSmramRangeCount
  )
{
  EFI_STATUS            Status;
  UINTN                 Size;
  EFI_SMRAM_DESCRIPTOR  *FullSmramRanges;
  UINTN                 AdditionSmramRangeCount;

  //
  // Get SMRAM information.
  //
  Size   = 0;
  Status = mSmmAccess->GetCapabilities ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  mSmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Reserve one entry SMM Core in the full SMRAM ranges.
  //
  AdditionSmramRangeCount = 1;

  *FullSmramRangeCount = mSmramRangeCount + AdditionSmramRangeCount;
  Size                 = (*FullSmramRangeCount) * sizeof (EFI_SMRAM_DESCRIPTOR);
  FullSmramRanges      = (EFI_SMRAM_DESCRIPTOR *)AllocateZeroPool (Size);
  ASSERT (FullSmramRanges != NULL);
  if (FullSmramRanges == NULL) {
    return NULL;
  }

  Status = mSmmAccess->GetCapabilities ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, &Size, FullSmramRanges);

  ASSERT_EFI_ERROR (Status);

  return FullSmramRanges;
}

/**
  The Entry Point for SMM IPL at PEI stage

  Load SMM Core into SMRAM.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
StandaloneMmIplPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS             Status;
  UINTN                  Index;
  UINT64                 MaxSize;
  MM_CORE_DATA_HOB_DATA  SmmCoreDataHobData;
  EFI_SMRAM_DESCRIPTOR   *MmramRanges;

  //
  // Build Hob for SMM and DXE phase
  //
  SmmCoreDataHobData.Address = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (mMmCorePrivateData)));
  ASSERT (SmmCoreDataHobData.Address != 0);
  gMmCorePrivate = (VOID *)(UINTN)SmmCoreDataHobData.Address;
  CopyMem ((VOID *)(UINTN)SmmCoreDataHobData.Address, &mMmCorePrivateData, sizeof (mMmCorePrivateData));
  DEBUG ((DEBUG_INFO, "gMmCorePrivate - 0x%x\n", gMmCorePrivate));

  BuildGuidDataHob (
    &gMmCoreDataHobGuid,
    (VOID *)&SmmCoreDataHobData,
    sizeof (SmmCoreDataHobData)
    );

  //
  // Get SMM Access PPI
  //
  Status = PeiServicesLocatePpi (&gPeiSmmAccessPpiGuid, 0, NULL, (VOID **)&mSmmAccess);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMRAM information
  //
  gMmCorePrivate->MmramRanges = (EFI_PHYSICAL_ADDRESS)(UINTN)GetFullSmramRanges (PeiServices, (UINTN *)&gMmCorePrivate->MmramRangeCount);
  ASSERT (gMmCorePrivate->MmramRanges != 0);
  if (gMmCorePrivate->MmramRanges == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Open all SMRAM ranges
  //
  for (Index = 0; Index < mSmramRangeCount; Index++) {
    Status = mSmmAccess->Open ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, Index);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Print debug message that the SMRAM window is now open.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL opened SMRAM window\n"));

  //
  // Find the largest SMRAM range between 1MB and 4GB that is at least 256KB - 4K in size
  //
  mCurrentSmramRange = NULL;
  MmramRanges        = (EFI_MMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges;
  if (MmramRanges == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to retrieve MmramRanges\n"));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < gMmCorePrivate->MmramRangeCount; Index++) {
    //
    // Skip any SMRAM region that is already allocated, needs testing, or needs ECC initialization
    //
    if ((MmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    if (MmramRanges[Index].CpuStart >= BASE_1MB) {
      if ((MmramRanges[Index].CpuStart + MmramRanges[Index].PhysicalSize) <= BASE_4GB) {
        if (MmramRanges[Index].PhysicalSize >= MaxSize) {
          MaxSize            = MmramRanges[Index].PhysicalSize;
          mCurrentSmramRange = &MmramRanges[Index];
        }
      }
    }
  }

  if (mCurrentSmramRange != NULL) {
    //
    // Print debug message showing SMRAM window that will be used by SMM IPL and SMM Core
    //
    DEBUG ((
      DEBUG_INFO,
      "SMM IPL found SMRAM window %p - %p\n",
      (VOID *)(UINTN)mCurrentSmramRange->CpuStart,
      (VOID *)(UINTN)(mCurrentSmramRange->CpuStart + mCurrentSmramRange->PhysicalSize - 1)
      ));

    GetSmramCacheRange (mCurrentSmramRange, &mSmramCacheBase, &mSmramCacheSize);
  } else {
    //
    // Print error message that there are not enough SMRAM resources to load the SMM Core.
    //
    DEBUG ((DEBUG_ERROR, "SMM IPL could not find a large enough SMRAM region to load SMM Core\n"));
  }

  return EFI_SUCCESS;
}
