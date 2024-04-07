/** @file
  SMM IPL that load the SMM Core into SMRAM at PEI stage

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
#include <Guid/SmramMemoryReserve.h>
#include <StandaloneMmIplPei.h>

//
// SMM IPL global variables
//
EFI_SMRAM_DESCRIPTOR  *mCurrentSmramRange;
EFI_PHYSICAL_ADDRESS  mSmramCacheBase;
UINT64                mSmramCacheSize;
EFI_PHYSICAL_ADDRESS  mMmramRanges = 0;
UINT64                mMmramRangeCount = 0;
EFI_PHYSICAL_ADDRESS  mMmCoreImageAddress;
UINT64                mMmCoreImageSize;
EFI_PHYSICAL_ADDRESS  mMmCoreEntryPoint;
EFI_PHYSICAL_ADDRESS  mMmFvBaseAddress;
UINT64                mMmFvSize;
EFI_GUID              *mMmCoreFileName;
VOID                  *mMemMap = NULL;

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
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
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
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

  CommunicationInOut->IsCommBufferValid = TRUE;
  CopyMem ((EFI_PHYSICAL_ADDRESS *)(UINTN)MmCommBufferData->FixedCommBuffer, CommBuffer, TempCommSize);

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
     Status = Status | MAX_BIT;
  }

  CommunicationInOut->IsCommBufferValid = FALSE;
  DEBUG ((DEBUG_INFO, "StandaloneSmmIpl Communicate Exit (%r)\n", Status));

  return Status;
}

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

  SmramRanges = (EFI_SMRAM_DESCRIPTOR *)(UINTN)mMmramRanges;
  do {
    FoundAdjacentRange = FALSE;
    for (Index = 0; Index < mMmramRangeCount; Index++) {
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
  DEBUG ((DEBUG_INFO, "Context1- 0x%016lx\n", Context1));
  DEBUG ((DEBUG_INFO, "EntryPoint- 0x%016lx\n", EntryPoint));
  return EntryPoint (Context1);
}

/**
  Get the fixed loading address from image header assigned by build tool. This function only be called
  when Loading module at Fixed address feature enabled.

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that needs to be examined by this function.
  @retval EFI_SUCCESS               An fixed loading address is assigned to this image by build tools .
  @retval EFI_NOT_FOUND             The image has no assigned fixed loading address.
**/
EFI_STATUS
GetPeCoffImageFixLoadingAssignedAddress (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  UINTN                            SectionHeaderOffset;
  EFI_STATUS                       Status;
  EFI_IMAGE_SECTION_HEADER         SectionHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *ImgHdr;
  EFI_PHYSICAL_ADDRESS             FixLoadingAddress;
  UINT16                           Index;
  UINTN                            Size;
  UINT16                           NumberOfSections;
  EFI_PHYSICAL_ADDRESS             SmramBase;
  UINT64                           ValueInSectionHeader;

  FixLoadingAddress = 0;
  Status            = EFI_NOT_FOUND;
  SmramBase         = mCurrentSmramRange->CpuStart;
  //
  // Get PeHeader pointer
  //
  ImgHdr              = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((CHAR8 *)ImageContext->Handle + ImageContext->PeCoffHeaderOffset);
  SectionHeaderOffset = (UINTN)(
                                ImageContext->PeCoffHeaderOffset +
                                sizeof (UINT32) +
                                sizeof (EFI_IMAGE_FILE_HEADER) +
                                ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
                                );
  NumberOfSections = ImgHdr->Pe32.FileHeader.NumberOfSections;

  //
  // Get base address from the first section header that doesn't point to code section.
  //
  for (Index = 0; Index < NumberOfSections; Index++) {
    //
    // Read section header from file
    //
    Size   = sizeof (EFI_IMAGE_SECTION_HEADER);
    Status = ImageContext->ImageRead (
                             ImageContext->Handle,
                             SectionHeaderOffset,
                             &Size,
                             &SectionHeader
                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = EFI_NOT_FOUND;

    if ((SectionHeader.Characteristics & EFI_IMAGE_SCN_CNT_CODE) == 0) {
      //
      // Build tool saves the offset to SMRAM base as image base in PointerToRelocations & PointerToLineNumbers fields in the
      // first section header that doesn't point to code section in image header. And there is an assumption that when the
      // feature is enabled, if a module is assigned a loading address by tools, PointerToRelocations & PointerToLineNumbers
      // fields should NOT be Zero, or else, these 2 fields should be set to Zero
      //
      ValueInSectionHeader = ReadUnaligned64 ((UINT64 *)&SectionHeader.PointerToRelocations);
      if (ValueInSectionHeader != 0) {
        //
        // Found first section header that doesn't point to code section in which build tool saves the
        // offset to SMRAM base as image base in PointerToRelocations & PointerToLineNumbers fields
        //
        FixLoadingAddress = (EFI_PHYSICAL_ADDRESS)(SmramBase + (INT64)ValueInSectionHeader);

        if ((SmramBase > FixLoadingAddress) && (SmramBase <= FixLoadingAddress)) {
          //
          // The assigned address is valid. Return the specified loading address
          //
          ImageContext->ImageAddress = FixLoadingAddress;
          Status                     = EFI_SUCCESS;
        }
      }

      break;
    }

    SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
  }

  DEBUG ((DEBUG_INFO|DEBUG_LOAD, "LOADING MODULE FIXED INFO: Loading module at fixed address %x, Status = %r \n", FixLoadingAddress, Status));
  return Status;
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
    }

    return EFI_SUCCESS;
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
  UINTN                         PlatformHobSize;
  UINTN                         MmioCount;
  EFI_PHYSICAL_ADDRESS          FoundationHobEndAddress;
  EFI_MEMORY_DESCRIPTOR         *MmioMemoryMap;
  EFI_HOB_HANDOFF_INFO_TABLE    *HobList;
  EFI_STATUS                     Status;

  //
  // Get platform HOBs' size.
  //
  PlatformHobSize = 0;
  Status     = CreateMmPlatformHob (NULL, &PlatformHobSize, NULL);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    ASSERT (PlatformHobSize != 0);
  }

  //
  // Allocate memory buffer for MMIO memory map.
  //
  MmioCount     = PlatformHobSize / sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  MmioMemoryMap = AllocatePool (PlatformHobSize);
  mMemMap       = MmioMemoryMap;

  //
  // Create MMIO memory map.
  //
  PlatformHobSize = 0;
  Status = CreateMmPlatformHob (NULL, &PlatformHobSize, MmioMemoryMap);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    ASSERT (PlatformHobSize != 0);
  }

  //
  // Get foundation HOBs' size.
  //
  FoundationHobSize = 0;
  Status = CreateMmFoundationHobList (NULL, &FoundationHobSize, &MmioCount, MmioMemoryMap);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    ASSERT (FoundationHobSize != 0);
  }

  //
  // If there is no any foundation and platform HOBs, return current PEI HOB list.
  //
  *HobSize = FoundationHobSize + PlatformHobSize;
  if (*HobSize == 0) {
    HobList = GetHobList ();
    return HobList;
  }

  *HobSize += sizeof (EFI_HOB_HANDOFF_INFO_TABLE) + sizeof (EFI_HOB_GENERIC_HEADER);
  HobList = AllocatePages (EFI_SIZE_TO_PAGES (*HobSize));
  if (HobList != NULL){
    //
    // Creat MM HOB list header.
    //
    MmIplHobConstructor ((EFI_PHYSICAL_ADDRESS)(UINTN)HobList, *HobSize);
    //
    // Creat MM foundation HOB list.
    //
    if (FoundationHobSize != 0) {
      CreateMmFoundationHobList (HobList, &FoundationHobSize, &MmioCount, MmioMemoryMap);
    }

    FoundationHobEndAddress = HobList->EfiEndOfHobList;
    //
    // Creat MM platform HOB list.
    //
    if (PlatformHobSize != 0) {
      Status = CreateMmPlatformHob ((VOID *)(UINTN)FoundationHobEndAddress, &PlatformHobSize, NULL);
    }
    //
    // Creat MM HOB list end.
    //
    CreateEndOfList (FoundationHobEndAddress + PlatformHobSize);
  }

  return HobList;
}

/**
  Load the SMM Core image into SMRAM and executes the SMM Core from SMRAM.

  @param[in, out] SmramRange            Descriptor for the range of SMRAM to reload the
                                        currently executing image, the rang of SMRAM to
                                        hold SMM Core will be excluded.
  @param[in, out] SmramRangeSmmCore     Descriptor for the range of SMRAM to hold SMM Core.

  @return  EFI_STATUS

**/
EFI_STATUS
ExecuteSmmCoreFromSmram (
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramRange,
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramRangeSmmCore
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
      DEBUG ((DEBUG_INFO, "SMM IPL calling Standalone MM Core at SMRAM address - 0x%016lx\n", ImageContext.EntryPoint));

      //
      // Execute image
      //
      LoadSmmCore (ImageContext.EntryPoint, MmHobList);
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
  // Always free memory allocated buffer for MM IPL Hobs
  //
  if ((MmHobList != NULL) && (MmHobSize != 0)) {
    FreePages (MmHobList, EFI_SIZE_TO_PAGES (MmHobSize));
  }

  //
  // Always free memory allocated buffer for MMIO memory map.
  //
  if (mMemMap != NULL) {
    FreePool (mMemMap);
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
  UINTN                           Size;
  UINTN                           Index;
  EFI_SMRAM_DESCRIPTOR            *FullSmramRanges;
  UINTN                           AdditionSmramRangeCount;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *NewDescriptorBlock;
  EFI_HOB_GUID_TYPE               *GuidHob;
  UINTN                           MmramRangeCount;

  //
  // Get SMRAM information.
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "SmramMemoryReserve HOB not found\n"));
  }

  DescriptorBlock = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)(GET_GUID_HOB_DATA (GuidHob));
  MmramRangeCount = DescriptorBlock->NumberOfSmmReservedRegions;

  //
  // Reserve one entry SMM Core in the full SMRAM ranges.
  //
  AdditionSmramRangeCount = 1;

  *FullSmramRangeCount = MmramRangeCount + AdditionSmramRangeCount;
  Size                 = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK) + ((*FullSmramRangeCount - 1) * sizeof (EFI_SMRAM_DESCRIPTOR));
  NewDescriptorBlock   = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)BuildGuidHob (
                                                             &gEfiSmmSmramMemoryGuid,
                                                             Size
                                                             );
  ASSERT (NewDescriptorBlock != NULL);
  if (NewDescriptorBlock == NULL) {
    return NULL;
  }

  NewDescriptorBlock->NumberOfSmmReservedRegions = (UINT32)*FullSmramRangeCount;
  FullSmramRanges                                = NewDescriptorBlock->Descriptor;

  //
  // Get SMRAM descriptors and fill to the full SMRAM ranges
  //
  for (Index = 0; Index < MmramRangeCount; Index++) {
    FullSmramRanges[Index].PhysicalStart = DescriptorBlock->Descriptor[Index].PhysicalStart;
    FullSmramRanges[Index].CpuStart      = DescriptorBlock->Descriptor[Index].CpuStart;
    FullSmramRanges[Index].PhysicalSize  = DescriptorBlock->Descriptor[Index].PhysicalSize;
    FullSmramRanges[Index].RegionState   = DescriptorBlock->Descriptor[Index].RegionState;
  }

  //
  // Scrub old one
  //
  ZeroMem (&GuidHob->Name, sizeof (GuidHob->Name));

  return FullSmramRanges;
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
  The Entry Point for SMM IPL at PEI stage.

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
  EFI_STATUS               Status;
  UINTN                    Index;
  UINT64                   MaxSize;
  EFI_SMRAM_DESCRIPTOR     *MmramRanges;
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
  // Get SMRAM information
  //
  mMmramRanges = (EFI_PHYSICAL_ADDRESS)(UINTN)GetFullSmramRanges (PeiServices, (UINTN *)&mMmramRangeCount);
  ASSERT (mMmramRanges != 0);
  if (mMmramRanges == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Print debug message that the SMRAM window is now open.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL opened SMRAM window\n"));

  //
  // Find the largest SMRAM range between 1MB and 4GB that is at least 256KB - 4K in size
  //
  mCurrentSmramRange = NULL;
  MmramRanges        = (EFI_MMRAM_DESCRIPTOR *)(UINTN)mMmramRanges;
  if (MmramRanges == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to retrieve MmramRanges\n"));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < mMmramRangeCount; Index++) {
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
    // Note: SmramRanges specific for SMM Core will put in the mMmramRangeCount - 1.
    //
    Status = ExecuteSmmCoreFromSmram (
               mCurrentSmramRange,
               &(((EFI_MMRAM_DESCRIPTOR *)(UINTN)mMmramRanges)[mMmramRangeCount - 1])
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
