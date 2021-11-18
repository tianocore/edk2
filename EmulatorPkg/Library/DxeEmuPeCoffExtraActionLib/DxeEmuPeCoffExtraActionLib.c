/** @file
  Provides services to perform additional actions to relocate and unload
  PE/Coff image for Emu environment specific purpose such as souce level debug.
  This version only works for DXE phase

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/EmuThunk.h>

#include <Library/PeCoffLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffExtraActionLib.h>

//
// Cache of UnixThunk protocol
//
EMU_THUNK_PROTOCOL  *mThunk = NULL;

/**
  The constructor function gets  the pointer of the WinNT thunk functions
  It will ASSERT() if Unix thunk protocol is not installed.

  @retval EFI_SUCCESS   Unix thunk protocol is found and cached.

**/
EFI_STATUS
EFIAPI
DxeEmuPeCoffLibExtraActionConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  //
  // Retrieve EmuThunkProtocol from GUID'ed HOB
  //
  GuidHob = GetFirstGuidHob (&gEmuThunkProtocolGuid);
  ASSERT (GuidHob != NULL);
  mThunk = (EMU_THUNK_PROTOCOL *)(*(UINTN *)(GET_GUID_HOB_DATA (GuidHob)));
  ASSERT (mThunk != NULL);

  return EFI_SUCCESS;
}

/**
  Performs additional actions after a PE/COFF image has been loaded and relocated.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that has already been loaded and relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  if (mThunk != NULL) {
    mThunk->PeCoffRelocateImageExtraAction (ImageContext);
  }
}

/**
  Performs additional actions just before a PE/COFF image is unloaded.  Any resources
  that were allocated by PeCoffLoaderRelocateImageExtraAction() must be freed.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that is being unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  if (mThunk != NULL) {
    mThunk->PeCoffUnloadImageExtraAction (ImageContext);
  }
}
