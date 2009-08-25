/**@file

Copyright (c) 2006 - 2009, Intel Corporation
Portions copyright (c) 2008-2009 Apple Inc. All rights reserved.
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiUnixPeCoffExtraActionLib.c

Abstract:

  Provides services to perform additional actions to relocate and unload
  PE/Coff image for UNIX environment specific purpose such as souce level debug.
  This version only works for PEI phase


**/
#include <PiPei.h>
#include <Ppi/UnixThunk.h>

#include <Library/PeCoffLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffExtraActionLib.h>

//
// Cache of UnixThunk protocol 
//
EFI_UNIX_THUNK_PROTOCOL   *mUnix = NULL;

/**
  The function caches the pointer of the Unix thunk functions
  It will ASSERT() if Unix thunk ppi is not installed.

  @retval EFI_SUCCESS   WinNT thunk protocol is found and cached.

**/
EFI_STATUS
EFIAPI
UnixPeCoffGetUnixThunkStucture (
  )
{
	PEI_UNIX_THUNK_PPI  *UnixThunkPpi;
  EFI_STATUS        Status;

  
  //
  // Locate Unix ThunkPpi for retrieving standard output handle
  //
  Status = PeiServicesLocatePpi (
              &gPeiUnixThunkPpiGuid,
              0,
              NULL,
              (VOID **) &UnixThunkPpi
              );

  ASSERT_EFI_ERROR (Status);

  mUnix  = (EFI_UNIX_THUNK_PROTOCOL *) UnixThunkPpi->UnixThunk ();

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
  if (mUnix == NULL) {
    UnixPeCoffGetUnixThunkStucture ();
  }
  mUnix->PeCoffRelocateImageExtraAction (ImageContext);
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
  if (mUnix == NULL) {
    UnixPeCoffGetUnixThunkStucture ();
  }
  mUnix->PeCoffUnloadImageExtraAction (ImageContext);
}
