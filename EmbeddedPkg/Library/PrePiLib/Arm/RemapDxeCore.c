/** @file
  Copyright (c) 2023, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrePi.h"

#include <Library/ArmMmuLib.h>

/**
  Remap the code section of the DXE core with the read-only and executable
  permissions.

  @param  ImageContext    The image context describing the loaded PE/COFF image

**/
VOID
EFIAPI
RemapDxeCore (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_SECTION_HEADER             *Section;
  UINTN                                Index;

  Hdr.Union = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((UINT8 *)ImageContext->Handle +
                                                  ImageContext->PeCoffHeaderOffset);
  ASSERT (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE);

  Section = (EFI_IMAGE_SECTION_HEADER *)((UINT8 *)Hdr.Union + sizeof (UINT32) +
                                         sizeof (EFI_IMAGE_FILE_HEADER) +
                                         Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                         );

  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    if ((Section[Index].Characteristics & EFI_IMAGE_SCN_CNT_CODE) != 0) {
      ArmSetMemoryRegionReadOnly (
        (UINTN)(ImageContext->ImageAddress + Section[Index].VirtualAddress),
        Section[Index].Misc.VirtualSize
        );

      ArmClearMemoryRegionNoExec (
        (UINTN)(ImageContext->ImageAddress + Section[Index].VirtualAddress),
        Section[Index].Misc.VirtualSize
        );
    }
  }
}
