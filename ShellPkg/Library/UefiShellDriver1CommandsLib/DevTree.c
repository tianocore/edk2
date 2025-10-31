/** @file
  Main file for DevTree shell Driver1 function.

  (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-d", TypeFlag  },
  { L"-l", TypeValue },
  { NULL,  TypeMax   }
};

/**
  Display a tree starting from this handle.

  @param[in] TheHandle      The handle to start with.
  @param[in] Lang           Optionally, a UEFI defined language code.
  @param[in] UseDevPaths    TRUE to display info from DevPath as identifiers.
                            FALSE will use component name protocol instead.
  @param[in] IndentCharCount   How many characters to indent (allows for recursion).
  @param[in] HiiString      The string from HII to use for output.

  @retval SHELL_SUCCESS     The operation was successful.
**/
SHELL_STATUS
DoDevTreeForHandle (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST CHAR8       *Lang OPTIONAL,
  IN CONST BOOLEAN     UseDevPaths,
  IN CONST UINTN       IndentCharCount,
  IN CONST CHAR16      *HiiString
  )
{
  SHELL_STATUS  ShellStatus;
  EFI_STATUS    Status;
  CHAR16        *FormatString;
  CHAR16        *Name;
  EFI_HANDLE    *ChildHandleBuffer;
  UINTN         ChildCount;
  UINTN         LoopVar;

  Status            = EFI_SUCCESS;
  ShellStatus       = SHELL_SUCCESS;
  Name              = NULL;
  ChildHandleBuffer = NULL;
  ChildCount        = 0;

  ASSERT (TheHandle != NULL);
  ASSERT (HiiString != NULL);

  if (ShellGetExecutionBreakFlag ()) {
    ShellStatus = SHELL_ABORTED;
    return ShellStatus;
  }

  //
  // We want controller handles.  they will not have LoadedImage or DriverBinding (or others...)
  //
  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiDriverBindingProtocolGuid,
                  NULL,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return SHELL_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return SHELL_SUCCESS;
  }

  FormatString = AllocateZeroPool (StrSize (HiiString) + (10)*sizeof (FormatString[0]));
  if (FormatString == NULL) {
    return SHELL_OUT_OF_RESOURCES;
  }

  //
  // we generate the format string on the fly so that we can control the
  // number of space characters that the first (empty) string has.  this
  // handles the indenting.
  //

  UnicodeSPrint (FormatString, StrSize (HiiString) + (10)*sizeof (FormatString[0]), L"%%%ds %s", IndentCharCount, HiiString);
  gEfiShellProtocol->GetDeviceName ((EFI_HANDLE)TheHandle, !UseDevPaths ? EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH : EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Lang, &Name);
  //
  // print out the information for ourselves
  //
  ShellPrintDefaultEx (
    FormatString,
    L"",
    ConvertHandleToHandleIndex (TheHandle),
    Name == NULL ? L"Unknown" : Name
    );

  FreePool (FormatString);
  if (Name != NULL) {
    FreePool (Name);
  }

  //
  // recurse on each child handle with IndentCharCount + 2
  //
  ParseHandleDatabaseForChildControllers (TheHandle, &ChildCount, &ChildHandleBuffer);
  for (LoopVar = 0; LoopVar < ChildCount && ShellStatus == SHELL_SUCCESS; LoopVar++) {
    ShellStatus = DoDevTreeForHandle (ChildHandleBuffer[LoopVar], Lang, UseDevPaths, IndentCharCount+2, HiiString);
    if (ShellStatus == SHELL_ABORTED) {
      break;
    }
  }

  if (ChildHandleBuffer != NULL) {
    FreePool (ChildHandleBuffer);
  }

  return (ShellStatus);
}

/**
  Function for 'devtree' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDevTree (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CHAR8         *Language;
  CONST CHAR16  *Lang;
  CHAR16        *HiiString;
  UINTN         LoopVar;
  EFI_HANDLE    TheHandle;
  BOOLEAN       FlagD;
  UINT64        Intermediate;
  UINTN         ParentControllerHandleCount;
  EFI_HANDLE    *ParentControllerHandleBuffer;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;
  Language    = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"devtree", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 2) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"devtree");
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    Lang = ShellCommandLineGetValue (Package, L"-l");
    if (Lang != NULL) {
      Language = AllocateZeroPool (StrSize (Lang));
      if (Language == NULL) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_OUT_MEM), gShellDriver1HiiHandle, L"devtree");
        ShellCommandLineFreeVarList (Package);
        return (SHELL_OUT_OF_RESOURCES);
      }

      AsciiSPrint (Language, StrSize (Lang), "%S", Lang);
    } else if (!ShellCommandLineGetFlag (Package, L"-l")) {
      ASSERT (Language == NULL);
      //      Language = AllocateZeroPool(10);
      //      AsciiSPrint(Language, 10, "en-us");
    } else {
      ASSERT (Language == NULL);
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"devtree", L"-l");
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    FlagD = ShellCommandLineGetFlag (Package, L"-d");

    Lang      = ShellCommandLineGetRawValue (Package, 1);
    HiiString = HiiGetString (gShellDriver1HiiHandle, STRING_TOKEN (STR_DEV_TREE_OUTPUT), Language);

    if (HiiString == NULL) {
      ASSERT (HiiString != NULL);
      SHELL_FREE_NON_NULL (Language);
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    if (Lang == NULL) {
      for (LoopVar = 1; ; LoopVar++) {
        TheHandle = ConvertHandleIndexToHandle (LoopVar);
        if (TheHandle == NULL) {
          break;
        }

        //
        // Skip handles that do not have device path protocol
        //
        Status = gBS->OpenProtocol (
                        TheHandle,
                        &gEfiDevicePathProtocolGuid,
                        NULL,
                        NULL,
                        NULL,
                        EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // Skip handles that do have parents
        //
        ParentControllerHandleBuffer = NULL;
        Status                       = PARSE_HANDLE_DATABASE_PARENTS (
                                         TheHandle,
                                         &ParentControllerHandleCount,
                                         &ParentControllerHandleBuffer
                                         );
        SHELL_FREE_NON_NULL (ParentControllerHandleBuffer);
        if (ParentControllerHandleCount > 0) {
          continue;
        }

        //
        // Start a devtree from TheHandle that has a device path and no parents
        //
        ShellStatus = DoDevTreeForHandle (TheHandle, Language, FlagD, 0, HiiString);
      }
    } else {
      Status = ShellConvertStringToUint64 (Lang, &Intermediate, TRUE, FALSE);
      if (EFI_ERROR (Status) || (ConvertHandleIndexToHandle ((UINTN)Intermediate) == NULL)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"devtree", Lang);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        ShellStatus = DoDevTreeForHandle (ConvertHandleIndexToHandle ((UINTN)Intermediate), Language, FlagD, 0, HiiString);
      }
    }

    if (HiiString != NULL) {
      FreePool (HiiString);
    }

    SHELL_FREE_NON_NULL (Language);
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
