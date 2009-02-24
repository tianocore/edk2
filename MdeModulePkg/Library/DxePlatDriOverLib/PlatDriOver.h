/** @file
  Internal include file for Platform Driver Override Library implementation.
  
  Copyright (c) 2007 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLAT_DRI_OVER_H_
#define _PLAT_DRI_OVER_H_

#include <PiDxe.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
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
#include <Library/PlatformDriverOverrideLib.h>

#include <Guid/OverrideVariable.h>
#include <VariableFormat.h>


#define PLATFORM_OVERRIDE_ITEM_SIGNATURE      SIGNATURE_32('p','d','o','i')
 typedef struct _PLATFORM_OVERRIDE_ITEM {
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  UINT32                                DriverInfoNum;
  EFI_DEVICE_PATH_PROTOCOL              *ControllerDevicePath;
  ///
  /// List of DRIVER_IMAGE_INFO
  ///
  LIST_ENTRY                            DriverInfoList;
  EFI_HANDLE                            LastReturnedImageHandle;
} PLATFORM_OVERRIDE_ITEM;

#define DRIVER_IMAGE_INFO_SIGNATURE           SIGNATURE_32('p','d','i','i')
typedef struct _DRIVER_IMAGE_INFO {
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_HANDLE                            ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL              *DriverImagePath;
  BOOLEAN                               UnLoadable;
  BOOLEAN                               UnStartable;
} DRIVER_IMAGE_INFO;

#define DEVICE_PATH_STACK_ITEM_SIGNATURE      SIGNATURE_32('d','p','s','i')
typedef struct _DEVICE_PATH_STACK_ITEM{
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
} DEVICE_PATH_STACK_ITEM;

/**
  Push a controller device path into a globle device path list.

  @param  DevicePath     The controller device path to push into stack

  @retval EFI_SUCCESS    Device path successfully pushed into the stack.

**/
EFI_STATUS
EFIAPI
PushDevPathStack (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

/**
  Pop a controller device path from a globle device path list

  @param  DevicePath     The controller device path popped from stack

  @retval EFI_SUCCESS    Controller device path successfully popped.
  @retval EFI_NOT_FOUND  Stack is empty.

**/
EFI_STATUS
EFIAPI
PopDevPathStack (
  OUT  EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  );

/**
  Check whether a controller device path is in a globle device path list

  @param  DevicePath     The controller device path to check

  @retval TRUE           DevicePath exists in the stack.
  @retval FALSE          DevicePath does not exist in the stack.

**/
BOOLEAN
EFIAPI
CheckExistInStack (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

/**
  Update the FV file device path if it is not valid.

  According to a file GUID, check a Fv file device path is valid. If it is invalid,
  try to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this funciton
  could promise the Fv file device path is right.

  @param  DevicePath               On input, the FV file device path to check
                                   On output, the updated valid FV file device path
  @param  FileGuid                 The FV file GUID
  @param  CallerImageHandle        Image handle of the caller

  @retval EFI_INVALID_PARAMETER    the input DevicePath or FileGuid is invalid
                                   parameter
  @retval EFI_UNSUPPORTED          the input DevicePath does not contain FV file
                                   GUID at all
  @retval EFI_ALREADY_STARTED      the input DevicePath has pointed to FV file, it
                                   is valid
  @retval EFI_SUCCESS              Successfully updated the invalid DevicePath,
                                   and return the updated device path in DevicePath

**/
EFI_STATUS
EFIAPI
UpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      **DevicePath,
  IN  EFI_GUID                          *FileGuid,
  IN  EFI_HANDLE                        CallerImageHandle
  );

/**
  Gets the data and size of a variable.

  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure return NULL.

  @param  Name                     String part of EFI variable name
  @param  VendorGuid               GUID part of EFI variable name
  @param  VariableSize             Returns the size of the EFI variable that was
                                   read

  @return Dynamically allocated memory that contains a copy of the EFI variable.
          Caller is responsible freeing the buffer.
  @retval NULL                     Variable was not read

**/
VOID *
EFIAPI
GetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );

/**
  Connect to the handle to a device on the device path.

  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created success, then still give one chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect      The device path which will be connected, it can
                                   be a multi-instance device path

  @retval EFI_SUCCESS              All handles associate with every device path
                                   node have been created
  @retval EFI_OUT_OF_RESOURCES     There is no resource to create new handles
  @retval EFI_NOT_FOUND            Create the handle associate with one device
                                   path node failed

**/
EFI_STATUS
EFIAPI
ConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  );

#endif
