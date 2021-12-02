/** @file
  Main file for devices shell Driver1 function.

  (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

/**
  Get lots of info about a device from its handle.

  @param[in] TheHandle       The device handle to get info on.
  @param[in, out] Type       On successful return R, B, or D (root, bus, or
                             device) will be placed in this buffer.
  @param[in, out] Cfg        On successful return this buffer will be
                             TRUE if the handle has configuration, FALSE
                             otherwise.
  @param[in, out] Diag       On successful return this buffer will be
                             TRUE if the handle has disgnostics, FALSE
                             otherwise.
  @param[in, out] Parents    On successful return this buffer will be
                             contain the number of parent handles.
  @param[in, out] Devices    On successful return this buffer will be
                             contain the number of devices controlled.
  @param[in, out] Children   On successful return this buffer will be
                             contain the number of child handles.
  @param[out] Name           The pointer to a buffer that will be allocated
                             and contain the string name of the handle.
                             The caller must free this memory.
  @param[in] Language        The language code as defined by the UEFI spec.

  @retval EFI_SUCCESS           The info is there.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
GetDeviceHandleInfo (
  IN EFI_HANDLE   TheHandle,
  IN OUT CHAR16   *Type,
  IN OUT BOOLEAN  *Cfg,
  IN OUT BOOLEAN  *Diag,
  IN OUT UINTN    *Parents,
  IN OUT UINTN    *Devices,
  IN OUT UINTN    *Children,
  OUT CHAR16      **Name,
  IN CONST CHAR8  *Language
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Count;

  if (  (TheHandle == NULL)
     || (Type == NULL)
     || (Cfg == NULL)
     || (Diag == NULL)
     || (Parents == NULL)
     || (Devices == NULL)
     || (Children == NULL)
     || (Name == NULL))
  {
    return (EFI_INVALID_PARAMETER);
  }

  *Cfg         = FALSE;
  *Diag        = FALSE;
  *Children    = 0;
  *Parents     = 0;
  *Devices     = 0;
  *Type        = L' ';
  *Name        = CHAR_NULL;
  HandleBuffer = NULL;
  Status       = EFI_SUCCESS;

  gEfiShellProtocol->GetDeviceName (TheHandle, EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Language, Name);

  Status = ParseHandleDatabaseForChildControllers (TheHandle, Children, NULL);
  //  if (!EFI_ERROR(Status)) {
  Status = PARSE_HANDLE_DATABASE_PARENTS (TheHandle, Parents, NULL);
  if (/*!EFI_ERROR(Status) && */ (Parents != NULL) && (Children != NULL)) {
    if (*Parents == 0) {
      *Type = L'R';
    } else if (*Children > 0) {
      *Type = L'B';
    } else {
      *Type = L'D';
    }
  }

  //  }
  Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (TheHandle, Devices, &HandleBuffer);
  if (!EFI_ERROR (Status) && (Devices != NULL) && (HandleBuffer != NULL)) {
    for (Count = 0; Count < *Devices; Count++) {
      if (!EFI_ERROR (gBS->OpenProtocol (HandleBuffer[Count], &gEfiDriverConfigurationProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        *Cfg = TRUE;
      }

      if (!EFI_ERROR (gBS->OpenProtocol (HandleBuffer[Count], &gEfiDriverDiagnosticsProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        *Diag = TRUE;
      }

      if (!EFI_ERROR (gBS->OpenProtocol (HandleBuffer[Count], &gEfiDriverDiagnostics2ProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        *Diag = TRUE;
      }
    }

    SHELL_FREE_NON_NULL (HandleBuffer);
  }

  return (Status);
}

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-sfo", TypeFlag  },
  { L"-l",   TypeValue },
  { NULL,    TypeMax   }
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
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CHAR8         *Language;
  EFI_HANDLE    *HandleList;
  EFI_HANDLE    *HandleListWalker;
  CHAR16        Type;
  BOOLEAN       Cfg;
  BOOLEAN       Diag;
  UINTN         Parents;
  UINTN         Devices;
  UINTN         Children;
  CHAR16        *Name;
  CONST CHAR16  *Lang;
  BOOLEAN       SfoFlag;

  ShellStatus = SHELL_SUCCESS;
  Language    = NULL;
  SfoFlag     = FALSE;

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
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"devices", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // if more than 0 'value' parameters  we have too many parameters
    //
    if (ShellCommandLineGetRawValue (Package, 1) != NULL) {
      //
      // error for too many parameters
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"devices");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // get the language if necessary
      //
      Lang = ShellCommandLineGetValue (Package, L"-l");
      if (Lang != NULL) {
        Language = AllocateZeroPool (StrSize (Lang));
        AsciiSPrint (Language, StrSize (Lang), "%S", Lang);
      } else if (!ShellCommandLineGetFlag (Package, L"-l")) {
        ASSERT (Language == NULL);
        //        Language = AllocateZeroPool(10);
        //        AsciiSPrint(Language, 10, "en-us");
      } else {
        ASSERT (Language == NULL);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"devices", L"-l");
        ShellCommandLineFreeVarList (Package);
        return (SHELL_INVALID_PARAMETER);
      }

      //
      // Print Header

      //
      if (ShellCommandLineGetFlag (Package, L"-sfo")) {
        ShellPrintHiiEx (-1, -1, Language, STRING_TOKEN (STR_GEN_SFO_HEADER), gShellDriver1HiiHandle, L"devices");
        SfoFlag = TRUE;
      } else {
        ShellPrintHiiEx (-1, -1, Language, STRING_TOKEN (STR_DEVICES_HEADER_LINES), gShellDriver1HiiHandle);
      }

      //
      // loop through each handle
      //
      HandleList = GetHandleListByProtocol (NULL);
      ASSERT (HandleList != NULL);
      for (HandleListWalker = HandleList
           ; HandleListWalker != NULL && *HandleListWalker != NULL /*&& !EFI_ERROR(Status)*/
           ; HandleListWalker++
           )
      {
        //
        // get all the info on each handle
        //
        Name   = NULL;
        Status = GetDeviceHandleInfo (*HandleListWalker, &Type, &Cfg, &Diag, &Parents, &Devices, &Children, &Name, Language);
        if ((Name != NULL) && ((Parents != 0) || (Devices != 0) || (Children != 0))) {
          ShellPrintHiiEx (
            -1,
            -1,
            Language,
            SfoFlag ? STRING_TOKEN (STR_DEVICES_ITEM_LINE_SFO) : STRING_TOKEN (STR_DEVICES_ITEM_LINE),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (*HandleListWalker),
            Type,
            Cfg ? (SfoFlag ? L'Y' : L'X') : (SfoFlag ? L'N' : L'-'),
            Diag ? (SfoFlag ? L'Y' : L'X') : (SfoFlag ? L'N' : L'-'),
            Parents,
            Devices,
            Children,
            Name != NULL ? Name : L"<UNKNOWN>"
            );
        }

        if (Name != NULL) {
          FreePool (Name);
        }

        if (ShellGetExecutionBreakFlag ()) {
          ShellStatus = SHELL_ABORTED;
          break;
        }
      }

      if (HandleList != NULL) {
        FreePool (HandleList);
      }
    }

    SHELL_FREE_NON_NULL (Language);
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
