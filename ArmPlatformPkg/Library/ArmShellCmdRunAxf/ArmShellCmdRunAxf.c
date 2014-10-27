/** @file
*
*  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/ArmShellCmdLib.h>

#include "ArmShellCmdRunAxf.h"

EFI_HANDLE gRunAxfHiiHandle = NULL;

#define RUNAXF_HII_GUID \
  { \
  0xf5a6413b, 0x78d5, 0x448e, { 0xa2, 0x15, 0x22, 0x82, 0x8e, 0xbc, 0x61, 0x61 } \
  }

EFI_GUID gRunAxfHiiGuid = RUNAXF_HII_GUID;

static EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL mShellDynCmdProtocolRunAxf = {
    L"runaxf",                             // *CommandName
    ShellDynCmdRunAxfHandler,              // Handler
    ShellDynCmdRunAxfGetHelp               // GetHelp
};

EFI_STATUS
ShellDynCmdRunAxfInstall (
  IN EFI_HANDLE             ImageHandle
  )
{
  EFI_STATUS Status;

  // Register our shell command
  Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                 &gEfiShellDynamicCommandProtocolGuid,
                 &mShellDynCmdProtocolRunAxf,
                 NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Load the manual page for our command
  //
  // 3rd parameter 'HII strings array' must be name of .uni strings file
  // followed by 'Strings', e.g. mycommands.uni must be specified as
  // 'mycommandsStrings' because the build Autogen process defines this as a
  // string array for the strings in your .uni file.  Examine your Build folder
  // under your package's DEBUG folder and you will find it defined in a
  // xxxStrDefs.h file.
  //
  gRunAxfHiiHandle = HiiAddPackages (&gRunAxfHiiGuid, ImageHandle,
                                     ArmShellCmdRunAxfStrings, NULL);
  if (gRunAxfHiiHandle == NULL) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
ShellDynCmdRunAxfUninstall (
  IN EFI_HANDLE             ImageHandle
  )
{

  EFI_STATUS Status;

  if (gRunAxfHiiHandle != NULL) {
    HiiRemovePackages (gRunAxfHiiHandle);
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (ImageHandle,
                 &gEfiShellDynamicCommandProtocolGuid,
                 &mShellDynCmdProtocolRunAxf,
                 NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
