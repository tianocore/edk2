/** @file
  Main file for Ver shell level 3 function.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-s", TypeFlag},
  {L"-terse", TypeFlag},
  {L"-t", TypeFlag},
  {L"-_pa", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'ver' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunVer (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  UINT8               Level;

  Level = PcdGet8(PcdShellSupportLevel);
  ProblemParam        = NULL;
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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"ver", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    }
    if (ShellCommandLineGetRawValue(Package, 1) != NULL) {
      //
      // we have too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"ver");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (ShellCommandLineGetFlag(Package, L"-s")) {
        if (ShellCommandLineGetFlag(Package, L"-terse") || ShellCommandLineGetFlag(Package, L"-t")){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellLevel3HiiHandle, L"ver", L"-t or -terse", L"-s");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          ShellPrintHiiEx (
            0,
            gST->ConOut->Mode->CursorRow,
            NULL,
            STRING_TOKEN (STR_VER_OUTPUT_SIMPLE),
            gShellLevel3HiiHandle,
            gEfiShellProtocol->MajorVersion,
            gEfiShellProtocol->MinorVersion
           );
        }
      } else {
        ShellPrintHiiEx (
          0,
          gST->ConOut->Mode->CursorRow,
          NULL,
          STRING_TOKEN (STR_VER_OUTPUT_SHELL),
          gShellLevel3HiiHandle,
          SupportLevel[Level],
          gEfiShellProtocol->MajorVersion,
          gEfiShellProtocol->MinorVersion
         );
        if (!ShellCommandLineGetFlag(Package, L"-terse") && !ShellCommandLineGetFlag(Package, L"-t")){
          ShellPrintHiiEx(
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_VER_OUTPUT_SUPPLIER),
            gShellLevel3HiiHandle,
            (CHAR16 *) PcdGetPtr (PcdShellSupplier)
           );


          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_VER_OUTPUT_UEFI),
            gShellLevel3HiiHandle,
            (gST->Hdr.Revision&0xffff0000)>>16,
            (gST->Hdr.Revision&0x0000ffff),
            gST->FirmwareVendor,
            gST->FirmwareRevision
           );
        }
      }
      //
      // implementation specific support for displaying processor architecture
      //
      if (ShellCommandLineGetFlag(Package, L"-_pa")) {
        ShellPrintEx(-1, -1, L"%d\r\n", sizeof(UINTN)==sizeof(UINT64)?64:32);
      }
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}

