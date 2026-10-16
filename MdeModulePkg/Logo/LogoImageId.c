/** @file
  Logo Image ID retrieval - HII variant.

  Returns the embedded HII logo image ID for LogoDxe.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiInternalFormRepresentation.h>

/**
  Get the logo image ID for the current build configuration.

  Returns the HII image ID for embedded logo (HII mode).

  @retval IMAGE_TOKEN(IMG_LOGO)  HII embedded logo image ID.

**/
EFI_IMAGE_ID
GetLogoImageId (
  VOID
  )
{
  return IMAGE_TOKEN (IMG_LOGO);
}
