/** @file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FV_LIB_H_
#define _FV_LIB_H_

#include <Uefi.h>
#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>

/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.

  @param  SearchType  Filter to find only files of this type.
                      Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param  FwVolHeader Pointer to the FV header of the volume to search.
                      This parameter must point to a valid FFS volume.
  @param  FileHeader  Pointer to the current file from which to begin searching.
                      This pointer will be updated upon return to reflect the file found.

  @retval EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FfsFindNextFile (
  IN EFI_FV_FILETYPE             SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  );

/**
  Given the input file pointer, search for the next matching section in the
  FFS volume.

  @param  SearchType    Filter to find only sections of this type.
  @param  FfsFileHeader Pointer to the current file to search.
  @param  SectionHeader Pointer to the Section matching SectionType in FfsFileHeader.
                        NULL if section not found

  @retval  EFI_NOT_FOUND  No files matching the search criteria were found
  @retval  EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FfsFindSection (
  IN EFI_SECTION_TYPE               SectionType,
  IN EFI_FFS_FILE_HEADER            *FfsFileHeader,
  IN OUT EFI_COMMON_SECTION_HEADER  **SectionHeader
  );

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
EFIAPI
FindFfsSectionInSections (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  );

/**
  Given the input file pointer, search for the next matching section in the
  FFS volume.

  @param  SearchType      Filter to find only sections of this type.
  @param  FfsFileHeader   Pointer to the current file to search.
  @param  SectionData     Pointer to the Section matching SectionType in FfsFileHeader.
                          NULL if section not found
  @param  SectionDataSize The size of SectionData

  @retval  EFI_NOT_FOUND  No files matching the search criteria were found
  @retval  EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FfsFindSectionData (
  IN EFI_SECTION_TYPE     SectionType,
  IN EFI_FFS_FILE_HEADER  *FfsFileHeader,
  OUT VOID                **SectionData,
  OUT UINTN               *SectionDataSize
  );

#endif
