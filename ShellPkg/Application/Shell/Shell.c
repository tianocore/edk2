/** @file
  This is THE shell (application)

  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright 2015-2018 Dell Technologies.<BR>
  Copyright (C) 2023, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Shell.h"

//
// Initialize the global structure
//
SHELL_INFO  ShellInfoObject = {
  {
    {
      {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
      }
    },
    0,
    NULL,
    NULL
  },
  NULL,
  NULL
};

STATIC CONST CHAR16  mStartupScript[]        = L"startup.nsh";

/**
  Function to start monitoring for CTRL-S using SimpleTextInputEx.  This
  feature's enabled state was not known when the shell initially launched.

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available.
**/
EFI_STATUS
InternalEfiShellStartCtrlSMonitor (
  VOID
  )
{
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleEx;
  EFI_KEY_DATA                       KeyData;
  EFI_STATUS                         Status;

  Status = gBS->OpenProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleEx,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SHELL_NO_IN_EX),
      ShellInfoObject.HiiHandle
      );
    return (EFI_SUCCESS);
  }

  KeyData.KeyState.KeyToggleState = 0;
  KeyData.Key.ScanCode            = 0;
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar         = L's';

  Status = SimpleEx->RegisterKeyNotify (
                       SimpleEx,
                       &KeyData,
                       NotificationFunction,
                       &ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle1
                       );

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle2
                         );
  }

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar        = 19;

  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle3
                         );
  }

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle4
                         );
  }

  return (Status);
}

/**
  The entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  CHAR16                          *TempString;
  UINTN                           Size;
  EFI_HANDLE                      ConInHandle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *OldConIn;

  if (PcdGet8 (PcdShellSupportLevel) > 3) {
    return (EFI_UNSUPPORTED);
  }

  //
  // Clear the screen
  //
  Status = gST->ConOut->ClearScreen (gST->ConOut);
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  //
  // Check PCDs for optional features that are not implemented yet.
  //
  if (  !FeaturePcdGet (PcdShellRequireHiiPlatform)
     || FeaturePcdGet (PcdShellSupportFrameworkHii)
        )
  {
    return (EFI_UNSUPPORTED);
  }

  //
  // turn off the watchdog timer
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // Enable the cursor to be visible
  //
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  //
  // If supporting EFI 1.1 we need to install HII protocol
  // only do this if PcdShellRequireHiiPlatform == FALSE
  //
  // remove EFI_UNSUPPORTED check above when complete.
  /// @todo add support for Framework HII

  //
  // install our (solitary) HII package
  //
  ShellInfoObject.HiiHandle = HiiAddPackages (&gEfiCallerIdGuid, gImageHandle, ShellStrings, NULL);
  if (ShellInfoObject.HiiHandle == NULL) {
    if (PcdGetBool (PcdShellSupportFrameworkHii)) {
      /// @todo Add our package into Framework HII
    }

    if (ShellInfoObject.HiiHandle == NULL) {
      Status = EFI_NOT_STARTED;
      goto FreeResources;
    }
  }

  //
  // Check the command line
  //
  Status = ProcessCommandLine ();
  if (EFI_ERROR (Status)) {
    goto FreeResources;
  }

  //
  // TODO(benjamindoron): Set the environment variable for nesting support
  //
  Size       = 0;
  TempString = NULL;
  if (!ShellProtocolsInfoObject.ShellProtocolBitUnion.Bits.NoNest) {
    //
    // No change.  require nesting in Shell Protocol Execute()
    //
    StrnCatGrow (
      &TempString,
      &Size,
      L"False",
      0
      );
  } else {
    StrnCatGrow (
      &TempString,
      &Size,
      mNoNestingTrue,
      0
      );
  }

  Status = InternalEfiShellSetEnv (mNoNestingEnvVarName, TempString, TRUE);
  SHELL_FREE_NON_NULL (TempString);
  Size = 0;

  //
  // Display the version
  //
  if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoVersion) {
    ShellPrintHiiEx (
      0,
      gST->ConOut->Mode->CursorRow,
      NULL,
      STRING_TOKEN (STR_VER_OUTPUT_MAIN_SHELL),
      ShellInfoObject.HiiHandle,
      SupportLevel[PcdGet8 (PcdShellSupportLevel)],
      gEfiShellProtocol->MajorVersion,
      gEfiShellProtocol->MinorVersion
      );

    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_VER_OUTPUT_MAIN_SUPPLIER),
      ShellInfoObject.HiiHandle,
      (CHAR16 *)PcdGetPtr (PcdShellSupplier)
      );

    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_VER_OUTPUT_MAIN_UEFI),
      ShellInfoObject.HiiHandle,
      (gST->Hdr.Revision&0xffff0000)>>16,
      (gST->Hdr.Revision&0x0000ffff),
      gST->FirmwareVendor,
      gST->FirmwareRevision
      );
  }

  //
  // Display the mapping
  //
  if ((PcdGet8 (PcdShellSupportLevel) >= 2) && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoMap) {
    Status = RunCommand (L"map");
    ASSERT_EFI_ERROR (Status);
  }

  if (!EFI_ERROR (Status)) {
    if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt) {
      //
      // Set up the event for CTRL-C monitoring...
      //
      Status = InernalEfiShellStartMonitor ();
    }

    if (!EFI_ERROR (Status) && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
      //
      // Set up the event for CTRL-S monitoring...
      //
      Status = InternalEfiShellStartCtrlSMonitor ();
    }

    if (!EFI_ERROR (Status) && ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
      //
      // close off the gST->ConIn
      //
      OldConIn    = gST->ConIn;
      ConInHandle = gST->ConsoleInHandle;
      gST->ConIn  = CreateSimpleTextInOnFile ((SHELL_FILE_HANDLE)&FileInterfaceNulFile, &gST->ConsoleInHandle);
    } else {
      OldConIn    = NULL;
      ConInHandle = NULL;
    }

    if (!EFI_ERROR (Status) && (PcdGet8 (PcdShellSupportLevel) >= 1)) {
      //
      // process the startup script or launch the called app.
      //
      Status = DoStartupScript (ShellProtocolInteractivityInfoObject.ImageDevPath, ShellProtocolInteractivityInfoObject.FileDevPath);
    }

    if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.Exit && !ShellCommandGetExit () && ((PcdGet8 (PcdShellSupportLevel) >= 3) || PcdGetBool (PcdShellForceConsole)) && !EFI_ERROR (Status) && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
      //
      // begin the UI waiting loop
      //
      do {
        UefiShellProtocolsLibExecuteWaitLoopTasks ();

        UefiShellProtocolInteractivityLibExecuteWaitLoopTasks ();

        //
        // Display Prompt
        //
        Status = DoShellPrompt ();
      } while (!ShellCommandGetExit ());
    }

    if ((OldConIn != NULL) && (ConInHandle != NULL)) {
      CloseSimpleTextInOnFile (gST->ConIn);
      gST->ConIn           = OldConIn;
      gST->ConsoleInHandle = ConInHandle;
    }
  }

FreeResources:
  //
  // uninstall protocols / free memory / etc...
  //
  if (ShellInfoObject.UserBreakTimer != NULL) {
    gBS->CloseEvent (ShellInfoObject.UserBreakTimer);
    DEBUG_CODE (
      ShellInfoObject.UserBreakTimer = NULL;
      );
  }

  if (ShellInfoObject.ShellInitSettings.FileName != NULL) {
    FreePool (ShellInfoObject.ShellInitSettings.FileName);
    DEBUG_CODE (
      ShellInfoObject.ShellInitSettings.FileName = NULL;
      );
  }

  if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
    FreePool (ShellInfoObject.ShellInitSettings.FileOptions);
    DEBUG_CODE (
      ShellInfoObject.ShellInitSettings.FileOptions = NULL;
      );
  }

  if (ShellInfoObject.HiiHandle != NULL) {
    HiiRemovePackages (ShellInfoObject.HiiHandle);
    DEBUG_CODE (
      ShellInfoObject.HiiHandle = NULL;
      );
  }

  if (ShellCommandGetExit ()) {
    return ((EFI_STATUS)ShellCommandGetExitCode ());
  }

  return (Status);
}

/**
  Process all Uefi Shell 2.0 command line options.

  see Uefi Shell 2.0 section 3.2 for full details.

  the command line must resemble the following:

  shell.efi [ShellOpt-options] [options] [file-name [file-name-options]]

  ShellOpt-options  Options which control the initialization behavior of the shell.
                    These options are read from the EFI global variable "ShellOpt"
                    and are processed before options or file-name.

  options           Options which control the initialization behavior of the shell.

  file-name         The name of a UEFI shell application or script to be executed
                    after initialization is complete. By default, if file-name is
                    specified, then -nostartup is implied. Scripts are not supported
                    by level 0.

  file-name-options The command-line options that are passed to file-name when it
                    is invoked.

  This will initialize the ShellInfoObject.ShellInitSettings global variable.

  @retval EFI_SUCCESS           The variable is initialized.
**/
EFI_STATUS
ProcessCommandLine (
  VOID
  )
{
  UINTN                           Size;
  UINTN                           LoopVar;
  CHAR16                          *CurrentArg;
  CHAR16                          *DelayValueStr;
  UINT64                          DelayValue;
  EFI_STATUS                      Status;
  EFI_UNICODE_COLLATION_PROTOCOL  *UnicodeCollation;

  // `file-name-options` will contain arguments to `file-name` that we don't
  // know about. This would cause ShellCommandLineParse to error, so we parse
  // arguments manually, ignoring those after the first thing that doesn't look
  // like a shell option (which is assumed to be `file-name`).

  Status = gBS->LocateProtocol (
                  &gEfiUnicodeCollation2ProtocolGuid,
                  NULL,
                  (VOID **)&UnicodeCollation
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (
                    &gEfiUnicodeCollationProtocolGuid,
                    NULL,
                    (VOID **)&UnicodeCollation
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // Set default options
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup      = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup    = FALSE;
  ShellProtocolsInfoObject.ShellProtocolBitUnion.Bits.NoConsoleOut = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn  = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt  = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoMap        = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoVersion    = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.Delay        = FALSE;
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.Exit         = FALSE;
  ShellProtocolsInfoObject.ShellProtocolBitUnion.Bits.NoNest   = FALSE;
  ShellInfoObject.ShellInitSettings.Delay                      = PcdGet32 (PcdShellDefaultDelay);

  //
  // Start LoopVar at 0 to parse only optional arguments at Argv[0]
  // and parse other parameters from Argv[1].  This is for use case that
  // UEFI Shell boot option is created, and OptionalData is provided
  // that starts with shell command-line options.
  //
  for (LoopVar = 0; LoopVar < gEfiShellParametersProtocol->Argc; LoopVar++) {
    CurrentArg = gEfiShellParametersProtocol->Argv[LoopVar];
    if (UnicodeCollation->StriColl (
                            UnicodeCollation,
                            L"-startup",
                            CurrentArg
                            ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-nostartup",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-noconsoleout",
                                   CurrentArg
                                   ) == 0)
    {
      ShellProtocolsInfoObject.ShellProtocolBitUnion.Bits.NoConsoleOut = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-noconsolein",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-nointerrupt",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-nomap",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoMap = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-noversion",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoVersion = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-nonest",
                                   CurrentArg
                                   ) == 0)
    {
      ShellProtocolsInfoObject.ShellProtocolBitUnion.Bits.NoNest = TRUE;
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-delay",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.Delay = TRUE;
      // Check for optional delay value following "-delay"
      if ((LoopVar + 1) >= gEfiShellParametersProtocol->Argc) {
        DelayValueStr = NULL;
      } else {
        DelayValueStr = gEfiShellParametersProtocol->Argv[LoopVar + 1];
      }

      if (DelayValueStr != NULL) {
        if (*DelayValueStr == L':') {
          DelayValueStr++;
        }

        if (!EFI_ERROR (
               ShellConvertStringToUint64 (
                 DelayValueStr,
                 &DelayValue,
                 FALSE,
                 FALSE
                 )
               ))
        {
          ShellInfoObject.ShellInitSettings.Delay = (UINTN)DelayValue;
          LoopVar++;
        }
      }
    } else if (UnicodeCollation->StriColl (
                                   UnicodeCollation,
                                   L"-exit",
                                   CurrentArg
                                   ) == 0)
    {
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.Exit = TRUE;
    } else if (StrnCmp (L"-", CurrentArg, 1) == 0) {
      // Unrecognized option
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_PROBLEM),
        ShellInfoObject.HiiHandle,
        CurrentArg
        );
      return EFI_INVALID_PARAMETER;
    } else {
      //
      // First argument should be Shell.efi image name
      //
      if (LoopVar == 0) {
        continue;
      }

      ShellInfoObject.ShellInitSettings.FileName = NULL;
      Size                                       = 0;
      //
      // If first argument contains a space, then add double quotes before the argument
      //
      if (StrStr (CurrentArg, L" ") != NULL) {
        StrnCatGrow (&ShellInfoObject.ShellInitSettings.FileName, &Size, L"\"", 0);
        if (ShellInfoObject.ShellInitSettings.FileName == NULL) {
          return (EFI_OUT_OF_RESOURCES);
        }
      }

      StrnCatGrow (&ShellInfoObject.ShellInitSettings.FileName, &Size, CurrentArg, 0);
      if (ShellInfoObject.ShellInitSettings.FileName == NULL) {
        return (EFI_OUT_OF_RESOURCES);
      }

      //
      // If first argument contains a space, then add double quotes after the argument
      //
      if (StrStr (CurrentArg, L" ") != NULL) {
        StrnCatGrow (&ShellInfoObject.ShellInitSettings.FileName, &Size, L"\"", 0);
        if (ShellInfoObject.ShellInitSettings.FileName == NULL) {
          return (EFI_OUT_OF_RESOURCES);
        }
      }

      //
      // We found `file-name`.
      //
      ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup = 1;
      LoopVar++;

      // Add `file-name-options`
      for (Size = 0; LoopVar < gEfiShellParametersProtocol->Argc; LoopVar++) {
        ASSERT ((ShellInfoObject.ShellInitSettings.FileOptions == NULL && Size == 0) || (ShellInfoObject.ShellInitSettings.FileOptions != NULL));
        //
        // Add a space between arguments
        //
        if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
          StrnCatGrow (&ShellInfoObject.ShellInitSettings.FileOptions, &Size, L" ", 0);
          if (ShellInfoObject.ShellInitSettings.FileOptions == NULL) {
            SHELL_FREE_NON_NULL (ShellInfoObject.ShellInitSettings.FileName);
            return (EFI_OUT_OF_RESOURCES);
          }
        }

        //
        // If an argument contains a space, then add double quotes before the argument
        //
        if (StrStr (gEfiShellParametersProtocol->Argv[LoopVar], L" ") != NULL) {
          StrnCatGrow (
            &ShellInfoObject.ShellInitSettings.FileOptions,
            &Size,
            L"\"",
            0
            );
          if (ShellInfoObject.ShellInitSettings.FileOptions == NULL) {
            SHELL_FREE_NON_NULL (ShellInfoObject.ShellInitSettings.FileName);
            return (EFI_OUT_OF_RESOURCES);
          }
        }

        StrnCatGrow (
          &ShellInfoObject.ShellInitSettings.FileOptions,
          &Size,
          gEfiShellParametersProtocol->Argv[LoopVar],
          0
          );
        if (ShellInfoObject.ShellInitSettings.FileOptions == NULL) {
          SHELL_FREE_NON_NULL (ShellInfoObject.ShellInitSettings.FileName);
          return (EFI_OUT_OF_RESOURCES);
        }

        //
        // If an argument contains a space, then add double quotes after the argument
        //
        if (StrStr (gEfiShellParametersProtocol->Argv[LoopVar], L" ") != NULL) {
          StrnCatGrow (
            &ShellInfoObject.ShellInitSettings.FileOptions,
            &Size,
            L"\"",
            0
            );
          if (ShellInfoObject.ShellInitSettings.FileOptions == NULL) {
            SHELL_FREE_NON_NULL (ShellInfoObject.ShellInitSettings.FileName);
            return (EFI_OUT_OF_RESOURCES);
          }
        }
      }
    }
  }

  // "-nointerrupt" overrides "-delay"
  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt) {
    ShellInfoObject.ShellInitSettings.Delay = 0;
  }

  return EFI_SUCCESS;
}

/**
  Function try to find location of the Startup.nsh file.

  The buffer is callee allocated and should be freed by the caller.

  @param    ImageDevicePath       The path to the image for shell.  first place to look for the startup script
  @param    FileDevicePath        The path to the file for shell.  second place to look for the startup script.

  @retval   NULL                  No Startup.nsh file was found.
  @return   !=NULL                Pointer to NULL-terminated path.
**/
CHAR16 *
LocateStartupScript (
  IN EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL  *FileDevicePath
  )
{
  CHAR16        *StartupScriptPath;
  CHAR16        *TempSpot;
  CONST CHAR16  *MapName;
  UINTN         Size;

  StartupScriptPath = NULL;
  Size              = 0;

  //
  // Try to find 'Startup.nsh' in the directory where the shell itself was launched.
  //
  MapName = ShellProtocolsInfoObject.NewEfiShellProtocol->GetMapFromDevicePath (&ImageDevicePath);
  if (MapName != NULL) {
    StartupScriptPath = StrnCatGrow (&StartupScriptPath, &Size, MapName, 0);
    if (StartupScriptPath == NULL) {
      //
      // Do not locate the startup script in sys path when out of resource.
      //
      return NULL;
    }

    TempSpot = StrStr (StartupScriptPath, L";");
    if (TempSpot != NULL) {
      *TempSpot = CHAR_NULL;
    }

    InternalEfiShellSetEnv (L"homefilesystem", StartupScriptPath, TRUE);

    StartupScriptPath = StrnCatGrow (&StartupScriptPath, &Size, ((FILEPATH_DEVICE_PATH *)FileDevicePath)->PathName, 0);
    PathRemoveLastItem (StartupScriptPath);
    StartupScriptPath = StrnCatGrow (&StartupScriptPath, &Size, mStartupScript, 0);
  }

  //
  // Try to find 'Startup.nsh' in the execution path defined by the environment variable PATH.
  //
  if ((StartupScriptPath == NULL) || EFI_ERROR (ShellIsFile (StartupScriptPath))) {
    SHELL_FREE_NON_NULL (StartupScriptPath);
    StartupScriptPath = ShellFindFilePath (mStartupScript);
  }

  return StartupScriptPath;
}

/**
  Handles all interaction with the default startup script.

  this will check that the correct command line parameters were passed, handle the delay, and then start running the script.

  @param ImagePath              the path to the image for shell.  first place to look for the startup script
  @param FilePath               the path to the file for shell.  second place to look for the startup script.

  @retval EFI_SUCCESS           the variable is initialized.
**/
EFI_STATUS
DoStartupScript (
  IN EFI_DEVICE_PATH_PROTOCOL  *ImagePath,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS     Status;
  EFI_STATUS     CalleeStatus;
  UINTN          Delay;
  EFI_INPUT_KEY  Key;
  CHAR16         *FileStringPath;
  CHAR16         *FullFileStringPath;
  UINTN          NewSize;

  CalleeStatus    = EFI_SUCCESS;
  Key.UnicodeChar = CHAR_NULL;
  Key.ScanCode    = 0;

  if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup && (ShellInfoObject.ShellInitSettings.FileName != NULL)) {
    //
    // launch something else instead
    //
    NewSize = StrSize (ShellInfoObject.ShellInitSettings.FileName);
    if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
      NewSize += StrSize (ShellInfoObject.ShellInitSettings.FileOptions) + sizeof (CHAR16);
    }

    FileStringPath = AllocateZeroPool (NewSize);
    if (FileStringPath == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    StrCpyS (FileStringPath, NewSize/sizeof (CHAR16), ShellInfoObject.ShellInitSettings.FileName);
    if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
      StrnCatS (FileStringPath, NewSize/sizeof (CHAR16), L" ", NewSize/sizeof (CHAR16) - StrLen (FileStringPath) -1);
      StrnCatS (FileStringPath, NewSize/sizeof (CHAR16), ShellInfoObject.ShellInitSettings.FileOptions, NewSize/sizeof (CHAR16) - StrLen (FileStringPath) -1);
    }

    Status = RunShellCommand (FileStringPath, &CalleeStatus);
    if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.Exit == TRUE) {
      ShellCommandRegisterExit (gEfiShellProtocol->BatchIsActive (), (UINT64)CalleeStatus);
    }

    FreePool (FileStringPath);
    return (Status);
  }

  //
  // for shell level 0 we do no scripts
  // Without the Startup bit overriding we allow for nostartup to prevent scripts
  //
  if (  (PcdGet8 (PcdShellSupportLevel) < 1)
     || (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup)
        )
  {
    return (EFI_SUCCESS);
  }

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  //
  // print out our warning and see if they press a key
  //
  for ( Status = EFI_UNSUPPORTED, Delay = ShellInfoObject.ShellInitSettings.Delay
        ; Delay != 0 && EFI_ERROR (Status)
        ; Delay--
        )
  {
    ShellPrintHiiEx (0, gST->ConOut->Mode->CursorRow, NULL, STRING_TOKEN (STR_SHELL_STARTUP_QUESTION), ShellInfoObject.HiiHandle, Delay);
    gBS->Stall (1000000);
    if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    }
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_CRLF), ShellInfoObject.HiiHandle);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  //
  // ESC was pressed
  //
  if ((Status == EFI_SUCCESS) && (Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_ESC)) {
    return (EFI_SUCCESS);
  }

  FileStringPath = LocateStartupScript (ImagePath, FilePath);
  if (FileStringPath != NULL) {
    FullFileStringPath = FullyQualifyPath (FileStringPath);
    if (FullFileStringPath == NULL) {
      Status = RunScriptFile (FileStringPath, NULL, FileStringPath, ShellProtocolsInfoObject.NewShellParametersProtocol);
    } else {
      Status = RunScriptFile (FullFileStringPath, NULL, FullFileStringPath, ShellProtocolsInfoObject.NewShellParametersProtocol);
      FreePool (FullFileStringPath);
    }

    FreePool (FileStringPath);
  } else {
    //
    // we return success since startup script is not mandatory.
    //
    Status = EFI_SUCCESS;
  }

  return (Status);
}

/**
  Function to perform the shell prompt looping.  It will do a single prompt,
  dispatch the result, and then return.  It is expected that the caller will
  call this function in a loop many times.

  @retval EFI_SUCCESS
  @retval RETURN_ABORTED
**/
EFI_STATUS
DoShellPrompt (
  VOID
  )
{
  UINTN         Column;
  UINTN         Row;
  CHAR16        *CmdLine;
  CONST CHAR16  *CurDir;
  UINTN         BufferSize;
  EFI_STATUS    Status;
  LIST_ENTRY    OldBufferList;

  CurDir = NULL;

  //
  // Get screen setting to decide size of the command line buffer
  //
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Column, &Row);
  BufferSize = Column * Row * sizeof (CHAR16);
  CmdLine    = AllocateZeroPool (BufferSize);
  if (CmdLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SaveBufferList (&OldBufferList);
  CurDir = ShellProtocolsInfoObject.NewEfiShellProtocol->GetEnv (L"cwd");

  //
  // Prompt for input
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, 0, gST->ConOut->Mode->CursorRow);

  if ((CurDir != NULL) && (StrLen (CurDir) > 1)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_CURDIR), ShellInfoObject.HiiHandle, CurDir);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_SHELL), ShellInfoObject.HiiHandle);
  }

  //
  // Read a line from the console
  //
  Status = ShellProtocolsInfoObject.NewEfiShellProtocol->ReadFile (ShellProtocolsInfoObject.NewShellParametersProtocol->StdIn, &BufferSize, CmdLine);

  //
  // Null terminate the string and parse it
  //
  if (!EFI_ERROR (Status)) {
    //
    // Reset the CTRL-C event just before running the command (yes we ignore the return values)
    //
    Status = gBS->CheckEvent (ShellProtocolsInfoObject.NewEfiShellProtocol->ExecutionBreak);

    CmdLine[BufferSize / sizeof (CHAR16)] = CHAR_NULL;
    Status                                = RunCommand (CmdLine);
  }

  //
  // Done with this command
  //
  RestoreBufferList (&OldBufferList);
  FreePool (CmdLine);
  return Status;
}
