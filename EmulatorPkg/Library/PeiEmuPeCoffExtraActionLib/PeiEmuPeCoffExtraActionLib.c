/** @file
  Provides services to perform additional actions to relocate and unload
  PE/Coff image for Emu environment specific purpose such as souce level debug.
  This version only works for PEI phase

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiPei.h>
#include <Ppi/EmuThunk.h>
#include <Protocol/EmuThunk.h>

#include <Library/PeCoffLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/EmuMagicPageLib.h>

//
// Cache of UnixThunk protocol
//
EMU_THUNK_PROTOCOL   *mThunk = NULL;

/**
  The function caches the pointer of the Unix thunk functions
  It will ASSERT() if Unix thunk ppi is not installed.

  @retval EFI_SUCCESS   WinNT thunk protocol is found and cached.

**/
EFI_STATUS
EFIAPI
EmuPeCoffGetThunkStucture (
  )
{
  EMU_THUNK_PPI     *ThunkPpi;
  EFI_STATUS        Status;


  //
  // Locate Unix ThunkPpi for retrieving standard output handle
  //
  Status = PeiServicesLocatePpi (
              &gEmuThunkPpiGuid,
              0,
              NULL,
              (VOID **) &ThunkPpi
              );
  ASSERT_EFI_ERROR (Status);

  EMU_MAGIC_PAGE()->Thunk = (EMU_THUNK_PROTOCOL *) ThunkPpi->Thunk ();

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
  if (EMU_MAGIC_PAGE()->Thunk == NULL) {
    EmuPeCoffGetThunkStucture ();
  }
    EMU_MAGIC_PAGE()->Thunk->PeCoffRelocateImageExtraAction (ImageContext);
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
  if (EMU_MAGIC_PAGE()->Thunk == NULL) {
    EmuPeCoffGetThunkStucture ();
  }
  EMU_MAGIC_PAGE()->Thunk->PeCoffUnloadImageExtraAction (ImageContext);
}
