/**@file
  Win Emulator specific PE/COFF Extra Action Library

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "WinHost.h"

VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  SecPeCoffLoaderRelocateImageExtraAction (ImageContext);
}

VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  SecPeCoffLoaderUnloadImageExtraAction (ImageContext);
}
