/** @file
  This driver effectuates OVMF's platform configuration settings and exposes
  them via HII.

  Copyright (C) 2014, Red Hat, Inc.
  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "PlatformConfig.h"

/**
  Load and execute the platform configuration.

  @retval EFI_SUCCESS            Configuration loaded and executed.
  @return                        Status codes from PlatformConfigLoad().
**/
STATIC
EFI_STATUS
EFIAPI
ExecutePlatformConfig (
  VOID
  )
{
  EFI_STATUS      Status;
  PLATFORM_CONFIG PlatformConfig;
  UINT64          OptionalElements;

  Status = PlatformConfigLoad (&PlatformConfig, &OptionalElements);
  if (EFI_ERROR (Status)) {
    DEBUG (((Status == EFI_NOT_FOUND) ? EFI_D_VERBOSE : EFI_D_ERROR,
      "%a: failed to load platform config: %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (OptionalElements & PLATFORM_CONFIG_F_GRAPHICS_RESOLUTION) {
    //
    // Pass the preferred resolution to GraphicsConsoleDxe via dynamic PCDs.
    //
    PcdSet32 (PcdVideoHorizontalResolution,
      PlatformConfig.HorizontalResolution);
    PcdSet32 (PcdVideoVerticalResolution,
      PlatformConfig.VerticalResolution);
  }

  return EFI_SUCCESS;
}


/**
  Entry point for this driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to SystemTable.

  @retval EFI_SUCESS            Driver has loaded successfully.

**/
EFI_STATUS
EFIAPI
PlatformInit (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ExecutePlatformConfig ();
  return EFI_SUCCESS;
}

/**
  Unload the driver.

  @param[in]  ImageHandle  Handle that identifies the image to evict.

  @retval EFI_SUCCESS  The image has been unloaded.
**/
EFI_STATUS
EFIAPI
PlatformUnload (
  IN  EFI_HANDLE  ImageHandle
  )
{
  return EFI_SUCCESS;
}
