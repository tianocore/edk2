/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatformDriOverride.h

Abstract:


**/

#ifndef _PLATFORM_DRI_OVERRIDE_H_
#define _PLATFORM_DRI_OVERRIDE_H_

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/PlatDriOverLib.h>

/**
  Retrieves the image handle of the platform override driver for a controller in the system.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImageHandle      On input, a pointer to the previous driver image
                                 handle returned by GetDriver().  On output, a
                                 pointer to the next driver image handle. Passing
                                 in a NULL,  will return the first driver image
                                 handle for ControllerHandle.

  @retval EFI_SUCCESS            The driver override for ControllerHandle was
                                 returned in DriverImageHandle.
  @retval EFI_NOT_FOUND          A driver override for ControllerHandle was not
                                 found.
  @retval EFI_INVALID_PARAMETER  The handle specified by ControllerHandle is not a
                                 valid handle. DriverImageHandle is not a handle
                                 that was returned on a previous  call to
                                 GetDriver().

**/
EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     * DriverImageHandle
  );

/**
  For the use of the ControllerHandle parameter in the GetDriverPath()
  But this API is very difficult to use, so not support.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImagePath        The device path for this Image.
  
  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
GetDriverPath (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                       **DriverImagePath
  );

/**
  For the use of the ControllerHandle parameter in the DriverLoaded()
  But this API is very difficult to use, so not support.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImagePath        The device path for this Image.
  @param  DriverImageHandle      On input, a pointer to the previous driver image
                                 handle returned by GetDriver().  On output, a
                                 pointer to the next driver image handle. Passing
                                 in a NULL,  will return the first driver image
                                 handle for ControllerHandle.
  
  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
DriverLoaded (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          * This,
  IN EFI_HANDLE                                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL                       * DriverImagePath,
  IN EFI_HANDLE                                     DriverImageHandle
  );
#endif
