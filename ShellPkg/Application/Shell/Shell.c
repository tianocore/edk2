/** @file
  This is THE shell (application)

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Shell.h"

//
// Initialize the global structure
//
SHELL_INFO ShellInfoObject = {
  NULL,
  NULL,
  FALSE,
  FALSE,
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    NULL,
    NULL
  },
  {0,0},
  {
    {0,0},
    0,
    0,
    TRUE
  },
  NULL,
  0,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  {0,0,NULL,NULL},
  {0,0},
  NULL,
  NULL,
  NULL,
  NULL,
  FALSE
};

STATIC CONST CHAR16 mScriptExtension[]      = L".NSH";
STATIC CONST CHAR16 mExecutableExtensions[] = L".NSH;.EFI";
STATIC CONST CHAR16 mStartupScript[]        = L"startup.nsh";

/**
  Function to start monitoring for CTRL-S using SimpleTextInputEx.  This 
  feature's enabled state was not known when the shell initially launched.

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough mnemory available.
**/
EFI_STATUS
EFIAPI
InternalEfiShellStartCtrlSMonitor(
  VOID
  )
{
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *SimpleEx;
  EFI_KEY_DATA                      KeyData;
  EFI_STATUS                        Status;

  Status = gBS->OpenProtocol(
    gST->ConsoleInHandle,
    &gEfiSimpleTextInputExProtocolGuid,
    (VOID**)&SimpleEx,
    gImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL,
      STRING_TOKEN (STR_SHELL_NO_IN_EX),
      ShellInfoObject.HiiHandle);
    return (EFI_SUCCESS);
  }

  KeyData.KeyState.KeyToggleState = 0;
  KeyData.Key.ScanCode            = 0;
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar         = L's';

  Status = SimpleEx->RegisterKeyNotify(
    SimpleEx,
    &KeyData,
    NotificationFunction,
    &ShellInfoObject.CtrlSNotifyHandle1);
  
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR(Status)) {
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &ShellInfoObject.CtrlSNotifyHandle2);
  }
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar         = 19;

  if (!EFI_ERROR(Status)) {
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &ShellInfoObject.CtrlSNotifyHandle2);
  }  
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR(Status)) {
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &ShellInfoObject.CtrlSNotifyHandle2);
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

  if (PcdGet8(PcdShellSupportLevel) > 3) {
    return (EFI_UNSUPPORTED);
  }

  //
  // Clear the screen
  //
  Status = gST->ConOut->ClearScreen(gST->ConOut);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  //
  // Populate the global structure from PCDs
  //
  ShellInfoObject.ImageDevPath                = NULL;
  ShellInfoObject.FileDevPath                 = NULL;
  ShellInfoObject.PageBreakEnabled            = PcdGetBool(PcdShellPageBreakDefault);
  ShellInfoObject.ViewingSettings.InsertMode  = PcdGetBool(PcdShellInsertModeDefault);
  ShellInfoObject.LogScreenCount              = PcdGet8   (PcdShellScreenLogCount  );

  //
  // verify we dont allow for spec violation
  //
  ASSERT(ShellInfoObject.LogScreenCount >= 3);

  //
  // Initialize the LIST ENTRY objects...
  //
  InitializeListHead(&ShellInfoObject.BufferToFreeList.Link);
  InitializeListHead(&ShellInfoObject.ViewingSettings.CommandHistory.Link);
  InitializeListHead(&ShellInfoObject.SplitList.Link);

  //
  // Check PCDs for optional features that are not implemented yet.
  //
  if (   PcdGetBool(PcdShellSupportOldProtocols)
      || !FeaturePcdGet(PcdShellRequireHiiPlatform)
      || FeaturePcdGet(PcdShellSupportFrameworkHii)
   ) {
    return (EFI_UNSUPPORTED);
  }

  //
  // turn off the watchdog timer
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // install our console logger.  This will keep a log of the output for back-browsing
  //
  Status = ConsoleLoggerInstall(ShellInfoObject.LogScreenCount, &ShellInfoObject.ConsoleInfo);
  if (!EFI_ERROR(Status)) {
    //
    // Enable the cursor to be visible
    //
    gST->ConOut->EnableCursor (gST->ConOut, TRUE);

    //
    // If supporting EFI 1.1 we need to install HII protocol
    // only do this if PcdShellRequireHiiPlatform == FALSE
    //
    // remove EFI_UNSUPPORTED check above when complete.
    ///@todo add support for Framework HII

    //
    // install our (solitary) HII package
    //
    ShellInfoObject.HiiHandle = HiiAddPackages (&gEfiCallerIdGuid, gImageHandle, ShellStrings, NULL);
    if (ShellInfoObject.HiiHandle == NULL) {
      if (PcdGetBool(PcdShellSupportFrameworkHii)) {
        ///@todo Add our package into Framework HII
      }
      if (ShellInfoObject.HiiHandle == NULL) {
        return (EFI_NOT_STARTED);
      }
    }

    //
    // create and install the EfiShellParametersProtocol
    //
    Status = CreatePopulateInstallShellParametersProtocol(&ShellInfoObject.NewShellParametersProtocol, &ShellInfoObject.RootShellInstance);
    ASSERT_EFI_ERROR(Status);
    ASSERT(ShellInfoObject.NewShellParametersProtocol != NULL);

    //
    // create and install the EfiShellProtocol
    //
    Status = CreatePopulateInstallShellProtocol(&ShellInfoObject.NewEfiShellProtocol);
    ASSERT_EFI_ERROR(Status);
    ASSERT(ShellInfoObject.NewEfiShellProtocol != NULL);

    //
    // Now initialize the shell library (it requires Shell Parameters protocol)
    //
    Status = ShellInitialize();
    ASSERT_EFI_ERROR(Status);

    Status = CommandInit();
    ASSERT_EFI_ERROR(Status);

    //
    // Check the command line
    //
    Status = ProcessCommandLine();

    //
    // If shell support level is >= 1 create the mappings and paths
    //
    if (PcdGet8(PcdShellSupportLevel) >= 1) {
      Status = ShellCommandCreateInitialMappingsAndPaths();
    }

    //
    // save the device path for the loaded image and the device path for the filepath (under loaded image)
    // These are where to look for the startup.nsh file
    //
    Status = GetDevicePathsForImageAndFile(&ShellInfoObject.ImageDevPath, &ShellInfoObject.FileDevPath);
    ASSERT_EFI_ERROR(Status);

    //
    // Display the version
    //
    if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoVersion) {
      ShellPrintHiiEx (
        0,
        gST->ConOut->Mode->CursorRow,
        NULL,
        STRING_TOKEN (STR_VER_OUTPUT_MAIN),
        ShellInfoObject.HiiHandle,
        SupportLevel[PcdGet8(PcdShellSupportLevel)],
        gEfiShellProtocol->MajorVersion,
        gEfiShellProtocol->MinorVersion,
        (gST->Hdr.Revision&0xffff0000)>>16,
        (gST->Hdr.Revision&0x0000ffff),
        gST->FirmwareVendor,
        gST->FirmwareRevision
       );
    }

    //
    // Display the mapping
    //
    if (PcdGet8(PcdShellSupportLevel) >= 2 && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoMap) {
      Status = RunCommand(L"map");
      ASSERT_EFI_ERROR(Status);
    }

    //
    // init all the built in alias'
    //
    Status = SetBuiltInAlias();
    ASSERT_EFI_ERROR(Status);

    //
    // Initialize environment variables
    //
    if (ShellCommandGetProfileList() != NULL) {
      Status = InternalEfiShellSetEnv(L"profiles", ShellCommandGetProfileList(), TRUE);
      ASSERT_EFI_ERROR(Status);
    }

    Size        = 100;
    TempString  = AllocateZeroPool(Size);

    UnicodeSPrint(TempString, Size, L"%d", PcdGet8(PcdShellSupportLevel));
    Status = InternalEfiShellSetEnv(L"uefishellsupport", TempString, TRUE);
    ASSERT_EFI_ERROR(Status);

    UnicodeSPrint(TempString, Size, L"%d.%d", ShellInfoObject.NewEfiShellProtocol->MajorVersion, ShellInfoObject.NewEfiShellProtocol->MinorVersion);
    Status = InternalEfiShellSetEnv(L"uefishellversion", TempString, TRUE);
    ASSERT_EFI_ERROR(Status);

    UnicodeSPrint(TempString, Size, L"%d.%d", (gST->Hdr.Revision & 0xFFFF0000) >> 16, gST->Hdr.Revision & 0x0000FFFF);
    Status = InternalEfiShellSetEnv(L"uefiversion", TempString, TRUE);
    ASSERT_EFI_ERROR(Status);

    FreePool(TempString);

    if (!EFI_ERROR(Status)) {
      if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt) {
        //
        // Set up the event for CTRL-C monitoring...
        //
        Status = InernalEfiShellStartMonitor();
      }

      if (!EFI_ERROR(Status) && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
        //
        // Set up the event for CTRL-S monitoring...
        //
        Status = InternalEfiShellStartCtrlSMonitor();
      }

      if (!EFI_ERROR(Status) && ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
        //
        // close off the gST->ConIn
        //
        OldConIn      = gST->ConIn;
        ConInHandle   = gST->ConsoleInHandle;
        gST->ConIn = CreateSimpleTextInOnFile((SHELL_FILE_HANDLE)&FileInterfaceNulFile, &gST->ConsoleInHandle);
      } else {
        OldConIn      = NULL;
        ConInHandle   = NULL;
      }

      if (!EFI_ERROR(Status) && PcdGet8(PcdShellSupportLevel) >= 1) {
        //
        // process the startup script or launch the called app.
        //
        Status = DoStartupScript(ShellInfoObject.ImageDevPath, ShellInfoObject.FileDevPath);
      }

      if (!ShellCommandGetExit() && (PcdGet8(PcdShellSupportLevel) >= 3 || PcdGetBool(PcdShellForceConsole)) && !EFI_ERROR(Status) && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
        //
        // begin the UI waiting loop
        //
        do {
          //
          // clean out all the memory allocated for CONST <something> * return values
          // between each shell prompt presentation
          //
          if (!IsListEmpty(&ShellInfoObject.BufferToFreeList.Link)){
            FreeBufferList(&ShellInfoObject.BufferToFreeList);
          }

          //
          // Reset page break back to default.
          //
          ShellInfoObject.PageBreakEnabled        = PcdGetBool(PcdShellPageBreakDefault);
          ShellInfoObject.ConsoleInfo->Enabled    = TRUE;
          ShellInfoObject.ConsoleInfo->RowCounter = 0;

          //
          // Reset the CTRL-C event (yes we ignore the return values)
          //
          Status = gBS->CheckEvent (ShellInfoObject.NewEfiShellProtocol->ExecutionBreak);

          //
          // Display Prompt
          //
          Status = DoShellPrompt();
        } while (!ShellCommandGetExit());
      }
      if (OldConIn != NULL && ConInHandle != NULL) {
        CloseSimpleTextInOnFile (gST->ConIn);
        gST->ConIn            = OldConIn;
        gST->ConsoleInHandle  = ConInHandle;
      }
    }
  }

  //
  // uninstall protocols / free memory / etc...
  //
  if (ShellInfoObject.UserBreakTimer != NULL) {
    gBS->CloseEvent(ShellInfoObject.UserBreakTimer);
    DEBUG_CODE(ShellInfoObject.UserBreakTimer = NULL;);
  }
  if (ShellInfoObject.ImageDevPath != NULL) {
    FreePool(ShellInfoObject.ImageDevPath);
    DEBUG_CODE(ShellInfoObject.ImageDevPath = NULL;);
  }
  if (ShellInfoObject.FileDevPath != NULL) {
    FreePool(ShellInfoObject.FileDevPath);
    DEBUG_CODE(ShellInfoObject.FileDevPath = NULL;);
  }
  if (ShellInfoObject.NewShellParametersProtocol != NULL) {
    CleanUpShellParametersProtocol(ShellInfoObject.NewShellParametersProtocol);
    DEBUG_CODE(ShellInfoObject.NewShellParametersProtocol = NULL;);
  }
  if (ShellInfoObject.NewEfiShellProtocol != NULL){
    if (ShellInfoObject.NewEfiShellProtocol->IsRootShell()){
      ShellInfoObject.NewEfiShellProtocol->SetEnv(L"cwd", L"", TRUE);
    }
    CleanUpShellProtocol(ShellInfoObject.NewEfiShellProtocol);
    DEBUG_CODE(ShellInfoObject.NewEfiShellProtocol = NULL;);
  }

  if (!IsListEmpty(&ShellInfoObject.BufferToFreeList.Link)){
    FreeBufferList(&ShellInfoObject.BufferToFreeList);
  }

  if (!IsListEmpty(&ShellInfoObject.SplitList.Link)){
    ASSERT(FALSE); ///@todo finish this de-allocation.
  }

  if (ShellInfoObject.ShellInitSettings.FileName != NULL) {
    FreePool(ShellInfoObject.ShellInitSettings.FileName);
    DEBUG_CODE(ShellInfoObject.ShellInitSettings.FileName = NULL;);
  }

  if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
    FreePool(ShellInfoObject.ShellInitSettings.FileOptions);
    DEBUG_CODE(ShellInfoObject.ShellInitSettings.FileOptions = NULL;);
  }

  if (ShellInfoObject.HiiHandle != NULL) {
    HiiRemovePackages(ShellInfoObject.HiiHandle);
    DEBUG_CODE(ShellInfoObject.HiiHandle = NULL;);
  }

  if (!IsListEmpty(&ShellInfoObject.ViewingSettings.CommandHistory.Link)){
    FreeBufferList(&ShellInfoObject.ViewingSettings.CommandHistory);
  }

  ASSERT(ShellInfoObject.ConsoleInfo != NULL);
  if (ShellInfoObject.ConsoleInfo != NULL) {
    ConsoleLoggerUninstall(ShellInfoObject.ConsoleInfo);
    FreePool(ShellInfoObject.ConsoleInfo);
    DEBUG_CODE(ShellInfoObject.ConsoleInfo = NULL;);
  }

  if (ShellCommandGetExit()) {
    return ((EFI_STATUS)ShellCommandGetExitCode());
  }
  return (Status);
}

/**
  Sets all the alias' that were registered with the ShellCommandLib library.

  @retval EFI_SUCCESS           all init commands were run sucessfully.
**/
EFI_STATUS
EFIAPI
SetBuiltInAlias(
  )
{
  EFI_STATUS          Status;
  CONST ALIAS_LIST    *List;
  ALIAS_LIST          *Node;

  //
  // Get all the commands we want to run
  //
  List = ShellCommandGetInitAliasList();

  //
  // for each command in the List
  //
  for ( Node = (ALIAS_LIST*)GetFirstNode(&List->Link)
      ; !IsNull (&List->Link, &Node->Link)
      ; Node = (ALIAS_LIST *)GetNextNode(&List->Link, &Node->Link)
   ){
    //
    // install the alias'
    //
    Status = InternalSetAlias(Node->CommandString, Node->Alias, TRUE);
    ASSERT_EFI_ERROR(Status);
  }
  return (EFI_SUCCESS);
}

/**
  Internal function to determine if 2 command names are really the same.

  @param[in] Command1       The pointer to the first command name.
  @param[in] Command2       The pointer to the second command name.

  @retval TRUE              The 2 command names are the same.
  @retval FALSE             The 2 command names are not the same.
**/
BOOLEAN
EFIAPI
IsCommand(
  IN CONST CHAR16 *Command1,
  IN CONST CHAR16 *Command2
  )
{
  if (StringNoCaseCompare(&Command1, &Command2) == 0) {
    return (TRUE);
  }
  return (FALSE);
}

/**
  Internal function to determine if a command is a script only command.

  @param[in] CommandName    The pointer to the command name.

  @retval TRUE              The command is a script only command.
  @retval FALSE             The command is not a script only command.
**/
BOOLEAN
EFIAPI
IsScriptOnlyCommand(
  IN CONST CHAR16 *CommandName
  )
{
  if (IsCommand(CommandName, L"for")
    ||IsCommand(CommandName, L"endfor")
    ||IsCommand(CommandName, L"if")
    ||IsCommand(CommandName, L"else")
    ||IsCommand(CommandName, L"endif")
    ||IsCommand(CommandName, L"goto")) {
    return (TRUE);
  }
  return (FALSE);
}

/**
  This function will populate the 2 device path protocol parameters based on the
  global gImageHandle.  The DevPath will point to the device path for the handle that has
  loaded image protocol installed on it.  The FilePath will point to the device path
  for the file that was loaded.

  @param[in, out] DevPath       On a sucessful return the device path to the loaded image.
  @param[in, out] FilePath      On a sucessful return the device path to the file.

  @retval EFI_SUCCESS           The 2 device paths were sucessfully returned.
  @retval other                 A error from gBS->HandleProtocol.

  @sa HandleProtocol
**/
EFI_STATUS
EFIAPI
GetDevicePathsForImageAndFile (
  IN OUT EFI_DEVICE_PATH_PROTOCOL **DevPath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath
  )
{
  EFI_STATUS                Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath;

  ASSERT(DevPath  != NULL);
  ASSERT(FilePath != NULL);

  Status = gBS->OpenProtocol (
                gImageHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID**)&LoadedImage,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
               );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ImageDevicePath,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
    if (!EFI_ERROR (Status)) {
      *DevPath  = DuplicateDevicePath (ImageDevicePath);
      *FilePath = DuplicateDevicePath (LoadedImage->FilePath);
      gBS->CloseProtocol(
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  gImageHandle,
                  NULL);
    }
    gBS->CloseProtocol(
                gImageHandle,
                &gEfiLoadedImageProtocolGuid,
                gImageHandle,
                NULL);
  }
  return (Status);
}

STATIC CONST SHELL_PARAM_ITEM mShellParamList[] = {
  {L"-nostartup",     TypeFlag},
  {L"-startup",       TypeFlag},
  {L"-noconsoleout",  TypeFlag},
  {L"-noconsolein",   TypeFlag},
  {L"-nointerrupt",   TypeFlag},
  {L"-nomap",         TypeFlag},
  {L"-noversion",     TypeFlag},
  {L"-startup",       TypeFlag},
  {L"-delay",         TypeValue},
  {NULL, TypeMax}
  };

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
EFIAPI
ProcessCommandLine(
  VOID
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  UINTN         Size;
  CONST CHAR16  *TempConst;
  UINTN         Count;
  UINTN         LoopVar;
  CHAR16        *ProblemParam;
  UINT64        Intermediate;

  Package       = NULL;
  ProblemParam  = NULL;
  
  Status = ShellCommandLineParse (mShellParamList, &Package, NULL, FALSE);

  Count = 1;
  Size = 0;
  TempConst = ShellCommandLineGetRawValue(Package, Count++);
  if (TempConst != NULL && StrLen(TempConst)) {
    ShellInfoObject.ShellInitSettings.FileName = AllocateZeroPool(StrSize(TempConst));
    if (ShellInfoObject.ShellInitSettings.FileName == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    StrCpy(ShellInfoObject.ShellInitSettings.FileName, TempConst);
    ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup = 1;
    for (LoopVar = 0 ; LoopVar < gEfiShellParametersProtocol->Argc ; LoopVar++) {
      if (StrCmp(gEfiShellParametersProtocol->Argv[LoopVar], ShellInfoObject.ShellInitSettings.FileName)==0) {
        LoopVar++;
        //
        // We found the file... add the rest of the params...
        //
        for ( ; LoopVar < gEfiShellParametersProtocol->Argc ; LoopVar++) {
          ASSERT((ShellInfoObject.ShellInitSettings.FileOptions == NULL && Size == 0) || (ShellInfoObject.ShellInitSettings.FileOptions != NULL));
          StrnCatGrow(&ShellInfoObject.ShellInitSettings.FileOptions,
                      &Size,
                      L" ",
                      0);
          if (ShellInfoObject.ShellInitSettings.FileOptions == NULL) {
            SHELL_FREE_NON_NULL(ShellInfoObject.ShellInitSettings.FileName);
            return (EFI_OUT_OF_RESOURCES);
          }
          StrnCatGrow(&ShellInfoObject.ShellInitSettings.FileOptions,
                      &Size,
                      gEfiShellParametersProtocol->Argv[LoopVar],
                      0);
          if (ShellInfoObject.ShellInitSettings.FileOptions == NULL) {
            SHELL_FREE_NON_NULL(ShellInfoObject.ShellInitSettings.FileName);
            return (EFI_OUT_OF_RESOURCES);
          }
        }
      }
    }
  } else {
    ShellCommandLineFreeVarList(Package);
    Package = NULL;
    Status = ShellCommandLineParse (mShellParamList, &Package, &ProblemParam, FALSE);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), ShellInfoObject.HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellCommandLineFreeVarList(Package);
      return (EFI_INVALID_PARAMETER);
    }
  }

  ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup      = ShellCommandLineGetFlag(Package, L"-startup");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup    = ShellCommandLineGetFlag(Package, L"-nostartup");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleOut = ShellCommandLineGetFlag(Package, L"-noconsoleout");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn  = ShellCommandLineGetFlag(Package, L"-noconsolein");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt  = ShellCommandLineGetFlag(Package, L"-nointerrupt");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoMap        = ShellCommandLineGetFlag(Package, L"-nomap");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoVersion    = ShellCommandLineGetFlag(Package, L"-noversion");
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.Delay        = ShellCommandLineGetFlag(Package, L"-delay");

  ShellInfoObject.ShellInitSettings.Delay = 5;

  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoInterrupt) {
    ShellInfoObject.ShellInitSettings.Delay = 0;
  } else if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.Delay) {
    TempConst = ShellCommandLineGetValue(Package, L"-delay");
    if (TempConst != NULL && *TempConst == L':') {
      TempConst++;
    }
    if (TempConst != NULL && !EFI_ERROR(ShellConvertStringToUint64(TempConst, &Intermediate, FALSE, FALSE))) {
      ShellInfoObject.ShellInitSettings.Delay = (UINTN)Intermediate;
    }
  }
  ShellCommandLineFreeVarList(Package);

  return (Status);
}

/**
  Handles all interaction with the default startup script.

  this will check that the correct command line parameters were passed, handle the delay, and then start running the script.

  @param ImagePath              the path to the image for shell.  first place to look for the startup script
  @param FilePath               the path to the file for shell.  second place to look for the startup script.

  @retval EFI_SUCCESS           the variable is initialized.
**/
EFI_STATUS
EFIAPI
DoStartupScript(
  EFI_DEVICE_PATH_PROTOCOL *ImagePath,
  EFI_DEVICE_PATH_PROTOCOL *FilePath
  )
{
  EFI_STATUS                    Status;
  UINTN                         Delay;
  EFI_INPUT_KEY                 Key;
  SHELL_FILE_HANDLE             FileHandle;
  EFI_DEVICE_PATH_PROTOCOL      *NewPath;
  EFI_DEVICE_PATH_PROTOCOL      *NamePath;
  CHAR16                        *FileStringPath;
  CHAR16                        *TempSpot;
  UINTN                         NewSize;
  CONST CHAR16                  *MapName;

  Key.UnicodeChar = CHAR_NULL;
  Key.ScanCode    = 0;
  FileHandle      = NULL;

  if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup && ShellInfoObject.ShellInitSettings.FileName != NULL) {
    //
    // launch something else instead
    //
    NewSize = StrSize(ShellInfoObject.ShellInitSettings.FileName);
    if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
      NewSize += StrSize(ShellInfoObject.ShellInitSettings.FileOptions) + sizeof(CHAR16);
    }
    FileStringPath = AllocateZeroPool(NewSize);
    if (FileStringPath == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    StrCpy(FileStringPath, ShellInfoObject.ShellInitSettings.FileName);
    if (ShellInfoObject.ShellInitSettings.FileOptions != NULL) {
      StrCat(FileStringPath, L" ");
      StrCat(FileStringPath, ShellInfoObject.ShellInitSettings.FileOptions);
    }
    Status = RunCommand(FileStringPath);
    FreePool(FileStringPath);
    return (Status);

  }

  //
  // for shell level 0 we do no scripts
  // Without the Startup bit overriding we allow for nostartup to prevent scripts
  //
  if ( (PcdGet8(PcdShellSupportLevel) < 1)
    || (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoStartup && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.Startup)
   ){
    return (EFI_SUCCESS);
  }

  gST->ConOut->EnableCursor(gST->ConOut, FALSE);
  //
  // print out our warning and see if they press a key
  //
  for ( Status = EFI_UNSUPPORTED, Delay = ShellInfoObject.ShellInitSettings.Delay * 10
      ; Delay != 0 && EFI_ERROR(Status)
      ; Delay--
     ){
    ShellPrintHiiEx(0, gST->ConOut->Mode->CursorRow, NULL, STRING_TOKEN (STR_SHELL_STARTUP_QUESTION), ShellInfoObject.HiiHandle, Delay/10);
    gBS->Stall (100000);
    if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    }
  }
  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_CRLF), ShellInfoObject.HiiHandle);
  gST->ConOut->EnableCursor(gST->ConOut, TRUE);

  //
  // ESC was pressed
  //
  if (Status == EFI_SUCCESS && Key.UnicodeChar == 0 && Key.ScanCode == SCAN_ESC) {
    return (EFI_SUCCESS);
  }

  //
  // Try the first location (must be file system)
  //
  MapName = ShellInfoObject.NewEfiShellProtocol->GetMapFromDevicePath(&ImagePath);
  if (MapName != NULL) {
    FileStringPath = NULL;
    NewSize = 0;
    FileStringPath = StrnCatGrow(&FileStringPath, &NewSize, MapName, 0);
    if (FileStringPath == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      TempSpot = StrStr(FileStringPath, L";");
      if (TempSpot != NULL) {
        *TempSpot = CHAR_NULL;
      }
      FileStringPath = StrnCatGrow(&FileStringPath, &NewSize, ((FILEPATH_DEVICE_PATH*)FilePath)->PathName, 0);
      PathRemoveLastItem(FileStringPath);
      FileStringPath = StrnCatGrow(&FileStringPath, &NewSize, mStartupScript, 0);
      Status = ShellInfoObject.NewEfiShellProtocol->OpenFileByName(FileStringPath, &FileHandle, EFI_FILE_MODE_READ);
      FreePool(FileStringPath);
    }
  }
  if (EFI_ERROR(Status)) {
    NamePath = FileDevicePath (NULL, mStartupScript);
    NewPath = AppendDevicePathNode (ImagePath, NamePath);
    FreePool(NamePath);

    //
    // Try the location
    //
    Status = InternalOpenFileDevicePath(NewPath, &FileHandle, EFI_FILE_MODE_READ, 0);
    FreePool(NewPath);
  }
  //
  // If we got a file, run it
  //
  if (!EFI_ERROR(Status) && FileHandle != NULL) {
    Status = RunScriptFileHandle (FileHandle, mStartupScript);
    ShellInfoObject.NewEfiShellProtocol->CloseFile(FileHandle);
  } else {
    FileStringPath = ShellFindFilePath(mStartupScript);
    if (FileStringPath == NULL) {
      //
      // we return success since we dont need to have a startup script
      //
      Status = EFI_SUCCESS;
      ASSERT(FileHandle == NULL);
    } else {
      Status = RunScriptFile(FileStringPath);
      FreePool(FileStringPath);
    }
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
EFIAPI
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

  CurDir  = NULL;

  //
  // Get screen setting to decide size of the command line buffer
  //
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Column, &Row);
  BufferSize  = Column * Row * sizeof (CHAR16);
  CmdLine     = AllocateZeroPool (BufferSize);
  if (CmdLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurDir = ShellInfoObject.NewEfiShellProtocol->GetEnv(L"cwd");

  //
  // Prompt for input
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, 0, gST->ConOut->Mode->CursorRow);

  if (CurDir != NULL && StrLen(CurDir) > 1) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_CURDIR), ShellInfoObject.HiiHandle, CurDir);
  } else {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_SHELL), ShellInfoObject.HiiHandle);
  }

  //
  // Read a line from the console
  //
  Status = ShellInfoObject.NewEfiShellProtocol->ReadFile(ShellInfoObject.NewShellParametersProtocol->StdIn, &BufferSize, CmdLine);

  //
  // Null terminate the string and parse it
  //
  if (!EFI_ERROR (Status)) {
    CmdLine[BufferSize / sizeof (CHAR16)] = CHAR_NULL;
    Status = RunCommand(CmdLine);
  }

  //
  // Done with this command
  //
  FreePool (CmdLine);
  return Status;
}

/**
  Add a buffer to the Buffer To Free List for safely returning buffers to other
  places without risking letting them modify internal shell information.

  @param Buffer   Something to pass to FreePool when the shell is exiting.
**/
VOID*
EFIAPI
AddBufferToFreeList(
  VOID *Buffer
  )
{
  BUFFER_LIST   *BufferListEntry;

  if (Buffer == NULL) {
    return (NULL);
  }

  BufferListEntry = AllocateZeroPool(sizeof(BUFFER_LIST));
  ASSERT(BufferListEntry != NULL);
  BufferListEntry->Buffer = Buffer;
  InsertTailList(&ShellInfoObject.BufferToFreeList.Link, &BufferListEntry->Link);
  return (Buffer);
}

/**
  Add a buffer to the Line History List

  @param Buffer     The line buffer to add.
**/
VOID
EFIAPI
AddLineToCommandHistory(
  IN CONST CHAR16 *Buffer
  )
{
  BUFFER_LIST *Node;

  Node = AllocateZeroPool(sizeof(BUFFER_LIST));
  ASSERT(Node != NULL);
  Node->Buffer = AllocateZeroPool(StrSize(Buffer));
  ASSERT(Node->Buffer != NULL);
  StrCpy(Node->Buffer, Buffer);

  InsertTailList(&ShellInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link);
}

/**
  Checks if a string is an alias for another command.  If yes, then it replaces the alias name
  with the correct command name.

  @param[in, out] CommandString    Upon entry the potential alias.  Upon return the
                                   command name if it was an alias.  If it was not
                                   an alias it will be unchanged.  This function may
                                   change the buffer to fit the command name.

  @retval EFI_SUCCESS             The name was changed.
  @retval EFI_SUCCESS             The name was not an alias.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
ShellConvertAlias(
  IN OUT CHAR16 **CommandString
  )
{
  CONST CHAR16  *NewString;

  NewString = ShellInfoObject.NewEfiShellProtocol->GetAlias(*CommandString, NULL);
  if (NewString == NULL) {
    return (EFI_SUCCESS);
  }
  FreePool(*CommandString);
  *CommandString = AllocateZeroPool(StrSize(NewString));
  if (*CommandString == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  StrCpy(*CommandString, NewString);
  return (EFI_SUCCESS);
}

/**
  Function allocates a new command line and replaces all instances of environment
  variable names that are correctly preset to their values.

  If the return value is not NULL the memory must be caller freed.

  @param[in] OriginalCommandLine    The original command line

  @retval NULL                      An error ocurred.
  @return                           The new command line with no environment variables present.
**/
CHAR16*
EFIAPI
ShellConvertVariables (
  IN CONST CHAR16 *OriginalCommandLine
  )
{
  CONST CHAR16        *MasterEnvList;
  UINTN               NewSize;
  CHAR16              *NewCommandLine1;
  CHAR16              *NewCommandLine2;
  CHAR16              *Temp;
  UINTN               ItemSize;
  CHAR16              *ItemTemp;
  SCRIPT_FILE         *CurrentScriptFile;
  ALIAS_LIST          *AliasListNode;

  ASSERT(OriginalCommandLine != NULL);

  ItemSize          = 0;
  NewSize           = StrSize(OriginalCommandLine);
  CurrentScriptFile = ShellCommandGetCurrentScriptFile();
  Temp              = NULL;

  ///@todo update this to handle the %0 - %9 for scripting only (borrow from line 1256 area) ? ? ?

  //
  // calculate the size required for the post-conversion string...
  //
  if (CurrentScriptFile != NULL) {
    for (AliasListNode = (ALIAS_LIST*)GetFirstNode(&CurrentScriptFile->SubstList)
      ;  !IsNull(&CurrentScriptFile->SubstList, &AliasListNode->Link)
      ;  AliasListNode = (ALIAS_LIST*)GetNextNode(&CurrentScriptFile->SubstList, &AliasListNode->Link)
   ){
      for (Temp = StrStr(OriginalCommandLine, AliasListNode->Alias)
        ;  Temp != NULL
        ;  Temp = StrStr(Temp+1, AliasListNode->Alias)
       ){
        //
        // we need a preceeding and if there is space no ^ preceeding (if no space ignore)
        //
        if ((((Temp-OriginalCommandLine)>2) && *(Temp-2) != L'^') || ((Temp-OriginalCommandLine)<=2)) {
          NewSize += StrSize(AliasListNode->CommandString);
        }
      }
    }
  }

  for (MasterEnvList = EfiShellGetEnv(NULL)
    ;  MasterEnvList != NULL && *MasterEnvList != CHAR_NULL //&& *(MasterEnvList+1) != CHAR_NULL
    ;  MasterEnvList += StrLen(MasterEnvList) + 1
   ){
    if (StrSize(MasterEnvList) > ItemSize) {
      ItemSize = StrSize(MasterEnvList);
    }
    for (Temp = StrStr(OriginalCommandLine, MasterEnvList)
      ;  Temp != NULL
      ;  Temp = StrStr(Temp+1, MasterEnvList)
     ){
      //
      // we need a preceeding and following % and if there is space no ^ preceeding (if no space ignore)
      //
      if (*(Temp-1) == L'%' && *(Temp+StrLen(MasterEnvList)) == L'%' &&
        ((((Temp-OriginalCommandLine)>2) && *(Temp-2) != L'^') || ((Temp-OriginalCommandLine)<=2))) {
        NewSize+=StrSize(EfiShellGetEnv(MasterEnvList));
      }
    }
  }

  //
  // Quick out if none were found...
  //
  if (NewSize == StrSize(OriginalCommandLine)) {
    ASSERT(Temp == NULL);
    Temp = StrnCatGrow(&Temp, NULL, OriginalCommandLine, 0);
    return (Temp);
  }

  //
  // now do the replacements...
  //
  NewCommandLine1 = AllocateZeroPool(NewSize);
  NewCommandLine2 = AllocateZeroPool(NewSize);
  ItemTemp        = AllocateZeroPool(ItemSize+(2*sizeof(CHAR16)));
  if (NewCommandLine1 == NULL || NewCommandLine2 == NULL || ItemTemp == NULL) {
    SHELL_FREE_NON_NULL(NewCommandLine1);
    SHELL_FREE_NON_NULL(NewCommandLine2);
    SHELL_FREE_NON_NULL(ItemTemp);
    return (NULL);
  }
  StrCpy(NewCommandLine1, OriginalCommandLine);
  for (MasterEnvList = EfiShellGetEnv(NULL)
    ;  MasterEnvList != NULL && *MasterEnvList != CHAR_NULL //&& *(MasterEnvList+1) != CHAR_NULL
    ;  MasterEnvList += StrLen(MasterEnvList) + 1
   ){
    StrCpy(ItemTemp, L"%");
    StrCat(ItemTemp, MasterEnvList);
    StrCat(ItemTemp, L"%");
    ShellCopySearchAndReplace(NewCommandLine1, NewCommandLine2, NewSize, ItemTemp, EfiShellGetEnv(MasterEnvList), TRUE, FALSE);
    StrCpy(NewCommandLine1, NewCommandLine2);
  }
  if (CurrentScriptFile != NULL) {
    for (AliasListNode = (ALIAS_LIST*)GetFirstNode(&CurrentScriptFile->SubstList)
      ;  !IsNull(&CurrentScriptFile->SubstList, &AliasListNode->Link)
      ;  AliasListNode = (ALIAS_LIST*)GetNextNode(&CurrentScriptFile->SubstList, &AliasListNode->Link)
   ){
    ShellCopySearchAndReplace(NewCommandLine1, NewCommandLine2, NewSize, AliasListNode->Alias, AliasListNode->CommandString, TRUE, FALSE);
    StrCpy(NewCommandLine1, NewCommandLine2);
    }
  }

  FreePool(NewCommandLine2);
  FreePool(ItemTemp);

  return (NewCommandLine1);
}

/**
  Internal function to run a command line with pipe usage.

  @param[in] CmdLine        The pointer to the command line.
  @param[in] StdIn          The pointer to the Standard input.
  @param[in] StdOut         The pointer to the Standard output.

  @retval EFI_SUCCESS       The split command is executed successfully.
  @retval other             Some error occurs when executing the split command.
**/
EFI_STATUS
EFIAPI
RunSplitCommand(
  IN CONST CHAR16             *CmdLine,
  IN       SHELL_FILE_HANDLE  *StdIn,
  IN       SHELL_FILE_HANDLE  *StdOut
  )
{
  EFI_STATUS        Status;
  CHAR16            *NextCommandLine;
  CHAR16            *OurCommandLine;
  UINTN             Size1;
  UINTN             Size2;
  SPLIT_LIST        *Split;
  SHELL_FILE_HANDLE *TempFileHandle;
  BOOLEAN           Unicode;

  ASSERT(StdOut == NULL);

  ASSERT(StrStr(CmdLine, L"|") != NULL);

  Status          = EFI_SUCCESS;
  NextCommandLine = NULL;
  OurCommandLine  = NULL;
  Size1           = 0;
  Size2           = 0;

  NextCommandLine = StrnCatGrow(&NextCommandLine, &Size1, StrStr(CmdLine, L"|")+1, 0);
  OurCommandLine  = StrnCatGrow(&OurCommandLine , &Size2, CmdLine                , StrStr(CmdLine, L"|") - CmdLine);

  if (NextCommandLine == NULL || OurCommandLine == NULL) {
    SHELL_FREE_NON_NULL(OurCommandLine);
    SHELL_FREE_NON_NULL(NextCommandLine);
    return (EFI_OUT_OF_RESOURCES);
  } else if (StrStr(OurCommandLine, L"|") != NULL || Size1 == 0 || Size2 == 0) {
    SHELL_FREE_NON_NULL(OurCommandLine);
    SHELL_FREE_NON_NULL(NextCommandLine);
    return (EFI_INVALID_PARAMETER);
  } else if (NextCommandLine[0] != CHAR_NULL &&
      NextCommandLine[0] == L'a' &&
      NextCommandLine[1] == L' '
     ){
    CopyMem(NextCommandLine, NextCommandLine+1, StrSize(NextCommandLine) - sizeof(NextCommandLine[0]));
    Unicode = FALSE;
  } else {
    Unicode = TRUE;
  }


  //
  // make a SPLIT_LIST item and add to list
  //
  Split = AllocateZeroPool(sizeof(SPLIT_LIST));
  ASSERT(Split != NULL);
  Split->SplitStdIn   = StdIn;
  Split->SplitStdOut  = ConvertEfiFileProtocolToShellHandle(CreateFileInterfaceMem(Unicode), NULL);
  ASSERT(Split->SplitStdOut != NULL);
  InsertHeadList(&ShellInfoObject.SplitList.Link, &Split->Link);

  Status = RunCommand(OurCommandLine);

  //
  // move the output from the first to the in to the second.
  //
  TempFileHandle      = Split->SplitStdOut;
  if (Split->SplitStdIn == StdIn) {
    Split->SplitStdOut = NULL;
  } else {
    Split->SplitStdOut  = Split->SplitStdIn;
  }
  Split->SplitStdIn   = TempFileHandle;
  ShellInfoObject.NewEfiShellProtocol->SetFilePosition(ConvertShellHandleToEfiFileProtocol(Split->SplitStdIn), 0);

  if (!EFI_ERROR(Status)) {
    Status = RunCommand(NextCommandLine);
  }

  //
  // remove the top level from the ScriptList
  //
  ASSERT((SPLIT_LIST*)GetFirstNode(&ShellInfoObject.SplitList.Link) == Split);
  RemoveEntryList(&Split->Link);

  //
  // Note that the original StdIn is now the StdOut...
  //
  if (Split->SplitStdOut != NULL && Split->SplitStdOut != StdIn) {
    ShellInfoObject.NewEfiShellProtocol->CloseFile(ConvertShellHandleToEfiFileProtocol(Split->SplitStdOut));
  }
  if (Split->SplitStdIn != NULL) {
    ShellInfoObject.NewEfiShellProtocol->CloseFile(ConvertShellHandleToEfiFileProtocol(Split->SplitStdIn));
  }

  FreePool(Split);
  FreePool(NextCommandLine);
  FreePool(OurCommandLine);

  return (Status);
}

/**
  Function will process and run a command line.

  This will determine if the command line represents an internal shell 
  command or dispatch an external application.

  @param[in] CmdLine      The command line to parse.

  @retval EFI_SUCCESS     The command was completed.
  @retval EFI_ABORTED     The command's operation was aborted.
**/
EFI_STATUS
EFIAPI
RunCommand(
  IN CONST CHAR16   *CmdLine
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                StatusCode;
  CHAR16                    *CommandName;
  SHELL_STATUS              ShellStatus;
  UINTN                     Argc;
  CHAR16                    **Argv;
  BOOLEAN                   LastError;
  CHAR16                    LeString[11];
  CHAR16                    *PostAliasCmdLine;
  UINTN                     PostAliasSize;
  CHAR16                    *PostVariableCmdLine;
  CHAR16                    *CommandWithPath;
  CONST EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  CONST CHAR16              *TempLocation;
  CONST CHAR16              *TempLocation2;
  SHELL_FILE_HANDLE         OriginalStdIn;
  SHELL_FILE_HANDLE         OriginalStdOut;
  SHELL_FILE_HANDLE         OriginalStdErr;
  SYSTEM_TABLE_INFO         OriginalSystemTableInfo;
  CHAR16                    *TempLocation3;
  UINTN                     Count;
  UINTN                     Count2;
  CHAR16                    *CleanOriginal;
  SPLIT_LIST                *Split;

  ASSERT(CmdLine != NULL);
  if (StrLen(CmdLine) == 0) {
    return (EFI_SUCCESS);
  }

  CommandName         = NULL;
  PostVariableCmdLine = NULL;
  PostAliasCmdLine    = NULL;
  CommandWithPath     = NULL;
  DevPath             = NULL;
  Status              = EFI_SUCCESS;
  CleanOriginal       = NULL;
  Split               = NULL;

  CleanOriginal = StrnCatGrow(&CleanOriginal, NULL, CmdLine, 0);
  if (CleanOriginal == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  while (CleanOriginal[StrLen(CleanOriginal)-1] == L' ') {
    CleanOriginal[StrLen(CleanOriginal)-1] = CHAR_NULL;
  }
  while (CleanOriginal[0] == L' ') {
    CopyMem(CleanOriginal, CleanOriginal+1, StrSize(CleanOriginal) - sizeof(CleanOriginal[0]));
  }

  CommandName = NULL;
  if (StrStr(CleanOriginal, L" ") == NULL){
    StrnCatGrow(&CommandName, NULL, CleanOriginal, 0);
  } else {
    StrnCatGrow(&CommandName, NULL, CleanOriginal, StrStr(CleanOriginal, L" ") - CleanOriginal);
  }

  ASSERT(PostAliasCmdLine == NULL);
  if (!ShellCommandIsCommandOnList(CommandName)) {
    //
    // Convert via alias
    //
    Status = ShellConvertAlias(&CommandName);
    PostAliasSize = 0;
    PostAliasCmdLine = StrnCatGrow(&PostAliasCmdLine, &PostAliasSize, CommandName, 0);
    PostAliasCmdLine = StrnCatGrow(&PostAliasCmdLine, &PostAliasSize, StrStr(CleanOriginal, L" "), 0);
    ASSERT_EFI_ERROR(Status);
  } else {
    PostAliasCmdLine = StrnCatGrow(&PostAliasCmdLine, NULL, CleanOriginal, 0);
  }

  if (CleanOriginal != NULL) {
    FreePool(CleanOriginal);
    CleanOriginal = NULL;
  }

  if (CommandName != NULL) {
    FreePool(CommandName);
    CommandName = NULL;
  }

  PostVariableCmdLine = ShellConvertVariables(PostAliasCmdLine);

  //
  // we can now free the modified by alias command line
  //
  if (PostAliasCmdLine != NULL) {
    FreePool(PostAliasCmdLine);
    PostAliasCmdLine = NULL;
  }

  if (PostVariableCmdLine == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  while (PostVariableCmdLine[StrLen(PostVariableCmdLine)-1] == L' ') {
    PostVariableCmdLine[StrLen(PostVariableCmdLine)-1] = CHAR_NULL;
  }
  while (PostVariableCmdLine[0] == L' ') {
    CopyMem(PostVariableCmdLine, PostVariableCmdLine+1, StrSize(PostVariableCmdLine) - sizeof(PostVariableCmdLine[0]));
  }

  //
  // We dont do normal processing with a split command line (output from one command input to another)
  //
  TempLocation3 = NULL;
  if (StrStr(PostVariableCmdLine, L"|") != NULL) {
    for (TempLocation3 = PostVariableCmdLine ; TempLocation3 != NULL && *TempLocation3 != CHAR_NULL ; TempLocation3++) {
      if (*TempLocation3 == L'^' && *(TempLocation3+1) == L'|') {
        TempLocation3++;
      } else if (*TempLocation3 == L'|') {
        break;
      }
    }
  }
  if (TempLocation3 != NULL && *TempLocation3 != CHAR_NULL) {
    //
    // are we in an existing split???
    //
    if (!IsListEmpty(&ShellInfoObject.SplitList.Link)) {
      Split = (SPLIT_LIST*)GetFirstNode(&ShellInfoObject.SplitList.Link);
    }

    if (Split == NULL) {
      Status = RunSplitCommand(PostVariableCmdLine, NULL, NULL);
    } else {
      Status = RunSplitCommand(PostVariableCmdLine, Split->SplitStdIn, Split->SplitStdOut);
    }
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_SPLIT), ShellInfoObject.HiiHandle, PostVariableCmdLine);
    }
  } else {

    //
    // If this is a mapped drive change handle that...
    //
    if (PostVariableCmdLine[(StrLen(PostVariableCmdLine)-1)] == L':' && StrStr(PostVariableCmdLine, L" ") == NULL) {
      Status = ShellInfoObject.NewEfiShellProtocol->SetCurDir(NULL, PostVariableCmdLine);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_MAPPING), ShellInfoObject.HiiHandle, PostVariableCmdLine);
      }
      FreePool(PostVariableCmdLine);
      return (Status);
    }

    ///@todo update this section to divide into 3 ways - run internal command, run split (above), and run an external file...
    ///      We waste a lot of time doing processing like StdIn,StdOut,Argv,Argc for things that are external files...



    Status = UpdateStdInStdOutStdErr(ShellInfoObject.NewShellParametersProtocol, PostVariableCmdLine, &OriginalStdIn, &OriginalStdOut, &OriginalStdErr, &OriginalSystemTableInfo);
    if (EFI_ERROR(Status)) {
      if (Status == EFI_NOT_FOUND) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_REDUNDA_REDIR), ShellInfoObject.HiiHandle);
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_REDIR), ShellInfoObject.HiiHandle);
      }
    } else {
      while (PostVariableCmdLine[StrLen(PostVariableCmdLine)-1] == L' ') {
        PostVariableCmdLine[StrLen(PostVariableCmdLine)-1] = CHAR_NULL;
      }
      while (PostVariableCmdLine[0] == L' ') {
        CopyMem(PostVariableCmdLine, PostVariableCmdLine+1, StrSize(PostVariableCmdLine) - sizeof(PostVariableCmdLine[0]));
      }

      //
      // get the argc and argv updated for internal commands
      //
      Status = UpdateArgcArgv(ShellInfoObject.NewShellParametersProtocol, PostVariableCmdLine, &Argv, &Argc);
      ASSERT_EFI_ERROR(Status);

      for (Count = 0 ; Count < ShellInfoObject.NewShellParametersProtocol->Argc ; Count++) {
        if (StrStr(ShellInfoObject.NewShellParametersProtocol->Argv[Count], L"-?") == ShellInfoObject.NewShellParametersProtocol->Argv[Count]
        ||  (ShellInfoObject.NewShellParametersProtocol->Argv[0][0] == L'?' && ShellInfoObject.NewShellParametersProtocol->Argv[0][1] == CHAR_NULL)
          ) {
          //
          // We need to redo the arguments since a parameter was -?
          // move them all down 1 to the end, then up one then replace the first with help
          //
          FreePool(ShellInfoObject.NewShellParametersProtocol->Argv[Count]);
          ShellInfoObject.NewShellParametersProtocol->Argv[Count] = NULL;
          for (Count2 = Count ; (Count2 + 1) < ShellInfoObject.NewShellParametersProtocol->Argc ; Count2++) {
            ShellInfoObject.NewShellParametersProtocol->Argv[Count2] = ShellInfoObject.NewShellParametersProtocol->Argv[Count2+1];
          }
          ShellInfoObject.NewShellParametersProtocol->Argv[Count2] = NULL;
          for (Count2 = ShellInfoObject.NewShellParametersProtocol->Argc -1 ; Count2 > 0 ; Count2--) {
            ShellInfoObject.NewShellParametersProtocol->Argv[Count2] = ShellInfoObject.NewShellParametersProtocol->Argv[Count2-1];
          }
          ShellInfoObject.NewShellParametersProtocol->Argv[0] = NULL;
          ShellInfoObject.NewShellParametersProtocol->Argv[0] = StrnCatGrow(&ShellInfoObject.NewShellParametersProtocol->Argv[0], NULL, L"help", 0);
          break;
        }
      }

      //
      // command or file?
      //
      if (ShellCommandIsCommandOnList(ShellInfoObject.NewShellParametersProtocol->Argv[0])) {
        //
        // Run the command (which was converted if it was an alias)
        //
        if (!EFI_ERROR(Status))  {
          Status = ShellCommandRunCommandHandler(ShellInfoObject.NewShellParametersProtocol->Argv[0], &ShellStatus, &LastError);
          ASSERT_EFI_ERROR(Status);
          UnicodeSPrint(LeString, sizeof(LeString)*sizeof(LeString[0]), L"0x%08x", ShellStatus);
          DEBUG_CODE(InternalEfiShellSetEnv(L"DebugLasterror", LeString, TRUE););
          if (LastError) {
            InternalEfiShellSetEnv(L"Lasterror", LeString, TRUE);
          }
          //
          // Pass thru the exitcode from the app.
          //
          if (ShellCommandGetExit()) {
            Status = ShellStatus;
          } else if (ShellStatus != 0 && IsScriptOnlyCommand(ShellInfoObject.NewShellParametersProtocol->Argv[0])) {
            Status = EFI_ABORTED;
          }
        }
      } else {
        //
        // run an external file (or script)
        //
        if (StrStr(ShellInfoObject.NewShellParametersProtocol->Argv[0], L":") != NULL) {
          ASSERT (CommandWithPath == NULL);
          if (ShellIsFile(ShellInfoObject.NewShellParametersProtocol->Argv[0]) == EFI_SUCCESS) {
            CommandWithPath = StrnCatGrow(&CommandWithPath, NULL, ShellInfoObject.NewShellParametersProtocol->Argv[0], 0);
          }
        }
        if (CommandWithPath == NULL) {
          CommandWithPath = ShellFindFilePathEx(ShellInfoObject.NewShellParametersProtocol->Argv[0], mExecutableExtensions);
        }
        if (CommandWithPath == NULL || ShellIsDirectory(CommandWithPath) == EFI_SUCCESS) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_NOT_FOUND), ShellInfoObject.HiiHandle, ShellInfoObject.NewShellParametersProtocol->Argv[0]);
        } else {
          //
          // Check if it's a NSH (script) file.
          //
          TempLocation = CommandWithPath+StrLen(CommandWithPath)-4;
          TempLocation2 = mScriptExtension;
          if ((StrLen(CommandWithPath) > 4) && (StringNoCaseCompare((VOID*)(&TempLocation), (VOID*)(&TempLocation2)) == 0)) {
            Status = RunScriptFile (CommandWithPath);
          } else {
            DevPath = ShellInfoObject.NewEfiShellProtocol->GetDevicePathFromFilePath(CommandWithPath);
            ASSERT(DevPath != NULL);
            Status = InternalShellExecuteDevicePath(
              &gImageHandle,
              DevPath,
              PostVariableCmdLine,
              NULL,
              &StatusCode
             );

            //
            // Updatet last error status.
            //
            UnicodeSPrint(LeString, sizeof(LeString)*sizeof(LeString[0]), L"0x%08x", StatusCode);
            DEBUG_CODE(InternalEfiShellSetEnv(L"DebugLasterror", LeString, TRUE););
            InternalEfiShellSetEnv(L"Lasterror", LeString, TRUE);
          }
        }
      }

      //
      // Print some error info.
      //
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_ERROR), ShellInfoObject.HiiHandle, (VOID*)(Status));
      }

      CommandName = StrnCatGrow(&CommandName, NULL, ShellInfoObject.NewShellParametersProtocol->Argv[0], 0);

      RestoreArgcArgv(ShellInfoObject.NewShellParametersProtocol, &Argv, &Argc);

      RestoreStdInStdOutStdErr(ShellInfoObject.NewShellParametersProtocol, &OriginalStdIn, &OriginalStdOut, &OriginalStdErr, &OriginalSystemTableInfo);
    }
    if (CommandName != NULL) {
      if (ShellCommandGetCurrentScriptFile() != NULL &&  !IsScriptOnlyCommand(CommandName)) {
        //
        // if this is NOT a scipt only command return success so the script won't quit.
        // prevent killing the script - this is the only place where we know the actual command name (after alias and variable replacement...)
        //
        Status = EFI_SUCCESS;
      }
    }
  }

  SHELL_FREE_NON_NULL(CommandName);
  SHELL_FREE_NON_NULL(CommandWithPath);
  SHELL_FREE_NON_NULL(PostVariableCmdLine);

  return (Status);
}

STATIC CONST UINT16 InvalidChars[] = {L'*', L'?', L'<', L'>', L'\\', L'/', L'\"', 0x0001, 0x0002};
/**
  Function determins if the CommandName COULD be a valid command.  It does not determine whether
  this is a valid command.  It only checks for invalid characters.

  @param[in] CommandName    The name to check

  @retval TRUE              CommandName could be a command name
  @retval FALSE             CommandName could not be a valid command name
**/
BOOLEAN
EFIAPI
IsValidCommandName(
  IN CONST CHAR16     *CommandName
  )
{
  UINTN Count;
  if (CommandName == NULL) {
    ASSERT(FALSE);
    return (FALSE);
  }
  for ( Count = 0
      ; Count < sizeof(InvalidChars) / sizeof(InvalidChars[0])
      ; Count++
     ){
    if (ScanMem16(CommandName, StrSize(CommandName), InvalidChars[Count]) != NULL) {
      return (FALSE);
    }
  }
  return (TRUE);
}

/**
  Function to process a NSH script file via SHELL_FILE_HANDLE.

  @param[in] Handle             The handle to the already opened file.
  @param[in] Name               The name of the script file.

  @retval EFI_SUCCESS           the script completed sucessfully
**/
EFI_STATUS
EFIAPI
RunScriptFileHandle (
  IN SHELL_FILE_HANDLE  Handle,
  IN CONST CHAR16       *Name
  )
{
  EFI_STATUS          Status;
  SCRIPT_FILE         *NewScriptFile;
  UINTN               LoopVar;
  CHAR16              *CommandLine;
  CHAR16              *CommandLine2;
  CHAR16              *CommandLine3;
  SCRIPT_COMMAND_LIST *LastCommand;
  BOOLEAN             Ascii;
  BOOLEAN             PreScriptEchoState;
  BOOLEAN             PreCommandEchoState;
  CONST CHAR16        *CurDir;
  UINTN               LineCount;
  CHAR16              LeString[50];

  ASSERT(!ShellCommandGetScriptExit());

  PreScriptEchoState = ShellCommandGetEchoState();

  NewScriptFile = (SCRIPT_FILE*)AllocateZeroPool(sizeof(SCRIPT_FILE));
  if (NewScriptFile == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // Set up the name
  //
  ASSERT(NewScriptFile->ScriptName == NULL);
  NewScriptFile->ScriptName = StrnCatGrow(&NewScriptFile->ScriptName, NULL, Name, 0);
  if (NewScriptFile->ScriptName == NULL) {
    DeleteScriptFileStruct(NewScriptFile);
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // Save the parameters (used to replace %0 to %9 later on)
  //
  NewScriptFile->Argc = ShellInfoObject.NewShellParametersProtocol->Argc;
  if (NewScriptFile->Argc != 0) {
    NewScriptFile->Argv = (CHAR16**)AllocateZeroPool(NewScriptFile->Argc * sizeof(CHAR16*));
    if (NewScriptFile->Argv == NULL) {
      DeleteScriptFileStruct(NewScriptFile);
      return (EFI_OUT_OF_RESOURCES);
    }
    for (LoopVar = 0 ; LoopVar < 10 && LoopVar < NewScriptFile->Argc; LoopVar++) {
      ASSERT(NewScriptFile->Argv[LoopVar] == NULL);
      NewScriptFile->Argv[LoopVar] = StrnCatGrow(&NewScriptFile->Argv[LoopVar], NULL, ShellInfoObject.NewShellParametersProtocol->Argv[LoopVar], 0);
      if (NewScriptFile->Argv[LoopVar] == NULL) {
        DeleteScriptFileStruct(NewScriptFile);
        return (EFI_OUT_OF_RESOURCES);
      }
    }
  } else {
    NewScriptFile->Argv = NULL;
  }

  InitializeListHead(&NewScriptFile->CommandList);
  InitializeListHead(&NewScriptFile->SubstList);

  //
  // Now build the list of all script commands.
  //
  LineCount = 0;
  while(!ShellFileHandleEof(Handle)) {
    CommandLine = ShellFileHandleReturnLine(Handle, &Ascii);
    LineCount++;
    if (CommandLine == NULL || StrLen(CommandLine) == 0) {
      continue;
    }
    NewScriptFile->CurrentCommand = AllocateZeroPool(sizeof(SCRIPT_COMMAND_LIST));
    if (NewScriptFile->CurrentCommand == NULL) {
      DeleteScriptFileStruct(NewScriptFile);
      return (EFI_OUT_OF_RESOURCES);
    }

    NewScriptFile->CurrentCommand->Cl   = CommandLine;
    NewScriptFile->CurrentCommand->Data = NULL;
    NewScriptFile->CurrentCommand->Line = LineCount;

    InsertTailList(&NewScriptFile->CommandList, &NewScriptFile->CurrentCommand->Link);
  }

  //
  // Add this as the topmost script file
  //
  ShellCommandSetNewScript (NewScriptFile);

  //
  // Now enumerate through the commands and run each one.
  //
  CommandLine = AllocateZeroPool(PcdGet16(PcdShellPrintBufferSize));
  if (CommandLine == NULL) {
    DeleteScriptFileStruct(NewScriptFile);
    return (EFI_OUT_OF_RESOURCES);
  }
  CommandLine2 = AllocateZeroPool(PcdGet16(PcdShellPrintBufferSize));
  if (CommandLine2 == NULL) {
    FreePool(CommandLine);
    DeleteScriptFileStruct(NewScriptFile);
    return (EFI_OUT_OF_RESOURCES);
  }

  for ( NewScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)GetFirstNode(&NewScriptFile->CommandList)
      ; !IsNull(&NewScriptFile->CommandList, &NewScriptFile->CurrentCommand->Link)
      ; // conditional increment in the body of the loop
  ){
    ASSERT(CommandLine2 != NULL);
    StrCpy(CommandLine2, NewScriptFile->CurrentCommand->Cl);

    //
    // NULL out comments
    //
    for (CommandLine3 = CommandLine2 ; CommandLine3 != NULL && *CommandLine3 != CHAR_NULL ; CommandLine3++) {
      if (*CommandLine3 == L'^') {
        if (*(CommandLine3+1) == L'#' || *(CommandLine3+1) == L':') {
          CopyMem(CommandLine3, CommandLine3+1, StrSize(CommandLine3) - sizeof(CommandLine3[0]));
        }
      } else if (*CommandLine3 == L'#') {
        *CommandLine3 = CHAR_NULL;
      }
    }

    if (CommandLine2 != NULL && StrLen(CommandLine2) >= 1) {
      //
      // Due to variability in starting the find and replace action we need to have both buffers the same.
      //
      StrCpy(CommandLine, CommandLine2);

      //
      // Remove the %0 to %9 from the command line (if we have some arguments)
      //
      if (NewScriptFile->Argv != NULL) {
        switch (NewScriptFile->Argc) {
          default:
            Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%9", NewScriptFile->Argv[9], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 9:
            Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%8", NewScriptFile->Argv[8], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 8:
            Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%7", NewScriptFile->Argv[7], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 7:
            Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%6", NewScriptFile->Argv[6], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 6:
            Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%5", NewScriptFile->Argv[5], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 5:
            Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%4", NewScriptFile->Argv[4], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 4:
            Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%3", NewScriptFile->Argv[3], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 3:
            Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%2", NewScriptFile->Argv[2], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 2:
            Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%1", NewScriptFile->Argv[1], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
          case 1:
            Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%0", NewScriptFile->Argv[0], FALSE, TRUE);
            ASSERT_EFI_ERROR(Status);
            break;
          case 0:
            break;
        }
      }
      Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%1", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%2", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%3", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%4", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%5", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%6", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%7", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine,  CommandLine2, PcdGet16 (PcdShellPrintBufferSize), L"%8", L"\"\"", FALSE, FALSE);
      Status = ShellCopySearchAndReplace(CommandLine2,  CommandLine, PcdGet16 (PcdShellPrintBufferSize), L"%9", L"\"\"", FALSE, FALSE);

      StrCpy(CommandLine2, CommandLine);

      LastCommand = NewScriptFile->CurrentCommand;

      for (CommandLine3 = CommandLine2 ; CommandLine3[0] == L' ' ; CommandLine3++);

      if (CommandLine3 != NULL && CommandLine3[0] == L':' ) {
        //
        // This line is a goto target / label
        //
      } else {
        if (CommandLine3 != NULL && StrLen(CommandLine3) > 0) {
          if (CommandLine3[0] == L'@') {
            //
            // We need to save the current echo state
            // and disable echo for just this command.
            //
            PreCommandEchoState = ShellCommandGetEchoState();
            ShellCommandSetEchoState(FALSE);
            Status = RunCommand(CommandLine3+1);

            //
            // Now restore the pre-'@' echo state.
            //
            ShellCommandSetEchoState(PreCommandEchoState);
          } else {
            if (ShellCommandGetEchoState()) {
              CurDir = ShellInfoObject.NewEfiShellProtocol->GetEnv(L"cwd");
              if (CurDir != NULL && StrLen(CurDir) > 1) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_CURDIR), ShellInfoObject.HiiHandle, CurDir);
              } else {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_SHELL), ShellInfoObject.HiiHandle);
              }
              ShellPrintEx(-1, -1, L"%s\r\n", CommandLine2);
            }
            Status = RunCommand(CommandLine3);
          }
        }

        if (ShellCommandGetScriptExit()) {
          UnicodeSPrint(LeString, sizeof(LeString)*sizeof(LeString[0]), L"0x%Lx", ShellCommandGetExitCode());
          DEBUG_CODE(InternalEfiShellSetEnv(L"DebugLasterror", LeString, TRUE););
          InternalEfiShellSetEnv(L"Lasterror", LeString, TRUE);

          ShellCommandRegisterExit(FALSE, 0);
          Status = EFI_SUCCESS;
          break;
        }
        if (EFI_ERROR(Status)) {
          break;
        }
        if (ShellCommandGetExit()) {
          break;
        }
      }
      //
      // If that commend did not update the CurrentCommand then we need to advance it...
      //
      if (LastCommand == NewScriptFile->CurrentCommand) {
        NewScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)GetNextNode(&NewScriptFile->CommandList, &NewScriptFile->CurrentCommand->Link);
        if (!IsNull(&NewScriptFile->CommandList, &NewScriptFile->CurrentCommand->Link)) {
          NewScriptFile->CurrentCommand->Reset = TRUE;
        }
      }
    } else {
      NewScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)GetNextNode(&NewScriptFile->CommandList, &NewScriptFile->CurrentCommand->Link);
      if (!IsNull(&NewScriptFile->CommandList, &NewScriptFile->CurrentCommand->Link)) {
        NewScriptFile->CurrentCommand->Reset = TRUE;
      }
    }
  }


  FreePool(CommandLine);
  FreePool(CommandLine2);
  ShellCommandSetNewScript (NULL);

  //
  // Only if this was the last script reset the state.
  //
  if (ShellCommandGetCurrentScriptFile()==NULL) {
    ShellCommandSetEchoState(PreScriptEchoState);
  }
  return (EFI_SUCCESS);
}

/**
  Function to process a NSH script file.

  @param[in] ScriptPath         Pointer to the script file name (including file system path).

  @retval EFI_SUCCESS           the script completed sucessfully
**/
EFI_STATUS
EFIAPI
RunScriptFile (
  IN CONST CHAR16 *ScriptPath
  )
{
  EFI_STATUS          Status;
  SHELL_FILE_HANDLE   FileHandle;

  if (ShellIsFile(ScriptPath) != EFI_SUCCESS) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = ShellOpenFileByName(ScriptPath, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  Status = RunScriptFileHandle(FileHandle, ScriptPath);

  ShellCloseFile(&FileHandle);

  return (Status);
}
