/** @file
  Locate the entry point for the PEI Core

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include "SecMain.h"

/**
  Find core image base.

  @param   FirmwareVolumePtr        Point to the firmware volume for finding core image.
  @param   FileType                 The FileType for searching, either SecCore or PeiCore.
  @param   CoreImageBase            The base address of the core image.

**/
EFI_STATUS
EFIAPI
FindImageBase (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FirmwareVolumePtr,
  IN  EFI_FV_FILETYPE             FileType,
  OUT EFI_PHYSICAL_ADDRESS        *CoreImageBase
  )
{
  EFI_PHYSICAL_ADDRESS       CurrentAddress;
  EFI_PHYSICAL_ADDRESS       EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER        *File;
  UINT32                     Size;
  EFI_PHYSICAL_ADDRESS       EndOfFile;
  EFI_COMMON_SECTION_HEADER  *Section;
  EFI_PHYSICAL_ADDRESS       EndOfSection;

  *CoreImageBase = 0;

  CurrentAddress      = (EFI_PHYSICAL_ADDRESS)(UINTN)FirmwareVolumePtr;
  EndOfFirmwareVolume = CurrentAddress + FirmwareVolumePtr->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + FirmwareVolumePtr->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    File = (EFI_FFS_FILE_HEADER *)(UINTN)CurrentAddress;
    if (IS_FFS_FILE2 (File)) {
      Size = FFS_FILE2_SIZE (File);
      if (Size <= 0x00FFFFFF) {
        return EFI_NOT_FOUND;
      }
    } else {
      Size = FFS_FILE_SIZE (File);
      if (Size < sizeof (EFI_FFS_FILE_HEADER)) {
        return EFI_NOT_FOUND;
      }
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    //
    // Look for particular Core file (either SEC Core or PEI Core)
    //
    if (File->Type != FileType) {
      continue;
    }

    //
    // Loop through the FFS file sections within the FFS file
    //
    if (IS_FFS_FILE2 (File)) {
      EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN)((UINT8 *)File + sizeof (EFI_FFS_FILE_HEADER2));
    } else {
      EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN)((UINT8 *)File + sizeof (EFI_FFS_FILE_HEADER));
    }

    for ( ; ;) {
      CurrentAddress = (EndOfSection + 3) & 0xfffffffffffffffcULL;
      Section        = (EFI_COMMON_SECTION_HEADER *)(UINTN)CurrentAddress;

      if (IS_SECTION2 (Section)) {
        Size = SECTION2_SIZE (Section);
        if (Size <= 0x00FFFFFF) {
          return EFI_NOT_FOUND;
        }
      } else {
        Size = SECTION_SIZE (Section);
        if (Size < sizeof (EFI_COMMON_SECTION_HEADER)) {
          return EFI_NOT_FOUND;
        }
      }

      EndOfSection = CurrentAddress + Size;
      if (EndOfSection > EndOfFile) {
        return EFI_NOT_FOUND;
      }

      //
      // Look for executable sections
      //
      if ((Section->Type == EFI_SECTION_PE32) || (Section->Type == EFI_SECTION_TE)) {
        if (File->Type == FileType) {
          if (IS_SECTION2 (Section)) {
            *CoreImageBase = (PHYSICAL_ADDRESS)(UINTN)((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER2));
          } else {
            *CoreImageBase = (PHYSICAL_ADDRESS)(UINTN)((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER));
          }
        }

        break;
      }
    }

    //
    // Either SEC Core or PEI Core images found
    //
    if (*CoreImageBase != 0) {
      return EFI_SUCCESS;
    }
  }
}

/**
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug information. It will report them if
  remote debug is enabled.

  @param   SecCoreFirmwareVolumePtr Point to the firmware volume for finding SecCore.
  @param   PeiCoreFirmwareVolumePtr Point to the firmware volume for finding PeiCore.
  @param   PeiCoreEntryPoint        The entry point of the PEI core.

**/
VOID
EFIAPI
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *SecCoreFirmwareVolumePtr,
  IN  EFI_FIRMWARE_VOLUME_HEADER  *PeiCoreFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT    *PeiCoreEntryPoint
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          SecCoreImageBase;
  EFI_PHYSICAL_ADDRESS          PeiCoreImageBase;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;

  //
  // Find SEC Core image base
  //
  Status = FindImageBase (SecCoreFirmwareVolumePtr, EFI_FV_FILETYPE_SECURITY_CORE, &SecCoreImageBase);
  ASSERT_EFI_ERROR (Status);

  ZeroMem ((VOID *)&ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));
  //
  // Report SEC Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = SecCoreImageBase;
  ImageContext.PdbPointer   = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Find PEI Core image base
  //
  Status = FindImageBase (PeiCoreFirmwareVolumePtr, EFI_FV_FILETYPE_PEI_CORE, &PeiCoreImageBase);
  ASSERT_EFI_ERROR (Status);

  //
  // Report PEI Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = PeiCoreImageBase;
  ImageContext.PdbPointer   = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)PeiCoreImageBase, (VOID **)PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    *PeiCoreEntryPoint = 0;
  }

  return;
}
