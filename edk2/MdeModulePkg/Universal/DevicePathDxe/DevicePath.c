/** @file
  Device Path Driver to produce DevPathUtilities Protocol, DevPathFromText Protocol
  and DevPathToText Protocol.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DevicePath.h"

EFI_HANDLE  mDevicePathHandle = NULL;

GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DEVICE_PATH_UTILITIES_PROTOCOL mDevicePathUtilities = {
  GetDevicePathSizeProtocolInterface,
  DuplicateDevicePathProtocolInterface,
  AppendDevicePathProtocolInterface,
  AppendDeviceNodeProtocolInterface,
  AppendDevicePathInstanceProtocolInterface,
  GetNextDevicePathInstanceProtocolInterface,
  IsDevicePathMultiInstanceProtocolInterface,
  CreateDeviceNodeProtocolInterface
};

GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DEVICE_PATH_TO_TEXT_PROTOCOL   mDevicePathToText = {
  ConvertDeviceNodeToText,
  ConvertDevicePathToText
};

GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL mDevicePathFromText = {
  ConvertTextToDeviceNode,  
  ConvertTextToDevicePath  
};

GLOBAL_REMOVE_IF_UNREFERENCED const EFI_GUID mEfiDevicePathMessagingUartFlowControlGuid = DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL;
GLOBAL_REMOVE_IF_UNREFERENCED const EFI_GUID mEfiDevicePathMessagingSASGuid             = DEVICE_PATH_MESSAGING_SAS;

EFI_STATUS
EFIAPI
DevicePathEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

  Routine Description:
    Entry point for EFI drivers.

  Arguments:
   ImageHandle - EFI_HANDLE
   SystemTable - EFI_SYSTEM_TABLE

  Returns:
    EFI_SUCCESS
    others

--*/
{
  EFI_STATUS  Status;
 
  Status = EFI_UNSUPPORTED;
  if (FeaturePcdGet (PcdDevicePathSupportDevicePathToText)) {
    if (FeaturePcdGet (PcdDevicePathSupportDevicePathFromText)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      &gEfiDevicePathToTextProtocolGuid,    &mDevicePathToText,
                      &gEfiDevicePathFromTextProtocolGuid,  &mDevicePathFromText,
                      NULL
                      );
    } else {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      &gEfiDevicePathToTextProtocolGuid,    &mDevicePathToText,
                      NULL
                      );
    }
  } else {
    if (FeaturePcdGet (PcdDevicePathSupportDevicePathFromText)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      &gEfiDevicePathFromTextProtocolGuid,  &mDevicePathFromText,
                      NULL
                      );
    } else {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      NULL
                      );
    }
  }
  return Status;
}
