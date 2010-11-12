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
  IN UINTN        *Parents,
  IN UINTN        *Devices,
  IN UINTN        *Children,
  OUT CHAR16      **Name,
  IN CONST CHAR8  *Language
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    *HandleBuffer;
  UINTN         Count;

  if (TheHandle == NULL 
    || Type == NULL
    || Cfg == NULL
    || Diag == NULL
    || Parents == NULL
    || Devices == NULL
    || Children == NULL
    || Name == NULL ) {
    return (EFI_INVALID_PARAMETER);
  }

  *Cfg          = FALSE;
  *Diag         = FALSE;
  *Children     = 0;
  *Parents      = 0;
  *Devices      = 0;
  *Type         = L' ';
  *Name         = CHAR_NULL;
  HandleBuffer  = NULL;
  Status        = EFI_SUCCESS;

  gEfiShellProtocol->GetDeviceName(TheHandle, EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, Name);

  Status = ParseHandleDatabaseForChildControllers(TheHandle, Children, NULL);
  if (!EFI_ERROR(Status)) {
    Status = PARSE_HANDLE_DATABASE_PARENTS(TheHandle, Parents, NULL);
    if (/*!EFI_ERROR(Status) && */Parents != NULL && Children != NULL) {
      if (*Parents == 0) {
        *Type = L'R';
      } else if (*Children > 0) {
        *Type = L'B';
      } else {
        *Type = L'D';
      }
    }
  }
  Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS(TheHandle, Devices, &HandleBuffer);
  if (!EFI_ERROR(Status) && Devices != NULL && HandleBuffer != NULL) {
    for (Count = 0 ; Count < *Devices ; Count++) {
      if (!EFI_ERROR(gBS->OpenProtocol(HandleBuffer[Count], &gEfiDriverConfigurationProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        *Cfg = TRUE;
      }
      if (!EFI_ERROR(gBS->OpenProtocol(HandleBuffer[Count], &gEfiDriverDiagnosticsProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        *Diag = TRUE;
      }
      if (!EFI_ERROR(gBS->OpenProtocol(HandleBuffer[Count], &gEfiDriverDiagnostics2ProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        *Diag = TRUE;
      }
    }
    SHELL_FREE_NON_NULL(HandleBuffer);
  }

  return (Status);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-l", TypeValue},
  {NULL, TypeMax}
  };

/**
  Function for 'devices' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDevices (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
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
  UINTN               Parents;
  UINTN               Devices;
  UINTN               Children;
  CHAR16              *Name;
  CONST CHAR16        *Lang;

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
      Lang = ShellCommandLineGetValue(Package, L"-l");
      if (Lang != NULL) {
        Language = AllocateZeroPool(StrSize(Lang));
        AsciiSPrint(Language, StrSize(Lang), "%S", Lang);
      } else if (!ShellCommandLineGetFlag(Package, L"-l")){
        ASSERT(Language == NULL);
//        Language = AllocateZeroPool(10);
//        AsciiSPrint(Language, 10, "en-us");
      } else {
        ASSERT(Language == NULL);
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"-l");
        ShellCommandLineFreeVarList (Package);
        return (SHELL_INVALID_PARAMETER);
      }


      //
      // Print Header
      //
      ShellPrintHiiEx(-1, -1, Language, STRING_TOKEN (STR_DEVICES_HEADER_LINES), gShellDriver1HiiHandle);

      //
      // loop through each handle
      //
      HandleList = GetHandleListByProtocol(NULL);
      ASSERT(HandleList != NULL);
      for (HandleListWalker = HandleList
        ;  HandleListWalker != NULL && *HandleListWalker != NULL /*&& !EFI_ERROR(Status)*/
        ;  HandleListWalker++
       ){

        //
        // get all the info on each handle
        //
        Name = NULL;
        Status = GetDeviceHandleInfo(*HandleListWalker, &Type, &Cfg, &Diag, &Parents, &Devices, &Children, &Name, Language);
        if (Parents != 0 || Devices != 0 || Children != 0) {
          ShellPrintHiiEx(
            -1,
            -1,
            Language,
            STRING_TOKEN (STR_DEVICES_ITEM_LINE),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex(*HandleListWalker),
            Type,
            Cfg?L'X':L'-',
            Diag?L'X':L'-',
            Parents,
            Devices,
            Children,
            Name!=NULL?Name:L"<UNKNOWN>");
        }
        if (Name != NULL) {
          FreePool(Name);
        }
      }

      if (HandleList != NULL) {
        FreePool(HandleList);
      }
 
    }
    SHELL_FREE_NON_NULL(Language);
    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}

