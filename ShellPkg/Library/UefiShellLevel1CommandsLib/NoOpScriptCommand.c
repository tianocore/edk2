/** @file
  Main file for else and endif shell level 1 functions.  Does nothing really...

  Copyright (c) 2009-2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel1CommandsLib.h"

SHELL_STATUS
EFIAPI
ShellCommandRunNoOpScriptCommand (
  VOID                          *RESERVED
  )
{
  EFI_STATUS Status;
  //
  // ASSERT that we can init...
  //
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // We best be in a script...
  //
  ASSERT(gEfiShellProtocol->BatchIsActive());

  //
  // Do nothing...
  //
  return (SHELL_SUCCESS);
}

