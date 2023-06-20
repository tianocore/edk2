/** @file
  SMM IPL that load the SMM Core into SMRAM at PEI stage

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiSmm.h>
#include <StandaloneMm.h>
#include <Ppi/SmmAccess.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Guid/MmCoreData.h>
#include <StandaloneMmIplPei.h>

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
  Load SMM core to dispatch other Standalone MM drivers.

  @param  Entry                     Entry of Standalone MM Foundation.
  @param  Context1                  A pointer to the context to pass into the EntryPoint
                                    function.
  @retval EFI_SUCCESS               Successfully loaded SMM core.
  @retval Others                    Failed to load SMM core.
**/
EFI_STATUS
LoadSmmCore (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  )
{
  STANDALONE_MM_FOUNDATION_ENTRY_POINT  EntryPoint;

  EntryPoint = (STANDALONE_MM_FOUNDATION_ENTRY_POINT)(UINTN)Entry;
  return EntryPoint (Context1);
}

/**
  Search all the available firmware volumes for SMM Core driver

  @param  MmFvBaseAddress      Base address of FV which included SMM Core driver.
  @param  MmCoreImageAddress   Image address of SMM Core driver.

  @retval EFI_SUCCESS          The specified FFS section was returned.
  @retval EFI_NOT_FOUND        The specified FFS section could not be found.

**/
EFI_STATUS
LocateMmFvForMmCore (
  OUT EFI_PHYSICAL_ADDRESS  *MmFvBaseAddress,
  OUT VOID                  **MmCoreImageAddress
  )
{
  EFI_STATUS           Status;
  UINTN                FvIndex;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;
  EFI_PE32_SECTION     *SectionData;
  EFI_FV_INFO          VolumeInfo;

  //
  // Search all FV
  //
  VolumeHandle = NULL;
  for (FvIndex = 0; ; FvIndex++) {
    Status = PeiServicesFfsFindNextVolume (FvIndex, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Search PEIM FFS
    //
    FileHandle = NULL;
    Status     = PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_MM_CORE_STANDALONE, VolumeHandle, &FileHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Search Section
    //
    Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, MmCoreImageAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Great!
    //
    SectionData = (EFI_PE32_SECTION *)((UINT8 *)*MmCoreImageAddress - sizeof (EFI_PE32_SECTION));
    ASSERT (SectionData->Type == EFI_SECTION_PE32);

    //
    // This is SMM BFV
    //
    Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
    if (!EFI_ERROR (Status)) {
      *MmFvBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Load the SMM Core image into SMRAM and executes the SMM Core from SMRAM.

  @param[in, out] SmramRange            Descriptor for the range of SMRAM to reload the
                                        currently executing image, the rang of SMRAM to
                                        hold SMM Core will be excluded.
  @param[in, out] SmramRangeSmmCore     Descriptor for the range of SMRAM to hold SMM Core.

  @param[in]      Context               Context to pass into SMM Core

  @return  EFI_STATUS

**/
EFI_STATUS
ExecuteSmmCoreFromSmram (
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramRange,
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramRangeSmmCore,
  IN     VOID                  *Context
  )
{
  EFI_STATUS                    Status;
  VOID                          *SourceBuffer;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                         PageCount;
  VOID                          *HobList;
  EFI_PHYSICAL_ADDRESS          SourceFvBaseAddress;

  Status = PeiServicesGetHobList (&HobList);
  ASSERT_EFI_ERROR (Status);

  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type SMM_CORE
  //
  Status = LocateMmFvForMmCore (&SourceFvBaseAddress, &SourceBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gMmCorePrivate->StandaloneBfvAddress = SourceFvBaseAddress;

  //
  // Initialize ImageContext
  //
  ImageContext.Handle    = SourceBuffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate memory for the image being loaded from the EFI_SRAM_DESCRIPTOR
  // specified by SmramRange
  //
  PageCount = (UINTN)EFI_SIZE_TO_PAGES ((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

  ASSERT ((SmramRange->PhysicalSize & EFI_PAGE_MASK) == 0);
  ASSERT (SmramRange->PhysicalSize > EFI_PAGES_TO_SIZE (PageCount));

  SmramRange->PhysicalSize        -= EFI_PAGES_TO_SIZE (PageCount);
  SmramRangeSmmCore->CpuStart      = SmramRange->CpuStart + SmramRange->PhysicalSize;
  SmramRangeSmmCore->PhysicalStart = SmramRange->PhysicalStart + SmramRange->PhysicalSize;
  SmramRangeSmmCore->RegionState   = SmramRange->RegionState | EFI_ALLOCATED;
  SmramRangeSmmCore->PhysicalSize  = EFI_PAGES_TO_SIZE (PageCount);

  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress = SmramRangeSmmCore->CpuStart;

  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);

  //
  // Print debug message showing SMM Core load address.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL loading SMM Core at SMRAM address %p\n", (VOID *)(UINTN)ImageContext.ImageAddress));

  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (!EFI_ERROR (Status)) {
    //
    // Relocate the image in our new buffer
    //
    Status = PeCoffLoaderRelocateImage (&ImageContext);
    if (!EFI_ERROR (Status)) {
      //
      // Flush the instruction cache so the image data are written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      //
      // Print debug message showing SMM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "SMM IPL calling SMM Core at SMRAM address %p\n", (VOID *)(UINTN)ImageContext.EntryPoint));

      gMmCorePrivate->MmCoreImageBase = ImageContext.ImageAddress;
      gMmCorePrivate->MmCoreImageSize = ImageContext.ImageSize;
      DEBUG ((DEBUG_INFO, "SmmCoreImageBase - 0x%016lx\n", gMmCorePrivate->MmCoreImageBase));
      DEBUG ((DEBUG_INFO, "SmmCoreImageSize - 0x%016lx\n", gMmCorePrivate->MmCoreImageSize));

      gMmCorePrivate->MmCoreEntryPoint = ImageContext.EntryPoint;

      //
      // Print debug message showing Standalone MM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "SMM IPL calling Standalone MM Core at SMRAM address - 0x%016lx\n", gMmCorePrivate->MmCoreEntryPoint));

      //
      // If StandaloneMmIpl driver is IA32 image, but Standalone MM core is X64 image
      // Switch 32 bit protect mode to X64 mode and load SMM core
      //
      if ((sizeof (UINTN) == sizeof (UINT32)) && (ImageContext.Machine == EFI_IMAGE_MACHINE_X64)) {
        //
        // Switch to X64 mode and load SMM core
        //
        LoadSmmCoreIA32ToX64 (ImageContext.EntryPoint, HobList);
      } else {
        //
        // Execute image
        //
        LoadSmmCore (ImageContext.EntryPoint, HobList);
      }
    }
  }

  //
  // If the load operation, relocate operation, or the image execution return an
  // error, then free memory allocated from the EFI_SRAM_DESCRIPTOR specified by
  // SmramRange
  //
  if (EFI_ERROR (Status)) {
    SmramRange->PhysicalSize += EFI_PAGES_TO_SIZE (PageCount);
  }

  //
  // Always free memory allocated by GetFileBufferByFilePath ()
  //
  FreePool (SourceBuffer);

  return Status;
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

    //
    // Load SMM Core into SMRAM and execute it from SMRAM
    // Note: SmramRanges specific for SMM Core will put in the gMmCorePrivate->MmramRangeCount - 1.
    //
    Status = ExecuteSmmCoreFromSmram (
               mCurrentSmramRange,
               &(((EFI_MMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges)[gMmCorePrivate->MmramRangeCount - 1]),
               gMmCorePrivate
               );
    if (EFI_ERROR (Status)) {
      //
      // Print error message that the SMM Core failed to be loaded and executed.
      //
      DEBUG ((DEBUG_ERROR, "SMM IPL could not load and execute SMM Core from SMRAM\n"));
    }
  } else {
    //
    // Print error message that there are not enough SMRAM resources to load the SMM Core.
    //
    DEBUG ((DEBUG_ERROR, "SMM IPL could not find a large enough SMRAM region to load SMM Core\n"));
  }

  return EFI_SUCCESS;
}
