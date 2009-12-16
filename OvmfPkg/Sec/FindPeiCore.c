/** @file
  Locate the entry point for the PEI Core

  Copyright (c) 2008 - 2009, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include "SecMain.h"


VOID
EFIAPI
FindPeiCoreEntryPoint (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *BootFirmwareVolumePtr,
  OUT VOID                             **PeiCoreEntryPoint
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;
  EFI_COMMON_SECTION_HEADER   *Section;
  EFI_PHYSICAL_ADDRESS        EndOfSection;

  *PeiCoreEntryPoint = NULL;

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) BootFirmwareVolumePtr;
  EndOfFirmwareVolume = CurrentAddress + BootFirmwareVolumePtr->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + BootFirmwareVolumePtr->HeaderLength; ; ) {

    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      return;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    Size = *(UINT32*) File->Size & 0xffffff;
    if (Size < (sizeof (*File) + sizeof (*Section))) {
      return;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return;
    }

    //
    // Look for PEI Core files
    //
    if (File->Type != EFI_FV_FILETYPE_PEI_CORE) {
      continue;
    }

    //
    // Loop through the FFS file sections within the PEI Core FFS file
    //
    EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN) (File + 1);
    for (;;) {
      CurrentAddress = (EndOfSection + 3) & 0xfffffffffffffffcULL;
      Section = (EFI_COMMON_SECTION_HEADER*)(UINTN) CurrentAddress;

      Size = *(UINT32*) Section->Size & 0xffffff;
      if (Size < sizeof (*Section)) {
        return;
      }

      EndOfSection = CurrentAddress + Size;
      if (EndOfSection > EndOfFile) {
        return;
      }

      //
      // Look for executable sections
      //
      if (Section->Type == EFI_SECTION_PE32) {
        Status = PeCoffLoaderGetEntryPoint ((VOID*) (Section + 1), PeiCoreEntryPoint);
        if (!EFI_ERROR (Status)) {
          return;
        }
      }
    }

  }
}

