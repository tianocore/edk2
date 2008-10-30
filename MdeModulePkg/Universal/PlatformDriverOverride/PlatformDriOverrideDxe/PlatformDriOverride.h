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

#ifndef PLATFORM_DRI_OVERRIDE_H_
#define PLATFORM_DRI_OVERRIDE_H_

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/PlatDriOverLib.h>

EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     * DriverImageHandle
  );

EFI_STATUS
EFIAPI
GetDriverPath (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                       **DriverImagePath
  );

EFI_STATUS
EFIAPI
DriverLoaded (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          * This,
  IN EFI_HANDLE                                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL                       * DriverImagePath,
  IN EFI_HANDLE                                     DriverImageHandle
  );
#endif
