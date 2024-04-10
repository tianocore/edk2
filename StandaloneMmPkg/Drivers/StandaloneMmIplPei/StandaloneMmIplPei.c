/** @file
  MM IPL that load the SMM Core into MMRAM at PEI stage

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiSmm.h>
#include <StandaloneMm.h>
#include <Ppi/SmmControl.h>
#include <Ppi/MmCommunication.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/MmPlatformHobProducerLib.h>
#include <Library/MmUnblockMemoryLib.h>
#include <Protocol/SmmCommunication.h>
#include <Guid/MmCommBuffer.h>
#include <Guid/MmramMemoryReserve.h>
#include <StandaloneMmIplPei.h>

//
// SMM IPL global variables
//
EFI_PHYSICAL_ADDRESS  mMmramCacheBase;
UINT64                mMmramCacheSize;
EFI_PHYSICAL_ADDRESS  mMmramRanges = 0;
UINT64                mMmramRangeCount = 0;
EFI_PHYSICAL_ADDRESS  mMmCoreImageAddress;
UINT64                mMmCoreImageSize;
EFI_PHYSICAL_ADDRESS  mMmCoreEntryPoint;
EFI_PHYSICAL_ADDRESS  mMmFvBaseAddress;
UINT64                mMmFvSize;
EFI_GUID              *mMmCoreFileName;
VOID                  *mPlatformHobList;
UINTN                 mPlatformHobSize;

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
  );

EFI_PEI_MM_COMMUNICATION_PPI  mMmCommunicationPpi = { Communicate };

EFI_PEI_PPI_DESCRIPTOR  mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMmCommunicationPpiGuid,
  &mMmCommunicationPpi
};

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
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiEndOfPeiSignalPpiGuid,
    EndOfPeiCallback
  }
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
  MM_COMM_BUFFER_DATA         *MmCommBufferData;
  COMMUNICATION_IN_OUT        *CommunicationInOut;

  DEBUG ((DEBUG_INFO, "StandaloneSmmIpl Communicate Enter\n"));

  GuidHob = GetFirstGuidHob (&gEdkiiCommunicationBufferGuid);
  if (GuidHob != NULL) {
    MmCommBufferData   = GET_GUID_HOB_DATA (GuidHob);
    CommunicationInOut = (COMMUNICATION_IN_OUT *)(UINTN)MmCommBufferData->CommunicationInOut;
  } else {
    DEBUG ((DEBUG_ERROR, "MmCommBufferData is not existed !!!\n"));
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

  if (TempCommSize > MmCommBufferData->FixedCommBufferSize) {
    DEBUG ((DEBUG_ERROR, "Communicate buffer size is over MAX size\n"));
    ASSERT (TempCommSize > MmCommBufferData->FixedCommBufferSize);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)(UINTN)MmCommBufferData->FixedCommBuffer, CommBuffer, TempCommSize);
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
  CopyMem (CommBuffer, (VOID *)(MmCommBufferData->FixedCommBuffer), CommunicationInOut->ReturnBufferSize);

  Status    = (EFI_STATUS)CommunicationInOut->ReturnStatus;
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "StandaloneSmmIpl Communicate failed (%r)\n", Status));
  } else {
    CommunicationInOut->IsCommBufferValid = FALSE;
    DEBUG ((DEBUG_INFO, "StandaloneSmmIpl Communicate Exit (%r)\n", Status));
  }
  return Status;
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
  DEBUG ((DEBUG_INFO, "Context1- 0x%016lx\n", Context1));
  DEBUG ((DEBUG_INFO, "EntryPoint- 0x%016lx\n", EntryPoint));
  return EntryPoint (Context1);
}

/**
  Search all the available firmware volumes for SMM Core driver.

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
      *MmFvBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      mMmFvBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      mMmFvSize        = VolumeInfo.FvSize;
      mMmCoreFileName  = &FileHandle->Name;
      MmUnblockMemoryRequest (mMmFvBaseAddress, EFI_SIZE_TO_PAGES (mMmFvSize));

      return EFI_SUCCESS;
    } else {

      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Create HOB list for Standalone MM core.

  @param  HobSize              HOB size of fundation and platform HOB list.

  @retval HobList              If fundation and platform HOBs not existed,
                               it is pointed to PEI HOB List. If existed, 
                               it is pointed to fundation and platform HOB list.

**/
VOID *
CreatMmHobList (
  OUT UINTN  *HobSize
)
{
  UINTN                         FoundationHobSize;
  EFI_PHYSICAL_ADDRESS          FoundationHobEndAddress;
  EFI_HOB_HANDOFF_INFO_TABLE    *HobList;
  EFI_STATUS                     Status;

  //
  // Get platform HOBs' size.
  //
  mPlatformHobSize = 0;
  Status     = CreateMmPlatformHob (NULL, &mPlatformHobSize);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    ASSERT (mPlatformHobSize != 0);
  }

  //
  // Creat platform HOBs for MM foundation to get MMIO HOB data.
  //
  mPlatformHobList = NULL;
  mPlatformHobList = AllocatePages (EFI_SIZE_TO_PAGES (mPlatformHobSize + sizeof (EFI_HOB_GENERIC_HEADER)));
  if (mPlatformHobList != NULL) {
    ZeroMem (mPlatformHobList, mPlatformHobSize + sizeof (EFI_HOB_GENERIC_HEADER));
    Status = CreateMmPlatformHob (mPlatformHobList, &mPlatformHobSize);
    CreateEndOfList ((UINTN)mPlatformHobList + mPlatformHobSize);
    if (Status != EFI_SUCCESS) {
      ASSERT (FALSE);
    }
  }

  //
  // Get foundation HOBs' size.
  //
  FoundationHobSize = 0;
  Status = CreateMmFoundationHobList (NULL, &FoundationHobSize, mPlatformHobList, mPlatformHobSize);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    ASSERT (FoundationHobSize != 0);
  }

  //
  // If there is no any foundation and platform HOBs, return current PEI HOB list.
  //
  *HobSize = FoundationHobSize + mPlatformHobSize;
  if (*HobSize == 0) {
    HobList = GetHobList ();
    return HobList;
  }

  *HobSize += sizeof (EFI_HOB_HANDOFF_INFO_TABLE) + sizeof (EFI_HOB_GENERIC_HEADER);
  HobList = AllocatePages (EFI_SIZE_TO_PAGES (*HobSize));

  if (HobList != NULL){

    ZeroMem (HobList, *HobSize);
    //
    // Creat MM HOB list header.
    //
    MmIplHobConstructor ((EFI_PHYSICAL_ADDRESS)(UINTN)HobList, *HobSize);
    //
    // Creat MM foundation HOB list.
    //
    if (FoundationHobSize != 0) {
      CreateMmFoundationHobList (HobList, &FoundationHobSize, mPlatformHobList, mPlatformHobSize);
    }

    FoundationHobEndAddress = HobList->EfiEndOfHobList;
    //
    // Creat MM platform HOB list.
    //
    if (mPlatformHobSize != 0) {
      Status = CreateMmPlatformHob ((VOID *)(UINTN)FoundationHobEndAddress, &mPlatformHobSize);
    }
    //
    // Creat MM HOB list end.
    //
    CreateEndOfList (FoundationHobEndAddress + mPlatformHobSize);
  }

  return HobList;
}

/**
  Load the MM Core image into MMRAM and executes the MM Core from MMRAM.

  @param[in, out] MmramRange            Descriptor for the range of MMRAM to reload the
                                        currently executing image, the rang of SMRAM to
                                        hold SMM Core will be excluded.
  @param[in, out] MmramRangeSmmCore     Descriptor for the range of MMRAM to hold MM Core.

  @return  EFI_STATUS

**/
EFI_STATUS
ExecuteMmCoreFromMmram (
  IN OUT EFI_MMRAM_DESCRIPTOR  *MmramRange,
  IN OUT EFI_MMRAM_DESCRIPTOR  *MmramRangeSmmCore
  )
{
  EFI_STATUS                    Status;
  VOID                          *SourceBuffer;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                         PageCount;
  VOID                          *HobList;
  VOID                          *MmHobList;
  UINTN                         MmHobSize;
  EFI_PHYSICAL_ADDRESS          SourceFvBaseAddress;

  Status = PeiServicesGetHobList (&HobList);
  ASSERT_EFI_ERROR (Status);

  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type SMM_CORE.
  //
  Status = LocateMmFvForMmCore (&SourceFvBaseAddress, &SourceBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
  // specified by MmramRange
  //
  PageCount = (UINTN)EFI_SIZE_TO_PAGES ((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

  ASSERT ((MmramRange->PhysicalSize & EFI_PAGE_MASK) == 0);
  ASSERT (MmramRange->PhysicalSize > EFI_PAGES_TO_SIZE (PageCount));

  MmramRange->PhysicalSize        -= EFI_PAGES_TO_SIZE (PageCount);
  MmramRangeSmmCore->CpuStart      = MmramRange->CpuStart + MmramRange->PhysicalSize;
  MmramRangeSmmCore->PhysicalStart = MmramRange->PhysicalStart + MmramRange->PhysicalSize;
  MmramRangeSmmCore->RegionState   = MmramRange->RegionState | EFI_ALLOCATED;
  MmramRangeSmmCore->PhysicalSize  = EFI_PAGES_TO_SIZE (PageCount);

  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress = MmramRangeSmmCore->CpuStart;

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
      mMmCoreImageAddress = ImageContext.ImageAddress;
      mMmCoreImageSize    = ImageContext.ImageSize;
      mMmCoreEntryPoint   = ImageContext.EntryPoint;
      DEBUG ((DEBUG_INFO, "MmCoreImageBase  - 0x%016lx\n", mMmCoreImageAddress));
      DEBUG ((DEBUG_INFO, "MmCoreImageSize  - 0x%016lx\n", mMmCoreImageSize));
      DEBUG ((DEBUG_INFO, "MmCoreEntryPoint - 0x%016lx\n", mMmCoreEntryPoint));

      //
      // Flush the instruction cache so the image data are written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      //
      // Initialize mMmCoreEntryPoint for build memory allocation module HOB
      //
      mMmCoreEntryPoint = ImageContext.EntryPoint;

      //
      // Create MM HOB list for Standalone MM Core.
      //
      MmHobSize = 0;
      MmHobList = CreatMmHobList (&MmHobSize);

      //
      // Print debug message showing Standalone MM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "SMM IPL calling Standalone MM Core at MMRAM address - 0x%016lx\n", ImageContext.EntryPoint));

      //
      // Execute image
      //
      LoadSmmCore (ImageContext.EntryPoint, MmHobList);
    }
  }

  //
  // If the load operation, relocate operation, or the image execution return an
  // error, then free memory allocated from the EFI_SRAM_DESCRIPTOR specified by
  // MmramRange
  //
  if (EFI_ERROR (Status)) {
    MmramRange->PhysicalSize += EFI_PAGES_TO_SIZE (PageCount);
  }

  //
  // Always free memory allocated buffer for MM IPL Hobs
  //
  if ((MmHobList != NULL) && (MmHobSize != 0)) {
    FreePages (MmHobList, EFI_SIZE_TO_PAGES (MmHobSize));
  }

  //
  // Always free memory allocated buffer for platform Hobs
  //
  if ((mPlatformHobList != NULL) && (mPlatformHobSize != 0)) {
    FreePages (mPlatformHobList, EFI_SIZE_TO_PAGES (mPlatformHobSize));
  }

  //
  // Always free memory allocated by GetFileBufferByFilePath ()
  //
  FreePool (SourceBuffer);

  return Status;
}

/**
  Get full MMRAM ranges.

  It will get MMRAM ranges from SmmAccess PPI. It will also reserve one entry
  for SMM core.

  @param[out] FullMmramRangeCount   Output pointer to full MMRAM range count.

  @return Pointer to full MMRAM ranges.

**/
EFI_MMRAM_DESCRIPTOR *
GetFullMmramRanges (
  OUT  UINTN  *FullMmramRangeCount
  )
{
  UINTN                           Size;
  UINTN                           Index;
  UINTN                           MmramRangeCount;
  UINTN                           AdditionMmramRangeCount;
  EFI_MMRAM_DESCRIPTOR            *FullMmramRanges;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *NewDescriptorBlock;
  EFI_HOB_GUID_TYPE               *GuidHob;

  //
  // Get MMRAM information.
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "MmramMemoryReserve HOB not found\n"));
  }

  DescriptorBlock = (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK *)(GET_GUID_HOB_DATA (GuidHob));
  MmramRangeCount = DescriptorBlock->NumberOfMmReservedRegions;

  //
  // Reserve one entry MM Core in the full MMRAM ranges.
  //
  AdditionMmramRangeCount = 1;

  *FullMmramRangeCount = MmramRangeCount + AdditionMmramRangeCount;
  Size                 = sizeof (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK) + ((*FullMmramRangeCount - 1) * sizeof (EFI_MMRAM_DESCRIPTOR));
  NewDescriptorBlock   = (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK *)BuildGuidHob (
                                                             &gEfiSmmSmramMemoryGuid,
                                                             Size
                                                             );
  ASSERT (NewDescriptorBlock != NULL);
  if (NewDescriptorBlock == NULL) {
    return NULL;
  }

  NewDescriptorBlock->NumberOfMmReservedRegions = (UINT32)*FullMmramRangeCount;
  FullMmramRanges                                = NewDescriptorBlock->Descriptor;

  //
  // Get MMRAM descriptors and fill to the full MMRAM ranges
  //
  for (Index = 0; Index < MmramRangeCount; Index++) {
    FullMmramRanges[Index].PhysicalStart = DescriptorBlock->Descriptor[Index].PhysicalStart;
    FullMmramRanges[Index].CpuStart      = DescriptorBlock->Descriptor[Index].CpuStart;
    FullMmramRanges[Index].PhysicalSize  = DescriptorBlock->Descriptor[Index].PhysicalSize;
    FullMmramRanges[Index].RegionState   = DescriptorBlock->Descriptor[Index].RegionState;
  }

  //
  // Scrub old one
  //
  ZeroMem (&GuidHob->Name, sizeof (GuidHob->Name));

  return FullMmramRanges;
}

/**
  Locate the MM Core image into MMRAM and executes from MMRAM to dispatch MM drivers.

  @retval  EFI_STATUS        Locate and execute MM core successfully.
  @retval  Other             Locate and execute MM core failed.

**/
EFI_STATUS
LoadMmCoreDispatchMmDrivers (
  VOID
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINT64                MaxSize;
  EFI_MMRAM_DESCRIPTOR  *MmramRanges;
  EFI_MMRAM_DESCRIPTOR  *CurrentMmramRange;

  //
  // Get MMRAM information
  //
  mMmramRanges = (EFI_PHYSICAL_ADDRESS)(UINTN)GetFullMmramRanges ((UINTN *)&mMmramRangeCount);
  ASSERT (mMmramRanges != 0);
  if (mMmramRanges == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Find the largest MMRAM range between 1MB and 4GB that is at least 256KB - 4K in size
  //
  CurrentMmramRange = NULL;
  MmramRanges        = (EFI_MMRAM_DESCRIPTOR *)(UINTN)mMmramRanges;
  if (MmramRanges == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to retrieve MmramRanges\n"));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < mMmramRangeCount; Index++) {
    //
    // Skip any MMRAM region that is already allocated, needs testing, or needs ECC initialization
    //
    if ((MmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    if (MmramRanges[Index].CpuStart >= BASE_1MB) {
      if ((MmramRanges[Index].CpuStart + MmramRanges[Index].PhysicalSize) <= BASE_4GB) {
        if (MmramRanges[Index].PhysicalSize >= MaxSize) {
          MaxSize            = MmramRanges[Index].PhysicalSize;
          CurrentMmramRange = &MmramRanges[Index];
        }
      }
    }
  }

  if (CurrentMmramRange != NULL) {
    //
    // Print debug message showing MMRAM window that will be used by SMM IPL and SMM Core
    //
    DEBUG ((
      DEBUG_INFO,
      "SMM IPL found MMRAM window %p - %p\n",
      (VOID *)(UINTN)CurrentMmramRange->CpuStart,
      (VOID *)(UINTN)(CurrentMmramRange->CpuStart + CurrentMmramRange->PhysicalSize - 1)
      ));

    //
    // Load MM Core into MMRAM and execute it from MMRAM
    // Note: MmramRanges specific for MM Core will put in the mMmramRangeCount - 1.
    //
    Status = ExecuteMmCoreFromMmram (
               CurrentMmramRange,
               &(((EFI_MMRAM_DESCRIPTOR *)(UINTN)mMmramRanges)[mMmramRangeCount - 1])
               );
    if (EFI_ERROR (Status)) {
      //
      // Print error message that the SMM Core failed to be loaded and executed.
      //
      DEBUG ((DEBUG_ERROR, "SMM IPL could not load and execute MM Core from MMRAM\n"));
    }
  } else {
    //
    // Print error message that there are not enough MMRAM resources to load the MM Core.
    //
    DEBUG ((DEBUG_ERROR, "MM IPL could not find a large enough MMRAM region to load MM Core\n"));
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

**/
VOID
SmmIplDispatchDriver (
  VOID
  )
{
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                       Size;

  while (TRUE) {
    //
    // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
    //
    CopyGuid (&CommunicateHeader.HeaderGuid, &gEdkiiEventMmDispatchGuid);
    CommunicateHeader.MessageLength = 1;
    CommunicateHeader.Data[0]       = 0;

    //
    // Generate the Software SMI and return the result
    //
    Size = sizeof (CommunicateHeader);
    Communicate (NULL, &CommunicateHeader, &Size);

    //
    // Return if there is no request to restart the SMM Core Dispatcher
    //
    if (CommunicateHeader.Data[0] != COMM_BUFFER_MM_DISPATCH_RESTART) {
      break;
    }
  }
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
  EFI_STATUS               Status;
  MM_COMM_BUFFER_DATA      MmCommBufferData;

  //
  // Initialize MM communication buffer data
  //
  ZeroMem (&MmCommBufferData, sizeof (MM_COMM_BUFFER_DATA));

  //
  // Set fixed communicate buffer size
  //
  MmCommBufferData.FixedCommBufferSize = PcdGet32 (PcdFixedCommBufferPages) * EFI_PAGE_SIZE;

  //
  // Allocate runtime memory for fixed communicate buffer
  //
  MmCommBufferData.FixedCommBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (PcdGet32 (PcdFixedCommBufferPages));
  if (MmCommBufferData.FixedCommBuffer == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate fixed communication buffer\n"));
    ASSERT (MmCommBufferData.FixedCommBuffer != 0);
    return EFI_OUT_OF_RESOURCES;
  } 

  //
  // Build MM unblock memory region HOB for communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBufferData.FixedCommBuffer, PcdGet32 (PcdFixedCommBufferPages));

  //
  // Allocate runtime memory for communication in and out parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBufferData.CommunicationInOut = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (COMMUNICATION_IN_OUT)));
  if (MmCommBufferData.CommunicationInOut == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate communication in/out buffer\n"));
    ASSERT (MmCommBufferData.CommunicationInOut != 0);
    return EFI_OUT_OF_RESOURCES;
  } 

  //
  // Build MM unblock memory region HOB for communication in/out buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBufferData.CommunicationInOut, EFI_SIZE_TO_PAGES (sizeof (COMMUNICATION_IN_OUT)));

  //
  // Build communication buffer Hob for SMM and DXE phase
  //
  BuildGuidDataHob (
    &gEdkiiCommunicationBufferGuid,
    &MmCommBufferData,
    sizeof (MM_COMM_BUFFER_DATA)
    );

  //
  // Locate and execute Mm Core to dispatch MM drivers.
  //
  Status = LoadMmCoreDispatchMmDrivers ();
  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Fail to load and execute MM Core\n"));
    return Status;
  }

  //
  // Install MmCommunicationPpi
  //
  Status = PeiServicesInstallPpi (&mPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Create exit boot services callback to call MmExitBootServiceHandler
  //
  Status = PeiServicesNotifyPpi (mNotifyList);
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch StandaloneMm drivers in MM
  //
  SmmIplDispatchDriver ();

  return EFI_SUCCESS;
}
