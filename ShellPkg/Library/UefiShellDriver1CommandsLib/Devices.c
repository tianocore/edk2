/** @file
  Main file for devices shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

EFI_STATUS
EFIAPI
GetDeviceHandleInfo (
  IN EFI_HANDLE   TheHandle,
  IN CHAR16       *Type,
  IN BOOLEAN      *Cfg,
  IN BOOLEAN      *Diag,
  IN UINT8        *Parents,
  IN UINT8        *Devices,
  IN UINT8        *Children,
  OUT CHAR16      **Name,
  IN CONST CHAR8  *Language
  ){
  *Name = NULL;

  gEfiShellProtocol->GetDeviceName(TheHandle, EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, Name);













  return (EFI_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-l", TypeValue},
  {NULL, TypeMax}
  };

SHELL_STATUS
EFIAPI
ShellCommandRunDevices (
  VOID                *RESERVED
  ) {
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CHAR8               *Language;
  EFI_HANDLE          *HandleList;
  EFI_HANDLE          *HandleListWalker;
  CHAR16              Type;
  BOOLEAN             Cfg;
  BOOLEAN             Diag;
  UINT8               Parents;
  UINT8               Devices;
  UINT8               Children;
  CHAR16              *Name;

  ShellStatus         = SHELL_SUCCESS;
  Language            = NULL;

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
    // if more than 0 'value' parameters  we have too many parameters
    //
    if (ShellCommandLineGetRawValue(Package, 1) != NULL){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // get the language if necessary
      //
      if (ShellCommandLineGetFlag(Package, L"-l") != FALSE) {
        Language = AllocateZeroPool(StrSize(ShellCommandLineGetValue(Package, L"-l")));
        AsciiSPrint(Language, StrSize(ShellCommandLineGetValue(Package, L"-l")), "%S", ShellCommandLineGetValue(Package, L"-l"));
      }

      //
      // Print Header
      //
      ShellPrintHiiEx(-1, -1, Language, STRING_TOKEN (STR_DEVICES_HEADER_LINES), gShellDriver1HiiHandle);

      //
      // loop through each handle
      //
      HandleList = GetHandleListByPotocol(NULL);
      ASSERT(HandleList != NULL);
      for (HandleListWalker = HandleList
        ;  HandleListWalker != NULL && *HandleListWalker != NULL && !EFI_ERROR(Status)
        ;  HandleListWalker++
        ){
        //
        // get all the info on each handle
        //
        Status = GetDeviceHandleInfo(*HandleListWalker, &Type, &Cfg, &Diag, &Parents, &Devices, &Children, &Name, Language);
        if (Parents != 0 || Devices != 0 || Children != 0) {
          ShellPrintHiiEx(
            -1,
            -1,
            Language,
            STRING_TOKEN (STR_DEVICES_ITEM_LINE),
            gShellDriver1HiiHandle,
            *HandleListWalker,
            Type,
            Cfg!=FALSE?L'X':L'-',
            Diag!=FALSE?L'X':L'-',
            Parents,
            Devices,
            Children,
            Name);
        }
        if (Name != NULL) {
          FreePool(Name);
        }
      }

      if (HandleList != NULL) {
        FreePool(HandleList);
      }
    }
    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}

