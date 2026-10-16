/** @file
  Logo Image ID retrieval API.

  Provides platform-specific logo image ID based on build configuration.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiInternalFormRepresentation.h>

/**
  Get the logo image ID for the current build configuration.

  Returns the HII image ID for embedded logo (HII mode) or 0 for FFS mode.

  @retval IMAGE_TOKEN(IMG_LOGO)  HII embedded logo image ID (LogoDxe variant).
  @retval 0                      No HII logo (LogoFfsDxe variant).

**/
EFI_IMAGE_ID
GetLogoImageId (
  VOID
  );
