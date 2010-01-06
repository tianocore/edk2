/** @file
  Locate the entry point for the PEI Core

  Copyright (c) 2008 - 2010, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include "SecMain.h"


/**
  Locates the main boot firmware volume.

  @param[in,out]  BootFv  On input, the base of the BootFv
                          On output, the decompressed main firmware volume

  @retval EFI_SUCCESS    The main firmware volume was located and decompressed
  @retval EFI_NOT_FOUND  The main firmware volume was not found

**/
EFI_STATUS
FindMainFv (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER   **BootFv
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  UINTN                       Distance;
  BOOLEAN                     Found;

  ASSERT (((UINTN) *BootFv & EFI_PAGE_MASK) == 0);

  Found = FALSE;
  Fv = *BootFv;
  Distance = (UINTN) (*BootFv)->FvLength;
  do {
    Fv = (EFI_FIRMWARE_VOLUME_HEADER*) ((UINT8*) Fv - EFI_PAGE_SIZE);
    Distance += EFI_PAGE_SIZE;
    if (Distance > SIZE_32MB) {
      return EFI_NOT_FOUND;
    }

    if (Fv->Signature != EFI_FVH_SIGNATURE) {
      continue;
    }

    if ((UINTN) Fv->FvLength > Distance) {
      continue;
    }

    *BootFv = Fv;
    return EFI_SUCCESS;

  } while (TRUE);
}


/**
  Locates a section within a series of sections
  with the specified section type.

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                             *Sections,
  IN  UINTN                            SizeOfSections,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfSections;
  EFI_COMMON_SECTION_HEADER   *Section;
  EFI_PHYSICAL_ADDRESS        EndOfSection;

  //
  // Loop through the FFS file sections within the PEI Core FFS file
  //
  EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN) Sections;
  EndOfSections = EndOfSection + SizeOfSections;
  for (;;) {
    if (EndOfSection == EndOfSections) {
      break;
    }
    CurrentAddress = (EndOfSection + 3) & ~(3ULL);
    if (CurrentAddress >= EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    Section = (EFI_COMMON_SECTION_HEADER*)(UINTN) CurrentAddress;
    DEBUG ((EFI_D_INFO, "Section->Type: 0x%x\n", Section->Type));

    Size = SECTION_SIZE (Section);
    if (Size < sizeof (*Section)) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfSection = CurrentAddress + Size;
    if (EndOfSection > EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the requested section type
    //
    if (Section->Type == SectionType) {
      *FoundSection = Section;
      return EFI_SUCCESS;
    }
    DEBUG ((EFI_D_INFO, "Section->Type (0x%x) != SectionType (0x%x)\n", Section->Type, SectionType));
  }

  return EFI_NOT_FOUND;
}


/**
  Locates a FFS file with the specified file type and a section
  within that file with the specified section type.

  @param[in]   Fv            The firmware volume to search
  @param[in]   FileType      The file type to locate
  @param[in]   SectionType   The section type to locate
  @param[out]  FoundSection  The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
EFIAPI
FindFfsFileAndSection (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *Fv,
  IN  EFI_FV_FILETYPE                  FileType,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;

  if (Fv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((EFI_D_INFO, "FV at %p does not have FV header signature\n", Fv));
    return EFI_VOLUME_CORRUPTED;
  }

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) Fv;
  EndOfFirmwareVolume = CurrentAddress + Fv->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + Fv->HeaderLength; ; ) {

    CurrentAddress = (EndOfFile + 7) & ~(7ULL);
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    Size = *(UINT32*) File->Size & 0xffffff;
    if (Size < (sizeof (*File) + sizeof (EFI_COMMON_SECTION_HEADER))) {
      return EFI_VOLUME_CORRUPTED;
    }
    DEBUG ((EFI_D_INFO, "File->Type: 0x%x\n", File->Type));

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the request file type
    //
    if (File->Type != FileType) {
      DEBUG ((EFI_D_INFO, "File->Type (0x%x) != FileType (0x%x)\n", File->Type, FileType));
      continue;
    }

    Status = FindFfsSectionInSections (
               (VOID*) (File + 1),
               (UINTN) EndOfFile - (UINTN) (File + 1),
               SectionType,
               FoundSection
               );
    if (!EFI_ERROR (Status) || (Status == EFI_VOLUME_CORRUPTED)) {
      return Status;
    }
  }
}


/**
  Locates the compressed main firmware volume and decompresses it.

  @param[in,out]  Fv            On input, the firmware volume to search
                                On output, the decompressed main FV

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
EFIAPI
DecompressGuidedFv (
  IN OUT EFI_FIRMWARE_VOLUME_HEADER       **Fv
  )
{
  EFI_STATUS                        Status;
  EFI_GUID_DEFINED_SECTION          *Section;
  UINT32                            OutputBufferSize;
  UINT32                            ScratchBufferSize;
  UINT16                            SectionAttribute;
  UINT32                            AuthenticationStatus;
  VOID                              *OutputBuffer;
  VOID                              *ScratchBuffer;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION *NewFvSection;
  EFI_FIRMWARE_VOLUME_HEADER        *NewFv;

  NewFvSection = (EFI_FIRMWARE_VOLUME_IMAGE_SECTION*) NULL;

  Status = FindFfsFileAndSection (
             *Fv,
             EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
             EFI_SECTION_GUID_DEFINED,
             (EFI_COMMON_SECTION_HEADER**) &Section
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to find GUID defined section\n"));
    return Status;
  }

  Status = ExtractGuidedSectionGetInfo (
             Section,
             &OutputBufferSize,
             &ScratchBufferSize,
             &SectionAttribute
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to GetInfo for GUIDed section\n"));
    return Status;
  }

  //PcdGet32 (PcdOvmfMemFvBase), PcdGet32 (PcdOvmfMemFvSize)
  OutputBuffer = (VOID*) ((UINT8*)(UINTN) PcdGet32 (PcdOvmfMemFvBase) + SIZE_1MB);
  ScratchBuffer = ALIGN_POINTER ((UINT8*) OutputBuffer + OutputBufferSize, SIZE_1MB);
  Status = ExtractGuidedSectionDecode (
             Section,
             &OutputBuffer,
             ScratchBuffer,
             &AuthenticationStatus
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error during GUID section decode\n"));
    return Status;
  }

  Status = FindFfsSectionInSections (
             OutputBuffer,
             OutputBufferSize,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             (EFI_COMMON_SECTION_HEADER**) &NewFvSection
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to find FV image in extracted data\n"));
    return Status;
  }

  NewFv = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN) PcdGet32 (PcdOvmfMemFvBase);
  CopyMem (NewFv, (VOID*) (NewFvSection + 1), PcdGet32 (PcdOvmfMemFvSize));

  if (NewFv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((EFI_D_ERROR, "Extracted FV at %p does not have FV header signature\n", NewFv));
    CpuDeadLoop ();
    return EFI_VOLUME_CORRUPTED;
  }

  *Fv = NewFv;
  return EFI_SUCCESS;
}


/**
  Locates the PEI Core entry point address

  @param[in]  Fv                 The firmware volume to search
  @param[out] PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
EFIAPI
FindPeiCoreEntryPointInFv (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *Fv,
  OUT VOID                             **PeiCoreEntryPoint
  )
{
  EFI_STATUS                  Status;
  EFI_COMMON_SECTION_HEADER   *Section;

  Status = FindFfsFileAndSection (
             Fv,
             EFI_FV_FILETYPE_PEI_CORE,
             EFI_SECTION_PE32,
             &Section
             );
  if (EFI_ERROR (Status)) {
    Status = FindFfsFileAndSection (
               Fv,
               EFI_FV_FILETYPE_PEI_CORE,
               EFI_SECTION_TE,
               &Section
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Unable to find PEI Core image\n"));
      return Status;
    }
  }

  return PeCoffLoaderGetEntryPoint ((VOID*) (Section + 1), PeiCoreEntryPoint);
}


/**
  Locates the PEI Core entry point address

  @param[in,out]  Fv                 The firmware volume to search
  @param[out]     PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
VOID
EFIAPI
FindPeiCoreEntryPoint (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER       **BootFv,
     OUT  VOID                             **PeiCoreEntryPoint
  )
{
  *PeiCoreEntryPoint = NULL;

  FindMainFv (BootFv);

  DecompressGuidedFv (BootFv);

  FindPeiCoreEntryPointInFv (*BootFv, PeiCoreEntryPoint);
}

