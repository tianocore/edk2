/** @file
  SMM IPL that load the SMM Core into SMRAM at PEI stage

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiSmm.h>
#include <Ppi/SmmAccess.h>
#include <Ppi/SmmControl.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/SmramMemoryReserve.h>
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
BOOLEAN               mSmmLocked = FALSE;
EFI_PHYSICAL_ADDRESS  mSmramCacheBase;
UINT64                mSmramCacheSize;

EFI_PEI_NOTIFY_DESCRIPTOR  mReadyToBootNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEventReadyToBootGuid,
  ReadyToBootEvent
};

/**
  This is the callback function on ready to boot.

  Close and Lock smram range on ready to boot stage.

  @param   PeiServices          General purpose services available to every PEIM.
  @param   NotifyDescriptor     The notification structure this PEIM registered on install.
  @param   Ppi                  Pointer to the PPI data associated with this function.
  @retval  EFI_SUCCESS          Close and lock smram ranges successfully.
  @retval  Other                Close and lock smram ranges failed.
**/
EFI_STATUS
EFIAPI
ReadyToBootEvent (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN  VOID                       *Ppi
  )
{
  EFI_STATUS  Status;

  //
  // Close all SMRAM ranges
  //
  Status = mSmmAccess->Close ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, 0);
  ASSERT_EFI_ERROR (Status);

  //
  // Lock all SMRAM ranges
  //
  Status = mSmmAccess->Lock ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, 0);
  ASSERT_EFI_ERROR (Status);

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
  UINT64                           SmmCodeSize;
  UINT64                           ValueInSectionHeader;

  //
  // Build tool will calculate the smm code size and then patch the PcdLoadFixAddressSmmCodePageNumber
  //
  SmmCodeSize       = EFI_PAGES_TO_SIZE (PcdGet32 (PcdLoadFixAddressSmmCodePageNumber));
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

        if ((SmramBase + SmmCodeSize > FixLoadingAddress) && (SmramBase <= FixLoadingAddress)) {
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
  Searches all the available firmware volumes and returns the first matching FFS section.

  This function searches all the firmware volumes for FFS files with FV file type specified by FileType
  The order that the firmware volumes is searched is not deterministic. For each available FV a search
  is made for FFS file of type FileType. If the FV contains more than one FFS file with the same FileType,
  the FileInstance instance will be the matched FFS file. For each FFS file found a search
  is made for FFS sections of type SectionType. If the FFS file contains at least SectionInstance instances
  of the FFS section specified by SectionType, then the SectionInstance instance is returned in Buffer.
  Buffer is allocated using AllocatePool(), and the size of the allocated buffer is returned in Size.
  It is the caller's responsibility to use FreePool() to free the allocated buffer.

  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().

  @param  FileType             Indicates the FV file type to search for within all available FVs.
  @param  FileInstance         Indicates which file instance within all available FVs specified by FileType.
                               FileInstance starts from zero.
  @param  SectionType          Indicates the FFS section type to search for within the FFS file
                               specified by FileType with FileInstance.
  @param  SectionInstance      Indicates which section instance within the FFS file
                               specified by FileType with FileInstance to retrieve. SectionInstance starts from zero.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.
                               Is it the caller's responsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.

  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough resources available to retrieve the matching FFS section.

**/
EFI_STATUS
EFIAPI
GetSectionFromAnyFvByFileType  (
  IN  EFI_FV_FILETYPE   FileType,
  IN  UINTN             FileInstance,
  IN  EFI_SECTION_TYPE  SectionType,
  IN  UINTN             SectionInstance,
  OUT VOID              **Buffer,
  OUT UINTN             *Size
  )
{
  EFI_STATUS           Status;
  UINTN                FvIndex;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;
  EFI_PE32_SECTION     *SectionData;
  UINT32               SectionSize;

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
    Status     = PeiServicesFfsFindNextFile (FileType, VolumeHandle, &FileHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Search Section
    //
    SectionData = NULL;
    Status      = PeiServicesFfsFindSectionData (SectionType, FileHandle, Buffer);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Great!
    //
    SectionData = (EFI_PE32_SECTION *)((UINT8 *)*Buffer - sizeof (EFI_PE32_SECTION));
    ASSERT (SectionData->Type == SectionType);
    SectionSize  = *(UINT32 *)SectionData->Size;
    SectionSize &= 0xFFFFFF;
    *Size        = SectionSize - sizeof (EFI_PE32_SECTION);

    if (FileType == EFI_FV_FILETYPE_MM_CORE_STANDALONE) {
      EFI_FV_INFO  VolumeInfo;
      //
      // This is SMM BFV
      //
      Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
      if (!EFI_ERROR (Status)) {
        gMmCorePrivate->StandaloneBfvAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      }
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
  UINTN                         SourceSize;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                         PageCount;
  VOID                          *HobList;

  Status = PeiServicesGetHobList (&HobList);
  ASSERT_EFI_ERROR (Status);

  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type SMM_CORE
  //
  Status = GetSectionFromAnyFvByFileType (
             EFI_FV_FILETYPE_MM_CORE_STANDALONE,
             0,
             EFI_SECTION_PE32,
             0,
             &SourceBuffer,
             &SourceSize
             );
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
  // if Loading module at Fixed Address feature is enabled, the SMM core driver will be loaded to
  // the address assigned by build tool.
  //
  if (PcdGet64 (PcdLoadModuleAtFixAddressEnable) != 0) {
    //
    // Get the fixed loading address assigned by Build tool
    //
    Status = GetPeCoffImageFixLoadingAssignedAddress (&ImageContext);
    if (!EFI_ERROR (Status)) {
      //
      // Since the memory range to load SMM CORE will be cut out in SMM core, so no need to allocate and free this range
      //
      PageCount = 0;
      //
      // Reserved Smram Region for SmmCore is not used, and remove it from SmramRangeCount.
      //
      gMmCorePrivate->MmramRangeCount--;
    } else {
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED ERROR: Loading module at fixed address at address failed\n"));
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
    }
  } else {
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
  }

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
      // Execute image
      //
      LoadSmmCore (ImageContext.EntryPoint, HobList);
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

  It will get SMRAM ranges from SmmAccess protocol and SMRAM reserved ranges from
  SmmConfiguration protocol, split the entries if there is overlap between them.
  It will also reserve one entry for SMM core.

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
  UINTN                 SmramRangeCount;

  //
  // Get SMRAM information.
  //
  Size   = 0;
  Status = mSmmAccess->GetCapabilities ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  SmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Reserve one entry SMM Core in the full SMRAM ranges.
  //
  AdditionSmramRangeCount = 1;
  if (PcdGet64 (PcdLoadModuleAtFixAddressEnable) != 0) {
    //
    // Reserve two entries for all SMM drivers & SMM Core in the full SMRAM ranges.
    //
    AdditionSmramRangeCount = 2;
  }

  *FullSmramRangeCount = SmramRangeCount + AdditionSmramRangeCount;
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
  The Entry Point for SMM IPL

  Load SMM Core into SMRAM.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmIplEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS             Status;
  UINTN                  Index;
  UINT64                 MaxSize;
  UINT64                 SmmCodeSize;
  MM_CORE_DATA_HOB_DATA  SmmCoreDataHobData;
  EFI_SMRAM_DESCRIPTOR   *MmramRanges;
  EFI_SMRAM_DESCRIPTOR   *SmramRangeSmmDriver;

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
  // Get SMM Access Protocol
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
  Status = mSmmAccess->Open ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, 0);
  ASSERT_EFI_ERROR (Status);

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
    // if Loading module at Fixed Address feature is enabled, save the SMRAM base to Load
    // Modules At Fixed Address Configuration Table.
    //
    if (PcdGet64 (PcdLoadModuleAtFixAddressEnable) != 0) {
      //
      // Build tool will calculate the smm code size and then patch the PcdLoadFixAddressSmmCodePageNumber
      //
      SmmCodeSize = LShiftU64 (PcdGet32 (PcdLoadFixAddressSmmCodePageNumber), EFI_PAGE_SHIFT);
      //
      // The SMRAM available memory is assumed to be larger than SmmCodeSize
      //
      ASSERT (mCurrentSmramRange->PhysicalSize > SmmCodeSize);
      //
      // Fill the Smram range for all SMM code
      //
      MmramRanges = (EFI_MMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges;
      //
      // Note: SmramRanges specific for all SMM code will put in the gMmCorePrivate->MmramRangeCount - 2.
      //
      SmramRangeSmmDriver                = &MmramRanges[gMmCorePrivate->MmramRangeCount - 2];
      SmramRangeSmmDriver->CpuStart      = mCurrentSmramRange->CpuStart;
      SmramRangeSmmDriver->PhysicalStart = mCurrentSmramRange->PhysicalStart;
      SmramRangeSmmDriver->RegionState   = mCurrentSmramRange->RegionState | EFI_ALLOCATED;
      SmramRangeSmmDriver->PhysicalSize  = SmmCodeSize;

      mCurrentSmramRange->PhysicalSize -= SmmCodeSize;
      mCurrentSmramRange->CpuStart      = mCurrentSmramRange->CpuStart + SmmCodeSize;
      mCurrentSmramRange->PhysicalStart = mCurrentSmramRange->PhysicalStart + SmmCodeSize;
    }

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

  //
  // If the SMM Core could not be loaded then close SMRAM window, free allocated
  // resources, and return an error so SMM IPL will be unloaded.
  //
  if ((mCurrentSmramRange == NULL) || EFI_ERROR (Status)) {
    //
    // Close all SMRAM ranges
    //
    Status = mSmmAccess->Close ((EFI_PEI_SERVICES **)PeiServices, mSmmAccess, 0);
    ASSERT_EFI_ERROR (Status);

    //
    // Print debug message that the SMRAM window is now closed.
    //
    DEBUG ((DEBUG_INFO, "SMM IPL closed SMRAM window\n"));

    //
    // Free all allocated resources
    //
    FreePool ((VOID *)(UINTN)gMmCorePrivate->MmramRanges);

    return EFI_UNSUPPORTED;
  }

  //
  // Create ready to boot for close and lock smram ranges
  //
  Status = PeiServicesNotifyPpi (&mReadyToBootNotifyList);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
