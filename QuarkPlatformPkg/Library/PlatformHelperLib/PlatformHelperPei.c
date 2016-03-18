/** @file
Implementation of Helper routines for PEI enviroment.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/I2cLib.h>

#include "CommonHeader.h"

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this source module.
//

//
// Routines exported by this source module.
//

/**
  Find pointer to RAW data in Firmware volume file.

  @param   FvNameGuid       Firmware volume to search. If == NULL search all.
  @param   FileNameGuid     Firmware volume file to search for.
  @param   SectionData      Pointer to RAW data section of found file.
  @param   SectionDataSize  Pointer to UNITN to get size of RAW data.

  @retval  EFI_SUCCESS            Raw Data found.
  @retval  EFI_INVALID_PARAMETER  FileNameGuid == NULL.
  @retval  EFI_NOT_FOUND          Firmware volume file not found.
  @retval  EFI_UNSUPPORTED        Unsupported in current enviroment (PEI or DXE).

**/
EFI_STATUS
EFIAPI
PlatformFindFvFileRawDataSection (
  IN CONST EFI_GUID                 *FvNameGuid OPTIONAL,
  IN CONST EFI_GUID                 *FileNameGuid,
  OUT VOID                          **SectionData,
  OUT UINTN                         *SectionDataSize
  )
{
  EFI_STATUS                        Status;
  UINTN                             Instance;
  EFI_PEI_FV_HANDLE                 VolumeHandle;
  EFI_PEI_FILE_HANDLE               FileHandle;
  EFI_SECTION_TYPE                  SearchType;
  EFI_FV_INFO                       VolumeInfo;
  EFI_FV_FILE_INFO                  FileInfo;
  CONST EFI_PEI_SERVICES            **PeiServices;

  if (FileNameGuid == NULL || SectionData == NULL || SectionDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *SectionData = NULL;
  *SectionDataSize = 0;

  PeiServices = GetPeiServicesTablePointer ();
  SearchType = EFI_SECTION_RAW;
  for (Instance = 0; !EFI_ERROR((PeiServicesFfsFindNextVolume (Instance, &VolumeHandle))); Instance++) {
    if (FvNameGuid != NULL) {
      Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (!CompareGuid (FvNameGuid, &VolumeInfo.FvName)) {
        continue;
      }
    }
    Status = PeiServicesFfsFindFileByName (FileNameGuid, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      Status = PeiServicesFfsGetFileInfo (FileHandle, &FileInfo);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (IS_SECTION2(FileInfo.Buffer)) {
        *SectionDataSize = SECTION2_SIZE(FileInfo.Buffer) - sizeof(EFI_COMMON_SECTION_HEADER2);
      } else {
        *SectionDataSize = SECTION_SIZE(FileInfo.Buffer) - sizeof(EFI_COMMON_SECTION_HEADER);
      }
      Status = PeiServicesFfsFindSectionData (SearchType, FileHandle, SectionData);
      if (!EFI_ERROR (Status)) {
        return Status;
      }
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Find free spi protect register and write to it to protect a flash region.

  @param   DirectValue      Value to directly write to register.
                            if DirectValue == 0 the use Base & Length below.
  @param   BaseAddress      Base address of region in Flash Memory Map.
  @param   Length           Length of region to protect.

  @retval  EFI_SUCCESS      Free spi protect register found & written.
  @retval  EFI_NOT_FOUND    Free Spi protect register not found.
  @retval  EFI_DEVICE_ERROR Unable to write to spi protect register.
**/
EFI_STATUS
EFIAPI
PlatformWriteFirstFreeSpiProtect (
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length
  )
{
  return WriteFirstFreeSpiProtect (
           QNC_RCRB_BASE,
           DirectValue,
           BaseAddress,
           Length,
           NULL
           );
}

/** Check if System booted with recovery Boot Stage1 image.

  @retval  TRUE    If system booted with recovery Boot Stage1 image.
  @retval  FALSE   If system booted with normal stage1 image.

**/
BOOLEAN
EFIAPI
PlatformIsBootWithRecoveryStage1 (
  VOID
  )
{
  BOOLEAN                           IsRecoveryBoot;
  QUARK_EDKII_STAGE1_HEADER         *Edk2ImageHeader;

  Edk2ImageHeader = (QUARK_EDKII_STAGE1_HEADER *) PcdGet32 (PcdEsramStage1Base);
  switch ((UINT8)Edk2ImageHeader->ImageIndex & QUARK_STAGE1_IMAGE_TYPE_MASK) {
  case QUARK_STAGE1_RECOVERY_IMAGE_TYPE:
    //
    // Recovery Boot
    //
    IsRecoveryBoot = TRUE;
    break;
  default:
    //
    // Normal Boot
    //
    IsRecoveryBoot = FALSE;
    break;
  }

  return IsRecoveryBoot;
}
