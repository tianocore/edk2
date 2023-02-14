/** @file
  Copyright (c) 2023, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrePi.h"

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
}
