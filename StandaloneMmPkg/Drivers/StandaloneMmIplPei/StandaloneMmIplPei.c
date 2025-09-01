/** @file
  MM IPL that load the MM Core into MMRAM at PEI stage

  Copyright (c) 2024 - 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmIplPei.h"

EFI_PEI_MM_COMMUNICATION_PPI   mMmCommunicationPpi  = { Communicate };
EFI_PEI_MM_COMMUNICATION3_PPI  mMmCommunication3Ppi = { Communicate3 };

EFI_PEI_PPI_DESCRIPTOR  mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiMmCommunicationPpiGuid,
    &mMmCommunicationPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMmCommunication3PpiGuid,
    &mMmCommunication3Ppi
  }
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
  EFI_STATUS              Status;
  EFI_PEI_MM_CONTROL_PPI  *MmControl;
  UINT8                   SmiCommand;
  UINTN                   Size;
  UINTN                   TempCommSize;
  EFI_HOB_GUID_TYPE       *GuidHob;
  MM_COMM_BUFFER          *MmCommBuffer;
  MM_COMM_BUFFER_STATUS   *MmCommBufferStatus;

  DEBUG ((DEBUG_INFO, "StandaloneMmIpl Communicate Enter\n"));

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  if (GuidHob != NULL) {
    MmCommBuffer       = GET_GUID_HOB_DATA (GuidHob);
    MmCommBufferStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)MmCommBuffer->Status;
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
  if ((CommBuffer == NULL) || (CommSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  } else {
    TempCommSize = *CommSize;
    //
    // CommSize must hold HeaderGuid and MessageLength
    //
    if (TempCommSize < OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (TempCommSize > EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)) {
    DEBUG ((DEBUG_ERROR, "Communicate buffer size (%d) is over MAX (%d) size!", TempCommSize, EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)));
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)(UINTN)MmCommBuffer->PhysicalStart, CommBuffer, TempCommSize);
  MmCommBufferStatus->IsCommBufferValid = TRUE;

  //
  // Generate Software SMI
  //
  Status = PeiServicesLocatePpi (&gEfiPeiMmControlPpiGuid, 0, NULL, (VOID **)&MmControl);
  ASSERT_EFI_ERROR (Status);

  Status = MmControl->Trigger (
                        (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                        MmControl,
                        (INT8 *)&SmiCommand,
                        &Size,
                        FALSE,
                        0
                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Return status from software SMI
  //
  *CommSize = (UINTN)MmCommBufferStatus->ReturnBufferSize;

  //
  // Copy the returned data to the non-mmram buffer (CommBuffer)
  //
  CopyMem (CommBuffer, (VOID *)(MmCommBuffer->PhysicalStart), *CommSize);

  Status = (EFI_STATUS)MmCommBufferStatus->ReturnStatus;
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "StandaloneMmIpl Communicate failed (%r)\n", Status));
  } else {
    MmCommBufferStatus->IsCommBufferValid = FALSE;
  }

  return Status;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATE3 instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
EFI_STATUS
EFIAPI
Communicate3 (
  IN CONST EFI_PEI_MM_COMMUNICATION3_PPI  *This,
  IN OUT VOID                             *CommBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_PEI_MM_CONTROL_PPI        *MmControl;
  UINT8                         SmiCommand;
  UINTN                         Size;
  UINTN                         TempCommSize;
  EFI_HOB_GUID_TYPE             *GuidHob;
  MM_COMM_BUFFER                *MmCommBuffer;
  MM_COMM_BUFFER_STATUS         *MmCommBufferStatus;
  EFI_MM_COMMUNICATE_HEADER_V3  *CommunicateHeader;

  DEBUG ((DEBUG_INFO, "StandaloneMmIpl Communicate Enter\n"));

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  if (GuidHob != NULL) {
    MmCommBuffer       = GET_GUID_HOB_DATA (GuidHob);
    MmCommBufferStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)MmCommBuffer->Status;
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
  } else {
    CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER_V3 *)CommBuffer;
    //
    // Check if the HeaderGuid is valid
    //
    if (CompareGuid (&CommunicateHeader->HeaderGuid, &gEfiMmCommunicateHeaderV3Guid)) {
      DEBUG ((DEBUG_ERROR, "HeaderGuid is not valid!\n"));
      return EFI_INVALID_PARAMETER;
    }

    TempCommSize = CommunicateHeader->BufferSize;
    //
    // CommSize must hold HeaderGuid and MessageLength
    //
    if (TempCommSize < sizeof (EFI_MM_COMMUNICATE_HEADER_V3)) {
      DEBUG ((DEBUG_ERROR, "Communicate buffer size (%d) is less than minimum size (%d)!", TempCommSize, sizeof (EFI_MM_COMMUNICATE_HEADER_V3)));
      return EFI_INVALID_PARAMETER;
    }
  }

  if (TempCommSize > EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)) {
    DEBUG ((DEBUG_ERROR, "Communicate buffer size (%d) is over MAX (%d) size!", TempCommSize, EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)));
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)(UINTN)MmCommBuffer->PhysicalStart, CommBuffer, TempCommSize);
  MmCommBufferStatus->IsCommBufferValid = TRUE;

  //
  // Generate Software SMI
  //
  Status = PeiServicesLocatePpi (&gEfiPeiMmControlPpiGuid, 0, NULL, (VOID **)&MmControl);
  ASSERT_EFI_ERROR (Status);

  Status = MmControl->Trigger (
                        (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                        MmControl,
                        (INT8 *)&SmiCommand,
                        &Size,
                        FALSE,
                        0
                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Return status from software SMI
  //
  TempCommSize = (UINTN)MmCommBufferStatus->ReturnBufferSize;

  //
  // Copy the returned data to the non-mmram buffer (CommBuffer)
  //
  CopyMem (CommBuffer, (VOID *)(MmCommBuffer->PhysicalStart), TempCommSize);

  CommunicateHeader->BufferSize = TempCommSize;

  Status = (EFI_STATUS)MmCommBufferStatus->ReturnStatus;
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "StandaloneMmIpl Communicate failed (%r)\n", Status));
  } else {
    MmCommBufferStatus->IsCommBufferValid = FALSE;
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
LocateMmCoreFv (
  OUT EFI_PHYSICAL_ADDRESS  *MmFvBase,
  OUT UINTN                 *MmFvSize,
  OUT EFI_GUID              *MmCoreFileName,
  OUT VOID                  **MmCoreImageAddress
  )
{
  EFI_STATUS               Status;
  UINTN                    FvIndex;
  EFI_PEI_FV_HANDLE        VolumeHandle;
  EFI_PEI_FILE_HANDLE      FileHandle;
  EFI_PE32_SECTION         *SectionData;
  EFI_FV_INFO              VolumeInfo;
  MM_CORE_FV_LOCATION_PPI  *MmCoreFvLocation;

  //
  // The producer of the MmCoreFvLocation PPI is responsible for ensuring
  // that it reports the correct Firmware Volume (FV) containing the MmCore.
  // If the gMmCoreFvLocationPpiGuid is not found, the system will search
  // all Firmware Volumes (FVs) to locate the FV that contains the MM Core.
  //
  Status = PeiServicesLocatePpi (&gMmCoreFvLocationPpiGuid, 0, NULL, (VOID **)&MmCoreFvLocation);
  if (Status == EFI_SUCCESS) {
    *MmFvBase  = MmCoreFvLocation->Address;
    *MmFvSize  = MmCoreFvLocation->Size;
    FileHandle = NULL;
    Status     = PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_MM_CORE_STANDALONE, (VOID *)(UINTN)MmCoreFvLocation->Address, &FileHandle);
    ASSERT_EFI_ERROR (Status);
    if (Status == EFI_SUCCESS) {
      ASSERT (FileHandle != NULL);
      if (FileHandle != NULL) {
        CopyGuid (MmCoreFileName, &((EFI_FFS_FILE_HEADER *)FileHandle)->Name);
        //
        // Search Section
        //
        Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, MmCoreImageAddress);
        ASSERT_EFI_ERROR (Status);

        //
        // Get MM Core section data.
        //
        SectionData = (EFI_PE32_SECTION *)((UINT8 *)*MmCoreImageAddress - sizeof (EFI_PE32_SECTION));
        ASSERT (SectionData->Type == EFI_SECTION_PE32);
      }
    }

    return EFI_SUCCESS;
  }

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
  UINTN                                 MmHobSize;
  EFI_GUID                              MmCoreFileName;
  UINTN                                 MmFvSize;
  EFI_PHYSICAL_ADDRESS                  MmFvBase;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  STANDALONE_MM_FOUNDATION_ENTRY_POINT  Entry;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK        *Block;
  EFI_PEI_MM_ACCESS_PPI                 *MmAccess;
  UINTN                                 Size;
  UINTN                                 Index;
  UINTN                                 MmramRangeCount;

  MmFvBase = 0;
  MmFvSize = 0;
  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type MM_CORE_STANDALONE.
  //
  Status = LocateMmCoreFv (&MmFvBase, &MmFvSize, &MmCoreFileName, &ImageContext.Handle);
  ASSERT_EFI_ERROR (Status);

  //
  // Prepare an MM access PPI for MM RAM.
  //
  MmAccess        = NULL;
  MmramRangeCount = 0;
  Status          = PeiServicesLocatePpi (
                      &gEfiPeiMmAccessPpiGuid,
                      0,
                      NULL,
                      (VOID **)&MmAccess
                      );
  if (!EFI_ERROR (Status)) {
    //
    // Open all MMRAM ranges, if MmAccess is available.
    //
    Size   = 0;
    Status = MmAccess->GetCapabilities ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), MmAccess, &Size, NULL);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      // This is not right...
      ASSERT (Status == EFI_BUFFER_TOO_SMALL);
      return EFI_DEVICE_ERROR;
    }

    MmramRangeCount = Size / sizeof (EFI_MMRAM_DESCRIPTOR);
    for (Index = 0; Index < MmramRangeCount; Index++) {
      Status = MmAccess->Open ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), MmAccess, Index);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "MM IPL failed to open MMRAM windows index %d - %r\n", Index, Status));
        ASSERT_EFI_ERROR (Status);
        goto Done;
      }
    }
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
                    ImageContext.ImageAddress,
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
      Entry  = (STANDALONE_MM_FOUNDATION_ENTRY_POINT)(UINTN)ImageContext.EntryPoint;
      Status = Entry (MmHobList);
      ASSERT_EFI_ERROR (Status);
      FreePages (MmHobList, EFI_SIZE_TO_PAGES (MmHobSize));
    }
  }

Done:
  if (MmAccess != NULL) {
    //
    // Close all MMRAM ranges, if MmAccess is available.
    //
    for (Index = 0; Index < MmramRangeCount; Index++) {
      Status = MmAccess->Close ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), MmAccess, Index);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "MM IPL failed to close MMRAM windows index %d - %r\n", Index, Status));
        ASSERT (FALSE);
        return Status;
      }

      //
      // Print debug message that the MMRAM window is now closed.
      //
      DEBUG ((DEBUG_INFO, "MM IPL closed MMRAM window index %d\n", Index));

      //
      // Lock the MMRAM (Note: Locking MMRAM may not be supported on all platforms)
      //
      Status = MmAccess->Lock ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), MmAccess, Index);
      if (EFI_ERROR (Status)) {
        //
        // Print error message that the MMRAM failed to lock...
        //
        DEBUG ((DEBUG_ERROR, "MM IPL could not lock MMRAM (Index %d) after executing MM Core %r\n", Index, Status));
        ASSERT (FALSE);
        return Status;
      }

      //
      // Print debug message that the MMRAM window is now closed.
      //
      DEBUG ((DEBUG_INFO, "MM IPL locked MMRAM window index %d\n", Index));
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
  EFI_MM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                      Size;
  EFI_STATUS                 Status;

  //
  // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
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
  UINT64          MmCommBufferPages;

  MmCommBufferPages = PcdGet32 (PcdMmCommBufferPages);

  MmCommBuffer = BuildGuidHob (&gMmCommBufferHobGuid, sizeof (MM_COMM_BUFFER));
  ASSERT (MmCommBuffer != NULL);

  //
  // Set MM communicate buffer size
  //
  MmCommBuffer->NumberOfPages = MmCommBufferPages;

  //
  // Allocate runtime memory for MM communicate buffer
  //
  MmCommBuffer->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (MmCommBufferPages);
  if (MmCommBuffer->PhysicalStart == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate MM communication buffer\n"));
    ASSERT (MmCommBuffer->PhysicalStart != 0);
  }

  //
  // Build MM unblock memory region HOB for MM communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->PhysicalStart, MmCommBufferPages);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate runtime memory for MM communication status parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBuffer->Status = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (MM_COMM_BUFFER_STATUS)));
  if (MmCommBuffer->Status == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for MM communication status\n"));
    ASSERT (MmCommBuffer->Status != 0);
  }

  //
  // Build MM unblock memory region HOB for MM communication status
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->Status, EFI_SIZE_TO_PAGES (sizeof (MM_COMM_BUFFER_STATUS)));
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
  Status = PeiServicesInstallPpi (mPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Create end of pei callback to call MmEndOfPeiHandler
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
