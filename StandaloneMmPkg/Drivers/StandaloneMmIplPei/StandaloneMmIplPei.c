/** @file
  MM IPL that load the MM Core into MMRAM at PEI stage

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmIplPei.h"

/**
  Search all the available firmware volumes for MM Core driver.

  @param  MmFvBase             Base address of FV which included MM Core driver.
  @param  MmFvSize             Size of FV which included MM Core driver.
  @param  MmCoreFileName       GUID of MM Core.
  @param  MmCoreImageAddress   MM Core image address.

  @retval EFI_SUCCESS          The specified FFS section was returned.
  @retval EFI_NOT_FOUND        The specified FFS section could not be found.

**/
EFI_STATUS
LocateMmCoreFv (
  OUT EFI_PHYSICAL_ADDRESS  *MmFvBase,
  OUT UINTN                 *MmFvSize,
  OUT EFI_GUID              *MmCoreFileName,
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
    // Search MM Core FFS
    //
    FileHandle = NULL;
    Status     = PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_MM_CORE_STANDALONE, VolumeHandle, &FileHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    ASSERT (FileHandle != NULL);
    if (FileHandle != NULL) {
      CopyGuid (MmCoreFileName, &((EFI_FFS_FILE_HEADER *)FileHandle)->Name);
    }

    //
    // Search Section
    //
    Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, MmCoreImageAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Get MM Core section data.
    //
    SectionData = (EFI_PE32_SECTION *)((UINT8 *)*MmCoreImageAddress - sizeof (EFI_PE32_SECTION));
    ASSERT (SectionData->Type == EFI_SECTION_PE32);

    //
    // This is the FV that contains MM Core.
    //
    Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
    if (!EFI_ERROR (Status)) {
      *MmFvBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      *MmFvSize = VolumeInfo.FvSize;
      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
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
  @param[out] NewBlock                  Pointer of new mmram block HOB.

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
  NewDescriptorBlock  = (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK *)BuildGuidHob (
                                                            &gEfiSmmSmramMemoryGuid,
                                                            sizeof (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK) + ((FullMmramRangeCount - 1) * sizeof (EFI_MMRAM_DESCRIPTOR))
                                                            );
  ASSERT (NewDescriptorBlock != NULL);

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
  // Scrub old one
  //
  ZeroMem (&MmramInfoHob->Name, sizeof (MmramInfoHob->Name));

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
  UINTN                                 PageCount;
  VOID                                  *MmHobList;
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
  // Unblock the MM FV range to be accessible from inside MM
  //
  if ((MmFvBase != 0) && (MmFvSize != 0)) {
    Status = MmUnblockMemoryRequest (MmFvBase, EFI_SIZE_TO_PAGES (MmFvSize));
    ASSERT_EFI_ERROR (Status);
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
    return Status;
  }

  PageCount = (UINTN)EFI_SIZE_TO_PAGES ((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

  //
  // Allocate memory for the image being loaded from unallocated mmram range
  //
  ImageContext.ImageAddress = MmIplAllocateMmramPage (PageCount, &Block);
  if (ImageContext.ImageAddress == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);

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
      DEBUG ((DEBUG_INFO, "MmCoreEntryPoint - 0x%016lx\n", ImageContext.EntryPoint));

      //
      // Flush the instruction cache so the image data are written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      //
      // Get HOB list for Standalone MM Core.
      //
      MmHobList = GetHobList ();

      //
      // Print debug message showing Standalone MM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "StandaloneMM IPL calling Standalone MM Core at MMRAM address - 0x%016lx\n", ImageContext.EntryPoint));

      //
      // Execute image
      //
      Entry  = (STANDALONE_MM_FOUNDATION_ENTRY_POINT)(UINTN)ImageContext.EntryPoint;
      Status = Entry (MmHobList);
      ASSERT_EFI_ERROR (Status);
    }
  }

  return Status;
}

/**
  Build communication buffer HOB.

  @return  MM_COMM_BUFFER     Pointer of MM communication buffer

**/
MM_COMM_BUFFER *
MmIplBuildCommBufferHob (
  VOID
  )
{
  EFI_STATUS      Status;
  MM_COMM_BUFFER  *MmCommBuffer;
  UINT64          FixedCommBufferPages;

  FixedCommBufferPages = PcdGet32 (PcdFixedCommBufferPages);

  MmCommBuffer = BuildGuidHob (&gEdkiiCommunicationBufferGuid, sizeof (MM_COMM_BUFFER));
  ASSERT (MmCommBuffer != NULL);

  //
  // Set fixed communicate buffer size
  //
  MmCommBuffer->FixedCommBufferSize = FixedCommBufferPages * EFI_PAGE_SIZE;

  //
  // Allocate runtime memory for fixed communicate buffer
  //
  MmCommBuffer->FixedCommBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (FixedCommBufferPages);
  if (MmCommBuffer->FixedCommBuffer == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate fixed communication buffer\n"));
    ASSERT (MmCommBuffer->FixedCommBuffer != 0);
  }

  //
  // Build MM unblock memory region HOB for communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->FixedCommBuffer, FixedCommBufferPages);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate runtime memory for communication in and out parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBuffer->CommunicationInOut = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (COMMUNICATION_IN_OUT)));
  if (MmCommBuffer->CommunicationInOut == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate communication in/out buffer\n"));
    ASSERT (MmCommBuffer->CommunicationInOut != 0);
  }

  //
  // Build MM unblock memory region HOB for communication in/out buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->CommunicationInOut, EFI_SIZE_TO_PAGES (sizeof (COMMUNICATION_IN_OUT)));
  ASSERT_EFI_ERROR (Status);

  return MmCommBuffer;
}

/**
  The Entry Point for MM IPL at PEI stage.

  Load MM Core into MMRAM.

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
  EFI_STATUS      Status;
  MM_COMM_BUFFER  *MmCommBuffer;

  //
  // Build communication buffer HOB.
  //
  MmCommBuffer = MmIplBuildCommBufferHob ();
  ASSERT (MmCommBuffer != NULL);

  //
  // Locate and execute Mm Core to dispatch MM drivers.
  //
  Status = ExecuteMmCoreFromMmram (MmCommBuffer);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
