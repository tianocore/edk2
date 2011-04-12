/** @file
  Main file for Reconnect shell Driver1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

/**
  Function for 'reconnect' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunReconnect (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SHELL_STATUS        ShellStatus;

  gInReconnect = TRUE;

  ShellStatus = ShellCommandRunDisconnect(ImageHandle, SystemTable);
  if (ShellStatus == SHELL_SUCCESS) {
    ShellStatus = ShellCommandRunConnect(ImageHandle, SystemTable);
  }

  gInReconnect = FALSE;

  return (ShellStatus);
}
