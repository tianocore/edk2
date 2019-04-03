/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"
#include <Library/FvLib.h>
#include <Library/ExtractGuidedSectionLib.h>

//
// List of file types supported by dispatcher
//
EFI_FV_FILETYPE mMmFileTypes[] = {
  EFI_FV_FILETYPE_MM,
  0xE, //EFI_FV_FILETYPE_MM_STANDALONE,
       //
       // Note: DXE core will process the FV image file, so skip it in MM core
       // EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
       //
};

EFI_STATUS
MmAddToDriverList (
  IN EFI_HANDLE   FvHandle,
  IN VOID         *Pe32Data,
  IN UINTN        Pe32DataSize,
  IN VOID         *Depex,
  IN UINTN        DepexSize,
  IN EFI_GUID     *DriverName
  );

BOOLEAN
FvHasBeenProcessed (
  IN EFI_HANDLE  FvHandle
  );

VOID
FvIsBeingProcesssed (
  IN EFI_HANDLE  FvHandle
  );

EFI_STATUS
MmCoreFfsFindMmDriver (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  )
/*++

Routine Description:
  Given the pointer to the Firmware Volume Header find the
  MM driver and return it's PE32 image.

Arguments:
  FwVolHeader - Pointer to memory mapped FV

Returns:
  other       - Failure

--*/
{
  EFI_STATUS                              Status;
  EFI_STATUS                              DepexStatus;
  EFI_FFS_FILE_HEADER                     *FileHeader;
  EFI_FV_FILETYPE                         FileType;
  VOID                                    *Pe32Data;
  UINTN                                   Pe32DataSize;
  VOID                                    *Depex;
  UINTN                                   DepexSize;
  UINTN                                   Index;
  EFI_COMMON_SECTION_HEADER               *Section;
  VOID                                    *SectionData;
  UINTN                                   SectionDataSize;
  UINT32                                  DstBufferSize;
  VOID                                    *ScratchBuffer;
  UINT32                                  ScratchBufferSize;
  VOID                                    *DstBuffer;
  UINT16                                  SectionAttribute;
  UINT32                                  AuthenticationStatus;
  EFI_FIRMWARE_VOLUME_HEADER              *InnerFvHeader;

  DEBUG ((DEBUG_INFO, "MmCoreFfsFindMmDriver - 0x%x\n", FwVolHeader));

  if (FvHasBeenProcessed (FwVolHeader)) {
    return EFI_SUCCESS;
  }

  FvIsBeingProcesssed (FwVolHeader);

  //
  // First check for encapsulated compressed firmware volumes
  //
  FileHeader = NULL;
  do {
    Status = FfsFindNextFile (EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
               FwVolHeader, &FileHeader);
    if (EFI_ERROR (Status)) {
      break;
    }
    Status = FfsFindSectionData (EFI_SECTION_GUID_DEFINED, FileHeader,
               &SectionData, &SectionDataSize);
    if (EFI_ERROR (Status)) {
      break;
    }
    Section = (EFI_COMMON_SECTION_HEADER *)(FileHeader + 1);
    Status = ExtractGuidedSectionGetInfo (Section, &DstBufferSize,
               &ScratchBufferSize, &SectionAttribute);
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
    DstBuffer = (VOID *)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize));
    if (DstBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Call decompress function
    //
    Status = ExtractGuidedSectionDecode (Section, &DstBuffer, ScratchBuffer,
                &AuthenticationStatus);
    FreePages (ScratchBuffer, EFI_SIZE_TO_PAGES (ScratchBufferSize));
    if (EFI_ERROR (Status)) {
      goto FreeDstBuffer;
    }

    DEBUG ((DEBUG_INFO,
      "Processing compressed firmware volume (AuthenticationStatus == %x)\n",
      AuthenticationStatus));

    Status = FindFfsSectionInSections (DstBuffer, DstBufferSize,
               EFI_SECTION_FIRMWARE_VOLUME_IMAGE, &Section);
    if (EFI_ERROR (Status)) {
      goto FreeDstBuffer;
    }

    InnerFvHeader = (VOID *)(Section + 1);
    Status = MmCoreFfsFindMmDriver (InnerFvHeader);
    if (EFI_ERROR (Status)) {
      goto FreeDstBuffer;
    }
  } while (TRUE);

  for (Index = 0; Index < sizeof (mMmFileTypes) / sizeof (mMmFileTypes[0]); Index++) {
    DEBUG ((DEBUG_INFO, "Check MmFileTypes - 0x%x\n", mMmFileTypes[Index]));
    FileType = mMmFileTypes[Index];
    FileHeader = NULL;
    do {
      Status = FfsFindNextFile (FileType, FwVolHeader, &FileHeader);
      if (!EFI_ERROR (Status)) {
        Status = FfsFindSectionData (EFI_SECTION_PE32, FileHeader, &Pe32Data, &Pe32DataSize);
        DEBUG ((DEBUG_INFO, "Find PE data - 0x%x\n", Pe32Data));
        DepexStatus = FfsFindSectionData (EFI_SECTION_MM_DEPEX, FileHeader, &Depex, &DepexSize);
        if (!EFI_ERROR (DepexStatus)) {
          MmAddToDriverList (FwVolHeader, Pe32Data, Pe32DataSize, Depex, DepexSize, &FileHeader->Name);
        }
      }
    } while (!EFI_ERROR (Status));
  }

  return EFI_SUCCESS;

FreeDstBuffer:
  FreePages (DstBuffer, EFI_SIZE_TO_PAGES (DstBufferSize));

  return Status;
}
