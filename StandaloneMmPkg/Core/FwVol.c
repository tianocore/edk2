/** @file
    Firmware volume helper interfaces.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"
#include <Library/FvLib.h>
#include <Library/ExtractGuidedSectionLib.h>

EFI_STATUS
MmAddToDriverList (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN VOID                        *Pe32Data,
  IN UINTN                       Pe32DataSize,
  IN VOID                        *Depex,
  IN UINTN                       DepexSize,
  IN EFI_GUID                    *DriverName
  );

BOOLEAN
FvHasBeenProcessed (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  );

VOID
FvIsBeingProcessed (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  );

/**
  Given the pointer to the Firmware Volume Header find the
  MM driver and return its PE32 image.

  @param [in] FwVolHeader   Pointer to memory mapped FV
  @param [in] Depth         Nesting depth of encapsulation sections. Callers
                            different from MmCoreFfsFindMmDriver() are
                            responsible for passing in a zero Depth.

  @retval  EFI_SUCCESS            Success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find section data.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
  @retval  EFI_VOLUME_CORRUPTED   Firmware volume is corrupted.
  @retval  EFI_UNSUPPORTED        Operation not supported.
  @retval  EFI_ABORTED            Recursion aborted because Depth has been
                                  greater than or equal to
                                  PcdFwVolMmMaxEncapsulationDepth.

**/
EFI_STATUS
MmCoreFfsFindMmDriver (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN  UINT32                      Depth
  )
{
  EFI_STATUS                  Status;
  EFI_STATUS                  DepexStatus;
  EFI_FFS_FILE_HEADER         *FileHeader;
  VOID                        *Pe32Data;
  UINTN                       Pe32DataSize;
  VOID                        *Depex;
  UINTN                       DepexSize;
  EFI_COMMON_SECTION_HEADER   *Section;
  VOID                        *SectionData;
  UINTN                       SectionDataSize;
  UINT32                      DstBufferSize;
  VOID                        *ScratchBuffer;
  UINT32                      ScratchBufferSize;
  VOID                        *AllocatedDstBuffer;
  VOID                        *DstBuffer;
  UINT16                      SectionAttribute;
  UINT32                      AuthenticationStatus;
  EFI_FIRMWARE_VOLUME_HEADER  *InnerFvHeader;

  DEBUG ((DEBUG_INFO, "MmCoreFfsFindMmDriver - 0x%x\n", FwVolHeader));

  if (Depth >= PcdGet32 (PcdFwVolMmMaxEncapsulationDepth)) {
    DEBUG ((DEBUG_ERROR, "%a: recursion aborted due to nesting depth\n", __func__));
    return EFI_ABORTED;
  }

  if (FvHasBeenProcessed (FwVolHeader)) {
    return EFI_SUCCESS;
  }

  FvIsBeingProcessed (FwVolHeader);

  //
  // First check for encapsulated compressed firmware volumes
  //
  FileHeader = NULL;
  do {
    Status = FfsFindNextFile (
               EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
               FwVolHeader,
               &FileHeader
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Check uncompressed firmware volumes
    //
    Status = FfsFindSectionData (
               EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
               FileHeader,
               &SectionData,
               &SectionDataSize
               );
    if (!EFI_ERROR (Status)) {
      if (SectionDataSize > sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
        InnerFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)SectionData;
        MmCoreFfsFindMmDriver (InnerFvHeader, Depth + 1);
        continue;
      }
    }

    //
    // Check compressed firmware volumes
    //
    Status = FfsFindSection (
               EFI_SECTION_GUID_DEFINED,
               FileHeader,
               &Section
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = ExtractGuidedSectionGetInfo (
               Section,
               &DstBufferSize,
               &ScratchBufferSize,
               &SectionAttribute
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Allocate scratch buffer
    //
    ScratchBuffer = (VOID *)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (ScratchBufferSize));
    if (ScratchBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Allocate destination buffer, extra one page for adjustment
    //
    AllocatedDstBuffer = (VOID *)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize));
    if (AllocatedDstBuffer == NULL) {
      FreePages (ScratchBuffer, EFI_SIZE_TO_PAGES (ScratchBufferSize));
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Call decompress function
    //
    DstBuffer = AllocatedDstBuffer;
    Status    = ExtractGuidedSectionDecode (
                  Section,
                  &DstBuffer,
                  ScratchBuffer,
                  &AuthenticationStatus
                  );
    FreePages (ScratchBuffer, EFI_SIZE_TO_PAGES (ScratchBufferSize));
    if (EFI_ERROR (Status)) {
      goto FreeDstBuffer;
    }

    //
    // Free allocated DstBuffer if it is not used
    //
    if (DstBuffer != AllocatedDstBuffer) {
      FreePages (AllocatedDstBuffer, EFI_SIZE_TO_PAGES (DstBufferSize));
      AllocatedDstBuffer = NULL;
    }

    DEBUG ((
      DEBUG_INFO,
      "Processing compressed firmware volume (AuthenticationStatus == %x)\n",
      AuthenticationStatus
      ));

    Status = FindFfsSectionInSections (
               DstBuffer,
               DstBufferSize,
               EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
               &Section
               );
    if (EFI_ERROR (Status)) {
      goto FreeDstBuffer;
    }

    if (IS_SECTION2 (Section)) {
      InnerFvHeader = (VOID *)((EFI_COMMON_SECTION_HEADER2 *)Section + 1);
    } else {
      InnerFvHeader = (VOID *)(Section + 1);
    }

    Status = MmCoreFfsFindMmDriver (InnerFvHeader, Depth + 1);
    if (EFI_ERROR (Status)) {
      goto FreeDstBuffer;
    }
  } while (TRUE);

  DEBUG ((DEBUG_INFO, "Check MmFileTypes - 0x%x\n", EFI_FV_FILETYPE_MM_STANDALONE));
  FileHeader = NULL;
  do {
    Status = FfsFindNextFile (EFI_FV_FILETYPE_MM_STANDALONE, FwVolHeader, &FileHeader);
    if (!EFI_ERROR (Status)) {
      Status = FfsFindSectionData (EFI_SECTION_PE32, FileHeader, &Pe32Data, &Pe32DataSize);
      DEBUG ((DEBUG_INFO, "Find PE data - 0x%x\n", Pe32Data));
      DepexStatus = FfsFindSectionData (EFI_SECTION_MM_DEPEX, FileHeader, &Depex, &DepexSize);
      if (!EFI_ERROR (DepexStatus)) {
        MmAddToDriverList (FwVolHeader, Pe32Data, Pe32DataSize, Depex, DepexSize, &FileHeader->Name);
      }
    }
  } while (!EFI_ERROR (Status));

  return EFI_SUCCESS;

FreeDstBuffer:
  if (AllocatedDstBuffer != NULL) {
    FreePages (AllocatedDstBuffer, EFI_SIZE_TO_PAGES (DstBufferSize));
  }

  return Status;
}
