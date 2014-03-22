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
