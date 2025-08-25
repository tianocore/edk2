/** @file
  MM IPL that loads the MM Core into MMRAM.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmIplPei.h"

/**
  Create HOB list for Standalone MM core.

  @param[out]  HobSize              HOB size of fundation and platform HOB list.
  @param[in]   MmCommBuffer         Pointer of MM communication buffer.
  @param[in]   MmFvBase             Base of MM FV which included MM core driver.
  @param[in]   MmFvSize             Size of MM FV which included MM core driver.
  @param[in]   MmCoreFileName       File GUID of MM core driver.
  @param[in]   MmCoreImageAddress   Address of MM core image.
  @param[in]   MmCoreImageSize      Size of MM core image.
  @param[in]   MmCoreEntryPoint     Entry point of MM core driver.
  @param[in]   Block                Pointer of MMRAM descriptor block.

  @retval HobList              If fundation and platform HOBs not existed,
                               it is pointed to PEI HOB List. If existed,
                               it is pointed to fundation and platform HOB list.
**/
VOID *
CreateMmHobList (
  OUT UINTN                           *HobSize,
  IN  MM_COMM_BUFFER                  *MmCommBuffer,
  IN  EFI_PHYSICAL_ADDRESS            MmFvBase,
  IN  UINT64                          MmFvSize,
  IN  EFI_GUID                        *MmCoreFileName,
  IN  PHYSICAL_ADDRESS                MmCoreImageAddress,
  IN  UINT64                          MmCoreImageSize,
  IN  PHYSICAL_ADDRESS                MmCoreEntryPoint,
  IN  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block
  )
{
  EFI_STATUS                 Status;
  VOID                       *HobList;
  VOID                       *PlatformHobList;
  UINTN                      PlatformHobSize;
  UINTN                      BufferSize;
  UINTN                      FoundationHobSize;
  EFI_HOB_MEMORY_ALLOCATION  *MmProfileDataHob;
  UINTN                      PhitHobSize;
  VOID                       *HobEnd;

  //
  // Get platform HOBs
  //
  PlatformHobSize = 0;
  Status          = CreateMmPlatformHob (NULL, &PlatformHobSize);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    if (PlatformHobSize == 0) {
      DEBUG ((DEBUG_ERROR, "%a: PlatformHobSize is zero, cannot create MM HOBs\n", __func__));
      ASSERT (PlatformHobSize != 0);
      return NULL;
    }

    //
    // Create platform HOBs for MM foundation to get MMIO HOB data.
    //
    PlatformHobList = AllocatePages (EFI_SIZE_TO_PAGES (PlatformHobSize));
    if (PlatformHobList == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Out of resource to create platform MM HOBs\n", __func__));
      ASSERT (PlatformHobList != NULL);
      return NULL;
    }

    BufferSize = PlatformHobSize;
    Status     = CreateMmPlatformHob (PlatformHobList, &PlatformHobSize);
    if (BufferSize != PlatformHobSize) {
      DEBUG ((DEBUG_ERROR, "%a: CreateMmPlatformHob returned unexpected size (%d != %d)\n", __func__, BufferSize, PlatformHobSize));
      FreePages (PlatformHobList, EFI_SIZE_TO_PAGES (PlatformHobSize));
      return NULL;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CreateMmPlatformHob failed (%r)\n", __func__, Status));
    return NULL;
  }

  //
  // Build memory allocation HOB in PEI HOB list for MM profile data.
  //
  MmProfileDataHob = NULL;
  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    MmProfileDataHob = BuildMmProfileDataHobInPeiHobList ();
  }

  //
  // Get size of foundation HOBs
  //
  FoundationHobSize = 0;
  Status            = CreateMmFoundationHobList (
                        NULL,
                        &FoundationHobSize,
                        PlatformHobList,
                        PlatformHobSize,
                        MmFvBase,
                        MmFvSize,
                        MmCoreFileName,
                        MmCoreImageAddress,
                        MmCoreImageSize,
                        MmCoreEntryPoint,
                        MmProfileDataHob,
                        Block
                        );
  if (PlatformHobSize != 0) {
    FreePages (PlatformHobList, EFI_SIZE_TO_PAGES (PlatformHobSize));
  }

  ASSERT (Status == RETURN_BUFFER_TOO_SMALL);
  ASSERT (FoundationHobSize != 0);

  PhitHobSize = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);
  //
  // Final result includes: PHIT HOB, Platform HOBs, Foundation HOBs and an END node.
  //
  *HobSize = PhitHobSize + PlatformHobSize + FoundationHobSize + sizeof (EFI_HOB_GENERIC_HEADER);
  HobList  = AllocatePages (EFI_SIZE_TO_PAGES (*HobSize));
  ASSERT (HobList != NULL);
  if (HobList == NULL) {
    DEBUG ((DEBUG_ERROR, "Out of resource to create MM HOBs\n"));
    CpuDeadLoop ();
  }

  HobEnd = (UINT8 *)(UINTN)HobList + PhitHobSize + PlatformHobSize + FoundationHobSize;
  //
  // Create MmHobHandoffInfoTable
  //
  CreateMmHobHandoffInfoTable (HobList, HobEnd);

  //
  // Get platform HOBs
  //
  Status = CreateMmPlatformHob ((UINT8 *)HobList + PhitHobSize, &PlatformHobSize);
  ASSERT_EFI_ERROR (Status);

  //
  // Get foundation HOBs
  //
  Status = CreateMmFoundationHobList (
             (UINT8 *)HobList + PhitHobSize + PlatformHobSize,
             &FoundationHobSize,
             HobList,
             PlatformHobSize,
             MmFvBase,
             MmFvSize,
             MmCoreFileName,
             MmCoreImageAddress,
             MmCoreImageSize,
             MmCoreEntryPoint,
             MmProfileDataHob,
             Block
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Create MM HOB list end.
  //
  MmIplCreateHob (HobEnd, EFI_HOB_TYPE_END_OF_HOB_LIST, sizeof (EFI_HOB_GENERIC_HEADER));

  return HobList;
}

/**
  Find largest unallocated MMRAM in current MMRAM descriptor block

  @param[in, out] LagestMmramRangeIndex  Lagest mmram range index.
  @param[in]      CurrentBlock           Current MMRAM descriptor block.

**/
VOID
FindLargestMmramRange (
  IN OUT UINTN                       *LagestMmramRangeIndex,
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *CurrentBlock
  )
{
  UINTN                 Index;
  UINT64                MaxSize;
  BOOLEAN               Found;
  EFI_MMRAM_DESCRIPTOR  *MmramRanges;

  MmramRanges = CurrentBlock->Descriptor;

  //
  // Find largest Mmram range.
  //
  Found = FALSE;
  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < CurrentBlock->NumberOfMmReservedRegions; Index++) {
    //
    // Skip any MMRAM region that is already allocated, needs testing, or needs ECC initialization
    //
    if ((MmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    if (MmramRanges[Index].CpuStart >= BASE_1MB) {
      if ((MmramRanges[Index].CpuStart + MmramRanges[Index].PhysicalSize) <= BASE_4GB) {
        if (MmramRanges[Index].PhysicalSize >= MaxSize) {
          Found                  = TRUE;
          *LagestMmramRangeIndex = Index;
          MaxSize                = MmramRanges[Index].PhysicalSize;
        }
      }
    }
  }

  if (Found == FALSE) {
    DEBUG ((DEBUG_ERROR, "Not found largest unlocated MMRAM\n"));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  return;
}

/**
  Allocate available MMRAM for MM core image.

  @param[in]  Pages                     Page count of MM core image.
  @param[out] NewBlock                  Pointer to new MMRAM blocks.

  @return  EFI_PHYSICAL_ADDRESS         Address for MM core image to be loaded in MMRAM.
**/
EFI_PHYSICAL_ADDRESS
MmIplAllocateMmramPage (
  IN  UINTN                           Pages,
  OUT EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  **NewBlock
  )
{
  UINTN                           LagestMmramRangeIndex;
  UINT32                          FullMmramRangeCount;
  EFI_HOB_GUID_TYPE               *MmramInfoHob;
  EFI_MMRAM_DESCRIPTOR            *Largest;
  EFI_MMRAM_DESCRIPTOR            *Allocated;
  EFI_MMRAM_DESCRIPTOR            *FullMmramRanges;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *CurrentBlock;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *NewDescriptorBlock;

  MmramInfoHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  ASSERT (MmramInfoHob != NULL);
  if (MmramInfoHob == NULL) {
    DEBUG ((DEBUG_WARN, "SmramMemoryReserve HOB not found\n"));
    return 0;
  }

  CurrentBlock = (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK *)(GET_GUID_HOB_DATA (MmramInfoHob));

  //
  // 1. Find largest unallocated MMRAM region
  //
  FindLargestMmramRange (&LagestMmramRangeIndex, CurrentBlock);
  ASSERT (LagestMmramRangeIndex < CurrentBlock->NumberOfMmReservedRegions);

  //
  // 2. Split the largest region and mark the allocated region as ALLOCATED
  //
  FullMmramRangeCount = CurrentBlock->NumberOfMmReservedRegions + 1;
  NewDescriptorBlock  = (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK *)AllocatePool (
                                                            sizeof (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK) + ((FullMmramRangeCount - 1) * sizeof (EFI_MMRAM_DESCRIPTOR))
                                                            );
  ASSERT (NewDescriptorBlock != NULL);
  if (NewDescriptorBlock == NULL) {
    return 0;
  }

  NewDescriptorBlock->NumberOfMmReservedRegions = FullMmramRangeCount;
  FullMmramRanges                               = NewDescriptorBlock->Descriptor;

  //
  // Get current MMRAM descriptors and fill to the full MMRAM ranges
  //
  CopyMem (NewDescriptorBlock->Descriptor, CurrentBlock->Descriptor, CurrentBlock->NumberOfMmReservedRegions * sizeof (EFI_MMRAM_DESCRIPTOR));

  Largest = &FullMmramRanges[LagestMmramRangeIndex];
  ASSERT ((Largest->PhysicalSize & EFI_PAGE_MASK) == 0);
  ASSERT (Largest->PhysicalSize > EFI_PAGES_TO_SIZE (Pages));

  Allocated = &NewDescriptorBlock->Descriptor[NewDescriptorBlock->NumberOfMmReservedRegions - 1];

  //
  // Allocate MMRAM
  //
  Largest->PhysicalSize   -= EFI_PAGES_TO_SIZE (Pages);
  Allocated->CpuStart      = Largest->CpuStart + Largest->PhysicalSize;
  Allocated->PhysicalStart = Largest->PhysicalStart + Largest->PhysicalSize;
  Allocated->RegionState   = Largest->RegionState | EFI_ALLOCATED;
  Allocated->PhysicalSize  = EFI_PAGES_TO_SIZE (Pages);

  //
  // New MMRAM descriptor block
  //
  *NewBlock = NewDescriptorBlock;

  return Allocated->CpuStart;
}

/**
  Load the MM Core image into MMRAM and executes the MM Core from MMRAM.

  @param[in] MmCommBuffer               MM communicate buffer

  @return    EFI_STATUS                 Execute MM core successfully.
             Other                      Execute MM core failed.
**/
EFI_STATUS
ExecuteMmCoreFromMmram (
  IN  MM_COMM_BUFFER  *MmCommBuffer
  )
{
  EFI_STATUS                            Status;
  EFI_STATUS                            Status2;
  UINTN                                 PageCount;
  VOID                                  *MmHobList;
  UINTN                                 MmHobSize;
  EFI_GUID                              MmCoreFileName;
  UINTN                                 MmFvSize;
  EFI_PHYSICAL_ADDRESS                  MmFvBase;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  STANDALONE_MM_FOUNDATION_ENTRY_POINT  Entry;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK        *Block;

  MmFvBase = 0;
  MmFvSize = 0;
  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type MM_CORE_STANDALONE.
  //
  Status = LocateMmCoreFv (&MmFvBase, &MmFvSize, &MmCoreFileName, &ImageContext.Handle);
  ASSERT_EFI_ERROR (Status);

  //
  // Open all MMRAM ranges if MmAccess is available.
  //
  Status = MmAccessOpen ();
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Initialize ImageContext
  //
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  PageCount = (UINTN)EFI_SIZE_TO_PAGES ((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

  //
  // Allocate memory for the image being loaded from unallocated mmram range
  //
  ImageContext.ImageAddress = MmIplAllocateMmramPage (PageCount, &Block);
  if (ImageContext.ImageAddress == 0) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);

  //
  // Call platform hook now that image info is known
  //
  PlatformHookBeforeMmLoad (&ImageContext);

  //
  // Print debug message showing MM Core load address.
  //
  DEBUG ((DEBUG_INFO, "StandaloneMM IPL loading MM Core at MMRAM address %p\n", (VOID *)(UINTN)ImageContext.ImageAddress));

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
      DEBUG ((DEBUG_INFO, "MmCoreImageBase  - 0x%016lx\n", ImageContext.ImageAddress));
      DEBUG ((DEBUG_INFO, "MmCoreImageSize  - 0x%016lx\n", ImageContext.ImageSize));

      //
      // Flush the instruction cache so the image data are written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      //
      // Create HOB list for Standalone MM Core.
      //
      MmHobSize = 0;
      MmHobList = CreateMmHobList (
                    &MmHobSize,
                    MmCommBuffer,
                    MmFvBase,
                    MmFvSize,
                    &MmCoreFileName,
                    ImageContext.DestinationAddress != 0 ? ImageContext.DestinationAddress : ImageContext.ImageAddress,
                    EFI_PAGES_TO_SIZE (PageCount),
                    ImageContext.EntryPoint,
                    Block
                    );

      //
      // Print debug message showing Standalone MM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "StandaloneMM IPL calling Standalone MM Core at MMRAM address - 0x%016lx\n", ImageContext.EntryPoint));

      //
      // Execute image
      //
      Status2 = PlatformHookCallMmCore (&ImageContext, MmHobList, &Status);
      if (Status2 == EFI_UNSUPPORTED) {
        Entry  = (STANDALONE_MM_FOUNDATION_ENTRY_POINT)(UINTN)ImageContext.EntryPoint;
        Status = Entry (MmHobList);
      }

      ASSERT_EFI_ERROR (Status);

      FreePages (MmHobList, EFI_SIZE_TO_PAGES (MmHobSize));
      FreePool (Block);
    }
  }

Done:
  Status = MmAccessClose ();

  return Status;
}

/**
  Dispatch StandaloneMm drivers in MM.

  StandaloneMm core will exit when MmEntryPoint was registered in CPU
  StandaloneMm driver, and issue a software SMI by communicate mode to
  dispatch other StandaloneMm drivers.

  @retval  EFI_SUCCESS      Dispatch StandaloneMm drivers successfully.
  @retval  Other            Dispatch StandaloneMm drivers failed.

**/
EFI_STATUS
MmIplDispatchMmDrivers (
  VOID
  )
{
  EFI_STATUS                 Status;
  UINTN                      Size;
  EFI_MM_COMMUNICATE_HEADER  CommunicateHeader;

  //
  // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&CommunicateHeader.HeaderGuid, &gEventMmDispatchGuid);
  CommunicateHeader.MessageLength = 1;
  CommunicateHeader.Data[0]       = 0;

  //
  // Generate the Software SMI and return the result
  //
  Size   = sizeof (CommunicateHeader);
  Status = Communicate (NULL, &CommunicateHeader, &Size);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
