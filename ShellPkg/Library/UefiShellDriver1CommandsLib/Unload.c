/** @file
  Main file for Unload shell Driver1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

/**
  Function to dump LoadedImage info from TheHandle.

  @param[in] TheHandle              The handle to dump info from.

  @retval EFI_SUCCESS               The info was dumped.
  @retval EFI_INVALID_PARAMETER     The handle did not have LoadedImage
**/
EFI_STATUS
DumpLoadedImageProtocolInfo (
  IN EFI_HANDLE  TheHandle
  )
{
  CHAR16  *TheString;

  TheString = GetProtocolInformationDump (TheHandle, &gEfiLoadedImageProtocolGuid, TRUE);
  if (TheString == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  ShellPrintDefaultEx (L"%s", TheString);

  SHELL_FREE_NON_NULL (TheString);

  return (EFI_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-n",       TypeFlag },
  { L"-v",       TypeFlag },
  { L"-verbose", TypeFlag },
  { NULL,        TypeMax  }
};

/**
  Function for 'unload' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunUnload (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  LIST_ENTRY             *Package;
  CHAR16                 *ProblemParam;
  SHELL_STATUS           ShellStatus;
  EFI_HANDLE             TheHandle;
  CONST CHAR16           *Param1;
  SHELL_PROMPT_RESPONSE  *Resp;
  UINT64                 Value;

  ShellStatus = SHELL_SUCCESS;
  Package     = NULL;
  Resp        = NULL;
  Value       = 0;
  TheHandle   = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"unload", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 2) {
      //
      // error for too many parameters
      //
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"unload");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) < 2) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle, L"unload");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Param1 = ShellCommandLineGetRawValue (Package, 1);
      if (Param1 != NULL) {
        Status    = ShellConvertStringToUint64 (Param1, &Value, TRUE, FALSE);
        TheHandle = ConvertHandleIndexToHandle ((UINTN)Value);
      }

      if (EFI_ERROR (Status) || (Param1 == NULL) || (TheHandle == NULL)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"unload", Param1);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        ASSERT (TheHandle != NULL);
        if (ShellCommandLineGetFlag (Package, L"-v") || ShellCommandLineGetFlag (Package, L"-verbose")) {
          DumpLoadedImageProtocolInfo (TheHandle);
        }

        if (!ShellCommandLineGetFlag (Package, L"-n")) {
          Status = ShellPrintHiiDefaultEx (STRING_TOKEN (STR_UNLOAD_CONF), gShellDriver1HiiHandle, (UINTN)TheHandle);
          Status = ShellPromptForResponse (ShellPromptResponseTypeYesNo, NULL, (VOID **)&Resp);
        }

        if (ShellCommandLineGetFlag (Package, L"-n") || ((Resp != NULL) && (*Resp == ShellPromptResponseYes))) {
          Status = gBS->UnloadImage (TheHandle);
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_HANDLE_RESULT), gShellDriver1HiiHandle, L"Unload", (UINTN)TheHandle, Status);
        }

        SHELL_FREE_NON_NULL (Resp);
      }
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    if (Status == EFI_SECURITY_VIOLATION) {
      ShellStatus = SHELL_SECURITY_VIOLATION;
    } else if (Status == EFI_INVALID_PARAMETER) {
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (EFI_ERROR (Status)) {
      ShellStatus = SHELL_NOT_FOUND;
    }
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
