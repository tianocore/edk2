/** @file
  Main file for Disconnect shell Driver1 function.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-r", TypeFlag},
  {L"-nc", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Disconnect everything.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
DisconnectAll(
  VOID
  )
{
  //
  // Stolen from UEFI 2.3 spec (May 2009 version)
  // Pages 171/172
  // Removed gBS local definition
  //

  //
  // Disconnect All Handles Example
  // The following example recusively disconnects all drivers from all
  // controllers in a platform.
  //
  EFI_STATUS Status;
//  EFI_BOOT_SERVICES *gBS;
  UINTN HandleCount;
  EFI_HANDLE *HandleBuffer;
  UINTN HandleIndex;
  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
    AllHandles,
    NULL,
    NULL,
    &HandleCount,
    &HandleBuffer
   );
  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status = gBS->DisconnectController (
        HandleBuffer[HandleIndex],
        NULL,
        NULL
       );
    }
    gBS->FreePool(HandleBuffer);
    //
    // end of stealing
    //
  }
  return (EFI_SUCCESS);
}

/**
  Function for 'disconnect' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDisconnect (
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
  CONST CHAR16        *Param3;
  EFI_HANDLE          Handle1;
  EFI_HANDLE          Handle2;
  EFI_HANDLE          Handle3;
  UINT64              Intermediate1;
  UINT64              Intermediate2;
  UINT64              Intermediate3;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"disconnect", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetFlag(Package, L"-r")){
      if (ShellCommandLineGetCount(Package) > 1){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"disconnect");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) < 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle, L"disconnect");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
         Status = DisconnectAll ();
         //
         // Reconnect all consoles if -nc is not provided
         //
         if (!ShellCommandLineGetFlag (Package, L"-nc")){
           ShellConnectFromDevPaths (L"ConInDev");
           ShellConnectFromDevPaths (L"ConOutDev");
           ShellConnectFromDevPaths (L"ErrOutDev");
           ShellConnectFromDevPaths (L"ErrOut");
           ShellConnectFromDevPaths (L"ConIn");
           ShellConnectFromDevPaths (L"ConOut");
         }
      }
    } else if (ShellCommandLineGetFlag (Package, L"-nc")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle, L"disconnect");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (ShellCommandLineGetCount(Package) > 4){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"disconnect");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) < 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle, L"disconnect");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // must have between 1 and 3 handles passed in ...
        //
        Param1  = ShellCommandLineGetRawValue(Package, 1);
        Param2  = ShellCommandLineGetRawValue(Package, 2);
        Param3  = ShellCommandLineGetRawValue(Package, 3);
        ShellConvertStringToUint64(Param1, &Intermediate1, TRUE, FALSE);
        Handle1 = Param1!=NULL?ConvertHandleIndexToHandle((UINTN)Intermediate1):NULL;
        ShellConvertStringToUint64(Param2, &Intermediate2, TRUE, FALSE);
        Handle2 = Param2!=NULL?ConvertHandleIndexToHandle((UINTN)Intermediate2):NULL;
        ShellConvertStringToUint64(Param3, &Intermediate3, TRUE, FALSE);
        Handle3 = Param3!=NULL?ConvertHandleIndexToHandle((UINTN)Intermediate3):NULL;

        if (Param1 != NULL && Handle1 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"disconnect", Param1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Param2 != NULL && Handle2 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"disconnect", Param2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Param3 != NULL && Handle3 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"disconnect", Param3);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Handle2 != NULL && EFI_ERROR(gBS->OpenProtocol(Handle2, &gEfiDriverBindingProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
          ASSERT(Param2 != NULL);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_HANDLE_NOT), gShellDriver1HiiHandle, L"disconnect", ShellStrToUintn(Param2), L"driver handle");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          ASSERT(Param1 != NULL);
          Status = gBS->DisconnectController(Handle1, Handle2, Handle3);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_3P_RESULT), gShellDriver1HiiHandle, L"Disconnect", (UINTN)Intermediate1, (UINTN)Intermediate2, (UINTN)Intermediate3, Status);
        }
      }
    }
  }
  if (ShellStatus == SHELL_SUCCESS) {
    if (Status == EFI_SECURITY_VIOLATION) {
      ShellStatus = SHELL_SECURITY_VIOLATION;
    } else if (Status == EFI_INVALID_PARAMETER) {
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (EFI_ERROR(Status)) {
      ShellStatus = SHELL_NOT_FOUND;
    }
  }
  return (ShellStatus);
}
