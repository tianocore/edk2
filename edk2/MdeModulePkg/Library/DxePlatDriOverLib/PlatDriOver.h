/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatDriOver.h

Abstract:


**/

#ifndef _PLAT_DRI_OVER_H_
#define _PLAT_DRI_OVER_H_

#include <PiDxe.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/BusSpecificDriverOverride.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PlatDriOverLib.h>

#include <Guid/OverrideVariable.h>


#define PLATFORM_OVERRIDE_ITEM_SIGNATURE      EFI_SIGNATURE_32('p','d','o','i')
 typedef struct _PLATFORM_OVERRIDE_ITEM{
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  UINT32                                DriverInfoNum;
  EFI_DEVICE_PATH_PROTOCOL              *ControllerDevicePath;
  LIST_ENTRY                            DriverInfoList;  //DRIVER_IMAGE_INFO List
  EFI_HANDLE                            LastReturnedImageHandle;
} PLATFORM_OVERRIDE_ITEM;

#define DRIVER_IMAGE_INFO_SIGNATURE           EFI_SIGNATURE_32('p','d','i','i')
typedef struct _DRIVER_IMAGE_INFO{
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_HANDLE                            ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL              *DriverImagePath;
  BOOLEAN                               UnLoadable;
  BOOLEAN                               UnStartable;
} DRIVER_IMAGE_INFO;

#define DEVICE_PATH_STACK_ITEM_SIGNATURE      EFI_SIGNATURE_32('d','p','s','i')
typedef struct _DEVICE_PATH_STACK_ITEM{
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
} DEVICE_PATH_STACK_ITEM;

EFI_STATUS
EFIAPI
PushDevPathStack (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

EFI_STATUS
EFIAPI
PopDevPathStack (
  OUT  EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  );

BOOLEAN
EFIAPI
CheckExistInStack (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

EFI_STATUS
EFIAPI
UpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      ** DevicePath,
  IN  EFI_GUID                          *FileGuid,
  IN  EFI_HANDLE                        CallerImageHandle
  );

VOID *
GetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );

EFI_STATUS
ConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  );

EFI_STATUS
BdsConnectDeviceByPciClassType (
  UINT8     ClassType,
  UINT8     SubClassCode,
  UINT8     PI,
  BOOLEAN   Recursive
  );

#endif
