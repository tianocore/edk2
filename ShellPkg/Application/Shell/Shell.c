/** @file
  This is THE shell (application)

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2013, Hewlett-Packard Development Company, L.P.
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
    {{
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0
    }},
    0,
    NULL,
    NULL
  },
  {{NULL, NULL}, NULL},
  {
    {{NULL, NULL}, NULL},
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
  {{NULL, NULL}, NULL, NULL},
  {{NULL, NULL}, NULL, NULL},
  NULL,
  NULL,
  NULL,
  NULL,
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
  Cleans off leading and trailing spaces and tabs.

  @param[in] String pointer to the string to trim them off.
**/
EFI_STATUS
EFIAPI
TrimSpaces(
  IN CHAR16 **String
  )
{
  ASSERT(String != NULL);
  ASSERT(*String!= NULL);
  //
  // Remove any spaces and tabs at the beginning of the (*String).
  //
  while (((*String)[0] == L' ') || ((*String)[0] == L'\t')) {
    CopyMem((*String), (*String)+1, StrSize((*String)) - sizeof((*String)[0]));
  }

  //
  // Remove any spaces and tabs at the end of the (*String).
  //
  while ((StrLen (*String) > 0) && (((*String)[StrLen((*String))-1] == L' ') || ((*String)[StrLen((*String))-1] == L'\t'))) {
    (*String)[StrLen((*String))-1] = CHAR_NULL;
  }

  return (EFI_SUCCESS);
}

/**
  Find a command line contains a split operation

  @param[in] CmdLine      The command line to parse.

  @retval                 A pointer to the | character in CmdLine or NULL if not present.
**/
CONST CHAR16*
EFIAPI
FindSplit(
  IN CONST CHAR16 *CmdLine
  )
{
  CONST CHAR16 *TempSpot;
  TempSpot = NULL;
  if (StrStr(CmdLine, L"|") != NULL) {
    for (TempSpot = CmdLine ; TempSpot != NULL && *TempSpot != CHAR_NULL ; TempSpot++) {
      if (*TempSpot == L'^' && *(TempSpot+1) == L'|') {
        TempSpot++;
      } else if (*TempSpot == L'|') {
        break;
      }
    }
  }
  return (TempSpot);
}

/**
  Determine if a command line contains a split operation

  @param[in] CmdLine      The command line to parse.

  @retval TRUE            CmdLine has a valid split.
  @retval FALSE           CmdLine does not have a valid split.
**/
BOOLEAN
EFIAPI
ContainsSplit(
  IN CONST CHAR16 *CmdLine
  )
{
  CONST CHAR16 *TempSpot;
  TempSpot = FindSplit(CmdLine);
  return (BOOLEAN) ((TempSpot != NULL) && (*TempSpot != CHAR_NULL));
}

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
      &ShellInfoObject.CtrlSNotifyHandle3);
  }  
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR(Status)) {
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &ShellInfoObject.CtrlSNotifyHandle4);
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
        STRING_TOKEN (STR_VER_OUTPUT_MAIN_SHELL),
        ShellInfoObject.HiiHandle,
        SupportLevel[PcdGet8(PcdShellSupportLevel)],
        gEfiShellProtocol->MajorVersion,
        gEfiShellProtocol->MinorVersion
       );

      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_VER_OUTPUT_MAIN_SUPPLIER),
        ShellInfoObject.HiiHandle,
        (CHAR16 *) PcdGetPtr (PcdShellSupplier)
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

      if (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.Exit && !ShellCommandGetExit() && (PcdGet8(PcdShellSupportLevel) >= 3 || PcdGetBool(PcdShellForceConsole)) && !EFI_ERROR(Status) && !ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleIn) {
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
      InternalEfiShellSetEnv(L"cwd", NULL, TRUE);
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
  {L"-_exit",         TypeFlag},
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
  ShellInfoObject.ShellInitSettings.BitUnion.Bits.Exit         = ShellCommandLineGetFlag(Package, L"-_exit");

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
  for ( Status = EFI_UNSUPPORTED, Delay = ShellInfoObject.ShellInitSettings.Delay
      ; Delay != 0 && EFI_ERROR(Status)
      ; Delay--
     ){
    ShellPrintHiiEx(0, gST->ConOut->Mode->CursorRow, NULL, STRING_TOKEN (STR_SHELL_STARTUP_QUESTION), ShellInfoObject.HiiHandle, Delay);
    gBS->Stall (1000000);
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
    Status = RunScriptFile (mStartupScript, FileHandle, L"", ShellInfoObject.NewShellParametersProtocol);
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
      Status = RunScriptFile(FileStringPath, NULL, L"", ShellInfoObject.NewShellParametersProtocol);
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
  CHAR16              *Temp2;
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

    //
    // Remove non-existant environment variables in scripts only
    //
    for (Temp = NewCommandLine1 ; Temp != NULL ; ) {
      Temp = StrStr(Temp, L"%");
      if (Temp == NULL) {
        break;
      }
      while (*(Temp - 1) == L'^') {
        Temp = StrStr(Temp + 1, L"%");
        if (Temp == NULL) {
          break;
       }
      }
      if (Temp == NULL) {
        break;
      }
      
      Temp2 = StrStr(Temp + 1, L"%");
      if (Temp2 == NULL) {
        break;
      }
      while (*(Temp2 - 1) == L'^') {
        Temp2 = StrStr(Temp2 + 1, L"%");
        if (Temp2 == NULL) {
          break;
        }
      }
      if (Temp2 == NULL) {
        break;
      }
      
      Temp2++;
      CopyMem(Temp, Temp2, StrSize(Temp2));
    }

  }

  //
  // Now cleanup any straggler intentionally ignored "%" characters
  //
  ShellCopySearchAndReplace(NewCommandLine1, NewCommandLine2, NewSize, L"^%", L"%", TRUE, FALSE);
  StrCpy(NewCommandLine1, NewCommandLine2);
  
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
  Take the original command line, substitute any variables, free 
  the original string, return the modified copy.

  @param[in] CmdLine  pointer to the command line to update.

  @retval EFI_SUCCESS           the function was successful.
  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
EFIAPI
ShellSubstituteVariables(
  IN CHAR16 **CmdLine
  )
{
  CHAR16      *NewCmdLine;
  NewCmdLine = ShellConvertVariables(*CmdLine);
  SHELL_FREE_NON_NULL(*CmdLine);
  if (NewCmdLine == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  *CmdLine = NewCmdLine;
  return (EFI_SUCCESS);
}

/**
  Take the original command line, substitute any alias in the first group of space delimited characters, free 
  the original string, return the modified copy.

  @param[in] CmdLine  pointer to the command line to update.

  @retval EFI_SUCCESS           the function was successful.
  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
EFIAPI
ShellSubstituteAliases(
  IN CHAR16 **CmdLine
  )
{
  CHAR16      *NewCmdLine;
  CHAR16      *CommandName;
  EFI_STATUS  Status;
  UINTN       PostAliasSize;
  ASSERT(CmdLine != NULL);
  ASSERT(*CmdLine!= NULL);


  CommandName = NULL;
  if (StrStr((*CmdLine), L" ") == NULL){
    StrnCatGrow(&CommandName, NULL, (*CmdLine), 0);
  } else {
    StrnCatGrow(&CommandName, NULL, (*CmdLine), StrStr((*CmdLine), L" ") - (*CmdLine));
  }

  //
  // This cannot happen 'inline' since the CmdLine can need extra space.
  //
  NewCmdLine = NULL;
  if (!ShellCommandIsCommandOnList(CommandName)) {
    //
    // Convert via alias
    //
    Status = ShellConvertAlias(&CommandName);
    if (EFI_ERROR(Status)){
      return (Status);
    }
    PostAliasSize = 0;
    NewCmdLine = StrnCatGrow(&NewCmdLine, &PostAliasSize, CommandName, 0);
    if (NewCmdLine == NULL) {
      SHELL_FREE_NON_NULL(CommandName);
      SHELL_FREE_NON_NULL(*CmdLine);
      return (EFI_OUT_OF_RESOURCES);
    }
    NewCmdLine = StrnCatGrow(&NewCmdLine, &PostAliasSize, StrStr((*CmdLine), L" "), 0);
    if (NewCmdLine == NULL) {
      SHELL_FREE_NON_NULL(CommandName);
      SHELL_FREE_NON_NULL(*CmdLine);
      return (EFI_OUT_OF_RESOURCES);
    }
  } else {
    NewCmdLine = StrnCatGrow(&NewCmdLine, NULL, (*CmdLine), 0);
  }

  SHELL_FREE_NON_NULL(*CmdLine);
  SHELL_FREE_NON_NULL(CommandName);
 
  //
  // re-assign the passed in double pointer to point to our newly allocated buffer
  //
  *CmdLine = NewCmdLine;

  return (EFI_SUCCESS);
}

/**
  Takes the Argv[0] part of the command line and determine the meaning of it.

  @param[in] CmdName  pointer to the command line to update.
  
  @retval Internal_Command    The name is an internal command.
  @retval File_Sys_Change     the name is a file system change.
  @retval Script_File_Name    the name is a NSH script file.
  @retval Unknown_Invalid     the name is unknown.
  @retval Efi_Application     the name is an application (.EFI).
**/
SHELL_OPERATION_TYPES
EFIAPI
GetOperationType(
  IN CONST CHAR16 *CmdName
  )
{
        CHAR16* FileWithPath;
  CONST CHAR16* TempLocation;
  CONST CHAR16* TempLocation2;

  FileWithPath = NULL;
  //
  // test for an internal command.
  //
  if (ShellCommandIsCommandOnList(CmdName)) {
    return (Internal_Command);
  }

  //
  // Test for file system change request.  anything ending with : and cant have spaces.
  //
  if (CmdName[(StrLen(CmdName)-1)] == L':') {
    if (StrStr(CmdName, L" ") != NULL) {
      return (Unknown_Invalid);
    }
    return (File_Sys_Change);
  }

  //
  // Test for a file
  //
  if ((FileWithPath = ShellFindFilePathEx(CmdName, mExecutableExtensions)) != NULL) {
    //
    // See if that file has a script file extension
    //
    if (StrLen(FileWithPath) > 4) {
      TempLocation = FileWithPath+StrLen(FileWithPath)-4;
      TempLocation2 = mScriptExtension;
      if (StringNoCaseCompare((VOID*)(&TempLocation), (VOID*)(&TempLocation2)) == 0) {
        SHELL_FREE_NON_NULL(FileWithPath);
        return (Script_File_Name);
      }
    }

    //
    // Was a file, but not a script.  we treat this as an application.
    //
    SHELL_FREE_NON_NULL(FileWithPath);
    return (Efi_Application);
  }
  
  SHELL_FREE_NON_NULL(FileWithPath);
  //
  // No clue what this is... return invalid flag...
  //
  return (Unknown_Invalid);
}

/**
  Determine if the first item in a command line is valid.

  @param[in] CmdLine            The command line to parse.

  @retval EFI_SUCCESS           The item is valid.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_NOT_FOUND         The operation type is unknown or invalid.
**/
EFI_STATUS 
EFIAPI
IsValidSplit(
  IN CONST CHAR16 *CmdLine
  )
{
  CHAR16        *Temp;
  CHAR16        *FirstParameter;
  CHAR16        *TempWalker;
  EFI_STATUS    Status;

  Temp           = NULL;

  Temp = StrnCatGrow(&Temp, NULL, CmdLine, 0);
  if (Temp == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  FirstParameter = StrStr(Temp, L"|");
  if (FirstParameter != NULL) {
    *FirstParameter = CHAR_NULL;
  }

  FirstParameter = NULL;

  //
  // Process the command line
  //
  Status = ProcessCommandLineToFinal(&Temp);

  if (!EFI_ERROR(Status)) {
    FirstParameter = AllocateZeroPool(StrSize(CmdLine));
    if (FirstParameter == NULL) {
      SHELL_FREE_NON_NULL(Temp);
      return (EFI_OUT_OF_RESOURCES);
    }
    TempWalker = (CHAR16*)Temp;
    GetNextParameter(&TempWalker, &FirstParameter);

    if (GetOperationType(FirstParameter) == Unknown_Invalid) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_NOT_FOUND), ShellInfoObject.HiiHandle, FirstParameter);
      SetLastError(SHELL_NOT_FOUND);
      Status = EFI_NOT_FOUND;
    }
  }

  SHELL_FREE_NON_NULL(Temp);
  SHELL_FREE_NON_NULL(FirstParameter);
  return Status;
}

/**
  Determine if a command line contains with a split contains only valid commands.

  @param[in] CmdLine      The command line to parse.

  @retval EFI_SUCCESS     CmdLine has only valid commands, application, or has no split.
  @retval EFI_ABORTED     CmdLine has at least one invalid command or application.
**/
EFI_STATUS
EFIAPI
VerifySplit(
  IN CONST CHAR16 *CmdLine
  )
{
  CONST CHAR16  *TempSpot;
  EFI_STATUS    Status;

  //
  // Verify up to the pipe or end character
  //
  Status = IsValidSplit(CmdLine);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  //
  // If this was the only item, then get out
  //
  if (!ContainsSplit(CmdLine)) {
    return (EFI_SUCCESS);
  }

  //
  // recurse to verify the next item
  //
  TempSpot = FindSplit(CmdLine)+1;
  return (VerifySplit(TempSpot));
}

/**
  Process a split based operation.

  @param[in] CmdLine    pointer to the command line to process

  @retval EFI_SUCCESS   The operation was successful
  @return               an error occured.
**/
EFI_STATUS
EFIAPI
ProcessNewSplitCommandLine(
  IN CONST CHAR16 *CmdLine
  )
{
  SPLIT_LIST                *Split;
  EFI_STATUS                Status;

  Status = VerifySplit(CmdLine);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  Split = NULL;

  //
  // are we in an existing split???
  //
  if (!IsListEmpty(&ShellInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST*)GetFirstNode(&ShellInfoObject.SplitList.Link);
  }

  if (Split == NULL) {
    Status = RunSplitCommand(CmdLine, NULL, NULL);
  } else {
    Status = RunSplitCommand(CmdLine, Split->SplitStdIn, Split->SplitStdOut);
  }
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_SPLIT), ShellInfoObject.HiiHandle, CmdLine);
  }
  return (Status);
}

/**
  Handle a request to change the current file system.

  @param[in] CmdLine  The passed in command line.

  @retval EFI_SUCCESS The operation was successful.
**/
EFI_STATUS
EFIAPI
ChangeMappedDrive(
  IN CONST CHAR16 *CmdLine
  )
{
  EFI_STATUS Status;
  Status = EFI_SUCCESS;

  //
  // make sure we are the right operation
  //
  ASSERT(CmdLine[(StrLen(CmdLine)-1)] == L':' && StrStr(CmdLine, L" ") == NULL);
  
  //
  // Call the protocol API to do the work
  //
  Status = ShellInfoObject.NewEfiShellProtocol->SetCurDir(NULL, CmdLine);

  //
  // Report any errors
  //
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_MAPPING), ShellInfoObject.HiiHandle, CmdLine);
  }

  return (Status);
}

/**
  Reprocess the command line to direct all -? to the help command.

  if found, will add "help" as argv[0], and move the rest later.

  @param[in,out] CmdLine        pointer to the command line to update
**/
EFI_STATUS
EFIAPI
DoHelpUpdate(
  IN OUT CHAR16 **CmdLine
  )
{
  CHAR16 *CurrentParameter;
  CHAR16 *Walker;
  CHAR16 *LastWalker;
  CHAR16 *NewCommandLine;
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  CurrentParameter = AllocateZeroPool(StrSize(*CmdLine));
  if (CurrentParameter == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  Walker = *CmdLine;
  while(Walker != NULL && *Walker != CHAR_NULL) {
    LastWalker = Walker;
    GetNextParameter(&Walker, &CurrentParameter);
    if (StrStr(CurrentParameter, L"-?") == CurrentParameter) {
      LastWalker[0] = L' ';
      LastWalker[1] = L' ';
      NewCommandLine = AllocateZeroPool(StrSize(L"help ") + StrSize(*CmdLine));
      if (NewCommandLine == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }
      StrCpy(NewCommandLine, L"help ");
      StrCat(NewCommandLine, *CmdLine);
      SHELL_FREE_NON_NULL(*CmdLine);
      *CmdLine = NewCommandLine;
      break;
    }
  }

  SHELL_FREE_NON_NULL(CurrentParameter);

  return (Status);
}

/**
  Function to update the shell variable "lasterror".

  @param[in] ErrorCode      the error code to put into lasterror.
**/
EFI_STATUS
EFIAPI
SetLastError(
  IN CONST SHELL_STATUS   ErrorCode
  )
{
  CHAR16 LeString[19];
  if (sizeof(EFI_STATUS) == sizeof(UINT64)) {
    UnicodeSPrint(LeString, sizeof(LeString), L"0x%Lx", ErrorCode);
  } else {
    UnicodeSPrint(LeString, sizeof(LeString), L"0x%x", ErrorCode);
  }
  DEBUG_CODE(InternalEfiShellSetEnv(L"debuglasterror", LeString, TRUE););
  InternalEfiShellSetEnv(L"lasterror", LeString, TRUE);

  return (EFI_SUCCESS);
}

/**
  Converts the command line to it's post-processed form.  this replaces variables and alias' per UEFI Shell spec.

  @param[in,out] CmdLine        pointer to the command line to update

  @retval EFI_SUCCESS           The operation was successful
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @return                       some other error occured
**/
EFI_STATUS
EFIAPI
ProcessCommandLineToFinal(
  IN OUT CHAR16 **CmdLine
  )
{
  EFI_STATUS                Status;
  TrimSpaces(CmdLine);

  Status = ShellSubstituteAliases(CmdLine);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  TrimSpaces(CmdLine);

  Status = ShellSubstituteVariables(CmdLine);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  TrimSpaces(CmdLine);

  //
  // update for help parsing
  //
  if (StrStr(*CmdLine, L"?") != NULL) {
    //
    // This may do nothing if the ? does not indicate help.
    // Save all the details for in the API below.
    //
    Status = DoHelpUpdate(CmdLine);
  }

  TrimSpaces(CmdLine);

  return (EFI_SUCCESS);
}

/**
  Run an internal shell command.

  This API will upadate the shell's environment since these commands are libraries.
  
  @param[in] CmdLine          the command line to run.
  @param[in] FirstParameter   the first parameter on the command line
  @param[in] ParamProtocol    the shell parameters protocol pointer

  @retval EFI_SUCCESS     The command was completed.
  @retval EFI_ABORTED     The command's operation was aborted.
**/
EFI_STATUS
EFIAPI
RunInternalCommand(
  IN CONST CHAR16                   *CmdLine,
  IN       CHAR16                   *FirstParameter,
  IN EFI_SHELL_PARAMETERS_PROTOCOL  *ParamProtocol
)
{
  EFI_STATUS                Status;
  UINTN                     Argc;
  CHAR16                    **Argv;
  SHELL_STATUS              CommandReturnedStatus;
  BOOLEAN                   LastError;

  //
  // get the argc and argv updated for internal commands
  //
  Status = UpdateArgcArgv(ParamProtocol, CmdLine, &Argv, &Argc);
  if (!EFI_ERROR(Status)) {
    //
    // Run the internal command.
    //
    Status = ShellCommandRunCommandHandler(FirstParameter, &CommandReturnedStatus, &LastError);

    if (!EFI_ERROR(Status)) {
      //
      // Update last error status.
      // some commands do not update last error.
      //
      if (LastError) {
        SetLastError(CommandReturnedStatus);
      }

      //
      // Pass thru the exitcode from the app.
      //
      if (ShellCommandGetExit()) {
        //
        // An Exit was requested ("exit" command), pass its value up.
        //
        Status = CommandReturnedStatus;
      } else if (CommandReturnedStatus != SHELL_SUCCESS && IsScriptOnlyCommand(FirstParameter)) {
        //
        // Always abort when a script only command fails for any reason
        //
        Status = EFI_ABORTED;
      } else if (ShellCommandGetCurrentScriptFile() != NULL && CommandReturnedStatus == SHELL_ABORTED) {
        //
        // Abort when in a script and a command aborted
        //
        Status = EFI_ABORTED;
      }
    }
  }

  //
  // This is guarenteed to be called after UpdateArgcArgv no matter what else happened.
  // This is safe even if the update API failed.  In this case, it may be a no-op.
  //
  RestoreArgcArgv(ParamProtocol, &Argv, &Argc);

  //
  // If a script is running and the command is not a scipt only command, then
  // change return value to success so the script won't halt (unless aborted).
  //
  // Script only commands have to be able halt the script since the script will
  // not operate if they are failing.
  //
  if ( ShellCommandGetCurrentScriptFile() != NULL
    && !IsScriptOnlyCommand(FirstParameter)
    && Status != EFI_ABORTED
    ) {
    Status = EFI_SUCCESS;
  }

  return (Status);
}

/**
  Function to run the command or file.

  @param[in] Type             the type of operation being run.
  @param[in] CmdLine          the command line to run.
  @param[in] FirstParameter   the first parameter on the command line
  @param[in] ParamProtocol    the shell parameters protocol pointer

  @retval EFI_SUCCESS     The command was completed.
  @retval EFI_ABORTED     The command's operation was aborted.
**/
EFI_STATUS
EFIAPI
RunCommandOrFile(
  IN       SHELL_OPERATION_TYPES    Type,
  IN CONST CHAR16                   *CmdLine,
  IN       CHAR16                   *FirstParameter,
  IN EFI_SHELL_PARAMETERS_PROTOCOL  *ParamProtocol
)
{
  EFI_STATUS                Status;
  EFI_STATUS                StatusCode;
  CHAR16                    *CommandWithPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  Status            = EFI_SUCCESS;
  CommandWithPath   = NULL;
  DevPath           = NULL;

  switch (Type) {
    case   Internal_Command:
      Status = RunInternalCommand(CmdLine, FirstParameter, ParamProtocol);
      break;
    case   Script_File_Name:
    case   Efi_Application:
      //
      // Process a fully qualified path
      //
      if (StrStr(FirstParameter, L":") != NULL) {
        ASSERT (CommandWithPath == NULL);
        if (ShellIsFile(FirstParameter) == EFI_SUCCESS) {
          CommandWithPath = StrnCatGrow(&CommandWithPath, NULL, FirstParameter, 0);
        }
      }

      //
      // Process a relative path and also check in the path environment variable
      //
      if (CommandWithPath == NULL) {
        CommandWithPath = ShellFindFilePathEx(FirstParameter, mExecutableExtensions);
      }

      //
      // This should be impossible now.
      //
      ASSERT(CommandWithPath != NULL);

      //
      // Make sure that path is not just a directory (or not found)
      //
      if (!EFI_ERROR(ShellIsDirectory(CommandWithPath))) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_NOT_FOUND), ShellInfoObject.HiiHandle, FirstParameter);
        SetLastError(SHELL_NOT_FOUND);
      }
      switch (Type) {
        case   Script_File_Name:
          Status = RunScriptFile (CommandWithPath, NULL, CmdLine, ParamProtocol);
          break;
        case   Efi_Application:
          //
          // Get the device path of the application image
          //
          DevPath = ShellInfoObject.NewEfiShellProtocol->GetDevicePathFromFilePath(CommandWithPath);
          if (DevPath == NULL){
            Status = EFI_OUT_OF_RESOURCES;
            break;
          }

          //
          // Execute the device path
          //
          Status = InternalShellExecuteDevicePath(
            &gImageHandle,
            DevPath,
            CmdLine,
            NULL,
            &StatusCode
           );

          SHELL_FREE_NON_NULL(DevPath);

          //
          // Update last error status.
          //
          SetLastError((SHELL_STATUS) StatusCode);
          break;
        default:
          //
          // Do nothing.
          //
          break;
      }
      break;
    default:
      //
      // Do nothing.
      //
      break;
  }

  SHELL_FREE_NON_NULL(CommandWithPath);

  return (Status);
}

/**
  Function to setup StdIn, StdErr, StdOut, and then run the command or file.

  @param[in] Type             the type of operation being run.
  @param[in] CmdLine          the command line to run.
  @param[in] FirstParameter   the first parameter on the command line.
  @param[in] ParamProtocol    the shell parameters protocol pointer

  @retval EFI_SUCCESS     The command was completed.
  @retval EFI_ABORTED     The command's operation was aborted.
**/
EFI_STATUS
EFIAPI
SetupAndRunCommandOrFile(
  IN SHELL_OPERATION_TYPES          Type,
  IN CHAR16                         *CmdLine,
  IN CHAR16                         *FirstParameter,
  IN EFI_SHELL_PARAMETERS_PROTOCOL  *ParamProtocol
)
{
  EFI_STATUS                Status;
  SHELL_FILE_HANDLE         OriginalStdIn;
  SHELL_FILE_HANDLE         OriginalStdOut;
  SHELL_FILE_HANDLE         OriginalStdErr;
  SYSTEM_TABLE_INFO         OriginalSystemTableInfo;

  //
  // Update the StdIn, StdOut, and StdErr for redirection to environment variables, files, etc... unicode and ASCII
  //
  Status = UpdateStdInStdOutStdErr(ParamProtocol, CmdLine, &OriginalStdIn, &OriginalStdOut, &OriginalStdErr, &OriginalSystemTableInfo);

  //
  // The StdIn, StdOut, and StdErr are set up.
  // Now run the command, script, or application
  //
  if (!EFI_ERROR(Status)) {
    Status = RunCommandOrFile(Type, CmdLine, FirstParameter, ParamProtocol);
  }

  //
  // Now print errors
  //
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_ERROR), ShellInfoObject.HiiHandle, (VOID*)(Status));
  }

  //
  // put back the original StdIn, StdOut, and StdErr
  //
  RestoreStdInStdOutStdErr(ParamProtocol, &OriginalStdIn, &OriginalStdOut, &OriginalStdErr, &OriginalSystemTableInfo);

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
  CHAR16                    *CleanOriginal;
  CHAR16                    *FirstParameter;
  CHAR16                    *TempWalker;
  SHELL_OPERATION_TYPES     Type;

  ASSERT(CmdLine != NULL);
  if (StrLen(CmdLine) == 0) {
    return (EFI_SUCCESS);
  }

  Status              = EFI_SUCCESS;
  CleanOriginal       = NULL;

  CleanOriginal = StrnCatGrow(&CleanOriginal, NULL, CmdLine, 0);
  if (CleanOriginal == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  TrimSpaces(&CleanOriginal);

  //
  // Handle case that passed in command line is just 1 or more " " characters.
  //
  if (StrLen (CleanOriginal) == 0) {
    SHELL_FREE_NON_NULL(CleanOriginal);
    return (EFI_SUCCESS);
  }

  Status = ProcessCommandLineToFinal(&CleanOriginal);
  if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(CleanOriginal);
    return (Status);
  }

  //
  // We dont do normal processing with a split command line (output from one command input to another)
  //
  if (ContainsSplit(CleanOriginal)) {
    Status = ProcessNewSplitCommandLine(CleanOriginal);
    SHELL_FREE_NON_NULL(CleanOriginal);
    return (Status);
  } 

  //
  // We need the first parameter information so we can determine the operation type
  //
  FirstParameter = AllocateZeroPool(StrSize(CleanOriginal));
  if (FirstParameter == NULL) {
    SHELL_FREE_NON_NULL(CleanOriginal);
    return (EFI_OUT_OF_RESOURCES);
  }
  TempWalker = CleanOriginal;
  GetNextParameter(&TempWalker, &FirstParameter);

  //
  // Depending on the first parameter we change the behavior
  //
  switch (Type = GetOperationType(FirstParameter)) {
    case   File_Sys_Change:
      Status = ChangeMappedDrive(CleanOriginal);
      break;
    case   Internal_Command:
    case   Script_File_Name:
    case   Efi_Application:
      Status = SetupAndRunCommandOrFile(Type, CleanOriginal, FirstParameter, ShellInfoObject.NewShellParametersProtocol);
      break;
    default:
      //
      // Whatever was typed, it was invalid.
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SHELL_NOT_FOUND), ShellInfoObject.HiiHandle, FirstParameter);
      SetLastError(SHELL_NOT_FOUND);
      break;
  }
 
  SHELL_FREE_NON_NULL(CleanOriginal);
  SHELL_FREE_NON_NULL(FirstParameter);

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
    if (CommandLine == NULL || StrLen(CommandLine) == 0 || CommandLine[0] == '#') {
      SHELL_FREE_NON_NULL(CommandLine);
      continue;
    }
    NewScriptFile->CurrentCommand = AllocateZeroPool(sizeof(SCRIPT_COMMAND_LIST));
    if (NewScriptFile->CurrentCommand == NULL) {
      SHELL_FREE_NON_NULL(CommandLine);
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
            // If command was "@echo -off" or "@echo -on" then don't restore echo state
            //
            if (StrCmp (L"@echo -off", CommandLine3) != 0 &&
                StrCmp (L"@echo -on", CommandLine3) != 0) {
              //
              // Now restore the pre-'@' echo state.
              //
              ShellCommandSetEchoState(PreCommandEchoState);
            }
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
          //
          // ShellCommandGetExitCode() always returns a UINT64
          //
          UnicodeSPrint(LeString, sizeof(LeString), L"0x%Lx", ShellCommandGetExitCode());
          DEBUG_CODE(InternalEfiShellSetEnv(L"debuglasterror", LeString, TRUE););
          InternalEfiShellSetEnv(L"lasterror", LeString, TRUE);

          ShellCommandRegisterExit(FALSE, 0);
          Status = EFI_SUCCESS;
          break;
        }
        if (ShellGetExecutionBreakFlag()) {
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
  @param[in] Handle             the handle of the script file already opened.
  @param[in] CmdLine            the command line to run.
  @param[in] ParamProtocol      the shell parameters protocol pointer

  @retval EFI_SUCCESS           the script completed sucessfully
**/
EFI_STATUS
EFIAPI
RunScriptFile (
  IN CONST CHAR16                   *ScriptPath,
  IN SHELL_FILE_HANDLE              Handle OPTIONAL,
  IN CONST CHAR16                   *CmdLine,
  IN EFI_SHELL_PARAMETERS_PROTOCOL  *ParamProtocol
  )
{
  EFI_STATUS          Status;
  SHELL_FILE_HANDLE   FileHandle;
  UINTN                     Argc;
  CHAR16                    **Argv;

  if (ShellIsFile(ScriptPath) != EFI_SUCCESS) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // get the argc and argv updated for scripts
  //
  Status = UpdateArgcArgv(ParamProtocol, CmdLine, &Argv, &Argc);
  if (!EFI_ERROR(Status)) {

    if (Handle == NULL) {
      //
      // open the file
      //
      Status = ShellOpenFileByName(ScriptPath, &FileHandle, EFI_FILE_MODE_READ, 0);
      if (!EFI_ERROR(Status)) {
        //
        // run it
        //
        Status = RunScriptFileHandle(FileHandle, ScriptPath);

        //
        // now close the file
        //
        ShellCloseFile(&FileHandle);
      }
    } else {
      Status = RunScriptFileHandle(Handle, ScriptPath);
    }
  }

  //
  // This is guarenteed to be called after UpdateArgcArgv no matter what else happened.
  // This is safe even if the update API failed.  In this case, it may be a no-op.
  //
  RestoreArgcArgv(ParamProtocol, &Argv, &Argc);

  return (Status);
}
