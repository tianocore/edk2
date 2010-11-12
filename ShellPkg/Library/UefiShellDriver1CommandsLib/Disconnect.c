/** @file
  Main file for Disconnect shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-r", TypeFlag},
  {NULL, TypeMax}
  };

EFI_STATUS
EFIAPI
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
    if (ShellCommandLineGetFlag(Package, L"-r")){
      if (ShellCommandLineGetCount(Package) > 1){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) < 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = DisconnectAll();
      }
    } else {
      if (ShellCommandLineGetCount(Package) > 4){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) < 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // must have between 1 and 3 handles passed in ...
        //
        Param1  = ShellCommandLineGetRawValue(Package, 1);
        Param2  = ShellCommandLineGetRawValue(Package, 2);
        Param3  = ShellCommandLineGetRawValue(Package, 3);
        Handle1 = Param1!=NULL?ConvertHandleIndexToHandle(StrHexToUintn(Param1)):NULL;
        Handle2 = Param2!=NULL?ConvertHandleIndexToHandle(StrHexToUintn(Param2)):NULL;
        Handle3 = Param3!=NULL?ConvertHandleIndexToHandle(StrHexToUintn(Param3)):NULL;

        if (Param1 != NULL && Handle1 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Param2 != NULL && Handle2 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Param3 != NULL && Handle3 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param3);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (EFI_ERROR(gBS->OpenProtocol(Handle1, &gEfiDevicePathProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
          ASSERT(Param1 != NULL);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_HANDLE_NOT), gShellDriver1HiiHandle, StrHexToUintn(Param1), L"controller handle");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Handle2 != NULL && EFI_ERROR(gBS->OpenProtocol(Handle2, &gEfiDriverBindingProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
          ASSERT(Param2 != NULL);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_HANDLE_NOT), gShellDriver1HiiHandle, StrHexToUintn(Param2), L"driver handle");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          ASSERT(Param1 != NULL);
          ASSERT(Param2 != NULL);
          ASSERT(Param3 != NULL);
          Status = gBS->DisconnectController(Handle1, Handle2, Handle3);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_3P_RESULT), gShellDriver1HiiHandle, L"Disconnect", Handle1, Handle2, Handle3, Status);
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
