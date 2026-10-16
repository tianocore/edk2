/** @file
  Logo Image ID retrieval - FFS variant.

  Returns 0 (no HII logo) for LogoFfsDxe when using FFS-based custom logos.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiInternalFormRepresentation.h>

/**
  Get the logo image ID for the current build configuration.

  Returns 0 (no HII logo) for FFS mode.

  @retval 0  No HII logo (FFS mode).

**/
EFI_IMAGE_ID
GetLogoImageId (
  VOID
  )
{
  return 0;
}
