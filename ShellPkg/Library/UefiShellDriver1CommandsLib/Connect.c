/** @file
  Main file for connect shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"
#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>

EFI_STATUS
EFIAPI
ConnectControllers (
  IN CONST EFI_HANDLE ControllerHandle,
  IN CONST EFI_HANDLE DriverHandle,
  IN CONST BOOLEAN    Recursive,
  IN CONST BOOLEAN    Output
  ){
  EFI_STATUS Status;
  EFI_HANDLE *ControllerHandleList;
  EFI_HANDLE *DriverHandleList;
  EFI_HANDLE *HandleWalker;

  ControllerHandleList  = NULL;
  Status                = EFI_NOT_FOUND;

  //
  // If we have a single handle to connect make that a 'list'
  //
  if (DriverHandle == NULL) {
    DriverHandleList = NULL;
  } else {
    DriverHandleList = AllocatePool(2*sizeof(EFI_HANDLE));
    DriverHandleList[0] = DriverHandle;
    DriverHandleList[1] = NULL;
  }

  //
  // do we connect all controllers (with a loop) or a single one...
  // This is where we call the gBS->ConnectController function.
  //
  if (ControllerHandle == NULL) {
    ControllerHandleList = GetHandleListByPotocol(&gEfiDevicePathProtocolGuid);
    for (HandleWalker = ControllerHandleList
      ;  HandleWalker != NULL && *HandleWalker != NULL
      ;  HandleWalker++
      ){
      Status = gBS->ConnectController(*HandleWalker, DriverHandleList, NULL, Recursive);
      if (Output) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_CON_RESULT), gShellDriver1HiiHandle, *HandleWalker, Status);
      }
    }
  } else {
    Status = gBS->ConnectController(ControllerHandle, DriverHandleList, NULL, Recursive);
    ASSERT(Output == FALSE);
  }

  //
  // Free any memory we allocated.
  //
  if (ControllerHandleList != NULL) {
    FreePool(ControllerHandleList);
  }
  if (DriverHandleList     != NULL) {
    FreePool(DriverHandleList);
  }
  return (Status);
}

EFI_STATUS
EFIAPI
ConnectFromDevPaths (
  IN CONST CHAR16 *Key
  ){
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathWalker;
  UINTN                     Length;
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;

  DevPath = NULL;
  Length  = 0;

  //
  // Get the DevicePath buffer from the variable...
  //
  Status = gRT->GetVariable((CHAR16*)Key, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &Length, DevPath);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    DevPath = AllocatePool(Length);
    Status = gRT->GetVariable((CHAR16*)Key, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &Length, DevPath);
  }

  //
  // walk the list of devices and connect them
  //
  for (DevPathWalker = DevPath
    ;  DevPathWalker < (DevPath + Length) && !EFI_ERROR(Status) && DevPath != NULL
    ;  DevPathWalker += GetDevicePathSize(DevPathWalker)
    ){
    //
    // get the correct handle from a given device path
    //
    if (StrCmp(Key, L"ConInDev") == 0) {
      Status = gBS->LocateDevicePath((EFI_GUID*)&gEfiConsoleInDeviceGuid, &DevPathWalker, &Handle);
    } else if (StrCmp(Key, L"ConOutDev") == 0) {
      Status = gBS->LocateDevicePath((EFI_GUID*)&gEfiConsoleOutDeviceGuid, &DevPathWalker, &Handle);
    } else {
      Handle = NULL;
      Status = EFI_INVALID_PARAMETER;
      ASSERT(FALSE);
    }
    if (!EFI_ERROR(Status)) {
      Status = ConnectControllers(Handle, NULL, FALSE, FALSE);
    }
  }

  if (DevPath != NULL) {
    FreePool(DevPath);
  }
  return (Status);
}

EFI_STATUS
EFIAPI
ConvertAndConnectControllers (
  IN CONST CHAR16   *StringHandle1,
  IN CONST CHAR16   *StringHandle2 OPTIONAL,
  IN CONST BOOLEAN  Recursive,
  IN CONST BOOLEAN  Output
  ){
  EFI_HANDLE Handle1;
  EFI_HANDLE Handle2;

  //
  // Convert the command line parameters to HANDLES.  They must be in HEX according to spec.
  //
  if (StringHandle1 != NULL) {
    Handle1 = (EFI_HANDLE)StrHexToUintn(StringHandle1);
  } else {
    Handle1 = NULL;
  }
  if (StringHandle2 != NULL) {
    Handle2 = (EFI_HANDLE)StrHexToUintn(StringHandle2);
  } else {
    Handle2 = NULL;
  }

  //
  // if only one is NULL verify it's the proper one...
  //
  if ( (Handle1 == NULL && Handle2 != NULL)
    || (Handle1 != NULL && Handle2 == NULL)
    ){
    //
    // Figure out which one should be NULL and move the handle to the right place.
    // If Handle1 is NULL then test Handle2 and vise versa.
    // The one that DOES has driver binding must be Handle2
    //
    if (Handle1 == NULL) {
      if (EFI_ERROR(gBS->OpenProtocol(Handle2, &gEfiDriverBindingProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        // swap
        Handle1 = Handle2;
        Handle2 = NULL;
      } else {
        // We're all good...
      }
    } else {
      if (EFI_ERROR(gBS->OpenProtocol(Handle1, &gEfiDriverBindingProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        // We're all good...
      } else {
        // swap
        Handle2 = Handle1;
        Handle1 = NULL;
      }
    }
  }

  return (ConnectControllers(Handle1, Handle2, Recursive, Output));
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-c", TypeFlag},
  {L"-r", TypeFlag},
  {NULL, TypeMax}
  };

SHELL_STATUS
EFIAPI
ShellCommandRunConnect (
  VOID                *RESERVED
  ) {
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;

  ShellStatus         = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if EFI_ERROR(Status) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // if more than 2 'value' parameters (plus the name one) or either -r or -c with any value parameters we have too many parameters
    //
    if ((ShellCommandLineGetCount() > 3)
      ||((ShellCommandLineGetFlag(Package, L"-r") != FALSE || ShellCommandLineGetFlag(Package, L"-c") != FALSE) && ShellCommandLineGetCount()!=0)
      ){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-c") != FALSE) {
      //
      // do the conin and conout from EFI variables
      // if the first fails dont 'loose' the error
      //
      Status = ConnectFromDevPaths(L"ConInDev");
      if (EFI_ERROR(Status)) {
        ConnectFromDevPaths(L"ConOutDev");
      } else {
        Status = ConnectFromDevPaths(L"ConOutDev");
      }
      ShellStatus = Status & (~MAX_BIT);
    } else {
      //
      // 0, 1, or 2 specific handles and possibly recursive
      //
      if (ShellCommandLineGetRawValue(Package, 1) != NULL && CommandLibGetHandleValue(StrHexToUintn(ShellCommandLineGetRawValue(Package, 1))) == NULL){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, ShellCommandLineGetRawValue(Package, 1));
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetRawValue(Package, 2) != NULL && CommandLibGetHandleValue(StrHexToUintn(ShellCommandLineGetRawValue(Package, 2))) == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, ShellCommandLineGetRawValue(Package, 2));
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = ConvertAndConnectControllers(ShellCommandLineGetRawValue(Package, 1), ShellCommandLineGetRawValue(Package, 2), ShellCommandLineGetFlag(Package, L"-r"), (BOOLEAN)(ShellCommandLineGetCount()!=0));
        ShellStatus = Status & (~MAX_BIT);
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}

