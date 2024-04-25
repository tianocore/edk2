/** @file
  MM IPL that load the SMM Core into MMRAM at PEI stage

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <StandaloneMmIplPei.h>

EFI_PEI_MM_COMMUNICATION_PPI  mMmCommunicationPpi = { Communicate };

EFI_PEI_PPI_DESCRIPTOR  mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMmCommunicationPpiGuid,
  &mMmCommunicationPpi
};

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiEndOfPeiSignalPpiGuid,
  EndOfPeiCallback
};

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_MM_COMMUNICATION_PPI  *This,
  IN OUT VOID                            *CommBuffer,
  IN OUT UINTN                           *CommSize
  )
{
  EFI_STATUS                  Status;
  PEI_SMM_CONTROL_PPI         *SmmControl;
  UINT8                       SmiCommand;
  UINTN                       Size;
  EFI_SMM_COMMUNICATE_HEADER  *CommunicateHeader;
  UINTN                       TempCommSize;
  EFI_HOB_GUID_TYPE           *GuidHob;
  MM_COMM_BUFFER              *MmCommBuffer;
  COMMUNICATION_IN_OUT        *CommunicationInOut;

  DEBUG ((DEBUG_INFO, "StandaloneSmmIpl Communicate Enter\n"));

  GuidHob = GetFirstGuidHob (&gEdkiiCommunicationBufferGuid);
  if (GuidHob != NULL) {
    MmCommBuffer       = GET_GUID_HOB_DATA (GuidHob);
    CommunicationInOut = (COMMUNICATION_IN_OUT *)(UINTN)MmCommBuffer->CommunicationInOut;
  } else {
    DEBUG ((DEBUG_ERROR, "MmCommBuffer is not existed !!!\n"));
    ASSERT (GuidHob != NULL);
    return EFI_NOT_FOUND;
  }

  SmiCommand = 0;
  Size       = sizeof (SmiCommand);

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)CommBuffer;

  if (CommSize == NULL) {
    TempCommSize = OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + CommunicateHeader->MessageLength;
  } else {
    TempCommSize = *CommSize;
    //
    // CommSize must hold HeaderGuid and MessageLength
    //
    if (TempCommSize < OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (TempCommSize > MmCommBuffer->FixedCommBufferSize) {
    DEBUG ((DEBUG_ERROR, "Communicate buffer size is over MAX size\n"));
    ASSERT (TempCommSize > MmCommBuffer->FixedCommBufferSize);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)(UINTN)MmCommBuffer->FixedCommBuffer, CommBuffer, TempCommSize);
  CommunicationInOut->IsCommBufferValid = TRUE;

  //
  // Generate Software SMI
  //
  Status = PeiServicesLocatePpi (&gPeiSmmControlPpiGuid, 0, NULL, (VOID **)&SmmControl);
  ASSERT_EFI_ERROR (Status);

  Status = SmmControl->Trigger (
                         (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                         SmmControl,
                         (INT8 *)&SmiCommand,
                         &Size,
                         FALSE,
                         0
                         );
  ASSERT_EFI_ERROR (Status);

  //
  // Return status from software SMI
  //
  *CommSize = (UINTN)CommunicationInOut->ReturnBufferSize;

  //
  // Copy the returned data to the non-mmram buffer (CommBuffer)
  //
  CopyMem (CommBuffer, (VOID *)(MmCommBuffer->FixedCommBuffer), CommunicationInOut->ReturnBufferSize);

  Status = (EFI_STATUS)CommunicationInOut->ReturnStatus;
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "StandaloneSmmIpl Communicate failed (%r)\n", Status));
  } else {
    CommunicationInOut->IsCommBufferValid = FALSE;
    DEBUG ((DEBUG_INFO, "StandaloneSmmIpl Communicate Exit (%r)\n", Status));
  }

  return Status;
}

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
LocateMmFvForMmCore (
  OUT EFI_PHYSICAL_ADDRESS  *MmFvBase,
  OUT UINTN                 *MmFvSize,
  OUT EFI_GUID              **MmCoreFileName,
  OUT VOID                  **MmCoreImageAddress
  )
{
  EFI_STATUS           Status;
  UINTN                FvIndex;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_FFS_FILE_HEADER  *FileHandle;
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
      *MmFvBase       = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      *MmFvSize       = VolumeInfo.FvSize;
      *MmCoreFileName = &FileHandle->Name;
      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Create HOB list for Standalone MM core.

  @param[out]  HobSize              HOB size of fundation and platform HOB list.
  @param[in]   MmCommBuffer         Pointer of MM communiction buffer.
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
CreatMmHobList (
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
  EFI_STATUS  Status;
  VOID        *HobList;
  VOID        *PlatformHobList;
  UINTN       PlatformHobSize;
  UINTN       BufferSize;
  UINTN       FoundationHobSize;

  //
  // Get platform HOBs
  //
  PlatformHobSize = 0;
  Status          = CreateMmPlatformHob (NULL, &PlatformHobSize);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    ASSERT (PlatformHobSize != 0);
    //
    // Creat platform HOBs for MM foundation to get MMIO HOB data.
    //
    PlatformHobList = AllocatePages (EFI_SIZE_TO_PAGES (PlatformHobSize));
    ASSERT (PlatformHobList != NULL);
    if (PlatformHobList == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Out of resource to create platform MM HOBs\n", __func__));
      CpuDeadLoop ();
    }

    BufferSize = PlatformHobSize;
    Status     = CreateMmPlatformHob (PlatformHobList, &PlatformHobSize);
    ASSERT_EFI_ERROR (Status);
    ASSERT (BufferSize == PlatformHobSize);
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Get size of foundation HOBs
  //
  FoundationHobSize = 0;
  Status            = CreateMmFoundationHobList (
                        NULL,
                        &FoundationHobSize,
                        PlatformHobList,
                        PlatformHobSize,
                        MmCommBuffer,
                        MmFvBase,
                        MmFvSize,
                        MmCoreFileName,
                        MmCoreImageAddress,
                        MmCoreImageSize,
                        MmCoreEntryPoint,
                        Block
                        );
  FreePages (PlatformHobList, EFI_SIZE_TO_PAGES (PlatformHobSize));
  ASSERT (Status == RETURN_BUFFER_TOO_SMALL);
  ASSERT (FoundationHobSize != 0);

  //
  // Final result includes platform HOBs, foundation HOBs and a END node.
  //
  *HobSize = PlatformHobSize + FoundationHobSize + sizeof (EFI_HOB_GENERIC_HEADER);
  HobList  = AllocatePages (EFI_SIZE_TO_PAGES (*HobSize));
  ASSERT (HobList != NULL);
  if (HobList == NULL) {
    DEBUG ((DEBUG_ERROR, "Out of resource to create MM HOBs\n"));
    CpuDeadLoop ();
  }

  //
  // Get platform HOBs
  //
  Status = CreateMmPlatformHob (HobList, &PlatformHobSize);
  ASSERT_EFI_ERROR (Status);

  //
  // Get foundation HOBs
  //
  Status = CreateMmFoundationHobList (
             (UINT8 *)HobList + PlatformHobSize,
             &FoundationHobSize,
             PlatformHobList,
             PlatformHobSize,
             MmCommBuffer,
             MmFvBase,
             MmFvSize,
             MmCoreFileName,
             MmCoreImageAddress,
             MmCoreImageSize,
             MmCoreEntryPoint,
             Block
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Creat MM HOB list end.
  //
  MmIplCreateHob ((UINT8 *)HobList + PlatformHobSize + FoundationHobSize, EFI_HOB_TYPE_END_OF_HOB_LIST, sizeof (EFI_HOB_GENERIC_HEADER));

  return HobList;
}

/**
  Find largest unallocated MMRAM in current MMRAM descriptor block

  @param[in]  CurrentBlock         Current MMRAM descriptor block.

  @retval     Largest unallocated MMRAM index in current MMRAM descriptor block,
              Return 0 if no found largest unlocated MMRAM, it will return 0.
**/
UINTN
FindLargestSmramRange (
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *CurrentBlock
  )
{
  UINTN                 Index;
  UINT64                MaxSize;
  EFI_MMRAM_DESCRIPTOR  *MmramRanges;

  MmramRanges = CurrentBlock->Descriptor;

  //
  // Find largest Mmram range.
  //
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
          DEBUG ((DEBUG_INFO, "Found largest unlocated MMRAM\n"));
          return Index;
        }
      }
    }
  }

  DEBUG ((DEBUG_ERROR, "Not found largest unlocated MMRAM\n"));
  ASSERT (FALSE);
  return 0;
}

/**
  Allocate available MMRAM for MM core image.

  @param[in]  Pages                     Page size of MM core image.
  @param[out] NewBlock                  Pointer of new mmram block HOB.

  @return  EFI_PHYSICAL_ADDRESS         Address for MM core image to be loaded in MMRAM.
**/
EFI_PHYSICAL_ADDRESS
MmIplAllocateSmramPage (
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

  CurrentBlock = (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK *)(GET_GUID_HOB_DATA (MmramInfoHob));

  //
  // 1. Find largest unallocated MMRAM region
  //
  LagestMmramRangeIndex = FindLargestSmramRange (CurrentBlock);
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
  // Allocate Mmram for MmCore Driver
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
  UINTN                                 MmHobSize;
  EFI_GUID                              *MmCoreFileName;
  UINTN                                 MmFvSize;
  EFI_PHYSICAL_ADDRESS                  MmFvBase;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  STANDALONE_MM_FOUNDATION_ENTRY_POINT  Entry;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK        *Block;

  MmFvBase = 0;
  MmFvSize = 0;
  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type SMM_CORE.
  //
  Status = LocateMmFvForMmCore (&MmFvBase, &MmFvSize, &MmCoreFileName, &ImageContext.Handle);
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
  ImageContext.ImageAddress = MmIplAllocateSmramPage (PageCount, &Block);

  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);

  //
  // Print debug message showing SMM Core load address.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL loading MM Core at MMRAM address %p\n", (VOID *)(UINTN)ImageContext.ImageAddress));

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
      // Create MM HOB list for Standalone MM Core.
      //
      MmHobSize = 0;
      MmHobList = CreatMmHobList (
                    &MmHobSize,
                    MmCommBuffer,
                    MmFvBase,
                    MmFvSize,
                    MmCoreFileName,
                    ImageContext.ImageAddress,
                    ImageContext.ImageSize,
                    ImageContext.EntryPoint,
                    Block
                    );

      //
      // Print debug message showing Standalone MM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "SMM IPL calling Standalone MM Core at MMRAM address - 0x%016lx\n", ImageContext.EntryPoint));

      //
      // Execute image
      //
      Entry  = (STANDALONE_MM_FOUNDATION_ENTRY_POINT)(UINTN)ImageContext.EntryPoint;
      Status = Entry (MmHobList);
      ASSERT_EFI_ERROR (Status);
      FreePages (MmHobList, EFI_SIZE_TO_PAGES (MmHobSize));
    }
  }

  return Status;
}

/**
  This is the callback function on end of PEI.

  This callback is used for call MmEndOfPeiHandler in standalone MM core.

  @param   PeiServices       General purpose services available to every PEIM.
  @param   NotifyDescriptor  The notification structure this PEIM registered on install.
  @param   Ppi               Pointer to the PPI data associated with this function.

  @retval  EFI_SUCCESS       Exit boot services successfully.
  @retval  Other             Exit boot services failed.
**/
EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN  VOID                       *Ppi
  )
{
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                       Size;
  EFI_STATUS                  Status;

  //
  // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&CommunicateHeader.HeaderGuid, &gEfiMmEndOfPeiProtocol);
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

/**
  Dispatch StandaloneMm drivers in MM.

  StandaloneMm core will exit when MmEntryPoint was registered in CPU
  StandaloneMm driver, and issue a software SMI by communicate mode to
  dispatch other StandaloneMm drivers.

  @retval  EFI_SUCCESS    Dispatch StandaloneMm drivers successfully.
  @retval  Other          Dispatch StandaloneMm drivers failed.

**/
EFI_STATUS
MmIplDispatchMmDrivers (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       Size;
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;

  //
  // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&CommunicateHeader.HeaderGuid, &gEdkiiEventMmDispatchGuid);
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

  MmCommBuffer = BuildGuidHob (&gEdkiiCommunicationBufferGuid, sizeof (MM_COMM_BUFFER));
  ASSERT (MmCommBuffer != NULL);

  //
  // Set fixed communicate buffer size
  //
  MmCommBuffer->FixedCommBufferSize = PcdGet32 (PcdFixedCommBufferPages) * EFI_PAGE_SIZE;

  //
  // Allocate runtime memory for fixed communicate buffer
  //
  MmCommBuffer->FixedCommBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (PcdGet32 (PcdFixedCommBufferPages));
  if (MmCommBuffer->FixedCommBuffer == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate fixed communication buffer\n"));
    ASSERT (MmCommBuffer->FixedCommBuffer != 0);
  }

  //
  // Build MM unblock memory region HOB for communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->FixedCommBuffer, PcdGet32 (PcdFixedCommBufferPages));
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

  //
  // Install MmCommunicationPpi
  //
  Status = PeiServicesInstallPpi (&mPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Create exit boot services callback to call MmExitBootServiceHandler
  //
  Status = PeiServicesNotifyPpi (&mNotifyList);
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch StandaloneMm drivers in MM
  //
  Status = MmIplDispatchMmDrivers ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
