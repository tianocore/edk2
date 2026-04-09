/** @file
Utility routines used by boot maintenance modules.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

/**
  Function deletes the variable specified by VarName and VarGuid.

  @param VarName           A Null-terminated Unicode string that is
                           the name of the vendor's variable.

  @param VarGuid           A unique identifier for the vendor.

  @retval  EFI_SUCCESS           The variable was found and removed
  @retval  EFI_UNSUPPORTED       The variable store was inaccessible
  @retval  EFI_NOT_FOUND         The variable was not found

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16    *VarName,
  IN EFI_GUID  *VarGuid
  )
{
  return gRT->SetVariable (
                VarName,
                VarGuid,
                0,
                0,
                NULL
                );
}

/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  UINTN  Count;
  UINTN  Size;

  Count = 0;
  while (GetNextDevicePathInstance (&DevicePath, &Size) != NULL) {
    Count += 1;
  }

  return Count;
}

/**
  Get a string from the Data Hub record based on
  a device path.

  @param DevPath         The device Path.

  @return A string located from the Data Hub records based on
          the device path.
  @retval NULL  If failed to get the String from Data Hub.

**/
UINT16 *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
{
  return NULL;
}
