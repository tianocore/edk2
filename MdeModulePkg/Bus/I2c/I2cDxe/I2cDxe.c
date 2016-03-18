/** @file
  This file implements the entrypoint and unload function for I2C DXE module.

  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "I2cDxe.h"

/**
  The user Entry Point for I2C module. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeI2c(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = InitializeI2cHost ( ImageHandle, SystemTable );
  if ( !EFI_ERROR ( Status ))
  {
    Status = InitializeI2cBus ( ImageHandle, SystemTable );
  }
  return Status;
}

/**
  This is the unload handle for I2C module.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
I2cUnload (
  IN EFI_HANDLE             ImageHandle
  )
{
  EFI_STATUS                        Status;

  //
  //  Disconnect the drivers
  //
  Status = I2cBusUnload ( ImageHandle );
  if ( !EFI_ERROR ( Status )) {
    Status = I2cHostUnload ( ImageHandle );
  }
  return Status;
}
