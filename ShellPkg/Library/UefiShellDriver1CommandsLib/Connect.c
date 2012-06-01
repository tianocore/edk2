/** @file
  Main file for connect shell Driver1 function.

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
  Connect controller(s) and driver(s).

  @param[in] ControllerHandle     The handle to the controller.  Should have driver binding on it.
  @param[in] DriverHandle         The handle to the driver.  Should have driver binding.
  @param[in] Recursive            TRUE to connect recursively, FALSE otherwise.
  @param[in] Output               TRUE to have info on the screen, FALSE otherwise.
  @param[in] AlwaysOutput         Override Output for errors.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
EFIAPI
ConnectControllers (
  IN CONST EFI_HANDLE ControllerHandle OPTIONAL,
  IN CONST EFI_HANDLE DriverHandle OPTIONAL,
  IN CONST BOOLEAN    Recursive,
  IN CONST BOOLEAN    Output,
  IN CONST BOOLEAN    AlwaysOutput
  )
{
  EFI_STATUS Status;
  EFI_STATUS Status2;
  EFI_HANDLE *ControllerHandleList;
  EFI_HANDLE *DriverHandleList;
  EFI_HANDLE *HandleWalker;

  ControllerHandleList  = NULL;
  Status                = EFI_NOT_FOUND;
  Status2               = EFI_NOT_FOUND;

  //
  // If we have a single handle to connect make that a 'list'
  //
  if (DriverHandle == NULL) {
    DriverHandleList = NULL;
  } else {
    DriverHandleList = AllocateZeroPool(2*sizeof(EFI_HANDLE));
    if (DriverHandleList == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    DriverHandleList[0] = DriverHandle;
    DriverHandleList[1] = NULL;
  }

  //
  // do we connect all controllers (with a loop) or a single one...
  // This is where we call the gBS->ConnectController function.
  //
  if (ControllerHandle == NULL) {
    ControllerHandleList = GetHandleListByProtocol(&gEfiDevicePathProtocolGuid);
    for (HandleWalker = ControllerHandleList
      ;  HandleWalker != NULL && *HandleWalker != NULL
      ;  HandleWalker++
     ){
      Status = gBS->ConnectController(*HandleWalker, DriverHandleList, NULL, Recursive);
      if (!EFI_ERROR(Status)) {
        Status2 = EFI_SUCCESS;
      }
      if ((Output && !EFI_ERROR(Status)) || AlwaysOutput) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_HANDLE_RESULT), gShellDriver1HiiHandle, L"Connect", ConvertHandleToHandleIndex(*HandleWalker), Status);
      }
    }
  } else {
    Status = gBS->ConnectController(ControllerHandle, DriverHandleList, NULL, Recursive);
    if (!EFI_ERROR(Status)) {
      Status2 = EFI_SUCCESS;
    }
    if ((Output && !EFI_ERROR(Status)) || AlwaysOutput) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_HANDLE_RESULT), gShellDriver1HiiHandle, L"Connect", ConvertHandleToHandleIndex(ControllerHandle), Status);
    }
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
  return (Status2);
}

/**
  Do a connect from an EFI variable via it's key name.

  @param[in] Key      The name of the EFI Variable.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
ConnectFromDevPaths (
  IN CONST CHAR16 *Key
  )
{
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
    DevPath = AllocateZeroPool(Length);
    Status = gRT->GetVariable((CHAR16*)Key, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &Length, DevPath);
  }

  Status = EFI_NOT_FOUND;
  //
  // walk the list of devices and connect them
  //
  for (DevPathWalker = DevPath
    ;  DevPathWalker < (DevPath + Length) && EFI_ERROR(Status) && DevPath != NULL
    ;  DevPathWalker += GetDevicePathSize(DevPathWalker)
   ){
    //
    // get the correct handle from a given device path
    //
    if ((StrCmp(Key, L"ConInDev") == 0)
      ||(StrCmp(Key, L"ConIn") == 0)
    ){
      Status = gBS->LocateDevicePath((EFI_GUID*)&gEfiConsoleInDeviceGuid, &DevPathWalker, &Handle);
      if (!EFI_ERROR(Status)) {
        Status = ConnectControllers(NULL, Handle, FALSE, TRUE, FALSE);
      }
    } else if ((StrCmp(Key, L"ConOutDev") == 0) 
            || (StrCmp(Key, L"ConErrDev") == 0) 
            || (StrCmp(Key, L"ConOut")    == 0) 
            || (StrCmp(Key, L"ConErr")    == 0)
            ){
      Status = gBS->LocateDevicePath((EFI_GUID*)&gEfiConsoleOutDeviceGuid, &DevPathWalker, &Handle);
      if (!EFI_ERROR(Status)) {
        Status = ConnectControllers(NULL, Handle, FALSE, TRUE, FALSE);
      }
    }
  }

  if (DevPath != NULL) {
    FreePool(DevPath);
  }
  return (Status);
}

/**
  Convert the handle identifiers from strings and then connect them.

  One of them should have driver binding and either can be NULL.

  @param[in] Handle1            The first handle.
  @param[in] Handle2            The second handle.
  @param[in] Recursive          TRUE to do connect recursively. FALSE otherwise.
  @param[in] Output             TRUE to have output to screen. FALSE otherwise.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
ConvertAndConnectControllers (
  IN EFI_HANDLE     *Handle1 OPTIONAL,
  IN EFI_HANDLE     *Handle2 OPTIONAL,
  IN CONST BOOLEAN  Recursive,
  IN CONST BOOLEAN  Output
  )
{
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

  return (ConnectControllers(Handle1, Handle2, Recursive, Output, (BOOLEAN)(Handle2 != NULL && Handle1 != NULL)));
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-c", TypeFlag},
  {L"-r", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'connect' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunConnect (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST CHAR16        *Param1;
  CONST CHAR16        *Param2;
  UINTN               Count;
  EFI_HANDLE          Handle1;
  EFI_HANDLE          Handle2;
  UINT64              Intermediate;

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
  if (EFI_ERROR(Status)) {
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
    Count = (gInReconnect?0x4:0x3);
    if ((ShellCommandLineGetCount(Package) > Count)
      ||((ShellCommandLineGetFlag(Package, L"-r") || ShellCommandLineGetFlag(Package, L"-c")) && ShellCommandLineGetCount(Package)>1)
      ||(ShellCommandLineGetFlag(Package, L"-r") && ShellCommandLineGetFlag(Package, L"-c") )
     ){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-c")) {
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
      if (EFI_ERROR(Status)) {
        ConnectFromDevPaths(L"ConErrDev");
      } else {
        Status = ConnectFromDevPaths(L"ConErrDev");
      }
      if (EFI_ERROR(Status)) {
        ConnectFromDevPaths(L"ConErr");
      } else {
        Status = ConnectFromDevPaths(L"ConErr");
      }
      if (EFI_ERROR(Status)) {
        ConnectFromDevPaths(L"ConIn");
      } else {
        Status = ConnectFromDevPaths(L"ConIn");
      }
      if (EFI_ERROR(Status)) {
        ConnectFromDevPaths(L"ConOut");
      } else {
        Status = ConnectFromDevPaths(L"ConOut");
      }
      if (EFI_ERROR(Status)) {
        ShellStatus = SHELL_DEVICE_ERROR;
      }
    } else {
      //
      // 0, 1, or 2 specific handles and possibly recursive
      //
      Param1  = ShellCommandLineGetRawValue(Package, 1);
      Param2  = ShellCommandLineGetRawValue(Package, 2);
      Count   = ShellCommandLineGetCount(Package);

      if (Param1 != NULL) {
        Status  = ShellConvertStringToUint64(Param1, &Intermediate, TRUE, FALSE);
        Handle1 = ConvertHandleIndexToHandle((UINTN)Intermediate);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      } else {
        Handle1 = NULL;
      }

      if (Param2 != NULL) {
        Status  = ShellConvertStringToUint64(Param2, &Intermediate, TRUE, FALSE);
        Handle2 = ConvertHandleIndexToHandle((UINTN)Intermediate);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      } else {
        Handle2 = NULL;
      }
      
      if (ShellStatus == SHELL_SUCCESS) {
        if (Param1 != NULL && Handle1 == NULL){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Param2 != NULL && Handle2 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Handle2 != NULL && Handle1 != NULL && EFI_ERROR(gBS->OpenProtocol(Handle2, &gEfiDriverBindingProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Status = ConvertAndConnectControllers(Handle1, Handle2, ShellCommandLineGetFlag(Package, L"-r"), (BOOLEAN)(Count!=0));
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CONNECT_NONE), gShellDriver1HiiHandle);
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}

